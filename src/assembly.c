#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "../include/assembly.h"

#define INSTRUCTION_CAPACITY 8

AsmNode* asm_program(IRNode *ir_node);
AsmNode* asm_function(IRNode *ir_function); 
void     asm_instruction_return(AsmNode *asm_function, IRNode *ir_return_instruction);
void     asm_instruction_unary(AsmNode *asm_function, IRNode *ir_unary_instruction); 
void     check_function_instruction_size(AsmNode *asm_function); 

AsmNode* generate_assembly(IRNode *ir_nodes) {  
  AsmNode *program = malloc(sizeof(AsmNode));

  program->type = ASM_PROGRAM;
  program->data.program.function = asm_function(ir_nodes->data.program.function); 

  return program;
}

AsmNode* asm_function(IRNode *ir_function) {
  AsmNode *function = malloc(sizeof(AsmNode));
  function->type = ASM_FUNCTION;
  function->data.function.name = ir_function->data.function.identifier;

  AsmNode *instructions = malloc(sizeof(AsmNode));
  
  function->data.function.instruction_count = 0;
  function->data.function.instruction_capacity = 0;
  function->data.function.instructions = instructions;

  for (int i = 0; i < ir_function->data.function.instruction_count; i++) {
    switch (ir_function->data.function.instructions[i].type) {
      case IR_INSTRUCTION_RET:
        //asm_instruction_return(function, ir_function->data.function.instructions[i].data.instruction_ret.value);
        asm_instruction_return(function, &ir_function->data.function.instructions[i]);
        break;
      case IR_INSTRUCTION_UNARY:
        asm_instruction_unary(function, &ir_function->data.function.instructions[i]);
        break;
      default:
        fprintf(stderr, "ERROR - Assembler: Could not resolve instruction type in asm_function\n");
        exit(1);
    }
  }

  return function;
}

void asm_instruction_unary(AsmNode *asm_function, IRNode *ir_unary_instruction) {
  AsmNode *source_node = malloc(sizeof(AsmNode));

  switch (ir_unary_instruction->data.unary.source->type) {
    case IR_VALUE_CONSTANT:
      source_node->type = ASM_OPERAND_IMM;
      source_node->data.operand_imm.value = ir_unary_instruction->data.unary.source->data.value_constant.value;
      break;
    case IR_VALUE_VAR:
      source_node->type = ASM_OPERAND_PSEUDO_REGISTER;
      source_node->data.operand_pseudo_register.identifier = ir_unary_instruction->data.unary.source->data.value_var.identifier;      
      break;
    default:
      fprintf(stderr, "ERROR - Assembler: Unary source value type %d not found in asm_instruction_unary\n", ir_unary_instruction->data.unary.source->type);
      exit(1);      
  }  

  AsmNode *destination_node = malloc(sizeof(AsmNode));

  switch (ir_unary_instruction->data.unary.destination->type) {
    case IR_VALUE_CONSTANT:
      destination_node->type = ASM_OPERAND_IMM;
      destination_node->data.operand_imm.value = ir_unary_instruction->data.unary.destination->data.value_constant.value;   
      break;
    case IR_VALUE_VAR:
      destination_node->type = ASM_OPERAND_PSEUDO_REGISTER;
      destination_node->data.operand_pseudo_register.identifier = ir_unary_instruction->data.unary.destination->data.value_var.identifier;      
      break;
    default:
      fprintf(stderr, "ERROR - Assembler: Unary destination value type %d not found in asm_instruction_unary\n", ir_unary_instruction->data.unary.destination->type);
      exit(1);      
  }  

  AsmNode *mov_node = malloc(sizeof(AsmNode));

  mov_node->type = ASM_INSTRUCTION_MOV;
  mov_node->data.instruction_mov.source = source_node;
  mov_node->data.instruction_mov.destination = destination_node;

  check_function_instruction_size(asm_function);

  asm_function->data.function.instructions[asm_function->data.function.instruction_count] = *mov_node;
  asm_function->data.function.instruction_count++;
  
  check_function_instruction_size(asm_function);

  AsmNode *ret_node = malloc(sizeof(AsmNode));
  ret_node->type = ASM_INSTRUCTION_RET;

  asm_function->data.function.instructions[asm_function->data.function.instruction_count] = *ret_node;
  asm_function->data.function.instruction_count++;  
}

