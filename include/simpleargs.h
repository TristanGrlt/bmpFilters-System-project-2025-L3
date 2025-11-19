// simpleargs: simpleargs module (simple arguments) for command-line argument
// processing.
//
// Provided under an MIT License. See end of file for details.
//
// See github.com/TristanGrlt/simpleargs for documentation and examples.
//
//  A simpleargs module allows declarative definition of command-line arguments
//    (required, optional, boolean), their automatic parsing, constraint
//    validation, and automatic generation of formatted help messages.
//
//  General operation:
//  - arguments are defined by the user via macros before including this header.
//      These macros (SIMPLEARGS_REQUIRED_ARGS, SIMPLEARGS_OPTIONAL_ARGS,
//      SIMPLEARGS_BOOLEAN_ARGS) contain calls to the argument definition macros
//      specific to each type;
//  - the definition macros (SIMPLEARGS_REQUIRED_*_ARG,
//      SIMPLEARGS_OPTIONAL_*_ARG, SIMPLEARGS_BOOLEAN_ARG) are reused multiple
//      times by the module with different definitions of their corresponding
//      internal macros (__SIMPLEARGS_REQUIRED_ARG, __SIMPLEARGS_OPTIONAL_ARG,
//      __SIMPLEARGS_BOOLEAN_ARG) to successively generate: argument counting,
//      the data structure, default initialization, parsing code, and help
//      formatting;
//  - the automatically generated simpleargs_args_t structure contains a member
//      for each defined argument. The type of each member matches the type
//      specified when defining the argument;
//  - argument parsing (function easyargs_parse_args) checks for the presence of
//      required arguments, applies typed parsers, validates defined
//      constraints, and automatically handles the help option;
//  - type parsers (simpleargs_parse_*) convert strings to typed values with
//      limit validation and error handling via errno. They set an error message
//      in case of conversion failure;
//  - the option prefixes (SIMPLEARGS_SHORT_PREFIX, SIMPLEARGS_LONG_PREFIX) as
//      well as the help options (SIMPLEARGS_SHORT_HELP, SIMPLEARGS_LONG_HELP)
//      can be customized by defining them before including this header;
//  - the easyargs_print_help function automatically generates a formatted help
//      message from the metadata (labels, descriptions, default values)
//      provided when defining arguments;
//  - options can be specified in short form (-t), long form (--threads), with
//      an attached value (-t4, --threads=4), or separated (-t 4, --threads 4);
//  - boolean arguments are flags that do not require a value. Their presence
//      sets them to true;
//  - constraints are boolean expressions evaluated after parsing where `value`
//      represents the parsed value. On failure, an error message including the
//      value is displayed;
//  - all parsing functions return false on error and display an appropriate
//      error message to stderr via ERROR_MESSAGE;
//  - the order of required arguments is determined by their definition order in
//      SIMPLEARGS_REQUIRED_ARGS. Optional and boolean arguments can appear in
//      any order after the required arguments;
//  - the module makes extensive use of preprocessor metaprogramming.

#ifndef SIMPLEARGS_H
#define SIMPLEARGS_H

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ============================================================================
// CONFIGURATION MACROS
// ============================================================================

// Customizable option prefixes
#ifndef SIMPLEARGS_SHORT_PREFIX
#define SIMPLEARGS_SHORT_PREFIX "-"
#endif

#ifndef SIMPLEARGS_LONG_PREFIX
#define SIMPLEARGS_LONG_PREFIX "--"
#endif

#ifndef SIMPLEARGS_SHORT_HELP
#define SIMPLEARGS_SHORT_HELP "h"
#endif

#ifndef SIMPLEARGS_LONG_HELP
#define SIMPLEARGS_LONG_HELP "help"
#endif

// ============================================================================
// ARGUMENT DEFINITION MACROS
// ============================================================================

// __SIMPLEARGS_REQUIRED_ARG(type, name, label, description, parser, constraint)
// is a macro function used internally that should not be set by the client. It
// is defined and undefined multiple times to perform various actions. This
// macro takes 6 parameters:
// - type: the type of the desired value
// - name: the name used in code to reference the desired value
// - label: the label used in help or usage messages for this argument
// - description: the description shown in help to explain the argument
// - parser: the function used to parse the value when read from a string
// - constraint: a boolean expression indicating the constraint to check
//   when parsing arguments. In constraint, `value` refers to the value
//   provided by the user (i.e. value > 0)
// - formatter: the formatter to use in printf related fonctions

// The SIMPLEARGS_REQUIRED_<type>_ARG(name, label, description, constraint)
// macros functions are used by the client to define required arguments. Each of
// these macros takes 4 parameters:
// - name: the name used in code to reference the desired value
// - label: the label used in help or usage messages for this argument
// - description: the description shown in help to explain the argument
// - constraint: a boolean expression indicating the constraint to check
//   when parsing arguments. In constraint, `value` refers to the value
//   provided by the user (i.e value > 0)

#define SIMPLEARGS_REQUIRED_STRING_ARG(name, label, description, constraint)   \
  __SIMPLEARGS_REQUIRED_ARG(const char *, name, label, description,            \
                            simpleargs_parse_string, constraint, "%s")

#define SIMPLEARGS_REQUIRED_CHAR_ARG(name, label, description, constraint)     \
  __SIMPLEARGS_REQUIRED_ARG(char, name, label, description,                    \
                            simpleargs_parse_char, constraint, "%c")

