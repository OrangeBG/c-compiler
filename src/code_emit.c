#include <stdio.h>
#include "../include/code_emit.h"

static char* get_8_byte_register(AsmRegisterType register_type); 
static char* get_4_byte_register(AsmRegisterType register_type); 
static char* get_1_byte_register(AsmRegisterType register_type); 
static char* get_xmm_register(AsmRegisterType register_type); 
static void  print_instruction_suffix(FILE *file, AsmType type); 
static void  print_static_initializer(FILE *file, Types value_type, InitialValue initial_value);
static void  print_condition_code(FILE *file, AsmConditionCode condition_code); 

void save_assembly_file(AsmNode *asm_node, FILE *file) {
  switch (asm_node->type) {
    case ASM_PROGRAM:
      for (int i = 0; i < asm_node->data.program.static_constant_pointers->count; i++) {
        save_assembly_file(asm_node->data.program.static_constant_pointers->asm_pointers[i], file);
      }
      
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
      fprintf(file, "\tpushq\t\t%%rbp\n");
      fprintf(file, "\tmovq\t\t%%rsp, %%rbp\n");

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
        case TYPE_INT:
          asm_node->data.static_variable.static_variable_symbol->static_initial_value.int_value == 0 ? fprintf(file, "\t.bss\n") : fprintf(file, "\t.data\n"); 
          break;
        case TYPE_UINT:
          asm_node->data.static_variable.static_variable_symbol->static_initial_value.uint_value == 0 ? fprintf(file, "\t.bss\n") : fprintf(file, "\t.data\n"); 
          break;
        case TYPE_LONG: 
          asm_node->data.static_variable.static_variable_symbol->static_initial_value.long_value == 0 ? fprintf(file, "\t.bss\n") : fprintf(file, "\t.data\n"); 
          break;
        case TYPE_ULONG: 
          asm_node->data.static_variable.static_variable_symbol->static_initial_value.ulong_value == 0 ? fprintf(file, "\t.bss\n") : fprintf(file, "\t.data\n"); 
          break;
        case TYPE_DOUBLE:
          fprintf(file, "\t.data\n"); 
          break;
        default:
          fprintf(stderr, "ERROR - Code Emit: Static Variable Symbol Value Type '%d' not found\n", asm_node->data.static_variable.static_variable_symbol->value_type);
          exit(1);
      }      

      #ifdef __linux__
        fprintf(file, "\t.align 4\n");
      #endif 

      #ifdef __APPLE__
        fprintf(file, "\t.balign 4\n");
      #endif 

      fprintf(file, "%s:\n", asm_node->data.static_variable.identifier);

      print_static_initializer(file, asm_node->data.static_variable.static_variable_symbol->value_type, asm_node->data.static_variable.static_variable_symbol->static_initial_value);

      fprintf(file, "\n");
      break;
    case ASM_STATIC_CONSTANT:
      #ifdef __linux__         
        fprintf(file, "\t.section .rodata\n");
        fprintf(file, "\t.align %d\n", asm_node->data.static_constant.alignment);
      #endif

      #if __APPLE__
        fprintf(file, "\t.literal%d\n", asm_node->data.static_constant.alignment);
        fprintf(file, "\t.balign %d\n", asm_node->data.static_constant.alignment);
      #endif

      fprintf(file, "%s:\n", asm_node->data.static_constant.identifier);
      
      print_static_initializer(file, asm_node->data.static_constant.static_init->value_type, asm_node->data.static_constant.static_init->static_initial_value);

      #if __APPLE__
        if (asm_node->data.static_constant.alignment == 16) {
          fprintf(file, "\t.quad 0\n");
        }
      #endif

      fprintf(file, "\n");
      
      break;
    case ASM_INSTRUCTION_MOV:
      fprintf(file, "\tmov");

      print_instruction_suffix(file, asm_node->data.instruction_mov.assembly_type);      
      fprintf(file, "\t\t");
      
      save_assembly_file(asm_node->data.instruction_mov.source, file);
      fprintf(file, ", ");
      save_assembly_file(asm_node->data.instruction_mov.destination, file);      
      fprintf(file, "\n");
      break;
    case ASM_INSTRUCTION_MOVSX:
      fprintf(file, "\tmovslq\t\t");
      save_assembly_file(asm_node->data.instruction_movsx.source, file);
      fprintf(file, ", ");
      save_assembly_file(asm_node->data.instruction_movsx.destination, file);      
      fprintf(file, "\n");
      break;
    case ASM_INSTRUCTION_CMP:
      if (asm_node->data.instruction_cmp.assembly_type == ASM_TYPE_DOUBLE) {
        fprintf(file, "\tcomisd");
      } else {
        fprintf(file, "\tcmp");
      }

      print_instruction_suffix(file, asm_node->data.instruction_cmp.assembly_type);
      if (asm_node->data.instruction_cmp.assembly_type == ASM_TYPE_DOUBLE) {
        fprintf(file, "\t");
      } else {
        fprintf(file, "\t\t");
      }
      
      save_assembly_file(asm_node->data.instruction_cmp.operand_1, file);
      fprintf(file, ", ");
      save_assembly_file(asm_node->data.instruction_cmp.operand_2, file);
      fprintf(file, "\n");
      break;
    case ASM_INSTRUCTION_JMP:
      fprintf(file, "\tjmp\t\tL%s\n", asm_node->data.instruction_label.identifier);
      break;
    case ASM_INSTRUCTION_JMPCC:
      fprintf(file, "\tj");
      print_condition_code(file, asm_node->data.instruction_jmp_cc.condition_code);
      fprintf(file, "\t\tL%s\n", asm_node->data.instruction_jmp_cc.identifier);
      break;
    case ASM_INSTRUCTION_SETCC:
      fprintf(file, "\tset");
      print_condition_code(file, asm_node->data.instruction_set_cc.condition_code);
      fprintf(file, "\t\t");

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
      fprintf(file, "\t\t");
      
      save_assembly_file(asm_node->data.instruction_unary.operand, file);
      fprintf(file, "\n");
      break;
    case ASM_INSTRUCTION_BINARY:
      switch (asm_node->data.instruction_binary.binary_op) {
        case ASM_BINARY_ADD:                  fprintf(file, "\tadd"); break;
        case ASM_BINARY_SUB:                  fprintf(file, "\tsub"); break;
        case ASM_BINARY_BITWISE_AND:          fprintf(file, "\tand"); break;
        case ASM_BINARY_BITWISE_OR:           fprintf(file, "\tor"); break;
        case ASM_BINARY_BITWISE_LEFT_SHIFT:   fprintf(file, "\tshl"); break;
        case ASM_BINARY_BITWISE_RIGHT_SHIFT:  fprintf(file, "\tshr"); break;
        case ASM_BINARY_DIV_DOUBLE:           fprintf(file, "\tdiv"); break;
        case ASM_BINARY_MULT:                 asm_node->data.instruction_binary.assembly_type == ASM_TYPE_DOUBLE ? fprintf(file, "\tmul") : fprintf(file, "\timul"); break;        
        case ASM_BINARY_BITWISE_XOR:          fprintf(file, "\txor"); break;
      }

      if (asm_node->data.instruction_binary.binary_op == ASM_BINARY_BITWISE_XOR && asm_node->data.instruction_binary.assembly_type == ASM_TYPE_DOUBLE) {
        fprintf(file, "pd");
      } else {
        print_instruction_suffix(file, asm_node->data.instruction_binary.assembly_type);
      }

      fprintf(file, "\t\t");
      
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
      fprintf(file, "\t\t");
      
      save_assembly_file(asm_node->data.instruction_idiv.operand, file);
      fprintf(file, "\n");
      break;
    case ASM_INSTRUCTION_DIV:
      fprintf(file, "\tdiv");

      print_instruction_suffix(file, asm_node->data.instruction_div.assembly_type);
      fprintf(file, "\t\t");
      
      save_assembly_file(asm_node->data.instruction_div.operand, file);
      fprintf(file, "\n");
      break;
    case ASM_INSTRUCTION_RET:
      //Function epilogue
      fprintf(file, "\tmovq\t\t%%rbp, %%rsp\n");
      fprintf(file, "\tpopq\t\t%%rbp\n");
      fprintf(file, "\tret\n");
      break;
    case ASM_INSTRUCTION_CALL:
      fprintf(file, "\tcall\t\t%s\n", asm_node->data.instruction_call.identifier);
      break;
    case ASM_INSTRUCTION_PUSH:
      fprintf(file, "\tpush\t\t"); 

      if (asm_node->data.instruction_push.operand->type == ASM_OPERAND_REGISTER) {
        char *operand_register = get_8_byte_register(asm_node->data.instruction_push.operand->data.operand_register.op_register);
        fprintf(file, "%s", operand_register);
      } else {
        save_assembly_file(asm_node->data.instruction_push.operand, file);
      }
      fprintf(file, "\n");
      break;
    case ASM_INSTRUCTION_CVTSI2SD:
      fprintf(file, "\tcvtsi2sd"); 

      print_instruction_suffix(file, asm_node->data.instruction_cvtsi2sd.source_assembly_type);

      fprintf(file, "\t\t"); 
      save_assembly_file(asm_node->data.instruction_cvtsi2sd.source_operand, file);
      fprintf(file, ", ");
      save_assembly_file(asm_node->data.instruction_cvtsi2sd.destination_operand, file);
      fprintf(file, "\n");
      break;
    case ASM_INSTRUCTION_CVTTSD2SI:
      fprintf(file, "\tcvttsd2si"); 

      print_instruction_suffix(file, asm_node->data.instruction_cvttsd2si.destination_assembly_type);

      fprintf(file, "\t"); 
      save_assembly_file(asm_node->data.instruction_cvttsd2si.source_operand, file);
      fprintf(file, ", ");
      save_assembly_file(asm_node->data.instruction_cvttsd2si.destination_operand, file);
      fprintf(file, "\n");
      break;
    case ASM_OPERAND_IMM:
      fprintf(file, "$%ld", asm_node->data.operand_imm.value);
      break;
    case ASM_OPERAND_REGISTER: {
      //TODO: Check to see if we only pull 32 bit registers here
      switch(asm_node->data.operand_register.op_register) {
        case ASM_REGISTER_XMM0:  fprintf(file, "%%xmm0"); return;
        case ASM_REGISTER_XMM1:  fprintf(file, "%%xmm1"); return;
        case ASM_REGISTER_XMM2:  fprintf(file, "%%xmm2"); return;
        case ASM_REGISTER_XMM3:  fprintf(file, "%%xmm3"); return;
        case ASM_REGISTER_XMM4:  fprintf(file, "%%xmm4"); return;
        case ASM_REGISTER_XMM5:  fprintf(file, "%%xmm5"); return;
        case ASM_REGISTER_XMM6:  fprintf(file, "%%xmm6"); return;
        case ASM_REGISTER_XMM7:  fprintf(file, "%%xmm7"); return;
        case ASM_REGISTER_XMM14: fprintf(file, "%%xmm14"); return;
        case ASM_REGISTER_XMM15: fprintf(file, "%%xmm15"); return;
        default: {
          char *operand_register = get_4_byte_register(asm_node->data.operand_register.op_register);
          fprintf(file, "%s", operand_register);
          return;
        }        
      }
    }
    case ASM_OPERAND_STACK:
      fprintf(file, "-%d(%%rbp)", asm_node->data.operand_stack.address);
      break;
    case ASM_OPERAND_DATA:
      fprintf(file, "%s(%%rip)", asm_node->data.operand_data.identifier);
      break;
    default:
      fprintf(stderr, "ERROR - Code Emit: No assembly type for '%d'\n", asm_node->type);
      exit(1);
  }
}

