#ifndef CODE_EMIT
#define CODE_EMIT

#include "../include/assembly.h"
#include <stdio.h>

void print_code_emit(FILE *file); 
void save_assembly_file(AsmNode *asm_node, FILE *file);

#endif
