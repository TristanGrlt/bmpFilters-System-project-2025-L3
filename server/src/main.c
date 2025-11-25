#include <fcntl.h>
#include <linux/limits.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <syslog.h>
#include <unistd.h>

#include "bmp.h"
#include "config.h"
#include "full_io.h"
#include "utils.h"

#define PID_FILE "/tmp/bmp_server.pid"
#define MUTEX_WORKER_COUNT "/mutex_worker_count"
#define MUTEX_CONFIG_BMP "/mutex_bmp_config"
#define WRITE_TIMEOUT 5

//---- [FILTERS] -------------------------------------------------------------//
//----------------------------------------------------------------------------//

#include "filters.h"
#include "opt_to_request.h"

//---- [LOCAL FUNC] ----------------------------------------------------------//
//----------------------------------------------------------------------------//

void start_worker(filter_request_t *rq);

//---- [LOG] -----------------------------------------------------------------//
//----------------------------------------------------------------------------//

#define MESSAGE_ERR_D(prog, func)                                              \
  {                                                                            \
    if (!daemon_mode) {                                                        \
      fprintf(stderr, "%s: %s: %s\n", (prog), (func), strerror(errno));        \
    } else {                                                                   \
      syslog(LOG_ERR, "%s: %s: %s", (prog), (func), strerror(errno));          \
    }                                                                          \
  }

#define MESSAGE_INFO_D(prog, func)                                             \
  {                                                                            \
    if (!daemon_mode) {                                                        \
      fprintf(stderr, "%s: %s\n", (prog), (func));                             \
    } else {                                                                   \
      syslog(LOG_INFO, "%s: %s", (prog), (func));                              \
    }                                                                          \
  }

//---- [TIME OUT] ------------------------------------------------------------//
//----------------------------------------------------------------------------//

void handle_sigalrm(int sig) {
  (void)sig;
  syslog(LOG_ERR, "Write operation timed out");
  exit(EXIT_FAILURE);
}

static void set_write_timeout(unsigned int seconds) {
  struct sigaction sa;
  memset(&sa, 0, sizeof(sa));
  sa.sa_handler = handle_sigalrm;
  sa.sa_flags = 0;
  sigaction(SIGALRM, &sa, NULL);
  alarm(seconds);
}

//---- [STOP] ----------------------------------------------------------------//
//----------------------------------------------------------------------------//

static atomic_int running = 1;
static sem_t *g_mutex_full = SEM_FAILED;

void handle_sigint(int sig) {
  (void)sig;
  running = 0;

  if (g_mutex_full != SEM_FAILED)
    sem_post(g_mutex_full);
}

//---- [SIGCHLD] -------------------------------------------------------------//
//----------------------------------------------------------------------------//

static sem_t *g_mutex_worker_count = SEM_FAILED;
void handle_sigchld(int sig) {
  (void)sig;
  int saved_errno = errno;
  while (waitpid(-1, NULL, WNOHANG) > 0) {
    if (g_mutex_worker_count != SEM_FAILED) {
      sem_post(g_mutex_worker_count);
    }
  }
  errno = saved_errno;
}

//---- [CONFIG] --------------------------------------------------------------//
//----------------------------------------------------------------------------//

static server_config_t g_config;
static sem_t *g_config_mutex = SEM_FAILED;

void handle_sighup(int sig) {
  (void)sig;
  P(g_config_mutex);
  int old_max_worker = g_config.max_workers;
  // LOCAL
  if (config_load(&g_config, CONFIG_FILE_PATH_LOCAL) < 0) {
    // SYS
    if (config_load(&g_config, CONFIG_FILE_PATH_SYSTEM) < 0) {
      syslog(LOG_WARNING, "Failed to reload config, keeping current settings");
      V(g_config_mutex);
      return;
    }
  }
  // MAX WORKER
  if (g_config.max_workers < old_max_worker) {
    for (int k = 0; k < old_max_worker - g_config.max_workers; ++k) {
      P(g_mutex_worker_count);
    }
  } else {
    for (int k = 0; k < g_config.max_workers - old_max_worker; ++k) {
      V(g_mutex_worker_count);
    }
  }

  syslog(LOG_INFO, "Config reloaded from %s", CONFIG_FILE_PATH_LOCAL);
  syslog(LOG_INFO, "max_workers = %d", g_config.max_workers);
  syslog(LOG_INFO, "min_threads = %d", g_config.min_threads);
  syslog(LOG_INFO, "max_threads = %d", g_config.max_threads);

  V(g_config_mutex);
}