#define SIMPLEARGS_REQUIRED_INT_ARG(name, label, description, constraint)      \
  __SIMPLEARGS_REQUIRED_ARG(int, name, label, description,                     \
                            simpleargs_parse_int, constraint, "%d")

#define SIMPLEARGS_REQUIRED_UINT_ARG(name, label, description, constraint)     \
  __SIMPLEARGS_REQUIRED_ARG(unsigned int, name, label, description,            \
                            simpleargs_parse_uint, constraint, "%u")

#define SIMPLEARGS_REQUIRED_LONG_ARG(name, label, description, constraint)     \
  __SIMPLEARGS_REQUIRED_ARG(long, name, label, description,                    \
                            simpleargs_parse_long, constraint, "%ld")

#define SIMPLEARGS_REQUIRED_ULONG_ARG(name, label, description, constraint)    \
  __SIMPLEARGS_REQUIRED_ARG(unsigned long, name, label, description,           \
                            simpleargs_parse_ulong, constraint, "%lu")

#define SIMPLEARGS_REQUIRED_LLONG_ARG(name, label, description, constraint)    \
  __SIMPLEARGS_REQUIRED_ARG(long long, name, label, description,               \
                            simpleargs_parse_llong, constraint, "%lld")

#define SIMPLEARGS_REQUIRED_ULLONG_ARG(name, label, description, constraint)   \
  __SIMPLEARGS_REQUIRED_ARG(unsigned long long, name, label, description,      \
                            simpleargs_parse_ullong, constraint, "%llu")

#define SIMPLEARGS_REQUIRED_SIZE_ARG(name, label, description, constraint)     \
  __SIMPLEARGS_REQUIRED_ARG(size_t, name, label, description,                  \
                            simpleargs_parse_ullong, constraint, "%zu")

#define SIMPLEARGS_REQUIRED_FLOAT_ARG(name, label, description, constraint,    \
                                      precision)                               \
  __SIMPLEARGS_REQUIRED_ARG(float, name, label, description,                   \
                            simpleargs_parse_float, constraint,                \
                            "%." #precision "g")

#define SIMPLEARGS_REQUIRED_DOUBLE_ARG(name, label, description, constraint,   \
                                       precision)                              \
  __SIMPLEARGS_REQUIRED_ARG(double, name, label, description,                  \
                            simpleargs_parse_double, constraint,               \
                            "%." #precision "g")

// __SIMPLEARGS_OPTIONAL_ARG(type, name, default, short_flag, long_flag, label,
// description, parser, constraint) is a macro function used internally that
// should not be set by the client. It is defined and undefined multiple times
// to perform various actions. This macro takes 9 parameters:
// - type: the type of the desired value
// - name: the name used in code to reference the desired value
// - dedault: the default value used if the argument is not changed by the user
// - short_flag: the short flag used in the commande line to set the value
//   (i.e -t)
// - long_flag: the long flag used in the commande line to set the value
//   (i.e --thread)
// - label: the label used in help or usage messages for this argument
// - description: the description shown in help to explain the argument
// - parser: the function used to parse the value when read from a string
// - constraint: a boolean expression indicating the constraint to check
//   when parsing arguments. In constraint, `value` refers to the value
//   provided by the user (i.e. value > 0)
// - formatter: the formatter to use in printf related fonctions

// SIMPLEARGS_OPTIONAL_<type>_ARG(name, default, short_flag, long_flag,
// label, description, constraint)
// macros functions are used by the client to define optional arguments with a
// default value if not set. Each of these macros takes 7 parameters:
// - name: the name used in code to reference the desired value
// - dedault: the default value used if the argument is not changed by the user
// - short_flag: the short flag used in the commande line to set the value
//   (i.e t)
// - long_flag: the long flag used in the commande line to set the value
//   (i.e thread)
// - label: the label used in help or usage messages for this argument
// - description: the description shown in help to explain the argument
// - constraint: a boolean expression indicating the constraint to check
//   when parsing arguments. In constraint, `value` refers to the value
//   provided by the user (i.e value > 0)

#define SIMPLEARGS_OPTIONAL_STRING_ARG(name, default, short_flag, long_flag,   \
                                       label, description, constraint)         \
  __SIMPLEARGS_OPTIONAL_ARG(const char *, name, default, short_flag,           \
                            long_flag, label, description,                     \
                            simpleargs_parse_string, constraint, "%s")

#define SIMPLEARGS_OPTIONAL_CHAR_ARG(name, default, short_flag, long_flag,     \
                                     label, description, constraint)           \
  __SIMPLEARGS_OPTIONAL_ARG(char, name, default, short_flag, long_flag, label, \
                            description, simpleargs_parse_char, constraint,    \
                            "%c")

#define SIMPLEARGS_OPTIONAL_INT_ARG(name, default, short_flag, long_flag,      \
                                    label, description, constraint)            \
  __SIMPLEARGS_OPTIONAL_ARG(int, name, default, short_flag, long_flag, label,  \
                            description, simpleargs_parse_int, constraint,     \
                            "%d")

#define SIMPLEARGS_OPTIONAL_UINT_ARG(name, default, short_flag, long_flag,     \
                                     label, description, constraint)           \
  __SIMPLEARGS_OPTIONAL_ARG(unsigned int, name, default, short_flag,           \
                            long_flag, label, description,                     \
                            simpleargs_parse_uint, constraint, "%u")

