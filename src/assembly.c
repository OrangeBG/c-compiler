#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "../include/assembly.h"
#include "../include/hash_table.h"

#define INSTRUCTION_CAPACITY 8

AsmNode* asm_program(IRNode *ir_node);
AsmNode* asm_function(IRNode *ir_function); 
AsmNode* asm_resolve_memory_mov_instructions(AsmNode *function);
void     asm_instruction_return(AsmNode *asm_function, IRNode *ir_return_instruction);
void     asm_instruction_unary(AsmNode *asm_function, IRNode *ir_unary_instruction); 
void     check_function_instruction_size(AsmNode *asm_function); 
void     asm_pseudo_register_pass(AsmNode *asm_function, int *stack_offset); 
void     asm_replace_pseudo_register(AsmNode *instruction, HashTable *stack_location_table, int *stack_offset); 
void     asm_instruction_allocate_stack(AsmNode *asm_function); 

AsmNode* generate_assembly(IRNode *ir_nodes) {  
  AsmNode *program = malloc(sizeof(AsmNode));

  program->type = ASM_PROGRAM;
  program->data.program.function = asm_function(ir_nodes->data.program.function); 

  int stack_offset = 0;

  asm_pseudo_register_pass(program->data.program.function, &stack_offset);

  if (program->data.program.function->data.function.instructions[0].type != ASM_INSTRUCTION_ALLOCATE_STACK) {
    fprintf(stderr, "ERROR - Assembler: First instruction is not Allocate Stack for the '%s' function", program->data.program.function->data.function.name);
    exit(1);
  } 
  
  program->data.program.function->data.function.instructions[0].data.instruction_allocate_stack.bytes_to_subtract = stack_offset;

  program->data.program.function = asm_resolve_memory_mov_instructions(program->data.program.function);

  return program;
}

AsmNode* asm_resolve_memory_mov_instructions(AsmNode *function) {
  AsmNode *new_instructions = malloc(sizeof(AsmNode));
  AsmNode *new_function = malloc(sizeof(AsmNode));
  new_function->type = ASM_FUNCTION;
  new_function->data.function.name = function->data.function.name;
  new_function->data.function.instruction_count = 0;
  new_function->data.function.instruction_capacity = 0;
  new_function->data.function.instructions = new_instructions;
  
  int new_instruction_count = 0;
  AsmNode *instructions = function->data.function.instructions;

  for (int i = 0; i < function->data.function.instruction_count; i++) {
    if (instructions[i].type != ASM_INSTRUCTION_MOV || (instructions[i].data.instruction_mov.destination->type != ASM_OPERAND_STACK || instructions[i].data.instruction_mov.source->type != ASM_OPERAND_STACK)) {
      check_function_instruction_size(new_function);

      AsmNode *new_instruction = malloc(sizeof(AsmNode));

      new_instruction->type = instructions[i].type;
      new_instruction->data = instructions[i].data;

      new_function->data.function.instructions[new_function->data.function.instruction_count] = *new_instruction;
      new_function->data.function.instruction_count++;

      continue;
    }

    AsmNode *new_source_mov_instruction = malloc(sizeof(AsmNode));
    new_source_mov_instruction->type = ASM_INSTRUCTION_MOV;
    new_source_mov_instruction->data.instruction_mov.source = instructions[i].data.instruction_mov.source;

    AsmNode *new_destination = malloc(sizeof(AsmNode));
    new_destination->type = ASM_OPERAND_REGISTER;
    new_destination->data.operand_register.op_register = ASM_REGISTER_R10;    
    new_source_mov_instruction->data.instruction_mov.destination = new_destination;    

    check_function_instruction_size(new_function);

    new_function->data.function.instructions[new_function->data.function.instruction_count] = *new_source_mov_instruction;
    new_function->data.function.instruction_count++;

    AsmNode *new_source = malloc(sizeof(AsmNode));
    new_source->type = ASM_OPERAND_REGISTER;
    new_source->data.operand_register.op_register = ASM_REGISTER_R10;
        
    AsmNode *new_destination_mov_instruction = malloc(sizeof(AsmNode));
    new_destination_mov_instruction->type = ASM_INSTRUCTION_MOV;
    new_destination_mov_instruction->data.instruction_mov.source = new_source;
    new_destination_mov_instruction->data.instruction_mov.destination = instructions[i].data.instruction_mov.destination;

    check_function_instruction_size(new_function);

    new_function->data.function.instructions[new_function->data.function.instruction_count] = *new_destination_mov_instruction;
    new_function->data.function.instruction_count++;
  }

  return new_function;
}