void print_code_emit(FILE *file) { 
  char file_char;

  while((file_char = fgetc(file)) != EOF) {
    printf("%c", file_char);
  }
}

static char* get_8_byte_register(AsmRegisterType register_type) {
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

static char* get_4_byte_register(AsmRegisterType register_type) {
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

static char* get_1_byte_register(AsmRegisterType register_type) {
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

static char* get_xmm_register(AsmRegisterType register_type) {
  switch(register_type) {
    case ASM_REGISTER_XMM0:  return "%xmm0";
    case ASM_REGISTER_XMM1:  return "%xmm1";
    case ASM_REGISTER_XMM2:  return "%xmm2";
    case ASM_REGISTER_XMM3:  return "%xmm3";
    case ASM_REGISTER_XMM4:  return "%xmm4";
    case ASM_REGISTER_XMM5:  return "%xmm5";
    case ASM_REGISTER_XMM6:  return "%xmm6";
    case ASM_REGISTER_XMM7:  return "%xmm7";
    case ASM_REGISTER_XMM14: return "%xmm14";
    case ASM_REGISTER_XMM15: return "%xmm15";
  }
}

static void print_instruction_suffix(FILE *file, AsmType type) {
  switch (type) {
    case ASM_TYPE_LONGWORD:   fprintf(file, "l"); break;
    case ASM_TYPE_QUADWORD:   fprintf(file, "q"); break;
    case ASM_TYPE_DOUBLE:     fprintf(file, "sd"); break;
  }
}

static void print_condition_code(FILE *file, AsmConditionCode condition_code) {
  switch (condition_code) {
    case ASM_CONDITION_EQUAL:         fprintf(file, "e"); break;
    case ASM_CONDITION_NOT_EQUAL:     fprintf(file, "ne"); break;
    case ASM_CONDITION_GREATER:       fprintf(file, "g"); break;
    case ASM_CONDITION_GREATER_EQUAL: fprintf(file, "ge"); break;
    case ASM_CONDITION_LESS:          fprintf(file, "l"); break;
    case ASM_CONDITION_LESS_EQUAL:    fprintf(file, "le"); break;
    case ASM_CONDITION_ABOVE:         fprintf(file, "a"); break;
    case ASM_CONDITION_ABOVE_EQUAL:   fprintf(file, "ae"); break;
    case ASM_CONDITION_BELOW:         fprintf(file, "b"); break;
    case ASM_CONDITION_BELOW_EQUAL:   fprintf(file, "be"); break;
  }
}

static void print_static_initializer(FILE *file, Types value_type, InitialValue initial_value) {
  switch (value_type) {
    case TYPE_INT:    initial_value.int_value == 0 ? fprintf(file, "\t.zero 4\n") : fprintf(file, "\t.long %d\n", initial_value.int_value); break;
    case TYPE_UINT:   initial_value.uint_value == 0 ? fprintf(file, "\t.zero 4\n") : fprintf(file, "\t.long %d\n", initial_value.uint_value); break;
    case TYPE_LONG:   initial_value.long_value == 0 ? fprintf(file, "\t.zero 8\n") : fprintf(file, "\t.quad %lu\n", initial_value.long_value); break;
    case TYPE_ULONG:  initial_value.ulong_value == 0 ? fprintf(file, "\t.zero 8\n") : fprintf(file, "\t.quad %lu\n", initial_value.ulong_value); break;
    case TYPE_DOUBLE: fprintf(file, "\t.double %a", initial_value.double_value); break;
    default:
      fprintf(stderr, "ERROR - Code Emit: Static Variable Symbol Value Type '%d' not found\n", value_type);
      exit(1);
  }      
}
