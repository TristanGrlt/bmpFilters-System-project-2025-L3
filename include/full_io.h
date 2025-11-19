#ifndef FULL_IO_H
#define FULL_IO_H

#include <stddef.h>
#include <sys/types.h>

ssize_t full_read(int fd, void *buf, size_t count);
ssize_t full_write(int fd, const void *buf, size_t count);

#endif
