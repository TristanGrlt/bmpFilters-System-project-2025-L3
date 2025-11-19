#include <errno.h>
#include <fcntl.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#include "request.h"
#include "utils.h"

//---- [ARGS] ----------------------------------------------------------------//
//----------------------------------------------------------------------------//

#define REQUIRED_ARG_COUNT 2

#define SIMPLEARGS_REQUIRED_ARGS                                               \
  SIMPLEARGS_REQUIRED_STRING_ARG(input_file, "input", "Input image path", true)

#define SIMPLEARGS_BOOLEAN_ARGS                                                \
  SIMPLEARGS_BOOLEAN_ARG(b_and_w, "b&w", "blackAndWhite",                      \
                         "Apply a black and white filter to the image")

#include "simpleargs.h"

//---- [CODE] ----------------------------------------------------------------//
//----------------------------------------------------------------------------//

int main(int argc, char *argv[]) {
  int ret = EXIT_SUCCESS;
  sem_t *mutex_empty = SEM_FAILED;
  sem_t *mutex_full = SEM_FAILED;
  sem_t *mutex_write = SEM_FAILED;

  if (argc < REQUIRED_ARG_COUNT + 1) {
    ERROR_MESSAGE(EXE(argv[0]), SIMPLEARGS_ERR_MISSING_ARGS, REQUIRED_ARG_COUNT,
                  argc - 1);
    easyargs_print_help(argv[0]);
    return EXIT_FAILURE;
  }
  simpleargs_args_t args = easyargs_make_default_args();
  if (!easyargs_parse_args(argc, argv, &args)) {
    return EXIT_FAILURE;
  }
  filter_request_t rq;
  rq.pid = getpid();
  strncpy(rq.path, args.input_file, MAX_PATH_LENGTH - 1);
  rq.path[MAX_PATH_LENGTH - 1] = '\0';
  if (args.b_and_w) {
    rq.filter = blanckAndWhite;
  }
  (void)rq;

  int fd = -1;
  request_t *rqs = MAP_FAILED;

  // MUTEX
  if ((mutex_empty = sem_open(REQUEST_EMPTY_PATH, 0)) == SEM_FAILED) {
    if (errno == ENOENT) {
      fprintf(
          stderr,
          "%s: Error: Server is not running. Please start the server first.\n",
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
dispose:
  if (rqs != MAP_FAILED) {
    if (munmap(rqs, sizeof(request_t)) == -1) {
      MESSAGE_ERR(argv[0], "munmap");
      ret = EXIT_FAILURE;
    }
  }
  if (fd != -1) {
    close(fd);
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
  return ret;
}
