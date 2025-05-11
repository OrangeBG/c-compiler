#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "../include/assembly.h"
#include "../include/hash_table.h"

#define INSTRUCTION_CAPACITY 8

AsmNode* asm_program(IRNode *ir_node);
AsmNode* asm_function(IRNode *ir_function); 
AsmNode* asm_resolve_instructions(AsmNode *function); 
AsmNode* asm_operand(IRNode *ir_operand);
void     asm_resolve_idiv_instructions(AsmNode *function, AsmNode *idiv_instruction);
void     asm_resolve_mov_memory_addresses(AsmNode *function, AsmNode *instruction); 
void     asm_resolve_binary_add_sub_memory_addresses(AsmNode *function, AsmNode *instruction); 
void     asm_resolve_binary_mul_memory_addresses(AsmNode *function, AsmNode *instruction); 
void     asm_instruction_return(AsmNode *asm_function, IRNode *ir_return_instruction);
void     asm_instruction_unary(AsmNode *asm_function, IRNode *ir_unary_instruction); 
void     asm_instruction_binary(AsmNode *asm_function, IRNode *ir_binary_instruction); 
void     asm_instruction_binary_division(AsmNode *asm_function, IRNode *ir_binary_instruction); 
void     check_function_instruction_size(AsmNode *asm_function); 
void     asm_pseudo_register_pass(AsmNode *asm_function, int *stack_offset); 
void     asm_replace_pseudo_register(AsmNode *instruction, HashTable *stack_location_table, int *stack_offset); 
void     asm_instruction_allocate_stack(AsmNode *asm_function); 
void     asm_add_instruction_to_function(AsmNode *function, AsmNode *instruction); 

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

  AsmNode *new_function = asm_resolve_instructions(program->data.program.function);

  free(program->data.program.function);
  program->data.program.function = new_function;
  return program;
}

AsmNode* asm_resolve_instructions(AsmNode *function) {
  AsmNode *new_instructions = malloc(sizeof(AsmNode));
  AsmNode *new_function = malloc(sizeof(AsmNode));
  new_function->type = ASM_FUNCTION;
  new_function->data.function.name = function->data.function.name;
  new_function->data.function.instruction_count = 0;
  new_function->data.function.instruction_capacity = 0;
  new_function->data.function.instructions = new_instructions;
  
  AsmNode *instructions = function->data.function.instructions;

  for (int i = 0; i < function->data.function.instruction_count; i++) {
    AsmNodeType instruction_type = instructions[i].type;

    if (instruction_type == ASM_INSTRUCTION_MOV && (instructions[i].data.instruction_mov.destination->type == ASM_OPERAND_STACK || instructions[i].data.instruction_mov.source->type == ASM_OPERAND_STACK)) {
      //MOV instructions cannot have both a source and destination as memory addresses
      asm_resolve_mov_memory_addresses(new_function, &instructions[i]);
      continue;
    } else if (instruction_type == ASM_INSTRUCTION_BINARY && (instructions[i].data.instruction_binary.operator == ASM_BINARY_ADD || instructions[i].data.instruction_binary.operator == ASM_BINARY_SUB)  && (instructions[i].data.instruction_binary.operand_1->type == ASM_OPERAND_STACK || instructions[i].data.instruction_binary.operand_2->type == ASM_OPERAND_STACK)) {
      //ADD and SUB instructions cannot have both a source and destination as memory addresses
      asm_resolve_binary_add_sub_memory_addresses(new_function, &instructions[i]);
      continue;
    } else if (instruction_type == ASM_INSTRUCTION_BINARY && instructions[i].data.instruction_binary.operator == ASM_BINARY_MULT && instructions[i].data.instruction_binary.operand_2->type == ASM_OPERAND_STACK) {
      //MUL instructions cannot use a memory address as its destination
      asm_resolve_binary_mul_memory_addresses(new_function, &instructions[i]);
      continue;
    } else if (instructions[i].type == ASM_INSTRUCTION_IDIV && instructions[i].data.instruction_idiv.operand->type == ASM_OPERAND_IMM) {
      //IDIV instructions need to be copied into a scratch buffer if the operand is a constant
      asm_resolve_idiv_instructions(new_function, &instructions[i]);
      continue;
    }

    AsmNode *new_instruction = malloc(sizeof(AsmNode));

    new_instruction->type = instructions[i].type;
    new_instruction->data = instructions[i].data;

    asm_add_instruction_to_function(new_function, new_instruction);
  }

  return new_function;
}