void asm_instruction_return(AsmNode *asm_function, IRNode *ir_return_instruction) {
  AsmNode *source_node = malloc(sizeof(AsmNode));

  switch (ir_return_instruction->data.instruction_ret.value->type) {
    case IR_VALUE_CONSTANT: {
        source_node->type = ASM_OPERAND_IMM;
        source_node->data.operand_imm.value = ir_return_instruction->data.instruction_ret.value->data.value_constant.value;      
      }
      break;
    case IR_VALUE_VAR:
        //TODO: I don't think this is needed
        source_node->type = ASM_OPERAND_PSEUDO_REGISTER;
        source_node->data.operand_pseudo_register.identifier = ir_return_instruction->data.instruction_ret.value->data.value_var.identifier;      
      break;
    default:
      fprintf(stderr, "ERROR - Assembler: Return value type %d not found in asm_instruction_return\n", ir_return_instruction->data.instruction_ret.value->type);
      exit(1);
  }

  AsmNode *destination_node = malloc(sizeof(AsmNode));
  destination_node->type = ASM_OPERAND_REGISTER;
  destination_node->data.operand_register.op_register = ASM_REGISTER_R10;  

  AsmNode *mov_node = malloc(sizeof(AsmNode));
  mov_node->type = ASM_INSTRUCTION_MOV;

  mov_node->data.instruction_mov.source = source_node;
  mov_node->data.instruction_mov.destination = destination_node;

  check_function_instruction_size(asm_function);

  asm_function->data.function.instructions[asm_function->data.function.instruction_count] = *mov_node;
  asm_function->data.function.instruction_count++;

  check_function_instruction_size(asm_function);

  AsmNode *ret_node = malloc(sizeof(AsmNode));
  ret_node->type = ASM_INSTRUCTION_RET;

  asm_function->data.function.instructions[asm_function->data.function.instruction_count] = *ret_node;
  asm_function->data.function.instruction_count++;  
}

void check_function_instruction_size(AsmNode *asm_function) {
  int current_count = asm_function->data.function.instruction_count;
  int current_capacity = asm_function->data.function.instruction_capacity;

  if (current_count == current_capacity) {
    int new_size = current_capacity == 0 ? INSTRUCTION_CAPACITY : current_capacity * INSTRUCTION_CAPACITY;

    AsmNode *instructions = realloc(asm_function->data.function.instructions, new_size * sizeof(AsmNode));

    asm_function->data.function.instruction_capacity = new_size;
    asm_function->data.function.instructions = instructions;
  } 
} 

void print_assembly(AsmNode *node) {
  switch(node->type) {    
    case ASM_PROGRAM:
      printf("Program \n");
      print_assembly(node->data.program.function);
      printf("\n");
      break;
    case ASM_FUNCTION:
      printf("Function: %s\n", node->data.function.name);
      printf("Inst Count: %d\n\n", node->data.function.instruction_count);

      for (int i = 0; i < node->data.function.instruction_count; i++) {
        print_assembly(&node->data.function.instructions[i]);
      }      
      break;
    case ASM_INSTRUCTION_MOV:
      printf("MOV Instruction \n");
      printf("Source ");
      print_assembly(node->data.instruction_mov.source);
      printf("Destination ");
      print_assembly(node->data.instruction_mov.destination);
      printf("\n");
      break;
    case ASM_INSTRUCTION_RET:
      printf("RET instruction %d\n", node->data.instruction_return.ret);
      break;
    case ASM_INSTRUCTION_UNARY:
      printf("UNARY Instruction ");
      print_assembly(node->data.instruction_unary.operand);
      printf("\n");
      break;
    case ASM_OPERAND_REGISTER:
      printf("Register %d\n", node->data.operand_register.op_register);
      break;
    case ASM_OPERAND_PSEUDO_REGISTER:
      printf("Pseudo Register %s\n", node->data.operand_pseudo_register.identifier);
      break;
    case ASM_OPERAND_IMM:
      printf("Register %d\n", node->data.operand_imm.value);
      break;
      //TODO:
      // ASM_INSTRUCTION_ALLOCATE_STACK,
      // ASM_OPERAND_PSEUDO_REGISTER,
      // ASM_OPERAND_STACK
    default:
      fprintf(stderr, "ERROR - Assembler: No print debug option for '%d' asm node type\n", node->type);
      break;
  }
}
