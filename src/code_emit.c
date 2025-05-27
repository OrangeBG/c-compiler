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
    case ASM_INSTRUCTION_CMP:
      fprintf(file, "\tcmpl\t");
      save_assembly_file(asm_node->data.instruction_cmp.operand_1, file);
      fprintf(file, ", ");
      save_assembly_file(asm_node->data.instruction_cmp.operand_2, file);
      fprintf(file, "\n");
      break;
    case ASM_INSTRUCTION_JMP:
      fprintf(file, "\tjmp\tL%s\n", asm_node->data.instruction_label.identifier);
      break;
    case ASM_INSTRUCTION_JMPCC:
      fprintf(file, "\tj");

      switch (asm_node->data.instruction_jmp_cc.condition_code) {
        case ASM_CONDITION_EQUAL:         fprintf(file, "e"); break;
        case ASM_CONDITION_NOT_EQUAL:     fprintf(file, "ne"); break;
        case ASM_CONDITION_GREATER:       fprintf(file, "g"); break;
        case ASM_CONDITION_GREATER_EQUAL: fprintf(file, "ge"); break;
        case ASM_CONDITION_LESS:          fprintf(file, "l"); break;
        case ASM_CONDITION_LESS_EQUAL:    fprintf(file, "le"); break;
      }
      fprintf(file, "\tL%s\n", asm_node->data.instruction_jmp_cc.identifier);
      break;
    case ASM_INSTRUCTION_SETCC:
      fprintf(file, "\tset");

      switch (asm_node->data.instruction_set_cc.condition_code) {
        case ASM_CONDITION_EQUAL:         fprintf(file, "e"); break;
        case ASM_CONDITION_NOT_EQUAL:     fprintf(file, "ne"); break;
        case ASM_CONDITION_GREATER:       fprintf(file, "g"); break;
        case ASM_CONDITION_GREATER_EQUAL: fprintf(file, "ge"); break;
        case ASM_CONDITION_LESS:          fprintf(file, "l"); break;
        case ASM_CONDITION_LESS_EQUAL:    fprintf(file, "le"); break;
      }
      fprintf(file, "\t");
      //1 Byte name registers for set cc
      if (asm_node->data.instruction_set_cc.operand->type == ASM_OPERAND_REGISTER) {
      switch (asm_node->data.instruction_set_cc.operand->data.operand_register.op_register) {
        case ASM_REGISTER_R10:
          printf("%%r10b");
          break;
        case ASM_REGISTER_R11:
          printf("%%r11b");
          break;
        case ASM_REGISTER_AX:
          printf("%%al");
          break;
        case ASM_REGISTER_DX:
          printf("%%dl");
          break;
      }
      } else {
        save_assembly_file(asm_node->data.instruction_set_cc.operand, file);
      }
      fprintf(file, "\n");
      break;
    case ASM_INSTRUCTION_LABEL:
      fprintf(file, "L%s:\n", asm_node->data.instruction_label.identifier);
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
        case ASM_BINARY_ADD:                  fprintf(file, "\taddl\t"); break;
        case ASM_BINARY_SUB:                  fprintf(file, "\tsubl\t"); break;
        case ASM_BINARY_MULT:                 fprintf(file, "\timull\t"); break;        
        case ASM_BINARY_BITWISE_AND:          fprintf(file, "\tandl\t"); break;
        case ASM_BINARY_BITWISE_OR:           fprintf(file, "\torl\t"); break;
        case ASM_BINARY_BITWISE_XOR:          fprintf(file, "\txorl\t"); break;
        case ASM_BINARY_BITWISE_LEFT_SHIFT:   fprintf(file, "\tshll\t"); break;
        case ASM_BINARY_BITWISE_RIGHT_SHIFT:  fprintf(file, "\tshrl\t"); break;
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

void print_code_emit(FILE *file) { 
  char file_char;

  while((file_char = fgetc(file)) != EOF) {
    printf("%c", file_char);
  }
}
