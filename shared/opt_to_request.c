#include <stdio.h>
#include <string.h>

#include "opt_to_request.h"

#define INPUT_ARG_LABEL "input"
#define INPUT_ARG_DESCRIPTION "Input image path"
#define OUTPUT_ARG_LABEL "output"
#define OUTPUT_ARG_DESCRIPTION "Output image path"

int process_options_to_request(int argc, char *argv[], arguments_t *arg) {
  // CHECK FOR HELP IN EACH ARGUMENT
  for (int i = 1; i < argc; ++i) {
    if (strcmp(argv[i],
               OPT_TO_REQUEST_SHORT_PREFIX OPT_TO_REQUEST_SHORT_HELP) == 0 ||
        strcmp(argv[i], OPT_TO_REQUEST_LONG_PREFIX OPT_TO_REQUEST_LONG_HELP) ==
            0) {
      print_help(argv[0]);
      return EXIT_FAILURE;
    }
  }

  if (argc < 4) {
    print_help(argv[0]);
    return EXIT_FAILURE;
  }

  arg->input = argv[1];
  arg->output = argv[2];

#define OPT_TO_REQUEST_SIMPLE_FILTER(filter_name, short_flag, long_flag, ...)  \
  else if (strcmp(argv[3], OPT_TO_REQUEST_SHORT_PREFIX short_flag) == 0) {     \
    arg->filter = filter_name;                                                 \
  }                                                                            \
  else if (strcmp(argv[3], OPT_TO_REQUEST_LONG_PREFIX long_flag) == 0) {       \
    arg->filter = filter_name;                                                 \
  }
#ifdef OPT_TO_REQUEST_SIMPLE_FILTERS
  if (false) {
  }
  OPT_TO_REQUEST_SIMPLE_FILTERS
  else {
    fprintf(stderr, "Error: Unknown filter '%s'\n", argv[3]);
    print_help(argv[0]);
    return EXIT_FAILURE;
  }
#endif
#undef OPT_TO_REQUEST_SIMPLE_FILTER

  return EXIT_SUCCESS;
}

void print_help(const char *exec_name) {
  // USAGE
  printf("USAGE:\n");
  printf("\t%s ", exec_name);
  printf("<%s> <%s> ", INPUT_ARG_LABEL, OUTPUT_ARG_LABEL);

#ifdef OPT_TO_REQUEST_SIMPLE_FILTERS
#define OPT_TO_REQUEST_SIMPLE_FILTER(filter_name, short_flag, long_flag, ...)  \
  printf("[%s%s|%s%s] ", OPT_TO_REQUEST_SHORT_PREFIX, short_flag,              \
         OPT_TO_REQUEST_LONG_PREFIX, long_flag);
  OPT_TO_REQUEST_SIMPLE_FILTERS
#undef OPT_TO_REQUEST_SIMPLE_FILTER
#endif

  printf("\n\n");

  // Calculate max width for formatting
  int max_width = 0;

  // Width for input/output arguments
  {
    int len = (int)strlen("<" INPUT_ARG_LABEL ">");
    if (len > max_width)
      max_width = len;
    len = (int)strlen("<" OUTPUT_ARG_LABEL ">");
    if (len > max_width)
      max_width = len;
  }

  // Width for help option
  {
    int len =
        (int)strlen(OPT_TO_REQUEST_SHORT_PREFIX OPT_TO_REQUEST_SHORT_HELP) + 2 +
        (int)strlen(OPT_TO_REQUEST_LONG_PREFIX OPT_TO_REQUEST_LONG_HELP);
    if (len > max_width)
      max_width = len;
  }

#ifdef OPT_TO_REQUEST_SIMPLE_FILTERS
#define OPT_TO_REQUEST_SIMPLE_FILTER(filter_name, short_flag, long_flag, ...)  \
  do {                                                                         \
    int len =                                                                  \
        (int)(strlen(short_flag) + strlen(OPT_TO_REQUEST_SHORT_PREFIX) + 2 +   \
              strlen(long_flag) + strlen(OPT_TO_REQUEST_LONG_PREFIX));         \
    if (len > max_width)                                                       \
      max_width = len;                                                         \
  } while (0);
  OPT_TO_REQUEST_SIMPLE_FILTERS
#undef OPT_TO_REQUEST_SIMPLE_FILTER
#endif

  // Print arguments section
  printf("ARGUMENTS:\n");
  printf("\t<%s>%*s\t%s\n", INPUT_ARG_LABEL,
         max_width - (int)strlen("<" INPUT_ARG_LABEL ">"), "",
         INPUT_ARG_DESCRIPTION);
  printf("\t<%s>%*s\t%s\n", OUTPUT_ARG_LABEL,
         max_width - (int)strlen("<" OUTPUT_ARG_LABEL ">"), "",
         OUTPUT_ARG_DESCRIPTION);
  printf("\n");

  // Print options section
  printf("OPTIONS:\n");

  // Help option
  {
    char help_str[256];
    snprintf(help_str, sizeof(help_str), "%s%s, %s%s",
             OPT_TO_REQUEST_SHORT_PREFIX, OPT_TO_REQUEST_SHORT_HELP,
             OPT_TO_REQUEST_LONG_PREFIX, OPT_TO_REQUEST_LONG_HELP);
    printf("\t%s%*s\tShow this help message\n", help_str,
           max_width - (int)strlen(help_str), "");
  }

  // Filter options
#ifdef OPT_TO_REQUEST_SIMPLE_FILTERS
#define OPT_TO_REQUEST_SIMPLE_FILTER(filter_name, short_flag, long_flag,       \
                                     description, ...)                         \
  do {                                                                         \
    char opt_str[256];                                                         \
    snprintf(opt_str, sizeof(opt_str), "%s%s, %s%s",                           \
             OPT_TO_REQUEST_SHORT_PREFIX, short_flag,                          \
             OPT_TO_REQUEST_LONG_PREFIX, long_flag);                           \
    printf("\t%s%*s\t%s\n", opt_str, max_width - (int)strlen(opt_str), "",     \
           description);                                                       \
  } while (0);
  OPT_TO_REQUEST_SIMPLE_FILTERS
#undef OPT_TO_REQUEST_SIMPLE_FILTER
#endif
}