void asm_pseudo_register_pass(AsmNode *asm_function, int *stack_offset) {
  HashTable stack_location_table;
  hash_table_init(&stack_location_table);
  
  for (int i = 0; i < asm_function->data.function.instruction_count; i++) {
    AsmNode *instruction = &asm_function->data.function.instructions[i];

    switch(instruction->type) {
      case ASM_INSTRUCTION_MOV:        
        if (instruction->data.instruction_mov.source->type == ASM_OPERAND_PSEUDO_REGISTER) {
         asm_replace_pseudo_register(instruction->data.instruction_mov.source, &stack_location_table, stack_offset);
        }

        if (instruction->data.instruction_mov.destination->type == ASM_OPERAND_PSEUDO_REGISTER) {
         asm_replace_pseudo_register(instruction->data.instruction_mov.destination, &stack_location_table, stack_offset);        
        }
        break;
      case ASM_INSTRUCTION_UNARY:
        if (instruction->data.instruction_unary.operand->type == ASM_OPERAND_PSEUDO_REGISTER) {
         asm_replace_pseudo_register(instruction->data.instruction_unary.operand, &stack_location_table, stack_offset);        
        }
        break;
      default:
        break;
    }
  }
}

void asm_replace_pseudo_register(AsmNode *pseudo_register, HashTable *stack_location_table, int * stack_offset) {
  AsmNode *stack_operand = malloc(sizeof(AsmNode));

  HashTableEntry *table_entry = hash_table_get_entry(stack_location_table, pseudo_register->data.operand_pseudo_register.identifier);

  //TODO: Shouldn't need to do table_entry->key == NULL, but is currently needed here. Need to investigate
  if (table_entry == NULL || table_entry->key == NULL) {
    HashTableEntry new_entry = {
      .key = pseudo_register->data.operand_pseudo_register.identifier,
      .value = {
        .integer = *stack_offset += 4,
        .type = HASH_INT
      }
    };

    hash_table_add_entry(stack_location_table, &new_entry);    

    pseudo_register->type = ASM_OPERAND_STACK;
    pseudo_register->data.operand_pseudo_register.identifier = NULL;
    pseudo_register->data.operand_stack.address = *stack_offset;

    return;
  }
  
  pseudo_register->type = ASM_OPERAND_STACK;
  pseudo_register->data.operand_pseudo_register.identifier = NULL;
  pseudo_register->data.operand_stack.address = table_entry->value.integer;
}

AsmNode* asm_function(IRNode *ir_function) {
  AsmNode *function = malloc(sizeof(AsmNode));
  function->type = ASM_FUNCTION;
  function->data.function.name = ir_function->data.function.identifier;

  AsmNode *instructions = malloc(sizeof(AsmNode));
  
  function->data.function.instruction_count = 0;
  function->data.function.instruction_capacity = 0;
  function->data.function.instructions = instructions;

  //Add the Allocate Stack instruction, but will allocate the stack offset value of the instruction in another pass after building the assembly nodes
  asm_instruction_allocate_stack(function);

  for (int i = 0; i < ir_function->data.function.instruction_count; i++) {
    switch (ir_function->data.function.instructions[i].type) {
      case IR_INSTRUCTION_RET:
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

void asm_instruction_allocate_stack(AsmNode *asm_function) {
  AsmNode *allocate_stack_instruction = malloc(sizeof(AsmNode));
  allocate_stack_instruction->type = ASM_INSTRUCTION_ALLOCATE_STACK;
  allocate_stack_instruction->data.instruction_allocate_stack.bytes_to_subtract = 0;

  check_function_instruction_size(asm_function);
  
  asm_function->data.function.instructions[asm_function->data.function.instruction_count] = *allocate_stack_instruction;
  asm_function->data.function.instruction_count++;
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
      printf("RET instruction\n");
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
    case ASM_OPERAND_STACK:
      printf("Stack %d\n", node->data.operand_stack.address);
      break;
    case ASM_INSTRUCTION_ALLOCATE_STACK:
      printf("Allocate Stack %d\n", node->data.instruction_allocate_stack.bytes_to_subtract);
      break;
    default:
      fprintf(stderr, "ERROR - Assembler: No print debug option for '%d' asm node type\n", node->type);
      break;
  }
}
