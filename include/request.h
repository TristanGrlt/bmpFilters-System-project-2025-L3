#ifndef REQUEST_H
#define REQUEST_H

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>

// #define PIPE_PATH "/tmp/sis"
#define REQUEST_FIFO_SIZE 10

#define REQUEST_FIFO_PATH "/filter_request_fifo"
#define REQUEST_EMPTY_PATH "/mutex_empty"
#define REQUEST_FULL_PATH "/mutex_full"
#define REQUEST_WRITE_PATH "/mutex_write"

#define MAX_PATH_LENGTH 256

typedef enum { identity, blanckAndWhite } filter_t;

typedef struct {
  pid_t pid;
  char path[MAX_PATH_LENGTH];
  filter_t filter;
} filter_request_t;

typedef struct {
  int write;
  filter_request_t buffer[REQUEST_FIFO_SIZE];
} request_t;

#endif