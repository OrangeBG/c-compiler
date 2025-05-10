#ifndef CODE_EMIT
#define CODE_EMIT

#include "../include/assembly.h"
#include <stdio.h>

void print_code_emit(AsmNode *asm_node); 
void save_assembly_file(AsmNode *asm_node, FILE *file);

#endif