#define SIMPLEARGS_OPTIONAL_LONG_ARG(name, default, short_flag, long_flag,     \
                                     label, description, constraint)           \
  __SIMPLEARGS_OPTIONAL_ARG(long, name, default, short_flag, long_flag, label, \
                            description, simpleargs_parse_long, constraint,    \
                            "%ld")

#define SIMPLEARGS_OPTIONAL_ULONG_ARG(name, default, short_flag, long_flag,    \
                                      label, description, constraint)          \
  __SIMPLEARGS_OPTIONAL_ARG(unsigned long, name, default, short_flag,          \
                            long_flag, label, description,                     \
                            simpleargs_parse_ulong, constraint, "%lu")

#define SIMPLEARGS_OPTIONAL_LLONG_ARG(name, default, short_flag, long_flag,    \
                                      label, description, constraint)          \
  __SIMPLEARGS_OPTIONAL_ARG(long long, name, default, short_flag, long_flag,   \
                            label, description, simpleargs_parse_llong,        \
                            constraint, "%lld")

#define SIMPLEARGS_OPTIONAL_ULLONG_ARG(name, default, short_flag, long_flag,   \
                                       label, description, constraint)         \
  __SIMPLEARGS_OPTIONAL_ARG(unsigned long long, name, default, short_flag,     \
                            long_flag, label, description,                     \
                            simpleargs_parse_ullong, constraint, "%llu")

#define SIMPLEARGS_OPTIONAL_SIZE_ARG(name, default, short_flag, long_flag,     \
                                     label, description, constraint)           \
  __SIMPLEARGS_OPTIONAL_ARG(size_t, name, default, short_flag, long_flag,      \
                            label, description, simpleargs_parse_ullong,       \
                            constraint, "%zu")

