#ifndef UTILS_H
#define UTILS_H

#include <errno.h>
#include <semaphore.h>

#define MESSAGE_ERR(prog, func)                                                \
        { fprintf(stderr, "%s: %s: %s\n", (prog), (func), strerror(errno)); }

#define V(sem) sem_post(sem)
#define P(sem) sem_wait(sem)

#endif