void asm_resolve_idiv_instructions(AsmNode *function, AsmNode *idiv_instruction) {
  AsmNode *mov_instruction = malloc(sizeof(AsmNode));
  mov_instruction->type = ASM_INSTRUCTION_MOV;
  mov_instruction->data.instruction_mov.source = idiv_instruction->data.instruction_idiv.operand;

  AsmNode *destination = malloc(sizeof(AsmNode));
  destination->type = ASM_OPERAND_REGISTER;
  destination->data.operand_register.op_register = ASM_REGISTER_R10;    

  mov_instruction->data.instruction_mov.destination = destination;

  asm_add_instruction_to_function(function, mov_instruction);

  AsmNode *new_idiv_instruction = malloc(sizeof(AsmNode));
  new_idiv_instruction->type = ASM_INSTRUCTION_IDIV;
  new_idiv_instruction->data.instruction_idiv.operand = destination;

  asm_add_instruction_to_function(function, new_idiv_instruction);
}

void asm_resolve_binary_mul_memory_addresses(AsmNode *function, AsmNode *instruction) {
  AsmNode *mov_instruction = malloc(sizeof(AsmNode));
  mov_instruction->type = ASM_INSTRUCTION_MOV;
  mov_instruction->data.instruction_mov.source = instruction->data.instruction_binary.operand_2;
  
  AsmNode *destination = malloc(sizeof(AsmNode));
  destination->type = ASM_OPERAND_REGISTER;
  destination->data.operand_register.op_register = ASM_REGISTER_R11;    

  mov_instruction->data.instruction_mov.destination = destination;
  
  asm_add_instruction_to_function(function, mov_instruction);

  AsmNode *mull_instruction = malloc(sizeof(AsmNode));
  mull_instruction->type = ASM_INSTRUCTION_BINARY;
  mull_instruction->data.instruction_binary.operator = ASM_BINARY_MULT;
  mull_instruction->data.instruction_binary.operand_1 = instruction->data.instruction_binary.operand_1;
  mull_instruction->data.instruction_binary.operand_2 = destination;

  asm_add_instruction_to_function(function, mull_instruction);

  AsmNode *mov_instruction_2 = malloc(sizeof(AsmNode));
  mov_instruction_2->type = ASM_INSTRUCTION_MOV;
  mov_instruction_2->data.instruction_mov.source = destination;
  mov_instruction_2->data.instruction_mov.destination = instruction->data.instruction_binary.operand_2;

  asm_add_instruction_to_function(function, mov_instruction_2);
}

void asm_resolve_binary_add_sub_memory_addresses(AsmNode *function, AsmNode *instruction) {
  AsmNode *mov_instruction = malloc(sizeof(AsmNode));
  mov_instruction->type = ASM_INSTRUCTION_MOV;
  mov_instruction->data.instruction_mov.source = instruction->data.instruction_binary.operand_1;

  AsmNode *destination = malloc(sizeof(AsmNode));
  destination->type = ASM_OPERAND_REGISTER;
  destination->data.operand_register.op_register = ASM_REGISTER_R10;    

  mov_instruction->data.instruction_mov.destination = destination;

  asm_add_instruction_to_function(function, mov_instruction);

  AsmNode *binary_instruction = malloc(sizeof(AsmNode));
  binary_instruction->type = ASM_INSTRUCTION_BINARY;
  binary_instruction->data.instruction_binary.operand_1 = destination;
  binary_instruction->data.instruction_binary.operand_2 = instruction->data.instruction_binary.operand_2;

  asm_add_instruction_to_function(function, binary_instruction);
}