//---- [DEAMON] --------------------------------------------------------------//
//----------------------------------------------------------------------------//

bool daemon_mode = true;

int write_pid_file(void) {
  int fd = open(PID_FILE, O_WRONLY | O_CREAT | O_TRUNC, PERMS);
  if (fd < 0) {
    return -1;
  }
  char pid_str[32];
  snprintf(pid_str, sizeof(pid_str), "%d\n", getpid());
  if (write(fd, pid_str, strlen(pid_str)) < 0) {
    close(fd);
    return -1;
  }
  close(fd);
  return 0;
}

int daemonize(void) {
  int fd;
  switch (fork()) {
  case -1:
    return -1;
  case 0:
    break;
  default:
    exit(EXIT_SUCCESS);
  }
  if (setsid() < 0) {
    return -1;
  }
  switch (fork()) {
  case -1:
    return -1;
  case 0:
    break;
  default:
    exit(EXIT_SUCCESS);
  }
  umask(0);
  if (chdir("/") < 0) {
    return -1;
  }
  close(STDIN_FILENO);
  close(STDOUT_FILENO);
  close(STDERR_FILENO);
  fd = open("/dev/null", O_RDWR);
  if (fd != -1) {
    dup2(fd, STDIN_FILENO);
    dup2(fd, STDOUT_FILENO);
    dup2(fd, STDERR_FILENO);
    if (fd > STDERR_FILENO) {
      close(fd);
    }
  }
  return 0;
}

//---- [MAIN] ----------------------------------------------------------------//
//----------------------------------------------------------------------------//
//----------------------------------------------------------------------------//

