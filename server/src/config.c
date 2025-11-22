#include "config.h"
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "utils.h"

#define MAX_LINE_LENGTH 256

void config_init_default(server_config_t *config) {
  config->max_workers = DEFAULT_MAX_WORKERS;
  config->min_threads = DEFAULT_MIN_THREADS;
  config->max_threads = DEFAULT_MAX_THREADS;
  config->is_valid = true;
}

// trim: supprime les caratère espace au début et à la fin de la chaine de
// caractère pointé par str
static char *trim(char *str) {
  while (isspace((unsigned char)*str)) {
    str++;
  }
  if (*str == '\0') {
    return str;
  }
  char *end = str + strlen(str) - 1;
  while (end > str && isspace((unsigned char)*end)) {
    end--;
  }
  *(end + 1) = '\0';
  return str;
}

// parse_config_line: traite une ligne de configuration line et applique sont
// interprétation dans config
static int parse_config_line(server_config_t *config, char *line) {
  char *trimmed = trim(line);
  if (trimmed[0] == '#' || trimmed[0] == '\0' || trimmed[0] == ';') {
    return 0;
  }
  char *equals = strchr(trimmed, '=');
  if (equals == nullptr) {
    return 0;
  }
  *equals = '\0';
  char *key = trim(trimmed);
  char *value = trim(equals + 1);

  if (strcmp(key, "max_workers") == 0) {
    config->max_workers = atoi(value);
  } else if (strcmp(key, "min_threads") == 0) {
    config->min_threads = atoi(value);
  } else if (strcmp(key, "max_threads") == 0) {
    config->max_threads = atoi(value);
  }
  return 0;
}

int config_load(server_config_t *config, const char *filepath) {
  int fd = open(filepath, O_RDONLY);
  if (fd == -1) {
    return -1;
  }
  config_init_default(config);

  char line[MAX_LINE_LENGTH];
  size_t line_pos = 0;
  char c;
  ssize_t n;

  while ((n = read(fd, &c, 1)) > 0) {
    if (c == '\n') {
      line[line_pos] = '\0';
      parse_config_line(config, line);
      line_pos = 0;
    } else if (line_pos < MAX_LINE_LENGTH - 1) {
      line[line_pos++] = c;
    }
  }

  if (line_pos > 0) {
    line[line_pos] = '\0';
    parse_config_line(config, line);
  }
  close(fd);
  config->is_valid = config_validate(config);
  return config->is_valid ? 0 : -1;
}

bool config_validate(server_config_t *config) {
  // Vérifier max_workers
  if (config->max_workers < 1 || config->max_workers > ABSOLUTE_MAX_WORKERS) {
    fprintf(stderr,
            "Config error: max_workers must be between 1 and %d (got %d)\n",
            ABSOLUTE_MAX_WORKERS, config->max_workers);
    return false;
  }

  // Vérifier min_threads
  if (config->min_threads < ABSOLUTE_MIN_THREADS ||
      config->min_threads > ABSOLUTE_MAX_THREADS) {
    fprintf(stderr,
            "Config error: min_threads must be between %d and %d (got %d)\n",
            ABSOLUTE_MIN_THREADS, ABSOLUTE_MAX_THREADS, config->min_threads);
    return false;
  }

  // Vérifier max_threads
  if (config->max_threads < ABSOLUTE_MIN_THREADS ||
      config->max_threads > ABSOLUTE_MAX_THREADS) {
    fprintf(stderr,
            "Config error: max_threads must be between %d and %d (got %d)\n",
            ABSOLUTE_MIN_THREADS, ABSOLUTE_MAX_THREADS, config->max_threads);
    return false;
  }

  // Vérifier cohérence min <= max
  if (config->min_threads > config->max_threads) {
    fprintf(stderr,
            "Config error: min_threads (%d) cannot be greater than max_threads "
            "(%d)\n",
            config->min_threads, config->max_threads);
    return false;
  }

  return true;
}

int config_get_thread_count(const server_config_t *config, off_t file_size) {
  if (!config->is_valid) {
    return DEFAULT_MIN_THREADS;
  }
  int thread_count =
      config->min_threads +
      (int)((file_size * (config->max_threads - config->min_threads)) /
            MAX_SIZE_FILE);
  if (thread_count < config->min_threads) {
    thread_count = config->min_threads;
  }
  if (thread_count > config->max_threads) {
    thread_count = config->max_threads;
  }
  return thread_count;
}