#ifndef UTILS_H
#define UTILS_H

#include <errno.h>
#include <semaphore.h>

#define MAX_SIZE_FILE 100000000 // 100MB

#define PERMS (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH)

#define MESSAGE_ERR(prog, func)                                                \
  { fprintf(stderr, "%s: %s: %s\n", (prog), (func), strerror(errno)); }

#define V(sem) sem_post(sem)
#define P(sem) sem_wait(sem)

#define FIFO_RESPONSE_BASE_PATH "/tmp/fifo_rep_" // ajouter le pid à la fin

#endif