#define SIMPLEARGS_OPTIONAL_FLOAT_ARG(name, default, short_flag, long_flag,    \
                                      label, description, constraint,          \
                                      precision)                               \
  __SIMPLEARGS_OPTIONAL_ARG(float, name, default, short_flag, long_flag,       \
                            label, description, simpleargs_parse_float,        \
                            constraint, "%." #precision "g")

#define SIMPLEARGS_OPTIONAL_DOUBLE_ARG(name, default, short_flag, long_flag,   \
                                       label, description, constraint,         \
                                       precision)                              \
  __SIMPLEARGS_OPTIONAL_ARG(double, name, default, short_flag, long_flag,      \
                            label, description, simpleargs_parse_double,       \
                            constraint, "%." #precision "g")

// __SIMPLEARGS_BOOLEAN_ARG(name, short_flag, long_flag, description)
// is a macro function used internally that should not be set by the client.
// This macro takes 4 parameters:
// - name: the name used in code to reference the boolean flag
// - short_flag: the short flag used in the command line (i.e v)
// - long_flag: the long flag used in the command line (i.e verbose)
// - description: the description shown in help to explain the flag

// SIMPLEARGS_BOOLEAN_ARG(name, short_flag, long_flag, description)
// is used by the client to define boolean flags (switches).
#define SIMPLEARGS_BOOLEAN_ARG(name, short_flag, long_flag, description)       \
  __SIMPLEARGS_BOOLEAN_ARG(name, short_flag, long_flag, description)

// ============================================================================
// EROR MESSAGE MACROS
// ============================================================================

// Error message macros
#define SIMPLEARGS_ERR_NULL_ARGS "Internal error: nullptr argc or argv"
#define SIMPLEARGS_ERR_NULL_STRUCT "Internal error: nullptr args structure"
#define SIMPLEARGS_ERR_MISSING_ARGS                                            \
  "Missing required arguments (expected %d, got %d)"
#define SIMPLEARGS_ERR_MISSING_REQUIRED                                        \
  "Missing required argument at position %d: <%s>"
#define SIMPLEARGS_ERR_INVALID_VALUE "Invalid value '%s' for argument <%s>: %s"
#define SIMPLEARGS_ERR_OPTION_REQUIRES_VALUE "Option '%s' requires a value"
#define SIMPLEARGS_ERR_UNKNOWN_OPTION "Unknown option: '%s'"
#define SIMPLEARGS_ERR_EMPTY_STRING "Empty string provided for argument <%s>"
#define SIMPLEARGS_ERR_CONSTRAINT_FAILED                                       \
  "Constraint failed for <%s>: value %s does not satisfy condition"
#define SIMPLEARGS_ERR_OVERFLOW "Value overflow for <%s>: '%s' is out of range"

// Parse error message macros
#define SIMPLEARGS_PARSE_ERR_INVALID_STRING "Empty or null string"
#define SIMPLEARGS_PARSE_ERR_INVALID_INT "Not a valid integer"
#define SIMPLEARGS_PARSE_ERR_OUT_OF_RANGE_INT "Value out of range for integer"
#define SIMPLEARGS_PARSE_ERR_INVALID_UINT "Not a valid unsigned integer"
#define SIMPLEARGS_PARSE_ERR_OUT_OF_RANGE_UINT                                 \
  "Value out of range for unsigned integer"
#define SIMPLEARGS_PARSE_ERR_INVALID_LONG "Not a valid long"
#define SIMPLEARGS_PARSE_ERR_OUT_OF_RANGE_LONG "Value out of range for long"
#define SIMPLEARGS_PARSE_ERR_INVALID_ULONG "Not a valid unsigned long"
#define SIMPLEARGS_PARSE_ERR_OUT_OF_RANGE_ULONG                                \
  "Value out of range for unsigned long"
#define SIMPLEARGS_PARSE_ERR_INVALID_LLONG "Not a valid long long"
#define SIMPLEARGS_PARSE_ERR_OUT_OF_RANGE_LLONG                                \
  "Value out of range for long long"
#define SIMPLEARGS_PARSE_ERR_INVALID_ULLONG "Not a valid unsigned long long"
#define SIMPLEARGS_PARSE_ERR_OUT_OF_RANGE_ULLONG                               \
  "Value out of range for unsigned long long"
#define SIMPLEARGS_PARSE_ERR_INVALID_FLOAT "Not a valid unsigned float"
#define SIMPLEARGS_PARSE_ERR_OUT_OF_RANGE_FLOAT "Value out of range for float"
#define SIMPLEARGS_PARSE_ERR_INVALID_DOUBLE "Not a valid double"
#define SIMPLEARGS_PARSE_ERR_OUT_OF_RANGE_DOUBLE "Value out of range for double"

// ============================================================================
// TOOLS
// ============================================================================

#define EXE(n) (strrchr((n), '/') != nullptr ? strrchr((n), '/') + 1 : (n))

#define ERROR_MESSAGE(exe, msg, ...)                                           \
  fprintf(stderr, "%s : " msg "\n", exe __VA_OPT__(, ) __VA_ARGS__)

// ============================================================================
// PARSER FUNCTIONS
// ============================================================================

// All parser functions take a string as input and parse it to the format
// corresponding to the function return value. If an error occurs, *err is set
// to an error message explaining the error; otherwise, *err is set to null.

// Character parser
static inline char simpleargs_parse_char(const char *s, const char **err) {
  if (s == nullptr || s[0] == '\0') {
    *err = SIMPLEARGS_PARSE_ERR_INVALID_STRING;
    return '\0';
  }
  *err = nullptr;
  return s[0];
}

// Integer parser
static inline int simpleargs_parse_int(const char *s, const char **err) {
  if (s == nullptr || s[0] == '\0') {
    *err = SIMPLEARGS_PARSE_ERR_INVALID_STRING;
    return 0;
  }
  char *endptr;
  errno = 0;
  long n = strtol(s, &endptr, 10);
  if (errno == ERANGE || n > INT_MAX || n < INT_MIN) {
    *err = SIMPLEARGS_PARSE_ERR_OUT_OF_RANGE_INT;
    return 0;
  }
  if (endptr == s || *endptr != '\0') {
    *err = SIMPLEARGS_PARSE_ERR_INVALID_INT;
    return 0;
  }
  *err = nullptr;
  return (int)n;
}

// Unsigned integer parser
static inline unsigned int simpleargs_parse_uint(const char *s,
                                                 const char **err) {
  if (s == nullptr || s[0] == '\0') {
    *err = SIMPLEARGS_PARSE_ERR_INVALID_STRING;
    return 0;
  }
  char *endptr;
  errno = 0;
  unsigned long n = strtoul(s, &endptr, 10);
  if (errno == ERANGE || n > UINT_MAX) {
    *err = SIMPLEARGS_PARSE_ERR_OUT_OF_RANGE_UINT;
    return 0;
  }
  if (endptr == s || *endptr != '\0') {
    *err = SIMPLEARGS_PARSE_ERR_INVALID_UINT;
    return 0;
  }
  *err = nullptr;
  return (unsigned int)n;
}

// Long parser
static inline long simpleargs_parse_long(const char *s, const char **err) {
  if (s == nullptr || s[0] == '\0') {
    *err = SIMPLEARGS_PARSE_ERR_INVALID_STRING;
    return 0;
  }
  char *endptr;
  errno = 0;
  long n = strtol(s, &endptr, 10);
  if (errno == ERANGE) {
    *err = SIMPLEARGS_PARSE_ERR_OUT_OF_RANGE_LONG;
    return 0;
  }
  if (endptr == s || *endptr != '\0') {
    *err = SIMPLEARGS_PARSE_ERR_INVALID_LONG;
    return 0;
  }
  *err = nullptr;
  return n;
}

// Unsigned long parser
static inline unsigned long simpleargs_parse_ulong(const char *s,
                                                   const char **err) {
  if (s == nullptr || s[0] == '\0') {
    *err = SIMPLEARGS_PARSE_ERR_INVALID_STRING;
    return 0;
  }
  char *endptr;
  errno = 0;
  unsigned long n = strtoul(s, &endptr, 10);
  if (errno == ERANGE) {
    *err = SIMPLEARGS_PARSE_ERR_OUT_OF_RANGE_ULONG;
    return 0;
  }
  if (endptr == s || *endptr != '\0') {
    *err = SIMPLEARGS_PARSE_ERR_INVALID_ULONG;
    return 0;
  }
  *err = nullptr;
  return n;
}

// Long long parser
static inline long long simpleargs_parse_llong(const char *s,
                                               const char **err) {
  if (s == nullptr || s[0] == '\0') {
    *err = SIMPLEARGS_PARSE_ERR_INVALID_STRING;
    return 0;
  }
  char *endptr;
  errno = 0;
  long long n = strtoll(s, &endptr, 10);
  if (errno == ERANGE) {
    *err = SIMPLEARGS_PARSE_ERR_OUT_OF_RANGE_LLONG;
    return 0;
  }
  if (endptr == s || *endptr != '\0') {
    *err = SIMPLEARGS_PARSE_ERR_INVALID_LLONG;
    return 0;
  }
  *err = nullptr;
  return n;
}

// Unsigned long long parser
static inline unsigned long long simpleargs_parse_ullong(const char *s,
                                                         const char **err) {
  if (s == nullptr || s[0] == '\0') {
    *err = SIMPLEARGS_PARSE_ERR_INVALID_STRING;
    return 0;
  }
  char *endptr;
  errno = 0;
  unsigned long long n = strtoull(s, &endptr, 10);
  if (errno == ERANGE) {
    *err = SIMPLEARGS_PARSE_ERR_OUT_OF_RANGE_ULLONG;
    return 0;
  }
  if (endptr == s || *endptr != '\0') {
    *err = SIMPLEARGS_PARSE_ERR_INVALID_ULLONG;
    return 0;
  }
  *err = nullptr;
  return n;
}

// Float parser
static inline float simpleargs_parse_float(const char *s, const char **err) {
  if (s == nullptr || s[0] == '\0') {
    *err = SIMPLEARGS_PARSE_ERR_INVALID_STRING;
    return 0.0f;
  }
  char *endptr;
  errno = 0;
  float n = strtof(s, &endptr);
  if (errno == ERANGE) {
    *err = SIMPLEARGS_PARSE_ERR_OUT_OF_RANGE_FLOAT;
    return 0.0f;
  }
  if (endptr == s || *endptr != '\0') {
    *err = SIMPLEARGS_PARSE_ERR_INVALID_FLOAT;
    return 0.0f;
  }
  *err = nullptr;
  return n;
}

// Double parser
static inline double simpleargs_parse_double(const char *s, const char **err) {
  if (s == nullptr || s[0] == '\0') {
    *err = SIMPLEARGS_PARSE_ERR_INVALID_STRING;
    return 0.0;
  }
  char *endptr;
  errno = 0;
  double n = strtod(s, &endptr);
  if (errno == ERANGE) {
    *err = SIMPLEARGS_PARSE_ERR_OUT_OF_RANGE_DOUBLE;
    return 0.0;
  }
  if (endptr == s || *endptr != '\0') {
    *err = SIMPLEARGS_PARSE_ERR_INVALID_DOUBLE;
    return 0.0;
  }
  *err = nullptr;
  return n;
}

// String parser
static inline const char *simpleargs_parse_string(const char *s,
                                                  const char **err) {
  if (s == nullptr || s[0] == '\0') {
    *err = SIMPLEARGS_PARSE_ERR_INVALID_STRING;
    return nullptr;
  }
  *err = nullptr;
  return s;
}

static inline void easyargs_print_help(const char *exec_name);

// easyargs_matches_bool_flag: checks if arg matches a boolean flag
static inline bool easyargs_matches_bool_flag(const char *arg,
                                              const char *short_flag,
                                              const char *long_flag) {
  size_t short_prefix_len = strlen(SIMPLEARGS_SHORT_PREFIX);
  if (strncmp(arg, SIMPLEARGS_SHORT_PREFIX, short_prefix_len) == 0) {
    const char *after_prefix = arg + short_prefix_len;
    if (strcmp(after_prefix, short_flag) == 0) {
      return true;
    }
  }
  size_t long_prefix_len = strlen(SIMPLEARGS_LONG_PREFIX);
  if (strncmp(arg, SIMPLEARGS_LONG_PREFIX, long_prefix_len) == 0) {
    const char *after_prefix = arg + long_prefix_len;
    if (strcmp(after_prefix, long_flag) == 0) {
      return true;
    }
  }
  return false;
}

//  easyargs_parse_option_value: returns the value associated with the short or
// long option indicated by short_flag and long_flag if it is present in the
// string pointed to by arg. Returns null and sets skip_next to 1 if the value
// is not in arg but the option exists anyway (the value associated with might
// be in the next arg processed). Returns null in all other cases.
static inline const char *easyargs_parse_option_value(const char *arg,
                                                      const char *short_flag,
                                                      const char *long_flag,
                                                      int *skip_next) {
  size_t short_prefix_len = strlen(SIMPLEARGS_SHORT_PREFIX);
  size_t short_flag_len = strlen(short_flag);
  if (strncmp(arg, SIMPLEARGS_SHORT_PREFIX, short_prefix_len) == 0) {
    const char *after_prefix = arg + short_prefix_len;
    if (strncmp(after_prefix, short_flag, short_flag_len) == 0) {
      const char *rest = after_prefix + short_flag_len;
      if (rest[0] == '=') {
        *skip_next = 0;
        return rest + 1;
      }
      if (rest[0] != '\0') {
        *skip_next = 0;
        return rest;
      }
      *skip_next = 1;
      return nullptr;
    }
  }
  size_t long_prefix_len = strlen(SIMPLEARGS_LONG_PREFIX);
  size_t long_flag_len = strlen(long_flag);
  if (strncmp(arg, SIMPLEARGS_LONG_PREFIX, long_prefix_len) == 0) {
    const char *after_prefix = arg + long_prefix_len;
    if (strncmp(after_prefix, long_flag, long_flag_len) == 0) {
      const char *rest = after_prefix + long_flag_len;
      if (rest[0] == '=') {
        *skip_next = 0;
        return rest + 1;
      }
      if (rest[0] != '\0') {
        *skip_next = 0;
        return rest;
      }
      *skip_next = 1;
      return nullptr;
    }
  }
  return nullptr;
}

// ============================================================================
// COUNT ARGUMENTS
// ============================================================================

#ifndef REQUIRED_ARG_COUNT
#define REQUIRED_ARG_COUNT 0
#endif

#ifdef SIMPLEARGS_REQUIRED_ARGS
#define __SIMPLEARGS_REQUIRED_ARG(...) +1
static const int easyargs_required_arg_count = 0 SIMPLEARGS_REQUIRED_ARGS;
#undef __SIMPLEARGS_REQUIRED_ARG
#else
static const int easyargs_required_arg_count = REQUIRED_ARG_COUNT;
#endif

#ifdef SIMPLEARGS_OPTIONAL_ARGS
#define __SIMPLEARGS_OPTIONAL_ARG(...) +1
static const int easyargs_optional_arg_count = 0 SIMPLEARGS_OPTIONAL_ARGS;
#undef __SIMPLEARGS_OPTIONAL_ARG
#else
static const int easyargs_optional_arg_count = 0;
#endif

#ifdef SIMPLEARGS_BOOLEAN_ARGS
#define __SIMPLEARGS_BOOLEAN_ARG(...) +1
static const int easyargs_boolean_arg_count = 0 SIMPLEARGS_BOOLEAN_ARGS;
#undef __SIMPLEARGS_BOOLEAN_ARG
#else
static const int easyargs_boolean_arg_count = 0;
#endif

// ============================================================================
// ARGS_T STRUCT
// ============================================================================

#define __SIMPLEARGS_REQUIRED_ARG(type, name, ...) type name;
#define __SIMPLEARGS_OPTIONAL_ARG(type, name, ...) type name;
#define __SIMPLEARGS_BOOLEAN_ARG(name, ...) bool name;

typedef struct {
#ifdef SIMPLEARGS_REQUIRED_ARGS
  SIMPLEARGS_REQUIRED_ARGS
#endif
#ifdef SIMPLEARGS_OPTIONAL_ARGS
  SIMPLEARGS_OPTIONAL_ARGS
#endif
#ifdef SIMPLEARGS_BOOLEAN_ARGS
  SIMPLEARGS_BOOLEAN_ARGS
#endif
} simpleargs_args_t;

#undef __SIMPLEARGS_REQUIRED_ARG
#undef __SIMPLEARGS_OPTIONAL_ARG
#undef __SIMPLEARGS_BOOLEAN_ARG

// ============================================================================
// DEFAULT ARGS BUILDER
// ============================================================================

static inline simpleargs_args_t easyargs_make_default_args(void) {
  simpleargs_args_t args = {
#define __SIMPLEARGS_REQUIRED_ARG(type, name, ...) .name = (type){0},
#define __SIMPLEARGS_OPTIONAL_ARG(type, name, default, ...) .name = default,
#define __SIMPLEARGS_BOOLEAN_ARG(name, ...) .name = false,
#ifdef SIMPLEARGS_REQUIRED_ARGS
      SIMPLEARGS_REQUIRED_ARGS
#endif
#ifdef SIMPLEARGS_OPTIONAL_ARGS
          SIMPLEARGS_OPTIONAL_ARGS
#endif
#ifdef SIMPLEARGS_BOOLEAN_ARGS
              SIMPLEARGS_BOOLEAN_ARGS
#endif
#undef __SIMPLEARGS_REQUIRED_ARG
#undef __SIMPLEARGS_OPTIONAL_ARG
#undef __SIMPLEARGS_BOOLEAN_ARG
  };
  return args;
}

// ============================================================================
// ARGUMENT PARSER
// ============================================================================

static inline bool easyargs_parse_args(int argc, char *argv[],
                                       simpleargs_args_t *args) {
  if (argc <= 0 || argv == nullptr) {
    ERROR_MESSAGE(EXE(argv[0]), SIMPLEARGS_ERR_NULL_ARGS);
    return false;
  }
  if (args == nullptr) {
    ERROR_MESSAGE(EXE(argv[0]), SIMPLEARGS_ERR_NULL_STRUCT);
    return false;
  }
  // Check for "--help" or "-h" (based on the corresponding macros)
  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], SIMPLEARGS_LONG_PREFIX SIMPLEARGS_LONG_HELP) == 0 ||
        strcmp(argv[i], SIMPLEARGS_SHORT_PREFIX SIMPLEARGS_SHORT_HELP) == 0) {
      easyargs_print_help(argv[0]);
      return false;
    }
  }
  // Check if enough required arguments
  if (argc < 1 + easyargs_required_arg_count) {
    ERROR_MESSAGE(EXE(argv[0]), SIMPLEARGS_ERR_MISSING_ARGS,
                  easyargs_required_arg_count, argc - 1);
    easyargs_print_help(argv[0]);
    return false;
  }
  // Parse required arguments
