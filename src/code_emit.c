#include <stdio.h>
#include "../include/code_emit.h"

void print_code_emit(AsmNode *asm_node) { 
  switch (asm_node->type) {
    case ASM_PROGRAM:
      print_code_emit(asm_node->data.program.function);
      break;
    case ASM_FUNCTION:
      printf("\t.globl %s\n", asm_node->data.function.name);
      printf("%s:\n", asm_node->data.function.name);

      for (int i = 0; i < asm_node->data.function.instruction_count; i++) {
        switch (asm_node->data.function.instructions[i].type) {
          case ASM_INSTRUCTION_MOV:
            printf("\tmovl\t%d, %%eax\n", asm_node->data.function.instructions[i].data.instruction_mov.source->data.operand_imm.value);

            break;
          case ASM_INSTRUCTION_RET:
            printf("\tret\n");
            break;
          default:
            fprintf(stderr, "ERROR - Code Emit: No instruction type support for '%d'", asm_node->data.function.instructions[i].type);
            break;
        }
      }
      break;
    default:
      fprintf(stderr, "ERROR - Code Emit: No assembly type for '%d'", asm_node->type);
  }
}
