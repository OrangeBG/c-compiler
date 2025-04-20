#include <stdio.h>
#include "../include/code_emit.h"

void print_code_emit(AsmNode *asm_node) { 
  switch (asm_node->type) {
    case ASM_PROGRAM:
      print_code_emit(asm_node->asm_program->function);
      break;
    case ASM_FUNCTION:
      printf("\t.globl <%s>\n", asm_node->asm_function->name);
      printf("<%s>\n", asm_node->asm_function->name);

      for (int i = 0; i < asm_node->asm_function->instruction_count; i++) {
        switch (asm_node->asm_function->instructions[i]->asm_instruction->type) {
          case ASM_INSTRUCTION_MOV:
            printf("\tmovl\t%d, %%TBD\n", asm_node->asm_function->instructions[i]->asm_instruction->instruction_mov->source->immediate_value->constant);
            break;
          case ASM_INSTRUCTION_RETURN:
            printf("\tret\n");
            break;
          default:
            fprintf(stderr, "ERROR - Code Emit: No instruction type support for '%d'", asm_node->asm_function->instructions[i]->asm_instruction->type);
            break;
        }
      }
      break;
    default:
      fprintf(stderr, "ERROR - Code Emit: No assembly type for '%d'", asm_node->type);
  }
}