#ifdef SIMPLEARGS_REQUIRED_ARGS
  {
    int arg_index = 1;
    int req_count = 0;
    const char *err = nullptr;
#define __SIMPLEARGS_REQUIRED_ARG(type, name, label, description, parser,      \
                                  constraint, formatter)                       \
  do {                                                                         \
    if (arg_index >= argc) {                                                   \
      ERROR_MESSAGE(EXE(argv[0]), SIMPLEARGS_ERR_MISSING_REQUIRED,             \
                    req_count + 1, label);                                     \
      return false;                                                            \
    }                                                                          \
    type value = parser(argv[arg_index], &err);                                \
    if (err != nullptr) {                                                      \
      ERROR_MESSAGE(EXE(argv[0]), SIMPLEARGS_ERR_INVALID_VALUE,                \
                    argv[arg_index], label, err);                              \
      return false;                                                            \
    }                                                                          \
    if (!(constraint)) {                                                       \
      char val_str[256];                                                       \
      snprintf(val_str, sizeof(val_str), formatter, value);                    \
      ERROR_MESSAGE(EXE(argv[0]), SIMPLEARGS_ERR_CONSTRAINT_FAILED, label,     \
                    val_str);                                                  \
      return false;                                                            \
    }                                                                          \
    args->name = value;                                                        \
    arg_index++;                                                               \
    req_count++;                                                               \
  } while (0);
    SIMPLEARGS_REQUIRED_ARGS