int main(int argc, char *argv[]) {
  (void)argc;
  int ret = EXIT_SUCCESS;
  request_t *rqs = MAP_FAILED;
  sem_t *mutex_empty = SEM_FAILED;
  sem_t *mutex_full = SEM_FAILED;
  sem_t *mutex_write = SEM_FAILED;
  int fd = -1;

  //---- [PARSE ARGS        ] ------------------------------------------------//
  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "--foreground") == 0 || strcmp(argv[i], "-f") == 0) {
      daemon_mode = false;
    }
  }

  //---- [DEAMON MODE       ] ------------------------------------------------//
  if (daemon_mode) {
    openlog("bmp_server", LOG_PID | LOG_CONS, LOG_DAEMON);
    if (daemonize() < 0) {
      MESSAGE_ERR_D(argv[0], "daemonize");
      return EXIT_FAILURE;
    }
  }

  MESSAGE_INFO_D(argv[0], "BMP Server starting...");

  //---- [LOAD CONFIG      ] ------------------------------------------------//
  config_init_default(&g_config);
  // LOCAL
  if (config_load(&g_config, CONFIG_FILE_PATH_LOCAL) < 0) {
    // SYS
    if (config_load(&g_config, CONFIG_FILE_PATH_SYSTEM) < 0) {
      MESSAGE_INFO_D(argv[0], "No config could be load, using defaults");
    } else {
      MESSAGE_INFO_D(argv[0], "Config loaded from system path");
    }
  } else {
    MESSAGE_INFO_D(argv[0], "Config loaded from local path");
  }
  // MUTEX CONFIG
  g_config_mutex = sem_open(MUTEX_CONFIG_BMP, O_CREAT | O_EXCL, PERMS, 1);
  if (g_config_mutex == SEM_FAILED) {
    errno = EBUSY;
    MESSAGE_ERR_D(argv[0], "The server is already running");
    exit(EXIT_FAILURE);
  }

  //---- [SIGNAL HANDLER    ] ------------------------------------------------//
  struct sigaction sa;
  memset(&sa, 0, sizeof(sa));
  sa.sa_handler = handle_sigint;
  sigaction(SIGINT, &sa, nullptr);

  struct sigaction sa_hup;
  memset(&sa_hup, 0, sizeof(sa_hup));
  sa_hup.sa_handler = handle_sighup;
  sa_hup.sa_flags = SA_RESTART;
  sigaction(SIGHUP, &sa_hup, nullptr);

  struct sigaction sa_chld;
  memset(&sa_chld, 0, sizeof(sa_chld));
  sa_chld.sa_handler = handle_sigchld;
  sa_chld.sa_flags = SA_RESTART | SA_NOCLDSTOP;
  sigaction(SIGCHLD, &sa_chld, nullptr);

  //---- [SHM               ] ------------------------------------------------//
  fd = shm_open(REQUEST_FIFO_PATH, O_CREAT | O_EXCL | O_RDWR, PERMS);
  if (fd == -1) {
    if (errno == EEXIST) {
      errno = EBUSY;
      MESSAGE_ERR_D(argv[0], "Server already running");
      return EXIT_FAILURE;
    }
    MESSAGE_ERR_D(argv[0], "shm_open");
    ret = EXIT_FAILURE;
    goto dispose;
  }

  //---- [PID FILE          ] ------------------------------------------------//
  if (daemon_mode) {
    if (write_pid_file() < 0) {
      syslog(LOG_ERR, "Failed to write PID file");
    }
  }

  //---- [MAP               ] ------------------------------------------------//
  if (ftruncate(fd, (off_t)sizeof(request_t)) == -1) {
    MESSAGE_ERR_D(argv[0], "ftruncate");
    ret = EXIT_FAILURE;
    goto dispose;
  }
  rqs = mmap(nullptr, sizeof(request_t), PROT_READ | PROT_WRITE, MAP_SHARED, fd,
             0);
  if (rqs == MAP_FAILED) {
    MESSAGE_ERR_D(argv[0], "mmap");
    ret = EXIT_FAILURE;
    goto dispose;
  }
  rqs->write = 0;

  //---- [MUTEX             ] ------------------------------------------------//
  if ((mutex_write = sem_open(REQUEST_WRITE_PATH, O_CREAT | O_EXCL, PERMS,
                              1)) == SEM_FAILED) {
    MESSAGE_ERR_D(argv[0], "sem_open");
    ret = EXIT_FAILURE;
    goto dispose;
  }
  if ((mutex_empty = sem_open(REQUEST_EMPTY_PATH, O_CREAT | O_EXCL, PERMS,
                              REQUEST_FIFO_SIZE)) == SEM_FAILED) {
    MESSAGE_ERR_D(argv[0], "sem_open");
    ret = EXIT_FAILURE;
    goto dispose;
  }
  if ((mutex_full = sem_open(REQUEST_FULL_PATH, O_CREAT | O_EXCL, PERMS, 0)) ==
      SEM_FAILED) {
    MESSAGE_ERR_D(argv[0], "sem_open");
    ret = EXIT_FAILURE;
    goto dispose;
  }
  P(g_config_mutex);
  sem_unlink(MUTEX_WORKER_COUNT);
  if ((g_mutex_worker_count = sem_open(MUTEX_WORKER_COUNT, O_CREAT | O_EXCL,
                                       PERMS, g_config.max_workers)) ==
      SEM_FAILED) {
    MESSAGE_ERR_D(argv[0], "sem_open");
    ret = EXIT_FAILURE;
    goto dispose;
  }
  V(g_config_mutex);

  MESSAGE_INFO_D(argv[0], "BMP Server is runing");

  //---- [READ REQUEST      ] ------------------------------------------------//
  g_mutex_full = mutex_full;
  int rd = 0;
  while (running) {
    P(g_mutex_worker_count);
    P(mutex_full);
    if (!running) {
      break;
    }
    filter_request_t rq = rqs->buffer[rd];
    rd = (rd + 1) % REQUEST_FIFO_SIZE;
    V(mutex_empty);
    switch (fork()) {
    case -1:
      MESSAGE_ERR_D(argv[0], "fork");
      V(g_mutex_worker_count);
      ret = EXIT_FAILURE;
      running = 0;
      break;
    case 0:
      MESSAGE_INFO_D(argv[0], "Processing new request");
      start_worker(&rq);
      MESSAGE_INFO_D(argv[0], "Processing ended for a request");
      exit(EXIT_SUCCESS);
    default:
      break;
    }
  }
  MESSAGE_INFO_D(argv[0], "Server is shuting down...");
