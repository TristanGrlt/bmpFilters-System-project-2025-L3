#include <fcntl.h>
#include <semaphore.h>
#include <signal.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <unistd.h>

#include "bmp.h"
#include "full_io.h"
#include "request.h"
#include "utils.h"

void start_worker(filter_request_t *rq);

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

//---- [CODE] ----------------------------------------------------------------//
//----------------------------------------------------------------------------//

int main(int argc, char *argv[]) {
  (void)argc;
  int ret = EXIT_SUCCESS;
  request_t *rqs = MAP_FAILED;
  sem_t *mutex_empty = SEM_FAILED;
  sem_t *mutex_full = SEM_FAILED;
  sem_t *mutex_write = SEM_FAILED;
  int fd = -1;

  // SETUP SIGNAL HANDLER
  struct sigaction sa;
  memset(&sa, 0, sizeof(sa));
  sa.sa_handler = handle_sigint;
  sigaction(SIGINT, &sa, nullptr);

  // CLEAN
  sem_unlink(REQUEST_EMPTY_PATH);
  sem_unlink(REQUEST_FULL_PATH);
  sem_unlink(REQUEST_WRITE_PATH);
  shm_unlink(REQUEST_FIFO_PATH);

  // SHM
  fd = shm_open(REQUEST_FIFO_PATH, O_CREAT | O_EXCL | O_RDWR, PERMS);
  if (fd == -1) {
    MESSAGE_ERR(argv[0], "shm_open");
    ret = EXIT_FAILURE;
    goto dispose;
  }
  if (ftruncate(fd, (off_t)sizeof(request_t)) == -1) {
    MESSAGE_ERR(argv[0], "ftruncate");
    ret = EXIT_FAILURE;
    goto dispose;
  }
  rqs = mmap(nullptr, sizeof(request_t), PROT_READ | PROT_WRITE, MAP_SHARED, fd,
             0);
  if (rqs == MAP_FAILED) {
    MESSAGE_ERR(argv[0], "mmap");
    ret = EXIT_FAILURE;
    goto dispose;
  }
  rqs->write = 0;

  // MUTEX
  if ((mutex_write = sem_open(REQUEST_WRITE_PATH, O_CREAT | O_EXCL, PERMS,
                              1)) == SEM_FAILED) {
    MESSAGE_ERR(argv[0], "sem_open");
    ret = EXIT_FAILURE;
    goto dispose;
  }
  if ((mutex_empty = sem_open(REQUEST_EMPTY_PATH, O_CREAT | O_EXCL, PERMS,
                              REQUEST_FIFO_SIZE)) == SEM_FAILED) {
    MESSAGE_ERR(argv[0], "sem_open");
    ret = EXIT_FAILURE;
    goto dispose;
  }
  if ((mutex_full = sem_open(REQUEST_FULL_PATH, O_CREAT | O_EXCL, PERMS, 0)) ==
      SEM_FAILED) {
    MESSAGE_ERR(argv[0], "sem_open");
    ret = EXIT_FAILURE;
    goto dispose;
  }

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
      MESSAGE_ERR(argv[0], "fork");
      ret = EXIT_FAILURE;
      running = 0;
      break;
    case 0:
      printf("Processing new request from pid:%d client\n", rq.pid);
      start_worker(&rq);
      printf("Process ended for request from pid:%d client\n", rq.pid);
      exit(EXIT_SUCCESS);
    default:
      waitpid(-1, NULL, WNOHANG);
      break;
    }
  }
  printf("\nServer is shuting down...\n");
dispose:
  if (rqs != MAP_FAILED && munmap(rqs, sizeof(request_t)) == -1) {
    MESSAGE_ERR(argv[0], "munmap");
    ret = EXIT_FAILURE;
  }
  if (fd != -1 && close(fd) == -1) {
    MESSAGE_ERR(argv[0], "close");
    ret = EXIT_FAILURE;
  }
  if (shm_unlink(REQUEST_FIFO_PATH) == -1) {
    MESSAGE_ERR(argv[0], "shm_unlink");
    ret = EXIT_FAILURE;
  }
  if (mutex_write != SEM_FAILED && sem_close(mutex_write) == -1) {
    MESSAGE_ERR(argv[0], "sem_close");
    ret = EXIT_FAILURE;
  }
  if (mutex_empty != SEM_FAILED && sem_close(mutex_empty) == -1) {
    MESSAGE_ERR(argv[0], "sem_close");
    ret = EXIT_FAILURE;
  }
  if (mutex_full != SEM_FAILED && sem_close(mutex_full) == -1) {
    MESSAGE_ERR(argv[0], "sem_close");
    ret = EXIT_FAILURE;
  }
  if (sem_unlink(REQUEST_EMPTY_PATH) == -1) {
    MESSAGE_ERR(argv[0], "sem_unlink");
    ret = EXIT_FAILURE;
  }
  if (sem_unlink(REQUEST_FULL_PATH) == -1) {
    MESSAGE_ERR(argv[0], "sem_unlink");
    ret = EXIT_FAILURE;
  }
  if (sem_unlink(REQUEST_WRITE_PATH) == -1) {
    MESSAGE_ERR(argv[0], "sem_unlink");
    ret = EXIT_FAILURE;
  }
  printf("Server is shut down !\n");
  return ret;
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
    MESSAGE_ERR("server worker", "open");
    ret = errno;
    goto dispose;
  }

  // CHECK VALIDE IMAGE SIZE
  if (lstat(rq->path, &s) != 0) {
    MESSAGE_ERR("server worker", rq->path);
    ret = errno;
    goto dispose;
  }
  if (s.st_size > MAX_SIZE_FILE) {
    errno = EFBIG;
    MESSAGE_ERR("server worker", rq->path);
    ret = errno;
    goto dispose;
  }

  // OPEN IMAGE FILE
  fd = open(rq->path, O_RDONLY);
  if (fd == -1) {
    MESSAGE_ERR("server worker", "open");
    ret = errno;
    goto dispose;
  }

  // MAP
  mapped_data = mmap(NULL, (size_t)s.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
  if (mapped_data == MAP_FAILED) {
    MESSAGE_ERR("server worker", "mmap");
    ret = errno;
    goto dispose;
  }
  img.file_h = (bmp_file_header_t *)mapped_data;
  img.dib_h =
      (bmp_dib_header_t *)((char *)mapped_data + sizeof(bmp_file_header_t));
  img.pixels = (u_int8_t *)mapped_data + img.file_h->pixel_array_offset;

  printf("\tWidth: %d, Height: %u\n", img.dib_h->width, img.dib_h->height);

dispose:
  if (fd != -1 && close(fd) == -1) {
    MESSAGE_ERR("server worker", "close");
    ret = EXIT_FAILURE;
  }
  if (mapped_data != MAP_FAILED) {
    if (munmap(mapped_data, (size_t)s.st_size) == -1) {
      MESSAGE_ERR("server worker", "munmap");
      ret = EXIT_FAILURE;
    }
  }
  if (fifo != -1) {
    full_write(fifo, &ret, sizeof(ret));
  }
  if (fifo != -1 && close(fifo) == -1) {
    MESSAGE_ERR("server worker", "close");
  }
  return;
}