#undef __SIMPLEARGS_REQUIRED_ARG
  }
#endif
  // Parse optional and boolean arguments
#define __SIMPLEARGS_OPTIONAL_ARG(type, name, default, short_flag, long_flag,  \
                                  label, description, parser, constraint,      \
                                  formatter)                                   \
  {                                                                            \
    int skip_next = 0;                                                         \
    const char *value_str = easyargs_parse_option_value(                       \
        argv[opt_index], short_flag, long_flag, &skip_next);                   \
    if (value_str != nullptr || skip_next) {                                   \
      if (value_str == nullptr) {                                              \
        if (opt_index + 1 >= argc) {                                           \
          ERROR_MESSAGE(EXE(argv[0]), SIMPLEARGS_ERR_OPTION_REQUIRES_VALUE,    \
                        argv[opt_index]);                                      \
          return false;                                                        \
        }                                                                      \
        value_str = argv[opt_index + 1];                                       \
      }                                                                        \
      const char *err = nullptr;                                               \
      type value = parser(value_str, &err);                                    \
      if (err != nullptr) {                                                    \
        ERROR_MESSAGE(EXE(argv[0]), SIMPLEARGS_ERR_INVALID_VALUE, value_str,   \
                      label, err);                                             \
        return false;                                                          \
      }                                                                        \
      if (!(constraint)) {                                                     \
        char val_str[256];                                                     \
        snprintf(val_str, sizeof(val_str), formatter, value);                  \
        ERROR_MESSAGE(EXE(argv[0]), SIMPLEARGS_ERR_CONSTRAINT_FAILED, label,   \
                      val_str);                                                \
        return false;                                                          \
      }                                                                        \
      args->name = value;                                                      \
      opt_index += skip_next;                                                  \
      continue;                                                                \
    }                                                                          \
  }