dispose:
  if (rqs != MAP_FAILED && munmap(rqs, sizeof(request_t)) == -1) {
    MESSAGE_ERR_D(argv[0], "munmap");
    ret = EXIT_FAILURE;
  }
  if (fd != -1 && close(fd) == -1) {
    MESSAGE_ERR_D(argv[0], "close");
    ret = EXIT_FAILURE;
  }
  if (shm_unlink(REQUEST_FIFO_PATH) == -1) {
    MESSAGE_ERR_D(argv[0], "shm_unlink");
    ret = EXIT_FAILURE;
  }
  if (mutex_write != SEM_FAILED && sem_close(mutex_write) == -1) {
    MESSAGE_ERR_D(argv[0], "sem_close");
    ret = EXIT_FAILURE;
  }
  if (mutex_empty != SEM_FAILED && sem_close(mutex_empty) == -1) {
    MESSAGE_ERR_D(argv[0], "sem_close");
    ret = EXIT_FAILURE;
  }
  if (mutex_full != SEM_FAILED && sem_close(mutex_full) == -1) {
    MESSAGE_ERR_D(argv[0], "sem_close");
    ret = EXIT_FAILURE;
  }
  if (g_config_mutex != SEM_FAILED && sem_close(g_config_mutex) == -1) {
    MESSAGE_ERR_D(argv[0], "sem_close");
    ret = EXIT_FAILURE;
  }
  if (g_mutex_worker_count != SEM_FAILED &&
      sem_close(g_mutex_worker_count) == -1) {
    MESSAGE_ERR_D(argv[0], "sem_close");
    ret = EXIT_FAILURE;
  }
  if (sem_unlink(REQUEST_EMPTY_PATH) == -1) {
    MESSAGE_ERR_D(argv[0], "sem_unlink");
    ret = EXIT_FAILURE;
  }
  if (sem_unlink(REQUEST_FULL_PATH) == -1) {
    MESSAGE_ERR_D(argv[0], "sem_unlink");
    ret = EXIT_FAILURE;
  }
  if (sem_unlink(REQUEST_WRITE_PATH) == -1) {
    MESSAGE_ERR_D(argv[0], "sem_unlink");
    ret = EXIT_FAILURE;
  }
  if (sem_unlink(MUTEX_WORKER_COUNT) == -1) {
    MESSAGE_ERR_D(argv[0], "sem_unlink");
    ret = EXIT_FAILURE;
  }
  if (sem_unlink(MUTEX_CONFIG_BMP) == -1) {
    MESSAGE_ERR_D(argv[0], "sem_unlink");
    ret = EXIT_FAILURE;
  }
  unlink(PID_FILE);
  MESSAGE_INFO_D(argv[0], "Server is shut down !");
  if (daemon_mode) {
    closelog();
  }
  return ret;
}

//---- [WORKER] --------------------------------------------------------------//
//----------------------------------------------------------------------------//
//----------------------------------------------------------------------------//

// apply_filter: apply filter to the image pointed by img
int apply_filter(filter_t filter, bmp_mapped_image_t *img);

// calculate_thread_count: Computes the optimal number of thread depending of
// the min and max thread limits define in the config file.
int calculate_thread_count(off_t file_size) {
  P(g_config_mutex);
  int count = config_get_thread_count(&g_config, file_size);
  V(g_config_mutex);
  return count;
}

