#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include "../include/debug.h"

typedef struct {
  double start;
  double end;
} Benchmark;

typedef struct {
  Benchmark lex;
  Benchmark parse;
  Benchmark sa_var_resolution;
  Benchmark sa_loop_label;
  Benchmark sa_type_check;
  Benchmark intermediate_rep;
  Benchmark assembly;
} StepBenchmarks;

StepBenchmarks step_benchmarks;

void debug_benchmark_step(DebugStep step, DebugStartStop start_stop) {
  double step_clock = (double)clock();

  switch (step) {
    case DEBUG_STEP_LEXER: start_stop == DEBUG_START ? step_benchmarks.lex.start = step_clock : (step_benchmarks.lex.end = step_clock); break; 
    case DEBUG_STEP_PARSE: start_stop == DEBUG_START ? step_benchmarks.parse.start = step_clock : (step_benchmarks.parse.end = step_clock); break;
    case DEBUG_STEP_SA_VAR_RESOLUTION: start_stop == DEBUG_START ? step_benchmarks.sa_var_resolution.start = step_clock : (step_benchmarks.sa_var_resolution.end = step_clock); break;
    case DEBUG_STEP_SA_LOOP_LABEL: start_stop == DEBUG_START ? step_benchmarks.sa_loop_label.start = step_clock : (step_benchmarks.sa_loop_label.end = step_clock); break;
    case DEBUG_STEP_SA_TYPE_CHECK: start_stop == DEBUG_START ? step_benchmarks.sa_type_check.start = step_clock : (step_benchmarks.sa_type_check.end = step_clock); break;
    case DEBUG_STEP_INTERMEDIATE_REP: start_stop == DEBUG_START ? step_benchmarks.intermediate_rep.start = step_clock : (step_benchmarks.intermediate_rep.end = step_clock); break;
    case DEBUG_STEP_ASSEMBLY: start_stop == DEBUG_START ? step_benchmarks.assembly.start = step_clock : (step_benchmarks.assembly.end = step_clock); break;
    default:
      fprintf(stderr, "ERROR - DEBUG: Unsupported debug step '%d' when attempting to benchmark", step);
      exit(1);
  }
}

void debug_print_benchmark() {
    printf("\n>> BENCHMARKS <<\n");
    printf("Lexer    : %f seconds\n", ((double) step_benchmarks.lex.end - (double)step_benchmarks.lex.start) / CLOCKS_PER_SEC);
    printf("Parser   : %f seconds\n", ((double) step_benchmarks.parse.end - (double)step_benchmarks.parse.start) / CLOCKS_PER_SEC);
    printf("SA Var Resolution: %f seconds\n", ((double) step_benchmarks.sa_var_resolution.end - (double)step_benchmarks.sa_var_resolution.start) / CLOCKS_PER_SEC);
    printf("SA Loop Labeling: %f seconds\n", ((double) step_benchmarks.sa_loop_label.end - (double)step_benchmarks.sa_loop_label.start) / CLOCKS_PER_SEC);
    printf("SA Type Check: %f seconds\n", ((double) step_benchmarks.sa_type_check.end - (double)step_benchmarks.sa_type_check.start) / CLOCKS_PER_SEC);
    printf("Int. Rep.: %f seconds\n", ((double) step_benchmarks.intermediate_rep.end - (double)step_benchmarks.intermediate_rep.start) / CLOCKS_PER_SEC);
    printf("Assembly : %f seconds\n", ((double) step_benchmarks.assembly.end - (double)step_benchmarks.assembly.start) / CLOCKS_PER_SEC);
    printf("Total Compiler Time: %f seconds\n", ((double) step_benchmarks.assembly.end - (double)step_benchmarks.lex.start) / CLOCKS_PER_SEC);
}
