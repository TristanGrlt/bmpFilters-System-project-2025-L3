#ifndef UTILS_H
#define UTILS_H

#include <errno.h>
#include <semaphore.h>

#define PERMS (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH)

#define MESSAGE_ERR(prog, func)                                                \
  { fprintf(stderr, "%s: %s: %s\n", (prog), (func), strerror(errno)); }

#define V(sem) sem_post(sem)
#define P(sem) sem_wait(sem)

#define FIFO_RESPONSE_BASE_PATH                                                \
  "/tmp/fifo_rep_" // Add the pid of the client at the end

#endif