// calculate_line_distribution: Computes the even distribution of lines
// among threads. Returns an array of (thread_count + 1) elements
// containing the start/end indices.
// Example: height=14, threads=3 -> [0, 5, 10, 14] (sizes: 5, 5, 4)
int32_t *calculate_line_distribution(int32_t height, int thread_count) {
  int32_t *distribution = malloc(sizeof(int32_t) * (size_t)(thread_count + 1));
  if (distribution == nullptr) {
    return nullptr;
  }
  int32_t base_lines = height / thread_count;
  int32_t extra_lines = height % thread_count;
  distribution[0] = 0;
  for (int i = 0; i < thread_count; i++) {
    int32_t lines_for_this_thread = base_lines + (i < extra_lines ? 1 : 0);
    distribution[i + 1] = distribution[i] + lines_for_this_thread;
  }
  return distribution;
}

void start_worker(filter_request_t *rq) {
  // sleep(2);
  int ret = EXIT_SUCCESS;
  struct stat s;
  char fifo_path[255];
  fifo_path[0] = '\0';
  int fifo = -1;
  int fd = -1;
  void *mapped_data = MAP_FAILED;
  bmp_mapped_image_t img;

  //---- [OPEN FIFO RESPONS ] ------------------------------------------------//
  snprintf(fifo_path, sizeof(fifo_path), "%s%d", FIFO_RESPONSE_BASE_PATH,
           rq->pid);
  if ((fifo = open(fifo_path, O_WRONLY)) == -1) {
    MESSAGE_ERR_D("server worker", "open");
    ret = errno;
    goto dispose;
  }

  //---- [CHECK IMAGE SIZE  ] ------------------------------------------------//
  if (lstat(rq->path, &s) != 0) {
    MESSAGE_ERR_D("server worker", rq->path);
    ret = errno;
    goto dispose;
  }
  if (s.st_size > MAX_SIZE_FILE) {
    ret = EFBIG;
    goto dispose;
  }

  //---- [OPEN IMAGE FILE   ] ------------------------------------------------//
  fd = open(rq->path, O_RDONLY);
  if (fd == -1) {
    MESSAGE_ERR_D("server worker", "open");
    ret = errno;
    goto dispose;
  }
  mapped_data = mmap(nullptr, (size_t)s.st_size, PROT_READ | PROT_WRITE,
                     MAP_PRIVATE, fd, 0);
  if (mapped_data == MAP_FAILED) {
    MESSAGE_ERR_D("server worker", "mmap");
    ret = errno;
    goto dispose;
  }
  img.file_h = (bmp_file_header_t *)mapped_data;

  //---- [CHECK TYPE VALIDITY] -----------------------------------------------//
  if (img.file_h->signature != BMP_SIGNATURE) {
    ret = EINVAL;
    goto dispose;
  }
  img.dib_h =
      (bmp_dib_header_t *)((char *)mapped_data + sizeof(bmp_file_header_t));
  img.pixels = (u_int8_t *)mapped_data + img.file_h->pixel_array_offset;

  if ((ret = apply_filter(rq->filter, &img)) != EXIT_SUCCESS) {
    goto dispose;
  }

  //---- [SEND IMAGE BACK   ] ------------------------------------------------//

  set_write_timeout(WRITE_TIMEOUT);
  if (full_write(fifo, &ret, sizeof(ret)) == -1) {
    MESSAGE_ERR_D("server worker", "full_write");
    ret = errno;
    goto dispose;
  }
  alarm(0);

  size_t count = (size_t)s.st_size;
  const char *ptr = (const char *)mapped_data;
  while (count > 0) {
    size_t n_w = count;
    if (n_w > PIPE_BUF) {
      n_w = PIPE_BUF;
    }
    set_write_timeout(WRITE_TIMEOUT);
    if (full_write(fifo, ptr, n_w) == -1) {
      MESSAGE_ERR_D("server worker", "full_write");
      ret = errno;
      goto dispose;
    }
    alarm(0);
    ptr += n_w;
    count -= n_w;
  }

dispose:
  alarm(0);
  if (fd != -1 && close(fd) == -1) {
    MESSAGE_ERR_D("server worker", "close");
    ret = EXIT_FAILURE;
  }
  if (mapped_data != MAP_FAILED) {
    if (munmap(mapped_data, (size_t)s.st_size) == -1) {
      MESSAGE_ERR_D("server worker", "munmap");
      ret = EXIT_FAILURE;
    }
  }
  set_write_timeout(WRITE_TIMEOUT);
  if (fifo != -1) {
    if (full_write(fifo, &ret, sizeof(ret)) == -1) {
      MESSAGE_ERR_D("server worker", "full_write");
    }
  }
  alarm(0);
  if (fifo != -1 && close(fifo) == -1) {
    MESSAGE_ERR_D("server worker", "close");
    ret = EXIT_FAILURE;
  }
  return;
}

