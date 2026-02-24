#ifndef ERROR
#define ERROR

#include <stdio.h>
#include <stdlib.h>

#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_RESET   "\x1b[0m" 

#define ASSERT_ENABLED 1

#if ASSERT_ENABLED 
#define assert(condition, message) \
  do { \
    if (!(condition)) {\
      fprintf(stderr, ANSI_COLOR_RED "PANIC: %s (%s: %d)\n" ANSI_COLOR_RESET, message, __FILE__, __LINE__);\
      exit(1);\
    }\
  } while(0); \
  
#else
  #define assert(compare, message)  // Empty macro if not enabled
#endif

#define panic(message, ...) \
  do { \
      fprintf(stderr, ANSI_COLOR_RED "PANIC: ");\
      fprintf(stderr, message, ##__VA_ARGS__);\
      fprintf(stderr, " (%s: %d)\n" ANSI_COLOR_RESET, __FILE__, __LINE__);\
      exit(1);\
    } while(0); \

#define input_error(message, ...) \
  do { \
      fprintf(stderr, ANSI_COLOR_RED "Error: ");\
      fprintf(stderr, message, ##__VA_ARGS__);\
      fprintf(stderr, "\n" ANSI_COLOR_RESET);\
      exit(1);\
    } while(0); \

#define input_error_with_line(message, line_number, ...) \
  do { \
      fprintf(stderr, ANSI_COLOR_RED "Error: ");\
      fprintf(stderr, message, ##__VA_ARGS__);\
      fprintf(stderr, " (line %d)", line_number);\
      fprintf(stderr, "\n" ANSI_COLOR_RESET);\
      exit(1);\
    } while(0); \

#endif
