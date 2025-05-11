#include <stdio.h>
#include "../include/code_emit.h"

void save_assembly_file(AsmNode *asm_node, FILE *file) {
  switch (asm_node->type) {
    case ASM_PROGRAM:
      save_assembly_file(asm_node->data.program.function, file);
      break;
    case ASM_FUNCTION:
      fprintf(file, "\t.globl %s\n", asm_node->data.function.name);
      fprintf(file, "%s:\n", asm_node->data.function.name);

      //Function prologue
      fprintf(file, "\tpushq\t%%rbp\n");
      fprintf(file, "\tmovq\t%%rsp, %%rbp\n");

      for (int i = 0; i < asm_node->data.function.instruction_count; i++) {
        save_assembly_file(&asm_node->data.function.instructions[i], file);
      }
      break;
    case ASM_INSTRUCTION_MOV:
      fprintf(file, "\tmovl\t");
      save_assembly_file(asm_node->data.instruction_mov.source, file);
      fprintf(file, ", ");
      save_assembly_file(asm_node->data.instruction_mov.destination, file);      
      fprintf(file, "\n");
      break;
    case ASM_INSTRUCTION_UNARY:
      if (asm_node->data.instruction_unary.operator == ASM_UNARY_NEG) {
        fprintf(file, "\tnegl\t");
      } else {
        fprintf(file, "\tnotl\t");
      }
      save_assembly_file(asm_node->data.instruction_unary.operand, file);
      fprintf(file, "\n");
      break;
    case ASM_INSTRUCTION_BINARY:
      switch (asm_node->data.instruction_binary.operator) {
        case ASM_BINARY_ADD:
          fprintf(file, "\taddl\t");
          break;
        case ASM_BINARY_SUB:
          fprintf(file, "\tsubl\t");
          break;
        case ASM_BINARY_MULT:
          fprintf(file, "\tmull\t");
          break;        
        case ASM_BINARY_BITWISE_AND:
          fprintf(file, "\tandl\t");
          break;
        case ASM_BINARY_BITWISE_OR:
          fprintf(file, "\torl\t");
          break;
        case ASM_BINARY_BITWISE_XOR:
          fprintf(file, "\txorl\t");
          break;
        case ASM_BINARY_BITWISE_LEFT_SHIFT:
          fprintf(file, "\tshll\t");
          break;
        case ASM_BINARY_BITWISE_RIGHT_SHIFT:
          fprintf(file, "\tshrl\t");
          break;
      }
      save_assembly_file(asm_node->data.instruction_binary.operand_1, file);
      fprintf(file, ", ");
      save_assembly_file(asm_node->data.instruction_binary.operand_2, file);
      fprintf(file, "\n");
      break;
    case ASM_INSTRUCTION_CDQ:
      fprintf(file, "\tcdq\n");
      break;
    case ASM_INSTRUCTION_IDIV:
      fprintf(file, "\tidivl\t");
      save_assembly_file(asm_node->data.instruction_idiv.operand, file);
      fprintf(file, "\n");
      break;
    case ASM_INSTRUCTION_RET:
      //Function epilogue
      fprintf(file, "\tmovq\t%%rbp, %%rsp\n");
      fprintf(file, "\tpopq\t%%rbp\n");
      fprintf(file, "\tret\n");
      break;
    case ASM_INSTRUCTION_ALLOCATE_STACK:
      fprintf(file, "\tsubq\t$%d, %%rsp\n", asm_node->data.instruction_allocate_stack.bytes_to_subtract);
      break;
    case ASM_OPERAND_IMM:
      fprintf(file, "$%d", asm_node->data.operand_imm.value);
      break;
    case ASM_OPERAND_REGISTER:
      switch (asm_node->data.operand_register.op_register) {
        case ASM_REGISTER_R10:
          fprintf(file, "%%r10d");
          break;
        case ASM_REGISTER_R11:
          fprintf(file, "%%r11d");
          break;
        case ASM_REGISTER_AX:
          fprintf(file, "%%eax");
          break;
        case ASM_REGISTER_DX:
          fprintf(file, "%%dx");
          break;
      }
      break;
    case ASM_OPERAND_STACK:
      fprintf(file, "-%d(%%rbp)", asm_node->data.operand_stack.address);
      break;
    default:
      fprintf(stderr, "ERROR - Code Emit: No assembly type for '%d'\n", asm_node->type);
  }
}

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
        case ASM_BINARY_BITWISE_AND:
          printf("\tandl\t");
          break;
        case ASM_BINARY_BITWISE_OR:
          printf("\torl\t");
          break;
        case ASM_BINARY_BITWISE_XOR:
          printf("\txorl\t");
          break;
        case ASM_BINARY_BITWISE_LEFT_SHIFT:
          printf("\tshll\t");
          break;
        case ASM_BINARY_BITWISE_RIGHT_SHIFT:
          printf("\tshrl\t");
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
