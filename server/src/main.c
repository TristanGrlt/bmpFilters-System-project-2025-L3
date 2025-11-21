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
#include "full_io.h"
#include "utils.h"

#define PID_FILE "/tmp/bmp_server.pid"

#define MIN_THREAD 4
#define MAX_THREAD 8

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

int main(int argc, char *argv[]) {
  (void)argc;
  int ret = EXIT_SUCCESS;
  request_t *rqs = MAP_FAILED;
  sem_t *mutex_empty = SEM_FAILED;
  sem_t *mutex_full = SEM_FAILED;
  sem_t *mutex_write = SEM_FAILED;
  int fd = -1;

  // PARSE ARGS
  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "--foreground") == 0 || strcmp(argv[i], "-f") == 0) {
      daemon_mode = false;
    }
  }

  signal(SIGCHLD, SIG_IGN);
  signal(SIGHUP, SIG_IGN);

  // DEAMON MODE
  if (daemon_mode) {
    openlog("bmp_server", LOG_PID | LOG_CONS, LOG_DAEMON);
    if (daemonize() < 0) {
      MESSAGE_ERR_D(argv[0], "daemonize");
      return EXIT_FAILURE;
    }
  }

  MESSAGE_INFO_D(argv[0], "BMP Server starting...");

  // SETUP SIGNAL HANDLER
  struct sigaction sa;
  memset(&sa, 0, sizeof(sa));
  sa.sa_handler = handle_sigint;
  sigaction(SIGINT, &sa, nullptr);

  // SHM
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

  // IF ONLY SERVER RUNING SET PID
  if (daemon_mode) {
    if (write_pid_file() < 0) {
      syslog(LOG_ERR, "Failed to write PID file");
    }
  }

  // MAP
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

  // MUTEX
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

  MESSAGE_INFO_D(argv[0], "BMP Server is runing");

  // READ REQUEST
  g_mutex_full = mutex_full;
  int rd = 0;
  while (running) {
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
      ret = EXIT_FAILURE;
      running = 0;
      break;
    case 0:
      MESSAGE_INFO_D(argv[0], "Processing new request");
      start_worker(&rq);
      MESSAGE_INFO_D(argv[0], "Processing ended for a request");
      exit(EXIT_SUCCESS);
    default:
      // waitpid(-1, NULL, WNOHANG);
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
  unlink(PID_FILE);
  MESSAGE_INFO_D(argv[0], "Server is shut down !");
  if (daemon_mode) {
    closelog();
  }
  return ret;
}

//---- [WORKER] --------------------------------------------------------------//
//----------------------------------------------------------------------------//

int apply_filter(filter_t filter, bmp_mapped_image_t *img);

int calculate_thread_count(off_t file_size) {
  int thread_count =
      MIN_THREAD +
      (int)((file_size * (MAX_THREAD - MIN_THREAD)) / MAX_SIZE_FILE);
  if (thread_count < MIN_THREAD)
    thread_count = MIN_THREAD;
  if (thread_count > MAX_THREAD)
    thread_count = MAX_THREAD;
  return thread_count;
}

// Calcule la répartition homogène des lignes entre les threads
// Retourne un tableau de (thread_count + 1) éléments contenant les indices de
// début/fin Exemple: height=14, threads=3 -> [0, 5, 10, 14] (tailles: 5, 5, 4)
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
  int ret = EXIT_SUCCESS;
  struct stat s;
  char fifo_path[256];
  fifo_path[0] = '\0';
  int fifo = -1;
  int fd = -1;
  void *mapped_data = MAP_FAILED;
  bmp_mapped_image_t img;

  // OPEN FIFO RESPONS
  snprintf(fifo_path, sizeof(fifo_path), "%s%d", FIFO_RESPONSE_BASE_PATH,
           rq->pid);
  if ((fifo = open(fifo_path, O_WRONLY)) == -1) {
    MESSAGE_ERR_D("server worker", "open");
    ret = errno;
    goto dispose;
  }

  // CHECK VALIDE IMAGE SIZE
  if (lstat(rq->path, &s) != 0) {
    MESSAGE_ERR_D("server worker", rq->path);
    ret = errno;
    goto dispose;
  }
  if (s.st_size > MAX_SIZE_FILE) {
    ret = EFBIG;
    goto dispose;
  }

  // OPEN IMAGE FILE
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

  // CHECK FILE TYPE VALIDITY
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

  // SEND IMAGE BACK
  if (full_write(fifo, &ret, sizeof(ret)) == -1) {
    MESSAGE_ERR_D("server worker", "full_write");
    ret = errno;
    goto dispose;
  }

  size_t count = (size_t)s.st_size;
  const char *ptr = (const char *)mapped_data;
  while (count > 0) {
    size_t n_w = count;
    if (n_w > PIPE_BUF) {
      n_w = PIPE_BUF;
    }
    if (full_write(fifo, ptr, n_w) == -1) {
      MESSAGE_ERR_D("server worker", "full_write");
      ret = errno;
      goto dispose;
    }
    ptr += n_w;
    count -= n_w;
  }

dispose:
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
  if (fifo != -1) {
    if (full_write(fifo, &ret, sizeof(ret)) == -1) {
      MESSAGE_ERR_D("server worker", "full_write");
    }
  }
  if (fifo != -1 && close(fifo) == -1) {
    MESSAGE_ERR_D("server worker", "close");
    ret = EXIT_FAILURE;
  }
  return;
}

int apply_filter(filter_t filter, bmp_mapped_image_t *img) {
  int ret = EXIT_SUCCESS;
  int thread_count = calculate_thread_count(img->file_h->file_size);
  pthread_t threads[MAX_THREAD];
  thread_filter_args_t *args = malloc(sizeof(*args) * (size_t)thread_count);
  if (args == NULL) {
    MESSAGE_ERR_D("apply_filter", "malloc args");
    return errno;
  }
  int32_t height =
      img->dib_h->height > 0 ? img->dib_h->height : -img->dib_h->height;
  int32_t *line_distribution =
      calculate_line_distribution(height, thread_count);
  if (line_distribution == nullptr) {
    MESSAGE_ERR_D("apply_filter", "malloc distribution");
    free(args);
    return errno;
  }

  // Déterminer la fonction de filtre à utiliser
  void *(*filter_func)(void *) = NULL;

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
    errno = EINVAL;
    MESSAGE_ERR_D("server worker", "Unsuported filter");
    ret = errno;
    goto dispose;
  }

  // Créer les threads (une seule fois)
  for (int i = 0; i < thread_count; i++) {
    args[i].img = img;
    args[i].start_line = line_distribution[i];
    args[i].end_line = line_distribution[i + 1];
    if (pthread_create(&threads[i], NULL, filter_func, &args[i]) != 0) {
      MESSAGE_ERR_D("apply_filter", "pthread_create");
      for (int j = 0; j < i; j++) {
        pthread_join(threads[j], NULL);
      }
      ret = EXIT_FAILURE;
      goto dispose;
    }
  }

  // Attendre tous les threads
  for (int i = 0; i < thread_count; i++) {
    pthread_join(threads[i], nullptr);
  }

dispose:
  free(line_distribution);
  free(args);
  return ret;
}