#define __SIMPLEARGS_BOOLEAN_ARG(name, short_flag, long_flag, description)     \
  if (easyargs_matches_bool_flag(argv[opt_index], short_flag, long_flag)) {    \
    args->name = true;                                                         \
    continue;                                                                  \
  }
  for (int opt_index = 1 + easyargs_required_arg_count; opt_index < argc;
       opt_index++) {
#ifdef SIMPLEARGS_OPTIONAL_ARGS
    SIMPLEARGS_OPTIONAL_ARGS
#endif
#ifdef SIMPLEARGS_BOOLEAN_ARGS
    SIMPLEARGS_BOOLEAN_ARGS
#endif
    ERROR_MESSAGE(EXE(argv[0]), SIMPLEARGS_ERR_UNKNOWN_OPTION, argv[opt_index]);
    return false;
  }
#undef __SIMPLEARGS_OPTIONAL_ARG
#undef __SIMPLEARGS_BOOLEAN_ARG
  return true;
}

// ============================================================================
// HELP PRINTER
// ============================================================================

static inline void easyargs_print_help(const char *exec_name) {
  printf("USAGE:\n");
  printf("\t%s ", exec_name);

#ifdef SIMPLEARGS_REQUIRED_ARGS
  if (easyargs_required_arg_count > 0 && easyargs_required_arg_count <= 3) {
#define __SIMPLEARGS_REQUIRED_ARG(type, name, label, ...) "<" label "> "
    printf(SIMPLEARGS_REQUIRED_ARGS);
#undef __SIMPLEARGS_REQUIRED_ARG
  } else if (easyargs_required_arg_count > 0) {
    printf("<ARGUMENTS> ");
  }
#endif

  if (easyargs_optional_arg_count + easyargs_boolean_arg_count <= 3) {
#ifdef SIMPLEARGS_OPTIONAL_ARGS
#define __SIMPLEARGS_OPTIONAL_ARG(type, name, default, short_flag, long_flag,  \
                                  label, ...)                                  \
  "[" SIMPLEARGS_SHORT_PREFIX short_flag "|" SIMPLEARGS_LONG_PREFIX long_flag  \
  " <" label ">] "
    printf(SIMPLEARGS_OPTIONAL_ARGS);
#undef __SIMPLEARGS_OPTIONAL_ARG
#endif

#ifdef SIMPLEARGS_BOOLEAN_ARGS
#define __SIMPLEARGS_BOOLEAN_ARG(name, short_flag, long_flag, ...)             \
  "[" SIMPLEARGS_SHORT_PREFIX short_flag "|" SIMPLEARGS_LONG_PREFIX long_flag  \
  "] "
    printf(SIMPLEARGS_BOOLEAN_ARGS);
#undef __SIMPLEARGS_BOOLEAN_ARG
#endif
  } else {
    printf("[OPTIONS]");
  }

  printf("\n\n");

  // Calculate max width for formatting
  int max_width = 0;

