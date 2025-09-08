#include <stdio.h>
#include "../include/code_emit.h"

char* get_8_byte_register(AsmRegisterType register_type); 
char* get_4_byte_register(AsmRegisterType register_type); 
char* get_1_byte_register(AsmRegisterType register_type); 
void print_instruction_suffix(FILE *file, AsmType type); 

void save_assembly_file(AsmNode *asm_node, FILE *file) {
  switch (asm_node->type) {
    case ASM_PROGRAM:
      for (int i = 0; i < asm_node->data.program.top_level_count; i++) {
        save_assembly_file(asm_node->data.program.top_level_pointers->asm_pointers[i], file);
      }

      #ifdef __linux__         
        fprintf(file, "\t.section .note.GNU-stack,\"\",@progbits\n");
      #endif
      break;
    case ASM_FUNCTION:
      if (asm_node->data.function.is_global) {        
        fprintf(file, "\t.globl %s\n", asm_node->data.function.name);
      }
      
      fprintf(file, "\t.text\n");

      #ifdef __APPLE__
        fprintf(file, "_%s:\n", asm_node->data.function.name);
      #else
        fprintf(file, "%s:\n", asm_node->data.function.name);
      #endif

      //Function prologue
      fprintf(file, "\tpushq\t%%rbp\n");
      fprintf(file, "\tmovq\t%%rsp, %%rbp\n");

      for (int i = 0; i < asm_node->data.function.instruction_count; i++) {
        save_assembly_file(asm_node->data.function.instruction_pointers->asm_pointers[i], file);
      }

      fprintf(file, "\n");
      break;
    case ASM_STATIC_VARIABLE:
      if (asm_node->data.static_variable.is_global) {
        fprintf(file, "\t.globl %s\n", asm_node->data.static_variable.identifier);
      }

      switch (asm_node->data.static_variable.static_variable_symbol->value_type) {
        case DECLARATION_SYMBOL_TYPE_INT:
          asm_node->data.static_variable.static_variable_symbol->static_initial_value.int_value == 0 ? fprintf(file, "\t.bss\n") : fprintf(file, "\t.data\n"); 
          break;
        case DECLARATION_SYMBOL_TYPE_LONG: 
          asm_node->data.static_variable.static_variable_symbol->static_initial_value.long_value == 0 ? fprintf(file, "\t.bss\n") : fprintf(file, "\t.data\n"); 
          break;
        default:
          fprintf(stderr, "ERROR - Code Emit: Static Variable Symbol Value Type '%d' not found", asm_node->data.static_variable.static_variable_symbol->value_type);
          exit(1);
      }      

      #ifdef __linux__
        fprintf(file, "\t.align 4\n");
      #endif 

      #ifdef __APPLE__
        fprintf(file, "\t.balign 4\n");
      #endif 

      fprintf(file, "%s:\n", asm_node->data.static_variable.identifier);

      switch (asm_node->data.static_variable.static_variable_symbol->value_type) {
        case DECLARATION_SYMBOL_TYPE_INT:
          asm_node->data.static_variable.static_variable_symbol->static_initial_value.int_value == 0 ? fprintf(file, "\t.zero 4\n") : fprintf(file, "\t.long %d\n", asm_node->data.static_variable.static_variable_symbol->static_initial_value.int_value); 
          break;
        case DECLARATION_SYMBOL_TYPE_LONG: 
          asm_node->data.static_variable.static_variable_symbol->static_initial_value.long_value == 0 ? fprintf(file, "\t.zero 8\n") : fprintf(file, "\t.quad %lu\n", asm_node->data.static_variable.static_variable_symbol->static_initial_value.long_value); 
          break;
        default:
          fprintf(stderr, "ERROR - Code Emit: Static Variable Symbol Value Type '%d' not found", asm_node->data.static_variable.static_variable_symbol->value_type);
          exit(1);
      }      

      fprintf(file, "\n");
      break;
    case ASM_INSTRUCTION_MOV:
      fprintf(file, "\tmov");

      print_instruction_suffix(file, asm_node->data.instruction_mov.assembly_type);      
      fprintf(file, "\t");
      
      save_assembly_file(asm_node->data.instruction_mov.source, file);
      fprintf(file, ", ");
      save_assembly_file(asm_node->data.instruction_mov.destination, file);      
      fprintf(file, "\n");
      break;
    case ASM_INSTRUCTION_MOVSX:
      fprintf(file, "\tmovslq\t");
      save_assembly_file(asm_node->data.instruction_movsx.source, file);
      fprintf(file, ", ");
      save_assembly_file(asm_node->data.instruction_movsx.destination, file);      
      fprintf(file, "\n");
      break;
    case ASM_INSTRUCTION_CMP:
      fprintf(file, "\tcmp");

      print_instruction_suffix(file, asm_node->data.instruction_cmp.assembly_type);
      fprintf(file, "\t");
      
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
        char *operand_register = get_1_byte_register(asm_node->data.instruction_set_cc.operand->data.operand_register.op_register);
        fprintf(file, "%s", operand_register);
      } else {
        save_assembly_file(asm_node->data.instruction_set_cc.operand, file);
      }
      fprintf(file, "\n");
      break;
    case ASM_INSTRUCTION_LABEL:
      fprintf(file, "L%s:\n", asm_node->data.instruction_label.identifier);
      break;
    case ASM_INSTRUCTION_UNARY:
      if (asm_node->data.instruction_unary.unary_op == ASM_UNARY_NEG) {
        fprintf(file, "\tneg");
      } else {
        fprintf(file, "\tnot");
      }

      print_instruction_suffix(file, asm_node->data.instruction_unary.assembly_type);
      fprintf(file, "\t");
      
      save_assembly_file(asm_node->data.instruction_unary.operand, file);
      fprintf(file, "\n");
      break;
    case ASM_INSTRUCTION_BINARY:
      switch (asm_node->data.instruction_binary.binary_op) {
        case ASM_BINARY_ADD:                  fprintf(file, "\tadd"); break;
        case ASM_BINARY_SUB:                  fprintf(file, "\tsub"); break;
        case ASM_BINARY_MULT:                 fprintf(file, "\timul"); break;        
        case ASM_BINARY_BITWISE_AND:          fprintf(file, "\tand"); break;
        case ASM_BINARY_BITWISE_OR:           fprintf(file, "\tor"); break;
        case ASM_BINARY_BITWISE_XOR:          fprintf(file, "\txor"); break;
        case ASM_BINARY_BITWISE_LEFT_SHIFT:   fprintf(file, "\tshl"); break;
        case ASM_BINARY_BITWISE_RIGHT_SHIFT:  fprintf(file, "\tshr"); break;
      }

      print_instruction_suffix(file, asm_node->data.instruction_binary.assembly_type);
      fprintf(file, "\t");
      
      save_assembly_file(asm_node->data.instruction_binary.operand_1, file);
      fprintf(file, ", ");
      save_assembly_file(asm_node->data.instruction_binary.operand_2, file);
      fprintf(file, "\n");
      break;
    case ASM_INSTRUCTION_CDQ:
      if (asm_node->data.instruction_cdq.assembly_type == ASM_TYPE_LONGWORD) {
        fprintf(file, "\tcdq\n");
      } else {
        fprintf(file, "\tcqo\n");
      }
      break;
    case ASM_INSTRUCTION_IDIV:
      fprintf(file, "\tidiv");

      print_instruction_suffix(file, asm_node->data.instruction_idiv.assembly_type);
      fprintf(file, "\t");
      
      save_assembly_file(asm_node->data.instruction_idiv.operand, file);
      fprintf(file, "\n");
      break;
    case ASM_INSTRUCTION_RET:
      //Function epilogue
      fprintf(file, "\tmovq\t%%rbp, %%rsp\n");
      fprintf(file, "\tpopq\t%%rbp\n");
      fprintf(file, "\tret\n");
      break;
    case ASM_INSTRUCTION_CALL:
      fprintf(file, "\tcall\t%s\n", asm_node->data.instruction_call.identifier);
      break;
    case ASM_INSTRUCTION_PUSH:
      fprintf(file, "\tpush\t"); 

      if (asm_node->data.instruction_push.operand->type == ASM_OPERAND_REGISTER) {
        char *operand_register = get_8_byte_register(asm_node->data.instruction_push.operand->data.operand_register.op_register);
        fprintf(file, "%s", operand_register);
      } else {
        save_assembly_file(asm_node->data.instruction_push.operand, file);
      }
      fprintf(file, "\n");
      break;
    case ASM_OPERAND_IMM:
      fprintf(file, "$%ld", asm_node->data.operand_imm.value);
      break;
    case ASM_OPERAND_REGISTER: {
      char *operand_register = get_4_byte_register(asm_node->data.operand_register.op_register);
      fprintf(file, "%s", operand_register);
      break;
    }
    case ASM_OPERAND_STACK:
      fprintf(file, "-%d(%%rbp)", asm_node->data.operand_stack.address);
      break;
    case ASM_OPERAND_DATA:
      fprintf(file, "%s(%%rip)", asm_node->data.operand_data.identifier);
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

char* get_8_byte_register(AsmRegisterType register_type) {
  switch(register_type) {
    case ASM_REGISTER_AX:  return "%rax";
    case ASM_REGISTER_DX:  return "%rdx";
    case ASM_REGISTER_CX:  return "%rcx";
    case ASM_REGISTER_DI:  return "%rdi";
    case ASM_REGISTER_SI:  return "%rsi";
    case ASM_REGISTER_R8:  return "%r8";
    case ASM_REGISTER_R9:  return "%r9";
    case ASM_REGISTER_R10: return "%r10";
    case ASM_REGISTER_R11: return "%r11";
  }
}

char* get_4_byte_register(AsmRegisterType register_type) {
  switch(register_type) {
    case ASM_REGISTER_AX:  return "%eax";
    case ASM_REGISTER_DX:  return "%edx";
    case ASM_REGISTER_CX:  return "%ecx";
    case ASM_REGISTER_DI:  return "%edi";
    case ASM_REGISTER_SI:  return "%esi";
    case ASM_REGISTER_R8:  return "%r8d";
    case ASM_REGISTER_R9:  return "%r9d";
    case ASM_REGISTER_R10: return "%r10d";
    case ASM_REGISTER_R11: return "%r11d";
    case ASM_REGISTER_SP:  return "%rsp";
  }
}

char* get_1_byte_register(AsmRegisterType register_type) {
  switch(register_type) {
    case ASM_REGISTER_AX:  return "%al";
    case ASM_REGISTER_DX:  return "%dl";
    case ASM_REGISTER_CX:  return "%cl";
    case ASM_REGISTER_DI:  return "%dil";
    case ASM_REGISTER_SI:  return "%sil";
    case ASM_REGISTER_R8:  return "%r8b";
    case ASM_REGISTER_R9:  return "%r9b";
    case ASM_REGISTER_R10: return "%r10b";
    case ASM_REGISTER_R11: return "%r11b";
  }
}

void print_instruction_suffix(FILE *file, AsmType type) {
  if (type == ASM_TYPE_LONGWORD) {
    fprintf(file, "l");
    return;
  }

  fprintf(file, "q");
}
