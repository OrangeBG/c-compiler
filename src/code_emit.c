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

      //Function prologue
      printf("\tpushq\t%%rbp\n");
      printf("\tmovq\t%%rsp, %%rbp\n");

      for (int i = 0; i < asm_node->data.function.instruction_count; i++) {
        print_code_emit(&asm_node->data.function.instructions[i]);
      }
      break;
    case ASM_INSTRUCTION_MOV:
      printf("\tmovl\t");
      print_code_emit(asm_node->data.instruction_mov.source);
      printf(", ");
      print_code_emit(asm_node->data.instruction_mov.destination);      
      printf("\n");
      break;
    case ASM_INSTRUCTION_UNARY:
      if (asm_node->data.instruction_unary.operator == ASM_UNARY_NEG) {
        printf("\tnegl\t");
      } else {
        printf("\tnotl\t");
      }
      print_code_emit(asm_node->data.instruction_unary.operand);
      printf("\n");
      break;
    case ASM_INSTRUCTION_BINARY:
      switch (asm_node->data.instruction_binary.operator) {
        case ASM_BINARY_ADD:
          printf("\taddl\t");
          break;
        case ASM_BINARY_SUB:
          printf("\tsubl\t");
          break;
        case ASM_BINARY_MULT:
          printf("\tmull\t");
          break;        
      }
      print_code_emit(asm_node->data.instruction_binary.operand_1);
      printf(", ");
      print_code_emit(asm_node->data.instruction_binary.operand_2);
      printf("\n");
      break;
    case ASM_INSTRUCTION_CDQ:
      printf("\tcdq\n");
      break;
    case ASM_INSTRUCTION_IDIV:
      printf("\tidivl\t");
      print_code_emit(asm_node->data.instruction_idiv.operand);
      printf("\n");
      break;
    case ASM_INSTRUCTION_RET:
      //Function epilogue
      printf("\tmovq\t%%rbp, %%rsp\n");
      printf("\tpopq\t%%rbp\n");

      printf("\tret\n");
      break;
    case ASM_INSTRUCTION_ALLOCATE_STACK:
      printf("\tsubq\t$%d, %%rsp\n", asm_node->data.instruction_allocate_stack.bytes_to_subtract);
      break;
    case ASM_OPERAND_IMM:
      printf("$%d", asm_node->data.operand_imm.value);
      break;
    case ASM_OPERAND_REGISTER:
      switch (asm_node->data.operand_register.op_register) {
        case ASM_REGISTER_R10:
          printf("%%r10d");
          break;
        case ASM_REGISTER_R11:
          printf("%%r11d");
          break;
        case ASM_REGISTER_AX:
          printf("%%eax");
          break;
        case ASM_REGISTER_DX:
          printf("%%dx");
          break;
      }
      break;
    case ASM_OPERAND_STACK:
      printf("-%d(%%rbp)", asm_node->data.operand_stack.address);
      break;
    default:
      fprintf(stderr, "ERROR - Code Emit: No assembly type for '%d'\n", asm_node->type);
  }
}