int apply_filter(filter_t filter, bmp_mapped_image_t *img) {
  int ret = EXIT_SUCCESS;
  int thread_count = calculate_thread_count(img->file_h->file_size);
  bool is_complex = false;

  pthread_t threads[ABSOLUTE_MAX_THREADS];
  thread_filter_args_t args[ABSOLUTE_MAX_THREADS];
  int32_t height =
      img->dib_h->height > 0 ? img->dib_h->height : -img->dib_h->height;
  int32_t *line_distribution =
      calculate_line_distribution(height, thread_count);
  if (line_distribution == nullptr) {
    MESSAGE_ERR_D("apply_filter", "distribution");
    return errno;
  }

  //---- [SELECT FILTER     ] ------------------------------------------------//
  void *(*filter_func)(void *) = NULL;
  // SIMPLE OPTIONS
  switch (filter) {
#define OPT_TO_REQUEST_SIMPLE_FILTER(filter_name, short_flag, long_flag,       \
                                     description, filter_func_ptr)             \
  case filter_name:                                                            \
    filter_func = filter_func_ptr;                                             \
    break;
#ifdef OPT_TO_REQUEST_SIMPLE_FILTERS
    OPT_TO_REQUEST_SIMPLE_FILTERS
#endif
#undef OPT_TO_REQUEST_SIMPLE_FILTER
  default:
    // COMPLEX OPTIONS
    is_complex = true;
    switch (filter) {
#define OPT_TO_REQUEST_COMPLEX_FILTER(filter_name, short_flag, long_flag,      \
                                      description, filter_func_ptr)            \
  case filter_name:                                                            \
    filter_func = filter_func_ptr;                                             \
    break;
#ifdef OPT_TO_REQUEST_COMPLEX_FILTERS
      OPT_TO_REQUEST_COMPLEX_FILTERS
#endif
#undef OPT_TO_REQUEST_COMPLEX_FILTER
    default:
      errno = EINVAL;
      MESSAGE_ERR_D("server worker", "Unsuported filter");
      ret = errno;
      goto dispose;
    }
  }
  bmp_mapped_image_t img_ref;
  if (is_complex) {
    size_t image_size = img->file_h->file_size;
    void *ref_data = malloc(image_size);
    if (ref_data == NULL) {
      MESSAGE_ERR_D("apply_filter", "malloc");
      ret = errno;
      goto dispose;
    }
    memcpy(ref_data, img->file_h, image_size);

    img_ref.file_h = (bmp_file_header_t *)ref_data;
    img_ref.dib_h =
        (bmp_dib_header_t *)((char *)ref_data + sizeof(bmp_file_header_t));
    img_ref.pixels = (uint8_t *)ref_data + img->file_h->pixel_array_offset;
  }

  //---- [THREAD            ] ------------------------------------------------//
  for (int i = 0; i < thread_count; i++) {
    args[i].img = img;
    args[i].start_line = line_distribution[i];
    args[i].end_line = line_distribution[i + 1];
    if (is_complex) {
      args[i].ref_img = &img_ref;
    }
    if (pthread_create(&threads[i], NULL, filter_func, &args[i]) != 0) {
      MESSAGE_ERR_D("apply_filter", "pthread_create");
      for (int j = 0; j < i; j++) {
        pthread_join(threads[j], NULL);
      }
      ret = EXIT_FAILURE;
      goto dispose;
    }
  }

  //---- [WAIT              ] ------------------------------------------------//
  for (int i = 0; i < thread_count; i++) {
    pthread_join(threads[i], nullptr);
  }

dispose:
  if (is_complex) {
    free(img_ref.file_h);
  }
  free(line_distribution);
  return ret;
}