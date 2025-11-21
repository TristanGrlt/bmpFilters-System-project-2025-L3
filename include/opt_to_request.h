#ifndef OPT_TO_REQUEST_H
#define OPT_TO_REQUEST_H

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "filters.h"

#define REQUEST_FIFO_SIZE 10
#define MAX_PATH_LENGTH 256

#define REQUEST_FIFO_PATH "/filter_request_fifo"
#define REQUEST_EMPTY_PATH "/mutex_empty"
#define REQUEST_FULL_PATH "/mutex_full"
#define REQUEST_WRITE_PATH "/mutex_write"

#ifndef OPT_TO_REQUEST_SHORT_PREFIX
#define OPT_TO_REQUEST_SHORT_PREFIX "-"
#endif

#ifndef OPT_TO_REQUEST_LONG_PREFIX
#define OPT_TO_REQUEST_LONG_PREFIX "--"
#endif

#ifndef OPT_TO_REQUEST_SHORT_HELP
#define OPT_TO_REQUEST_SHORT_HELP "h"
#endif

#ifndef OPT_TO_REQUEST_LONG_HELP
#define OPT_TO_REQUEST_LONG_HELP "help"
#endif

// CREATE ENUM FILTERS
#define OPT_TO_REQUEST_SIMPLE_FILTER(filter, ...) filter,
typedef enum {
#ifdef OPT_TO_REQUEST_SIMPLE_FILTERS
  OPT_TO_REQUEST_SIMPLE_FILTERS
#endif
} filter_t;
#undef OPT_TO_REQUEST_SIMPLE_FILTER

// STRUCTURE
typedef struct {
  char *input;
  char *output;
  filter_t filter;
} arguments_t;

typedef struct {
  pid_t pid;
  char path[MAX_PATH_LENGTH];
  filter_t filter;
} filter_request_t;

typedef struct {
  int write;
  filter_request_t buffer[REQUEST_FIFO_SIZE];
} request_t;

int process_options_to_request(int argc, char *argv[], arguments_t *arg);

void print_help(const char *exec_name);

#endif