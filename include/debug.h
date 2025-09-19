#ifndef DEBUG
#define DEBUG

typedef enum {
  DEBUG_STEP_LEXER,
  DEBUG_STEP_PARSE,
  DEBUG_STEP_SA_VAR_RESOLUTION,
  DEBUG_STEP_SA_LOOP_LABEL,
  DEBUG_STEP_SA_TYPE_CHECK,
  DEBUG_STEP_INTERMEDIATE_REP,
  DEBUG_STEP_ASSEMBLY,
  DEBUG_STEP_CODE_EMIT
} DebugStep;

typedef enum {
  DEBUG_START,
  DEBUG_END
} DebugStartStop;

void debug_benchmark_step(DebugStep step, DebugStartStop start_stop);
void debug_print_benchmark(); 

#endif


