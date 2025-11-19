#include "full_io.h"

#include <errno.h>
#include <unistd.h>

static ssize_t safe_write(int fd, const void *buf, size_t count) {
  ssize_t written = -1;
  do {
    written = write(fd, buf, count);
  } while (written == -1 && errno == EINTR);
  return written;
}

static ssize_t safe_read(int fd, void *buf, size_t count) {
  ssize_t read_bytes = -1;

  do {
    read_bytes = read(fd, buf, count);
  } while (read_bytes == -1 && errno == EINTR);

  return read_bytes;
}

ssize_t full_write(int fd, const void *buf, size_t count) {
  size_t total = 0;
  const char *ptr = (const char *)buf;
  while (count > 0) {
    ssize_t n_w = safe_write(fd, ptr, count);
    if (n_w < 0) {
      break;
    }
    if (n_w == 0) {
      errno = ENOSPC;
      break;
    }
    total += (size_t)n_w;
    ptr += n_w;
    count -= (size_t)n_w;
  }
  return (ssize_t)total;
}

ssize_t full_read(int fd, void *buf, size_t count) {
  size_t total = 0;
  char *ptr = (char *)buf;

  while (count > 0) {
    ssize_t n_r = safe_read(fd, ptr, count);
    if (n_r <= 0) {
      if (n_r < 0) {
        return total > 0 ? (ssize_t)total : n_r;
      }
      break;
    }
    total += (size_t)n_r;
    ptr += n_r;
    count -= (size_t)n_r;
  }

  return (ssize_t)total;
}
