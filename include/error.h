#ifndef ERROR
#define ERROR

#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_RESET   "\x1b[0m" 

#define ASSERT_ENABLED 1

#if ASSERT_ENABLED 
#define assert(condition, message) \
  do { \
    if (!(condition)) {\
      printf(ANSI_COLOR_RED "PANIC: %s (%s: %d)\n" ANSI_COLOR_RESET,  message, __FILE__, __LINE__);\
      exit(1);\
    }\
  } while(0); \
  
#else
  #define assert(compare, message)  // Empty macro if not enabled
#endif
#endif