#ifdef SIMPLEARGS_REQUIRED_ARGS
#define __SIMPLEARGS_REQUIRED_ARG(type, name, label, ...)                      \
  do {                                                                         \
    int len = (int)strlen(label) + 2;                                          \
    if (len > max_width)                                                       \
      max_width = len;                                                         \
  } while (0);
  SIMPLEARGS_REQUIRED_ARGS
#undef __SIMPLEARGS_REQUIRED_ARG
#endif

#ifdef SIMPLEARGS_OPTIONAL_ARGS
#define __SIMPLEARGS_OPTIONAL_ARG(type, name, default, short_flag, long_flag,  \
                                  label, ...)                                  \
  do {                                                                         \
    int len = (int)strlen(SIMPLEARGS_SHORT_PREFIX short_flag) + 1 +            \
              (int)strlen(SIMPLEARGS_LONG_PREFIX long_flag) + 3 +              \
              (int)strlen(label) + 2;                                          \
    if (len > max_width)                                                       \
      max_width = len;                                                         \
  } while (0);
  SIMPLEARGS_OPTIONAL_ARGS
#undef __SIMPLEARGS_OPTIONAL_ARG
#endif

#ifdef SIMPLEARGS_BOOLEAN_ARGS
#define __SIMPLEARGS_BOOLEAN_ARG(name, short_flag, long_flag, ...)             \
  do {                                                                         \
    int len = (int)strlen(SIMPLEARGS_SHORT_PREFIX short_flag) + 1 +            \
              (int)strlen(SIMPLEARGS_LONG_PREFIX long_flag);                   \
    if (len > max_width)                                                       \
      max_width = len;                                                         \
  } while (0);
  SIMPLEARGS_BOOLEAN_ARGS
#undef __SIMPLEARGS_BOOLEAN_ARG
#endif

// Print arguments section
#ifdef SIMPLEARGS_REQUIRED_ARGS
  printf("ARGUMENTS:\n");
#define __SIMPLEARGS_REQUIRED_ARG(type, name, label, description, ...)         \
  printf("\t<%s>%*s\t%s\n", label, max_width - (int)strlen(label) - 2, "",     \
         description);
  SIMPLEARGS_REQUIRED_ARGS
#undef __SIMPLEARGS_REQUIRED_ARG
  printf("\n");
#endif

// Print options section
#if defined(SIMPLEARGS_OPTIONAL_ARGS) || defined(SIMPLEARGS_BOOLEAN_ARGS)
  printf("OPTIONS:\n");

  // Always print help option
  {
    const char *help_str =
        SIMPLEARGS_SHORT_PREFIX "h, " SIMPLEARGS_LONG_PREFIX "help";
    printf("\t%s%*s\tShow this help message\n", help_str,
           max_width - (int)strlen(help_str), "");
  }

#ifdef SIMPLEARGS_OPTIONAL_ARGS
#define __SIMPLEARGS_OPTIONAL_ARG(type, name, default, short_flag, long_flag,  \
                                  label, description, parser, constraint,      \
                                  formatter)                                   \
  do {                                                                         \
    char opt_str[256];                                                         \
    snprintf(opt_str, sizeof(opt_str),                                         \
             SIMPLEARGS_SHORT_PREFIX "%s, " SIMPLEARGS_LONG_PREFIX "%s <%s>",  \
             short_flag, long_flag, label);                                    \
    printf("\t%s%*s\t%s (default: " formatter ")\n", opt_str,                  \
           max_width - (int)strlen(opt_str), "", description, default);        \
  } while (0);
  SIMPLEARGS_OPTIONAL_ARGS
#undef __SIMPLEARGS_OPTIONAL_ARG
#endif

#ifdef SIMPLEARGS_BOOLEAN_ARGS
#define __SIMPLEARGS_BOOLEAN_ARG(name, short_flag, long_flag, description)     \
  do {                                                                         \
    char opt_str[256];                                                         \
    snprintf(opt_str, sizeof(opt_str),                                         \
             SIMPLEARGS_SHORT_PREFIX "%s, " SIMPLEARGS_LONG_PREFIX "%s",       \
             short_flag, long_flag);                                           \
    printf("\t%s%*s\t%s\n", opt_str, max_width - (int)strlen(opt_str), "",     \
           description);                                                       \
  } while (0);
  SIMPLEARGS_BOOLEAN_ARGS
#undef __SIMPLEARGS_BOOLEAN_ARG
#endif

#endif
}

#endif