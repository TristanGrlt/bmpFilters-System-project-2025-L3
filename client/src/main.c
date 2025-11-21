#include <errno.h>
#include <fcntl.h>
#include <linux/limits.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include "full_io.h"
#include "utils.h"

//---- [FILTERS] -------------------------------------------------------------//
//----------------------------------------------------------------------------//

#include "filters.h"
#include "opt_to_request.h"

//---- [CODE] ----------------------------------------------------------------//
//----------------------------------------------------------------------------//

int main(int argc, char *argv[]) {
  int ret = EXIT_SUCCESS;
  filter_request_t rq;
  sem_t *mutex_empty = SEM_FAILED;
  sem_t *mutex_full = SEM_FAILED;
  sem_t *mutex_write = SEM_FAILED;
  int fd = -1;
  request_t *rqs = MAP_FAILED;
  int fifo = -1;
  char fifo_path[256];
  fifo_path[0] = '\0';

  // PARSE ARGS
  arguments_t args;
  if (process_options_to_request(argc, argv, &args) != EXIT_SUCCESS) {
    return EXIT_FAILURE;
  }

  // CREATE REQUEST
  rq.pid = getpid();
  strncpy(rq.path, args.input, MAX_PATH_LENGTH - 1);
  rq.path[MAX_PATH_LENGTH - 1] = '\0';
  rq.filter = args.filter;

  // MUTEX
  if ((mutex_empty = sem_open(REQUEST_EMPTY_PATH, 0)) == SEM_FAILED) {
    if (errno == ENOENT) {
      fprintf(stderr,
              "%s: Error: Server is not running. Please "
              "start the server first.\n",
              argv[0]);
      ret = EXIT_FAILURE;
      goto dispose;
    } else {
      MESSAGE_ERR(argv[0], "sem_open");
      ret = EXIT_FAILURE;
      goto dispose;
    }
  }
  if ((mutex_full = sem_open(REQUEST_FULL_PATH, 0)) == SEM_FAILED) {
    MESSAGE_ERR(argv[0], "sem_open");
    ret = EXIT_FAILURE;
    goto dispose;
  }
  if ((mutex_write = sem_open(REQUEST_WRITE_PATH, 0)) == SEM_FAILED) {
    MESSAGE_ERR(argv[0], "sem_open");
    ret = EXIT_FAILURE;
    goto dispose;
  }
  // SHM
  fd = shm_open(REQUEST_FIFO_PATH, O_RDWR, 0);
  if (fd == -1) {
    MESSAGE_ERR(argv[0], "shm_open");
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
  // WRITE REQUEST
  P(mutex_empty);
  P(mutex_write);
  rqs->buffer[rqs->write] = rq;
  rqs->write = (rqs->write + 1) % REQUEST_FIFO_SIZE;
  V(mutex_write);
  V(mutex_full);
  // WAIT RESPONSE
  snprintf(fifo_path, 255, "%s%d", FIFO_RESPONSE_BASE_PATH, getpid());
  if (mkfifo(fifo_path, PERMS) == -1) {
    MESSAGE_ERR(argv[0], "mkfifo");
    ret = EXIT_FAILURE;
    goto dispose;
  }
  fifo = open(fifo_path, O_RDONLY);
  if (fifo == -1) {
    MESSAGE_ERR(argv[0], "open");
    ret = EXIT_FAILURE;
    goto dispose;
  }
  // READ TO DETECT EXIT_FAILURE OF THE SERVER
  int err;
  full_read(fifo, &err, sizeof(err));
  if (err != EXIT_SUCCESS) {
    errno = err;
    MESSAGE_ERR(argv[0], "server");
    ret = EXIT_FAILURE;
    goto dispose;
  } else {
    printf("filter applyed with succes, getting the image back...\n");
  }

  // READ IMAGE BACK
  int fd_out = open(args.output, O_WRONLY | O_CREAT | O_TRUNC, PERMS);
  if (fd_out == -1) {
    MESSAGE_ERR(argv[0], "open output file");
    ret = EXIT_FAILURE;
    goto dispose;
  }

  struct stat s;
  if (stat(args.input, &s) != 0) {
    MESSAGE_ERR(argv[0], "stat");
    close(fd_out);
    ret = EXIT_FAILURE;
    goto dispose;
  }

  size_t count = (size_t)s.st_size;
  char buffer[PIPE_BUF];
  while (count > 0) {
    size_t n_r = count;
    if (n_r > PIPE_BUF) {
      n_r = PIPE_BUF;
    }
    if (full_read(fifo, buffer, n_r) == -1) {
      MESSAGE_ERR(argv[0], "full_read");
      close(fd_out);
      ret = EXIT_FAILURE;
      goto dispose;
    }
    if (full_write(fd_out, buffer, n_r) == -1) {
      MESSAGE_ERR(argv[0], "full_write");
      close(fd_out);
      ret = EXIT_FAILURE;
      goto dispose;
    }
    count -= n_r;
  }

  if (close(fd_out) == -1) {
    MESSAGE_ERR(argv[0], "close output file");
    ret = EXIT_FAILURE;
    goto dispose;
  }

  printf("Image created with success\n");

dispose:
  if (rqs != MAP_FAILED) {
    if (munmap(rqs, sizeof(request_t)) == -1) {
      MESSAGE_ERR(argv[0], "munmap");
      ret = EXIT_FAILURE;
    }
  }
  if (fd != -1 && close(fd) == -1) {
    MESSAGE_ERR(argv[0], "close");
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
  if (mutex_write != SEM_FAILED && sem_close(mutex_write) == -1) {
    MESSAGE_ERR(argv[0], "sem_close");
    ret = EXIT_FAILURE;
  }
  if (fifo != -1 && close(fifo) == -1) {
    MESSAGE_ERR(argv[0], "close");
    ret = EXIT_FAILURE;
  }
  if (fifo_path[0] != '\0' && unlink(fifo_path) == -1) {
    MESSAGE_ERR(argv[0], "unlink");
    ret = EXIT_FAILURE;
  }
  return ret;
}