void asm_resolve_mov_memory_addresses(AsmNode *function, AsmNode *instruction) {
    AsmNode *new_source_mov_instruction = malloc(sizeof(AsmNode));
    new_source_mov_instruction->type = ASM_INSTRUCTION_MOV;
    new_source_mov_instruction->data.instruction_mov.source = instruction->data.instruction_mov.source;

    AsmNode *new_destination = malloc(sizeof(AsmNode));
    new_destination->type = ASM_OPERAND_REGISTER;
    new_destination->data.operand_register.op_register = ASM_REGISTER_R10;    
    new_source_mov_instruction->data.instruction_mov.destination = new_destination;    

    asm_add_instruction_to_function(function, new_source_mov_instruction);

    AsmNode *new_source = malloc(sizeof(AsmNode));
    new_source->type = ASM_OPERAND_REGISTER;
    new_source->data.operand_register.op_register = ASM_REGISTER_R10;
        
    AsmNode *new_destination_mov_instruction = malloc(sizeof(AsmNode));
    new_destination_mov_instruction->type = ASM_INSTRUCTION_MOV;
    new_destination_mov_instruction->data.instruction_mov.source = new_source;
    new_destination_mov_instruction->data.instruction_mov.destination = instruction->data.instruction_mov.destination;

    asm_add_instruction_to_function(function, new_destination_mov_instruction);
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
      case ASM_INSTRUCTION_BINARY:
        if (instruction->data.instruction_binary.operand_1->type == ASM_OPERAND_PSEUDO_REGISTER) {
         asm_replace_pseudo_register(instruction->data.instruction_binary.operand_1, &stack_location_table, stack_offset);        
        }

        if (instruction->data.instruction_binary.operand_2->type == ASM_OPERAND_PSEUDO_REGISTER) {
         asm_replace_pseudo_register(instruction->data.instruction_binary.operand_2, &stack_location_table, stack_offset);        
        }
        break;
      case ASM_INSTRUCTION_IDIV:
        if (instruction->data.instruction_idiv.operand->type == ASM_OPERAND_PSEUDO_REGISTER) {
         asm_replace_pseudo_register(instruction->data.instruction_idiv.operand, &stack_location_table, stack_offset);        
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

  //Adds the Allocate Stack instruction, but will allocate the stack offset value of the instruction in another pass after building the assembly nodes
  asm_instruction_allocate_stack(function);

  for (int i = 0; i < ir_function->data.function.instruction_count; i++) {
    switch (ir_function->data.function.instructions[i].type) {
      case IR_INSTRUCTION_RET:
        asm_instruction_return(function, &ir_function->data.function.instructions[i]);
        break;
      case IR_INSTRUCTION_UNARY:
        asm_instruction_unary(function, &ir_function->data.function.instructions[i]);
        break;
      case IR_INSTRUCTION_BINARY:        
        switch (ir_function->data.function.instructions[i].data.instruction_binary.op_type) {
          case IR_BINARY_ADD:
          case IR_BINARY_SUBTRACT:
          case IR_BINARY_MULTIPLY:
          case IR_BINARY_BITWISE_AND:
          case IR_BINARY_BITWISE_OR:
          case IR_BINARY_BITWISE_XOR:
          case IR_BINARY_BITWISE_LEFT_SHIFT:
          case IR_BINARY_BITWISE_RIGHT_SHIFT:
            asm_instruction_binary(function, &ir_function->data.function.instructions[i]);
            break;
          default:
            asm_instruction_binary_division(function, &ir_function->data.function.instructions[i]);
            break;
        }
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

  asm_add_instruction_to_function(asm_function, allocate_stack_instruction);
}

void asm_instruction_binary(AsmNode *asm_function, IRNode *ir_binary_instruction) {
  AsmNode *source_1 = asm_operand(ir_binary_instruction->data.instruction_binary.source_1);
  AsmNode *source_2 = asm_operand(ir_binary_instruction->data.instruction_binary.source_2);
  AsmNode *destination_node = asm_operand(ir_binary_instruction->data.instruction_binary.destination);

  AsmNode *mov_instruction = malloc(sizeof(AsmNode));
  mov_instruction->type = ASM_INSTRUCTION_MOV;
  mov_instruction->data.instruction_mov.source = source_1;
  mov_instruction->data.instruction_mov.destination = destination_node;

  AsmNode *binary_instruction = malloc(sizeof(AsmNode));
  binary_instruction->type = ASM_INSTRUCTION_BINARY;
  binary_instruction->data.instruction_binary.operand_1 = source_2;
  binary_instruction->data.instruction_binary.operand_2 = destination_node;

  switch (ir_binary_instruction->data.instruction_binary.op_type) {
    case IR_BINARY_ADD:
      binary_instruction->data.instruction_binary.operator = ASM_BINARY_ADD;
      break;
    case IR_BINARY_SUBTRACT:
      binary_instruction->data.instruction_binary.operator = ASM_BINARY_SUB;
      break;
    case IR_BINARY_MULTIPLY:
      binary_instruction->data.instruction_binary.operator = ASM_BINARY_MULT;
      break;
    case IR_BINARY_BITWISE_AND:
      binary_instruction->data.instruction_binary.operator = ASM_BINARY_BITWISE_AND;
      break;
    case IR_BINARY_BITWISE_OR:
      binary_instruction->data.instruction_binary.operator = ASM_BINARY_BITWISE_OR;
      break;
    case IR_BINARY_BITWISE_XOR:
      binary_instruction->data.instruction_binary.operator = ASM_BINARY_BITWISE_XOR;
      break;
    case IR_BINARY_BITWISE_LEFT_SHIFT:
      binary_instruction->data.instruction_binary.operator = ASM_BINARY_BITWISE_LEFT_SHIFT;
      break;
    case IR_BINARY_BITWISE_RIGHT_SHIFT:
      binary_instruction->data.instruction_binary.operator = ASM_BINARY_BITWISE_RIGHT_SHIFT;
      break;
    default:
      fprintf(stderr, "ERROR - Assembler: Operator type not found for binary operation");
      exit(1);
      break;
  }

  asm_add_instruction_to_function(asm_function, mov_instruction);
  asm_add_instruction_to_function(asm_function, binary_instruction);
}

void asm_instruction_binary_division(AsmNode *asm_function, IRNode *ir_binary_instruction) {
  AsmNode *source_1 = asm_operand(ir_binary_instruction->data.instruction_binary.source_1);
  AsmNode *source_2 = asm_operand(ir_binary_instruction->data.instruction_binary.source_2);
  AsmNode *destination_node = asm_operand(ir_binary_instruction->data.instruction_binary.destination);

  AsmNode *mov_instruction_1 = malloc(sizeof(AsmNode));
  mov_instruction_1->type = ASM_INSTRUCTION_MOV;
  mov_instruction_1->data.instruction_mov.source = source_1;

  AsmNode *mov_destination_1 = malloc(sizeof(AsmNode));
  mov_destination_1->type = ASM_OPERAND_REGISTER;
  mov_destination_1->data.operand_register.op_register = ASM_REGISTER_AX;
  
  mov_instruction_1->data.instruction_mov.destination = mov_destination_1;

  asm_add_instruction_to_function(asm_function, mov_instruction_1);

  AsmNode *cdq_instruction = malloc(sizeof(AsmNode));
  cdq_instruction->type = ASM_INSTRUCTION_CDQ;

  asm_add_instruction_to_function(asm_function, cdq_instruction);
  
  AsmNode *idiv_instruction = malloc(sizeof(AsmNode));
  idiv_instruction->type = ASM_INSTRUCTION_IDIV;
  idiv_instruction->data.instruction_idiv.operand = source_2;

  asm_add_instruction_to_function(asm_function, idiv_instruction);

  AsmNode *mov_instruction_2 = malloc(sizeof(AsmNode));
  mov_instruction_2->type = ASM_INSTRUCTION_MOV;
  mov_instruction_2->data.instruction_mov.destination = destination_node;

  AsmNode *mov_destination_2 = malloc(sizeof(AsmNode));
  mov_destination_2->type = ASM_OPERAND_REGISTER;
  
  if (ir_binary_instruction->data.instruction_binary.op_type == IR_BINARY_DIVIDE) {
    mov_destination_2->data.operand_register.op_register = ASM_REGISTER_AX;
  } else {
    mov_destination_2->data.operand_register.op_register = ASM_REGISTER_DX;
  }

  mov_instruction_2->data.instruction_mov.source = mov_destination_2;

  asm_add_instruction_to_function(asm_function, mov_instruction_2);
}
 
void asm_instruction_unary(AsmNode *asm_function, IRNode *ir_unary_instruction) {
  AsmNode *source_node = asm_operand(ir_unary_instruction->data.unary.source);
  AsmNode *destination_node = asm_operand(ir_unary_instruction->data.unary.destination);

  AsmNode *mov_node = malloc(sizeof(AsmNode));

  mov_node->type = ASM_INSTRUCTION_MOV;
  mov_node->data.instruction_mov.source = source_node;
  mov_node->data.instruction_mov.destination = destination_node;

  asm_add_instruction_to_function(asm_function, mov_node);
  
  AsmNode *ret_node = malloc(sizeof(AsmNode));
  ret_node->type = ASM_INSTRUCTION_UNARY;

  if (ir_unary_instruction->data.unary.op_type == IR_UNARY_NEGATE) {
    ret_node->data.instruction_unary.operator = ASM_UNARY_NEG;
  } else {
    ret_node->data.instruction_unary.operator = ASM_UNARY_NOT;
  }
  
  ret_node->data.instruction_unary.operand = destination_node;


  asm_add_instruction_to_function(asm_function, ret_node);
}

void asm_instruction_return(AsmNode *asm_function, IRNode *ir_return_instruction) {
  AsmNode *source_node = asm_operand(ir_return_instruction->data.instruction_ret.value);

  AsmNode *destination_node = malloc(sizeof(AsmNode));
  destination_node->type = ASM_OPERAND_REGISTER;
  destination_node->data.operand_register.op_register = ASM_REGISTER_AX;  

  AsmNode *mov_node = malloc(sizeof(AsmNode));
  mov_node->type = ASM_INSTRUCTION_MOV;

  mov_node->data.instruction_mov.source = source_node;
  mov_node->data.instruction_mov.destination = destination_node;

  asm_add_instruction_to_function(asm_function, mov_node);

  AsmNode *ret_node = malloc(sizeof(AsmNode));
  ret_node->type = ASM_INSTRUCTION_RET;

  asm_add_instruction_to_function(asm_function, ret_node);
}

void asm_add_instruction_to_function(AsmNode *function, AsmNode *instruction) {
  check_function_instruction_size(function);

  function->data.function.instructions[function->data.function.instruction_count] = *instruction;
  function->data.function.instruction_count++;
}

AsmNode* asm_operand(IRNode *ir_operand) {
  AsmNode *asm_operand = malloc(sizeof(AsmNode));

  switch (ir_operand->type) {
    case IR_VALUE_CONSTANT:
      asm_operand->type = ASM_OPERAND_IMM;
      asm_operand->data.operand_imm.value = ir_operand->data.value_constant.value;
      break;
    case IR_VALUE_VAR:
      asm_operand->type = ASM_OPERAND_PSEUDO_REGISTER;
      asm_operand->data.operand_pseudo_register.identifier = ir_operand->data.value_var.identifier;
      break;
    default:
      fprintf(stderr, "ERROR - Assembler: Binary operand value type %d not found in asm_binary_operand\n", ir_operand->type);
      exit(1);      
  }  

  return asm_operand;
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
      printf("Function: %s -> Instruction count: %d\n", node->data.function.name, node->data.function.instruction_count);

      for (int i = 0; i < node->data.function.instruction_count; i++) {
        print_assembly(&node->data.function.instructions[i]);
      }      
      break;
    case ASM_INSTRUCTION_MOV:
      printf("MOV -> ");
      printf("Src( ");
      print_assembly(node->data.instruction_mov.source);
      printf(") Dest( ");
      print_assembly(node->data.instruction_mov.destination);
      printf(")\n");
      break;
    case ASM_INSTRUCTION_RET:
      printf("RET -> \n");
      break;
    case ASM_INSTRUCTION_UNARY:
      printf("UNARY Instruction ");
      print_assembly(node->data.instruction_unary.operand);
      printf("\n");
      break;
    case ASM_INSTRUCTION_BINARY:
      switch (node->data.instruction_binary.operator) {
        case ASM_BINARY_ADD:
          printf("ADD -> ");
          break;
        case ASM_BINARY_SUB:
          printf("SUB -> ");
          break;
        case ASM_BINARY_MULT:
          printf("MUL -> ");
          break;
        case ASM_BINARY_BITWISE_AND:
          printf("AND -> ");
          break;
        case ASM_BINARY_BITWISE_OR:
          printf("OR -> ");
          break;
        case ASM_BINARY_BITWISE_XOR:
          printf("XOR -> ");
          break;
        case ASM_BINARY_BITWISE_LEFT_SHIFT:
          printf("SHL -> ");
          break;
        case ASM_BINARY_BITWISE_RIGHT_SHIFT:
          printf("SHR -> ");
          break;
      }
      printf("Src( ");
      print_assembly(node->data.instruction_binary.operand_1);
      printf(") Dest(");
      print_assembly(node->data.instruction_binary.operand_2);
      printf(")");
      printf("\n");
      break;
    case ASM_INSTRUCTION_CDQ:
      printf("CDQ Instruction\n");
      break;
    case ASM_INSTRUCTION_IDIV:
      printf("IDIV Instruction\n");
      print_assembly(node->data.instruction_idiv.operand);
      printf("\n");
      break;
    case ASM_OPERAND_REGISTER:
      printf("Register %d ", node->data.operand_register.op_register);
      break;
    case ASM_OPERAND_PSEUDO_REGISTER:
      printf("Pseudo Register %s ", node->data.operand_pseudo_register.identifier);
      break;
    case ASM_OPERAND_IMM:
      printf("Register %d ", node->data.operand_imm.value);
      break;
    case ASM_OPERAND_STACK:
      printf("Stack %d ", node->data.operand_stack.address);
      break;
    case ASM_INSTRUCTION_ALLOCATE_STACK:
      printf("RSP %d\n", node->data.instruction_allocate_stack.bytes_to_subtract);
      break;
    default:
      fprintf(stderr, "ERROR - Assembler: No print debug option for '%d' asm node type\n", node->type);
      break;
  }
}
