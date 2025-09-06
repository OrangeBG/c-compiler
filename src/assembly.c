#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "../include/assembly.h"
#include "../include/hash_table.h"
#include "../include/arena.h"
#include "../include/declaration_symbol.h"

#define NODE_POINTER_CAPACITY 8
#define ALIGNMENT_QUADWORD 8
#define ALIGNMENT_LONGWORD 4

void     asm_function(IRNode *ir_function, AsmNode *asm_function, Arena *asm_arena, DeclarationSymbolTable *declaration_symbol_table); 
void     asm_static_variable(IRNode *ir_static_variable, AsmNode *asm_static_variable);
AsmNode* asm_resolve_instructions(AsmNode *function, Arena *asm_arena); 
AsmNode* asm_operand(IRNode *ir_operand, Arena *asm_arena);
void     asm_resolve_idiv_instructions(AsmNode *function, AsmNode *idiv_instruction, Arena *asm_arena);
void     asm_resolve_mov_memory_addresses(AsmNode *function, AsmNode *instruction, Arena *asm_arena); 
void     asm_resolve_cmp_memory_addresses(AsmNode *function, AsmNode *instruction, Arena *asm_arena); 
void     asm_resolve_cmp_constant_in_operand_2(AsmNode *function, AsmNode *instruction, Arena *asm_arena); 
void     asm_resolve_binary_add_sub_memory_addresses(AsmNode *function, AsmNode *instruction, Arena *asm_arena); 
void     asm_resolve_binary_mul_memory_addresses(AsmNode *function, AsmNode *instruction, Arena *asm_arena); 
void     asm_pseudo_register_pass(AsmNode *asm_function, AsmBackendSymbolTable *backend_symbol_table, int *stack_offset); 
void     asm_replace_pseudo_register(AsmNode *instruction, AsmType instruction_type, HashTable *stack_location_table, AsmBackendSymbolTable *backend_symbol_table, int *stack_offset); 
void     asm_instruction_return(AsmNode *asm_function, IRNode *ir_return_instruction, Arena *asm_arena, DeclarationSymbolTable *declaration_symbol_table);
void     asm_instruction_unary(AsmNode *asm_function, IRNode *ir_unary_instruction, Arena *asm_arena, DeclarationSymbolTable *declaration_symbol_table); 
void     asm_instruction_unary_not(AsmNode *asm_function, IRNode *ir_unary_not_instruction, Arena *asm_arena, DeclarationSymbolTable *declaration_symbol_table); 
void     asm_instruction_binary(AsmNode *asm_function, IRNode *ir_binary_instruction, Arena *asm_arena, DeclarationSymbolTable *declaration_symbol_table); 
void     asm_instruction_binary_relational(AsmNode *asm_function, IRNode *ir_relational_instruction, Arena *asm_arena, DeclarationSymbolTable *declaration_symbol_table); 
void     asm_instruction_binary_division(AsmNode *asm_function, const IRNode *ir_binary_instruction, Arena *asm_arena, DeclarationSymbolTable *declaration_symbol_table); 
void     asm_instruction_allocate_rsp_stack(AsmNode *asm_function, int bytes, Arena *asm_arena); 
void     asm_instruction_jump(AsmNode *asm_function, IRNode *ir_jump_instruction, Arena *asm_arena); 
void     asm_instruction_jump_if_zero(AsmNode *asm_function, IRNode *ir_jump_if_zero_instruction, Arena *asm_arena, DeclarationSymbolTable *declaration_symbol_table); 
void     asm_instruction_jump_if_not_zero(AsmNode *asm_function, IRNode *ir_jump_if_not_zero_instruction, Arena *asm_arena, DeclarationSymbolTable *declaration_symbol_table); 
void     asm_instruction_copy(AsmNode *asm_function, IRNode *ir_copy_instruction, Arena *asm_arena, DeclarationSymbolTable *declaration_symbol_table);
void     asm_instruction_label(AsmNode *asm_function, IRNode *ir_label_instruction, Arena *asm_arena); 
void     asm_instruction_function_call(AsmNode *asm_function, IRNode *ir_function_call_instruction, Arena *asm_arena, DeclarationSymbolTable *declaration_symbol_table);
void     asm_instruction_sign_extend(AsmNode *asm_function, IRNode *ir_sign_extend_instruction, Arena *asm_arena); 
void     asm_instruction_truncate(AsmNode *asm_function, IRNode *ir_truncate_instruction, Arena *asm_arena); 
void     asm_add_instruction_to_function(AsmNode *function, AsmNode *instruction); 
void     asm_add_to_node_pointer(AsmNode *asm_node, AsmNodePointers *asm_node_pointer);
void     asm_init_node_pointer(AsmNodePointers *asm_node_pointer);
static AsmType convert_ir_value_to_asm_type(IRNode *ir_node, DeclarationSymbolTable *declaration_symbol_table); 
static bool is_instruction_quadword(AsmNode *instruction); 
static void convert_declaration_table_to_backend_table(DeclarationSymbolTable *declaration_symbol_table, AsmBackendSymbolTable *backend_symbol_table); 
static int round_stack_offset(int stack_offset); 

AsmNode* generate_assembly(IRNode *ir_nodes, DeclarationSymbolTable *declaration_symbol_table, AsmBackendSymbolTable *backend_symbol_table) {  
  Arena *asm_arena = malloc(sizeof(Arena));
  //TODO: Hardcoded capacity
  arena_init(asm_arena, sizeof(AsmNode), sizeof(AsmNode) * 1000, false);

  AsmNodePointers *node_pointer = malloc(sizeof(AsmNodePointers));
  asm_init_node_pointer(node_pointer);  
  
  AsmNode *program = arena_alloc(asm_arena);
  program->type = ASM_PROGRAM;
  program->data.program.top_level_count = 0;
  program->data.program.top_level_pointers = node_pointer;

  for (int i = 0; i < ir_nodes->data.program.top_level_count; i++) {
    AsmNode *top_level_declaration = arena_alloc(asm_arena);
    asm_add_to_node_pointer(top_level_declaration, node_pointer);
    
    if (ir_nodes->data.program.top_level_ptrs->node_pointers[i]->type == IR_FUNCTION) {
      asm_function(ir_nodes->data.program.top_level_ptrs->node_pointers[i], top_level_declaration, asm_arena, declaration_symbol_table);
    } else {
      asm_static_variable(ir_nodes->data.program.top_level_ptrs->node_pointers[i], top_level_declaration);
   }
    
    program->data.program.top_level_count++;
  }

  convert_declaration_table_to_backend_table(declaration_symbol_table, backend_symbol_table);

  for (int i = 0; i < program->data.program.top_level_count; i++) {
    AsmNode *top_level_node = program->data.program.top_level_pointers->asm_pointers[i];

    if (top_level_node->type != ASM_FUNCTION) {
      continue;
    }
    
    int stack_offset = 0;
    asm_pseudo_register_pass(top_level_node, backend_symbol_table, &stack_offset);
    
    if (top_level_node->data.function.instruction_pointers->asm_pointers[0]->type != ASM_INSTRUCTION_BINARY) {
      fprintf(stderr, "ERROR - Assembler: First instruction is not Binary Instruction for the '%s' function", program->data.program.top_level_pointers->asm_pointers[i]->data.function.name);
      exit(1);
    } 

    stack_offset = round_stack_offset(stack_offset);

    top_level_node->data.function.instruction_pointers->asm_pointers[0]->data.instruction_binary.operand_1->data.operand_imm.value = stack_offset;    

    AsmNode *new_function = asm_resolve_instructions(top_level_node, asm_arena);

    top_level_node = new_function;
  }
  
  return program;
}

AsmNode* asm_resolve_instructions(AsmNode *function, Arena *asm_arena) {
  AsmNodePointers *new_instructions = malloc(sizeof(AsmNodePointers));
  asm_init_node_pointer(new_instructions);
  
  AsmNode *new_function = arena_alloc(asm_arena);
  new_function->type = ASM_FUNCTION;
  new_function->data.function.name = function->data.function.name;
  new_function->data.function.is_global = function->data.function.is_global;
  new_function->data.function.instruction_count = 0;
  new_function->data.function.instruction_pointers = new_instructions;
  
  AsmNodePointers *instruction_ptr = function->data.function.instruction_pointers;

  for (int i = 0; i < function->data.function.instruction_count; i++) {
    AsmNodeType instruction_type = instruction_ptr->asm_pointers[i]->type;

    if (instruction_type == ASM_INSTRUCTION_MOV && instruction_ptr->asm_pointers[i]->data.instruction_mov.destination->type == ASM_OPERAND_STACK && instruction_ptr->asm_pointers[i]->data.instruction_mov.source->type == ASM_OPERAND_STACK) {
      //MOV instructions cannot have both a source and destination as memory addresses
      asm_resolve_mov_memory_addresses(new_function, instruction_ptr->asm_pointers[i], asm_arena);
      continue;
    } else if (instruction_type == ASM_INSTRUCTION_CMP && instruction_ptr->asm_pointers[i]->data.instruction_cmp.operand_1->type == ASM_OPERAND_STACK && instruction_ptr->asm_pointers[i]->data.instruction_cmp.operand_2->type == ASM_OPERAND_STACK) {
      //CMP instructions cannot have both a source and destination as memory addresses
      asm_resolve_cmp_memory_addresses(new_function, instruction_ptr->asm_pointers[i], asm_arena);
      continue;      
    } else if (instruction_type == ASM_INSTRUCTION_CMP && instruction_ptr->asm_pointers[i]->data.instruction_cmp.operand_2->type == ASM_OPERAND_IMM) {
      //CMP instructions cannot have a constant as the second operand.
      //TODO: Investigate if this is also needed for sub, add, and imul instructions
      asm_resolve_cmp_constant_in_operand_2(new_function, instruction_ptr->asm_pointers[i], asm_arena);
      continue;
    } else if (instruction_type == ASM_INSTRUCTION_BINARY && (instruction_ptr->asm_pointers[i]->data.instruction_binary.binary_op == ASM_BINARY_ADD || instruction_ptr->asm_pointers[i]->data.instruction_binary.binary_op == ASM_BINARY_SUB)  && (instruction_ptr->asm_pointers[i]->data.instruction_binary.operand_1->type == ASM_OPERAND_STACK && instruction_ptr->asm_pointers[i]->data.instruction_binary.operand_2->type == ASM_OPERAND_STACK)) {
      //ADD and SUB instructions cannot have both a source and destination as memory addresses
      asm_resolve_binary_add_sub_memory_addresses(new_function, instruction_ptr->asm_pointers[i], asm_arena);
      continue;
    } else if (instruction_type == ASM_INSTRUCTION_BINARY && instruction_ptr->asm_pointers[i]->data.instruction_binary.binary_op == ASM_BINARY_MULT && instruction_ptr->asm_pointers[i]->data.instruction_binary.operand_2->type == ASM_OPERAND_STACK) {
      //MUL instructions cannot use a memory address as its destination
      asm_resolve_binary_mul_memory_addresses(new_function, instruction_ptr->asm_pointers[i], asm_arena);
      continue;
    } else if (instruction_ptr->asm_pointers[i]->type == ASM_INSTRUCTION_IDIV && instruction_ptr->asm_pointers[i]->data.instruction_idiv.operand->type == ASM_OPERAND_IMM) {
      //IDIV instructions need to be copied into a scratch buffer if the operand is a constant
      asm_resolve_idiv_instructions(new_function, instruction_ptr->asm_pointers[i], asm_arena);
      continue;
    }

    AsmNode *new_instruction = arena_alloc(asm_arena); 
    new_instruction->type = instruction_ptr->asm_pointers[i]->type;
    new_instruction->data = instruction_ptr->asm_pointers[i]->data;

    asm_add_instruction_to_function(new_function, new_instruction);
  }

  return new_function;
}

void asm_resolve_idiv_instructions(AsmNode *function, AsmNode *idiv_instruction, Arena *asm_arena) {
  AsmNode *mov_instruction = arena_alloc(asm_arena);
  mov_instruction->type = ASM_INSTRUCTION_MOV;
  mov_instruction->data.instruction_mov.source = idiv_instruction->data.instruction_idiv.operand;
  mov_instruction->data.instruction_mov.assembly_type = idiv_instruction->data.instruction_idiv.assembly_type;

  AsmNode *destination = arena_alloc(asm_arena);
  destination->type = ASM_OPERAND_REGISTER;
  destination->data.operand_register.op_register = ASM_REGISTER_R10;    

  mov_instruction->data.instruction_mov.destination = destination;

  asm_add_instruction_to_function(function, mov_instruction);

  AsmNode *new_idiv_instruction = arena_alloc(asm_arena);
  new_idiv_instruction->type = ASM_INSTRUCTION_IDIV;
  new_idiv_instruction->data.instruction_idiv.operand = destination;
  new_idiv_instruction->data.instruction_idiv.assembly_type = idiv_instruction->data.instruction_idiv.assembly_type;

  asm_add_instruction_to_function(function, new_idiv_instruction);
}

void asm_resolve_binary_mul_memory_addresses(AsmNode *function, AsmNode *instruction, Arena *asm_arena) {
  AsmNode *mov_instruction = arena_alloc(asm_arena);
  mov_instruction->type = ASM_INSTRUCTION_MOV;
  mov_instruction->data.instruction_mov.source = instruction->data.instruction_binary.operand_2;
  mov_instruction->data.instruction_mov.assembly_type = instruction->data.instruction_binary.assembly_type;
  
  AsmNode *destination = arena_alloc(asm_arena);
  destination->type = ASM_OPERAND_REGISTER;
  destination->data.operand_register.op_register = ASM_REGISTER_R11;    

  mov_instruction->data.instruction_mov.destination = destination;
  
  asm_add_instruction_to_function(function, mov_instruction);

  AsmNode *mull_instruction = arena_alloc(asm_arena);
  mull_instruction->type = ASM_INSTRUCTION_BINARY;
  mull_instruction->data.instruction_binary.binary_op = ASM_BINARY_MULT;
  mull_instruction->data.instruction_binary.operand_1 = instruction->data.instruction_binary.operand_1;
  mull_instruction->data.instruction_binary.operand_2 = destination;
  mull_instruction->data.instruction_binary.assembly_type = instruction->data.instruction_binary.assembly_type;

  asm_add_instruction_to_function(function, mull_instruction);

  AsmNode *mov_instruction_2 = arena_alloc(asm_arena);
  mov_instruction_2->type = ASM_INSTRUCTION_MOV;
  mov_instruction_2->data.instruction_mov.source = destination;
  mov_instruction_2->data.instruction_mov.destination = instruction->data.instruction_binary.operand_2;
  mov_instruction_2->data.instruction_mov.assembly_type = instruction->data.instruction_binary.assembly_type;

  asm_add_instruction_to_function(function, mov_instruction_2);
}

void asm_resolve_binary_add_sub_memory_addresses(AsmNode *function, AsmNode *instruction, Arena *asm_arena) {
  AsmNode *mov_instruction = arena_alloc(asm_arena);
  mov_instruction->type = ASM_INSTRUCTION_MOV;
  mov_instruction->data.instruction_mov.source = instruction->data.instruction_binary.operand_1;
  mov_instruction->data.instruction_mov.assembly_type = instruction->data.instruction_binary.assembly_type;

  AsmNode *destination = arena_alloc(asm_arena);
  destination->type = ASM_OPERAND_REGISTER;
  destination->data.operand_register.op_register = ASM_REGISTER_R10;    

  mov_instruction->data.instruction_mov.destination = destination;

  asm_add_instruction_to_function(function, mov_instruction);

  AsmNode *binary_instruction = arena_alloc(asm_arena);
  binary_instruction->type = ASM_INSTRUCTION_BINARY;
  binary_instruction->data.instruction_binary.operand_1 = destination;
  binary_instruction->data.instruction_binary.operand_2 = instruction->data.instruction_binary.operand_2;
  binary_instruction->data.instruction_binary.assembly_type = instruction->data.instruction_binary.assembly_type;

  asm_add_instruction_to_function(function, binary_instruction);
}

void asm_resolve_cmp_constant_in_operand_2(AsmNode *function, AsmNode *instruction, Arena *asm_arena) {
  AsmNode *r11_register = arena_alloc(asm_arena);
  r11_register->type = ASM_OPERAND_REGISTER;
  r11_register->data.operand_register.op_register = ASM_REGISTER_R11;

  AsmNode *mov_instruction = arena_alloc(asm_arena);
  mov_instruction->type = ASM_INSTRUCTION_MOV;
  mov_instruction->data.instruction_mov.source = instruction->data.instruction_cmp.operand_2;
  mov_instruction->data.instruction_mov.assembly_type = instruction->data.instruction_cmp.assembly_type;
  mov_instruction->data.instruction_mov.destination = r11_register;

  asm_add_instruction_to_function(function, mov_instruction);

  AsmNode *eax_register = arena_alloc(asm_arena);
  eax_register->type = ASM_OPERAND_REGISTER;
  eax_register->data.operand_register.op_register = ASM_REGISTER_AX;
  
  AsmNode *cmp_instruction = arena_alloc(asm_arena);
  cmp_instruction->type = ASM_INSTRUCTION_CMP;
  cmp_instruction->data.instruction_cmp.operand_1 = eax_register;
  cmp_instruction->data.instruction_cmp.operand_2 = r11_register;
  cmp_instruction->data.instruction_cmp.assembly_type = instruction->data.instruction_cmp.assembly_type;

  asm_add_instruction_to_function(function, cmp_instruction);
}

void asm_resolve_cmp_memory_addresses(AsmNode *function, AsmNode *instruction, Arena *asm_arena) {
  AsmNode *r10_register = arena_alloc(asm_arena);
  r10_register->type = ASM_OPERAND_REGISTER;
  r10_register->data.operand_register.op_register = ASM_REGISTER_R10;

  AsmNode *mov_instruction = arena_alloc(asm_arena);
  mov_instruction->type = ASM_INSTRUCTION_MOV;
  mov_instruction->data.instruction_mov.source = instruction->data.instruction_cmp.operand_1;
  mov_instruction->data.instruction_mov.assembly_type = instruction->data.instruction_cmp.assembly_type;
  mov_instruction->data.instruction_mov.destination = r10_register;

  asm_add_instruction_to_function(function, mov_instruction);

  AsmNode *cmp_instruction = arena_alloc(asm_arena);
  cmp_instruction->type = ASM_INSTRUCTION_CMP;
  cmp_instruction->data.instruction_cmp.operand_1 = r10_register;
  cmp_instruction->data.instruction_cmp.operand_2 = instruction->data.instruction_cmp.operand_2;
  cmp_instruction->data.instruction_cmp.assembly_type = instruction->data.instruction_cmp.assembly_type;
  
  asm_add_instruction_to_function(function, cmp_instruction);
}

void asm_resolve_mov_memory_addresses(AsmNode *function, AsmNode *instruction, Arena *asm_arena) {
    AsmNode *new_source_mov_instruction = arena_alloc(asm_arena);
    new_source_mov_instruction->type = ASM_INSTRUCTION_MOV;
    new_source_mov_instruction->data.instruction_mov.source = instruction->data.instruction_mov.source;
    new_source_mov_instruction->data.instruction_mov.assembly_type = instruction->data.instruction_mov.assembly_type;

    AsmNode *new_destination = arena_alloc(asm_arena);
    new_destination->type = ASM_OPERAND_REGISTER;
    new_destination->data.operand_register.op_register = ASM_REGISTER_R10;    

    new_source_mov_instruction->data.instruction_mov.destination = new_destination;    

    asm_add_instruction_to_function(function, new_source_mov_instruction);

    AsmNode *new_source = arena_alloc(asm_arena);
    new_source->type = ASM_OPERAND_REGISTER;
    new_source->data.operand_register.op_register = ASM_REGISTER_R10;
        
    AsmNode *new_destination_mov_instruction = arena_alloc(asm_arena);
    new_destination_mov_instruction->type = ASM_INSTRUCTION_MOV;
    new_destination_mov_instruction->data.instruction_mov.source = new_source;
    new_destination_mov_instruction->data.instruction_mov.destination = instruction->data.instruction_mov.destination;
    new_destination_mov_instruction->data.instruction_mov.assembly_type = instruction->data.instruction_mov.assembly_type;

    asm_add_instruction_to_function(function, new_destination_mov_instruction);
}

void asm_pseudo_register_pass(AsmNode *asm_function, AsmBackendSymbolTable *backend_symbol_table, int *stack_offset) {
  HashTable stack_location_table;
  hash_table_init(&stack_location_table);
  
  for (int i = 0; i < asm_function->data.function.instruction_count; i++) {
    AsmNode *instruction = asm_function->data.function.instruction_pointers->asm_pointers[i];

    switch(instruction->type) {
      case ASM_INSTRUCTION_MOV:        
        if (instruction->data.instruction_mov.source->type == ASM_OPERAND_PSEUDO_REGISTER) {
         asm_replace_pseudo_register(instruction->data.instruction_mov.source, instruction->data.instruction_mov.assembly_type, &stack_location_table, backend_symbol_table, stack_offset);
        }

        if (instruction->data.instruction_mov.destination->type == ASM_OPERAND_PSEUDO_REGISTER) {
         asm_replace_pseudo_register(instruction->data.instruction_mov.destination, instruction->data.instruction_mov.assembly_type, &stack_location_table, backend_symbol_table, stack_offset);        
        }
        break;
      case ASM_INSTRUCTION_MOVSX:
        if (instruction->data.instruction_movsx.source->type == ASM_OPERAND_PSEUDO_REGISTER) {
         asm_replace_pseudo_register(instruction->data.instruction_movsx.source, ASM_TYPE_LONGWORD, &stack_location_table, backend_symbol_table, stack_offset);
        }

        if (instruction->data.instruction_movsx.destination->type == ASM_OPERAND_PSEUDO_REGISTER) {
         asm_replace_pseudo_register(instruction->data.instruction_movsx.destination, ASM_TYPE_LONGWORD, &stack_location_table, backend_symbol_table, stack_offset);
        }
        break;
      case ASM_INSTRUCTION_UNARY:
        if (instruction->data.instruction_unary.operand->type == ASM_OPERAND_PSEUDO_REGISTER) {
         asm_replace_pseudo_register(instruction->data.instruction_unary.operand, instruction->data.instruction_unary.assembly_type, &stack_location_table, backend_symbol_table, stack_offset);        
        }
        break;
      case ASM_INSTRUCTION_BINARY:
        if (instruction->data.instruction_binary.operand_1->type == ASM_OPERAND_PSEUDO_REGISTER) {
         asm_replace_pseudo_register(instruction->data.instruction_binary.operand_1, instruction->data.instruction_binary.assembly_type, &stack_location_table, backend_symbol_table, stack_offset);        
        }

        if (instruction->data.instruction_binary.operand_2->type == ASM_OPERAND_PSEUDO_REGISTER) {
         asm_replace_pseudo_register(instruction->data.instruction_binary.operand_2, instruction->data.instruction_binary.assembly_type, &stack_location_table, backend_symbol_table, stack_offset);        
        }
        break;
      case ASM_INSTRUCTION_IDIV:
        if (instruction->data.instruction_idiv.operand->type == ASM_OPERAND_PSEUDO_REGISTER) {
         asm_replace_pseudo_register(instruction->data.instruction_idiv.operand, instruction->data.instruction_idiv.assembly_type, &stack_location_table, backend_symbol_table, stack_offset);        
        }
        break;
      case ASM_INSTRUCTION_CMP:
        if (instruction->data.instruction_cmp.operand_1->type == ASM_OPERAND_PSEUDO_REGISTER) {
          asm_replace_pseudo_register(instruction->data.instruction_cmp.operand_1, instruction->data.instruction_cmp.assembly_type, &stack_location_table, backend_symbol_table, stack_offset);
        }        

        if (instruction->data.instruction_cmp.operand_2->type == ASM_OPERAND_PSEUDO_REGISTER) {
          asm_replace_pseudo_register(instruction->data.instruction_cmp.operand_2, instruction->data.instruction_cmp.assembly_type, &stack_location_table, backend_symbol_table, stack_offset);
        }        
        break;
      case ASM_INSTRUCTION_PUSH:
        if (instruction->data.instruction_push.operand->type == ASM_OPERAND_PSEUDO_REGISTER) {
          asm_replace_pseudo_register(instruction->data.instruction_push.operand, ASM_TYPE_LONGWORD, &stack_location_table, backend_symbol_table, stack_offset);
        }
        break;
      default:
        break;
    }
  }
}

void asm_replace_pseudo_register(AsmNode *pseudo_register, AsmType instruction_type, HashTable *stack_location_table, AsmBackendSymbolTable *backend_symbol_table, int *stack_offset) {
  HashTableEntry *table_entry = hash_table_get_entry(stack_location_table, pseudo_register->data.operand_pseudo_register.identifier);

  if (table_entry != NULL && table_entry->key != NULL) {
    pseudo_register->type = ASM_OPERAND_STACK;
    pseudo_register->data.operand_pseudo_register.identifier = NULL;
    pseudo_register->data.operand_stack.address = table_entry->value->integer;
    return;
  }

  HashTableEntry *existing_backend_symbol = hash_table_get_entry(backend_symbol_table->symbol_table, pseudo_register->data.operand_pseudo_register.identifier);

  if (existing_backend_symbol != NULL && existing_backend_symbol->key != NULL) {    
    AsmBackendSymbol *symbol = existing_backend_symbol->value->structure;    

    if (symbol->type == ASM_SYMBOL_FUNCTION_ENTRY) {
      fprintf(stderr, "ERROR - Assembler: ASM backend function symbol '%s' found when attempting to resolve pseudo registers. \n", existing_backend_symbol->key);
      exit(1);
    }
    
    if (!symbol->data.object_entry.is_static) {
      goto process_pseudo_register;
    }
    
    pseudo_register->type = ASM_OPERAND_DATA;
    pseudo_register->data.operand_data.identifier = pseudo_register->data.operand_pseudo_register.identifier;
    return;
  }

  process_pseudo_register:
      if (instruction_type == ASM_TYPE_QUADWORD) {
        *stack_offset += 8;
        *stack_offset = round_stack_offset(*stack_offset);
      } else {
        *stack_offset += 4;
      }
  
      HashValue value = {
        .integer = *stack_offset,
        .type = HASH_INT
      };
  
      HashTableEntry new_entry = {
        .key = pseudo_register->data.operand_pseudo_register.identifier,
        .value = &value
      };

      hash_table_add_entry(stack_location_table, &new_entry);    

      pseudo_register->type = ASM_OPERAND_STACK;
      pseudo_register->data.operand_pseudo_register.identifier = NULL;
      pseudo_register->data.operand_stack.address = *stack_offset;
}

void asm_function(IRNode *ir_function, AsmNode *asm_function, Arena *asm_arena, DeclarationSymbolTable *declaration_symbol_table) {
  asm_function->type = ASM_FUNCTION;
  asm_function->data.function.name = ir_function->data.function.identifier;
  asm_function->data.function.is_global = ir_function->data.function.is_global;
  
  AsmNodePointers *asm_pointers = malloc(sizeof(AsmNodePointers));
  asm_init_node_pointer(asm_pointers);
  asm_function->data.function.instruction_count = 0;
  asm_function->data.function.instruction_pointers = asm_pointers;

  //Adds the Allocate Stack instruction, but will allocate the stack offset value of the instruction in another pass after building the assembly nodes
  asm_instruction_allocate_rsp_stack(asm_function, 0, asm_arena);

  //Parameter instructions
  int stack_offset = 16;

  for (int i = 0; i < ir_function->data.function.parameter_count; i++) {    
    AsmNode *source_operand = arena_alloc(asm_arena);

    if (i == 0) {
      source_operand->data.operand_register.op_register = ASM_REGISTER_DI;
      source_operand->type = ASM_OPERAND_REGISTER;
    } else if (i == 1) {
      source_operand->data.operand_register.op_register = ASM_REGISTER_SI;
      source_operand->type = ASM_OPERAND_REGISTER;
    } else if (i == 2) {
      source_operand->data.operand_register.op_register = ASM_REGISTER_DX;
      source_operand->type = ASM_OPERAND_REGISTER;
    } else if (i == 3) {
      source_operand->data.operand_register.op_register = ASM_REGISTER_CX;
      source_operand->type = ASM_OPERAND_REGISTER;
    } else if (i == 4) {
      source_operand->data.operand_register.op_register = ASM_REGISTER_R8;
      source_operand->type = ASM_OPERAND_REGISTER;
    } else if (i == 5) {
      source_operand->data.operand_register.op_register = ASM_REGISTER_R9;
      source_operand->type = ASM_OPERAND_REGISTER;
    } else {
      source_operand->data.operand_stack.address = stack_offset;
      source_operand->type = ASM_OPERAND_STACK;
      stack_offset += 8;
    }
    
    AsmNode *destination_pseudo_register = arena_alloc(asm_arena);
    destination_pseudo_register->type = ASM_OPERAND_PSEUDO_REGISTER;
    destination_pseudo_register->data.operand_pseudo_register.identifier = ir_function->data.function.parameter_identifiers[i];

    AsmNode *mov_instruction = arena_alloc(asm_arena);
    mov_instruction->type = ASM_INSTRUCTION_MOV;
    mov_instruction->data.instruction_mov.source = source_operand;
    mov_instruction->data.instruction_mov.destination = destination_pseudo_register;

    asm_add_instruction_to_function(asm_function, mov_instruction);
  }

  //Function block instructions
  for (int i = 0; i < ir_function->data.function.instruction_count; i++) {
    IRNode *current_ir_node = ir_function->data.function.instruction_ptrs->node_pointers[i];
    switch (current_ir_node->type) {
      case IR_INSTRUCTION_RET:
        asm_instruction_return(asm_function, current_ir_node, asm_arena, declaration_symbol_table);
        break;
      case IR_INSTRUCTION_UNARY:
        if (current_ir_node->data.unary.op_type == IR_UNARY_NOT) {
          asm_instruction_unary_not(asm_function, current_ir_node, asm_arena, declaration_symbol_table);
        } else {
          asm_instruction_unary(asm_function, current_ir_node, asm_arena, declaration_symbol_table);
        }
        break;
      case IR_INSTRUCTION_BINARY:        
        switch (current_ir_node->data.instruction_binary.op_type) {
          case IR_BINARY_ADD:
          case IR_BINARY_SUBTRACT:
          case IR_BINARY_MULTIPLY:
          case IR_BINARY_BITWISE_AND:
          case IR_BINARY_BITWISE_OR:
          case IR_BINARY_BITWISE_XOR:
          case IR_BINARY_BITWISE_LEFT_SHIFT:
          case IR_BINARY_BITWISE_RIGHT_SHIFT:
            asm_instruction_binary(asm_function, current_ir_node, asm_arena, declaration_symbol_table);
            break;
          case IR_BINARY_EQUAL:
          case IR_BINARY_NOT_EQUAL:
          case IR_BINARY_GREATER_THAN:
          case IR_BINARY_GREATER_OR_EQUAL:
          case IR_BINARY_LESS_THAN:
          case IR_BINARY_LESS_OR_EQUAL:
            asm_instruction_binary_relational(asm_function, current_ir_node, asm_arena, declaration_symbol_table);
          case IR_BINARY_DIVIDE:
          case IR_BINARY_REMAINDER:
            asm_instruction_binary_division(asm_function, current_ir_node, asm_arena, declaration_symbol_table);            
            break;
        }
          break;
        case IR_INSTRUCTION_JUMP:
          asm_instruction_jump(asm_function, current_ir_node, asm_arena);
          break;
        case IR_INSTRUCTION_JUMP_IF_ZERO:
          asm_instruction_jump_if_zero(asm_function, current_ir_node, asm_arena, declaration_symbol_table);
          break;
        case IR_INSTRUCTION_JUMP_IF_NOT_ZERO:
          asm_instruction_jump_if_not_zero(asm_function, current_ir_node, asm_arena, declaration_symbol_table);
          break;
        case IR_INSTRUCTION_COPY:
          asm_instruction_copy(asm_function, current_ir_node, asm_arena, declaration_symbol_table);
          break;
        case IR_INSTRUCTION_LABEL:
          asm_instruction_label(asm_function, current_ir_node, asm_arena);
          break;
        case IR_INSTRUCTION_FUNCTION_CALL:
          asm_instruction_function_call(asm_function, current_ir_node, asm_arena, declaration_symbol_table);
          break;
        case IR_INSTRUCTION_SIGN_EXTEND:
          asm_instruction_sign_extend(asm_function, current_ir_node, asm_arena);
          break;
        case IR_INSTRUCTION_TRUNCATE:
          asm_instruction_truncate(asm_function, current_ir_node, asm_arena);
          break;
      default:
        fprintf(stderr, "ERROR - Assembler: Could not resolve instruction type in asm_function\n");
        exit(1);
    }
  }
}

void asm_static_variable(IRNode *ir_static_variable, AsmNode *asm_static_variable) {
  asm_static_variable->type = ASM_STATIC_VARIABLE;
  asm_static_variable->data.static_variable.identifier = ir_static_variable->data.static_variable.identifier;
  asm_static_variable->data.static_variable.static_variable_symbol = ir_static_variable->data.static_variable.static_variable_symbol;
  asm_static_variable->data.static_variable.is_global = ir_static_variable->data.static_variable.is_global;

  switch (ir_static_variable->data.static_variable.static_variable_symbol->value_type) {
    case DECLARATION_SYMBOL_TYPE_INT:   asm_static_variable->data.static_variable.alignment = ALIGNMENT_LONGWORD; break;
    case DECLARATION_SYMBOL_TYPE_LONG:  asm_static_variable->data.static_variable.alignment = ALIGNMENT_QUADWORD; break;
    default:
      fprintf(stderr, "ERROR: Assembler - Could not assign alignment value to static variable '%s'", ir_static_variable->data.static_variable.identifier);
      exit(1);
  }  
}

void asm_instruction_allocate_rsp_stack(AsmNode *asm_function, int bytes, Arena *asm_arena) {
  AsmNode *imm_operand = arena_alloc(asm_arena);
  imm_operand->type = ASM_OPERAND_IMM;
  imm_operand->data.operand_imm.value = bytes;

  AsmNode *register_operand = arena_alloc(asm_arena);
  register_operand->type = ASM_OPERAND_REGISTER;
  register_operand->data.operand_register.op_register = ASM_REGISTER_SP;

  AsmNode *binary_instruction = arena_alloc(asm_arena);
  binary_instruction->type = ASM_INSTRUCTION_BINARY;
  binary_instruction->data.instruction_binary.assembly_type = ASM_TYPE_QUADWORD;
  binary_instruction->data.instruction_binary.binary_op = ASM_BINARY_SUB;
  binary_instruction->data.instruction_binary.operand_1 = imm_operand;
  binary_instruction->data.instruction_binary.operand_2 = register_operand;

  asm_add_instruction_to_function(asm_function, binary_instruction);
}

void asm_instruction_label(AsmNode *asm_function, IRNode *ir_label_instruction, Arena *asm_arena) {
  AsmNode *label = arena_alloc(asm_arena);
  label->type = ASM_INSTRUCTION_LABEL;
  label->data.instruction_label.identifier = ir_label_instruction->data.instruction_label.identifier;

  asm_add_instruction_to_function(asm_function, label);
}

void asm_instruction_copy(AsmNode *asm_function, IRNode *ir_copy_instruction, Arena *asm_arena, DeclarationSymbolTable *declaration_symbol_table) {
  AsmNode *source = asm_operand(ir_copy_instruction->data.instruction_copy.source, asm_arena);
  AsmNode *destination = asm_operand(ir_copy_instruction->data.instruction_copy.destination, asm_arena);

  AsmType source_type = convert_ir_value_to_asm_type(ir_copy_instruction->data.instruction_copy.source, declaration_symbol_table);

  AsmNode *mov_instruction = arena_alloc(asm_arena);
  mov_instruction->type = ASM_INSTRUCTION_MOV;
  mov_instruction->data.instruction_mov.source = source;
  mov_instruction->data.instruction_mov.destination = destination;
  mov_instruction->data.instruction_mov.assembly_type = source_type;

  asm_add_instruction_to_function(asm_function, mov_instruction);
}

void asm_instruction_jump(AsmNode *asm_function, IRNode *ir_jump_instruction, Arena *asm_arena) {
  AsmNode *jmp_instruction = arena_alloc(asm_arena);
  jmp_instruction->type = ASM_INSTRUCTION_JMP;
  jmp_instruction->data.instruction_jmp.identifier = ir_jump_instruction->data.instruction_jump.target;

  asm_add_instruction_to_function(asm_function, jmp_instruction); 
}

void asm_instruction_jump_if_zero(AsmNode *asm_function, IRNode *ir_jump_if_zero_instruction, Arena *asm_arena, DeclarationSymbolTable *declaration_symbol_table) {
  AsmNode *imm = arena_alloc(asm_arena);
  imm->type = ASM_OPERAND_IMM;
  imm->data.operand_imm.value = 0;
  
  AsmNode *condition = asm_operand(ir_jump_if_zero_instruction->data.instruction_jump_if_zero.condition, asm_arena);
  AsmNode *cmp_instruction = arena_alloc(asm_arena);

  cmp_instruction->type = ASM_INSTRUCTION_CMP;
  cmp_instruction->data.instruction_cmp.operand_1 = imm;
  cmp_instruction->data.instruction_cmp.operand_2 = condition;
  
  //@NOTE: Need to add these so that the convert function doesn't fail. However, need to investigate on whether at this point we do something for assembly_type's when the operand is a pseudo register
  if (condition->type != ASM_OPERAND_PSEUDO_REGISTER) {
    AsmType source_type = convert_ir_value_to_asm_type(ir_jump_if_zero_instruction->data.instruction_jump_if_zero.condition, declaration_symbol_table);
    cmp_instruction->data.instruction_cmp.assembly_type = source_type;
  }

  asm_add_instruction_to_function(asm_function, cmp_instruction);
  
  AsmNode *jmp_instruction = arena_alloc(asm_arena);
  jmp_instruction->type = ASM_INSTRUCTION_JMPCC;
  jmp_instruction->data.instruction_jmp_cc.condition_code = ASM_CONDITION_EQUAL;
  jmp_instruction->data.instruction_jmp_cc.identifier = ir_jump_if_zero_instruction->data.instruction_jump_if_zero.target;

  asm_add_instruction_to_function(asm_function, jmp_instruction);
}

void asm_instruction_jump_if_not_zero(AsmNode *asm_function, IRNode *ir_jump_if_not_zero_instruction, Arena *asm_arena, DeclarationSymbolTable *declaration_symbol_table) {
  AsmNode *imm = arena_alloc(asm_arena);
  imm->type = ASM_OPERAND_IMM;
  imm->data.operand_imm.value = 0;
  
  AsmNode *condition = asm_operand(ir_jump_if_not_zero_instruction->data.instruction_jump_if_not_zero.condition, asm_arena);
  AsmNode *cmp_instruction = arena_alloc(asm_arena);

  cmp_instruction->type = ASM_INSTRUCTION_CMP;
  cmp_instruction->data.instruction_cmp.operand_1 = imm;
  cmp_instruction->data.instruction_cmp.operand_2 = condition;
  
  //@NOTE: Need to add these so that the convert function doesn't fail. However, need to investigate on whether at this point we do something for assembly_type's when the operand is a pseudo register
  if (condition->type != ASM_OPERAND_PSEUDO_REGISTER) {
    AsmType source_type = convert_ir_value_to_asm_type(ir_jump_if_not_zero_instruction->data.instruction_jump_if_not_zero.condition, declaration_symbol_table);
    cmp_instruction->data.instruction_cmp.assembly_type = source_type;
  }

  asm_add_instruction_to_function(asm_function, cmp_instruction);
  
  AsmNode *jmp_instruction = arena_alloc(asm_arena);
  jmp_instruction->type = ASM_INSTRUCTION_JMPCC;
  jmp_instruction->data.instruction_jmp_cc.condition_code = ASM_CONDITION_NOT_EQUAL;
  jmp_instruction->data.instruction_jmp_cc.identifier = ir_jump_if_not_zero_instruction->data.instruction_jump_if_not_zero.target;

  asm_add_instruction_to_function(asm_function, jmp_instruction);
}

void asm_instruction_binary(AsmNode *asm_function, IRNode *ir_binary_instruction, Arena *asm_arena, DeclarationSymbolTable *declaration_symbol_table) {
  AsmNode *source_1 = asm_operand(ir_binary_instruction->data.instruction_binary.source_1, asm_arena);
  AsmNode *source_2 = asm_operand(ir_binary_instruction->data.instruction_binary.source_2, asm_arena);
  AsmNode *destination_node = asm_operand(ir_binary_instruction->data.instruction_binary.destination, asm_arena);

  AsmType source_1_type = convert_ir_value_to_asm_type(ir_binary_instruction->data.instruction_binary.source_1, declaration_symbol_table);
  
  AsmNode *mov_instruction = arena_alloc(asm_arena);
  mov_instruction->type = ASM_INSTRUCTION_MOV;
  mov_instruction->data.instruction_mov.source = source_1;
  mov_instruction->data.instruction_mov.destination = destination_node;
  mov_instruction->data.instruction_mov.assembly_type = source_1_type;

  AsmNode *binary_instruction = arena_alloc(asm_arena);
  binary_instruction->type = ASM_INSTRUCTION_BINARY;
  binary_instruction->data.instruction_binary.operand_1 = source_2;
  binary_instruction->data.instruction_binary.operand_2 = destination_node;
  binary_instruction->data.instruction_binary.assembly_type = source_1_type;

  switch (ir_binary_instruction->data.instruction_binary.op_type) {
    case IR_BINARY_ADD:
      binary_instruction->data.instruction_binary.binary_op = ASM_BINARY_ADD;
      break;
    case IR_BINARY_SUBTRACT:
      binary_instruction->data.instruction_binary.binary_op = ASM_BINARY_SUB;
      break;
    case IR_BINARY_MULTIPLY:
      binary_instruction->data.instruction_binary.binary_op = ASM_BINARY_MULT;
      break;
    case IR_BINARY_BITWISE_AND:
      binary_instruction->data.instruction_binary.binary_op = ASM_BINARY_BITWISE_AND;
      break;
    case IR_BINARY_BITWISE_OR:
      binary_instruction->data.instruction_binary.binary_op = ASM_BINARY_BITWISE_OR;
      break;
    case IR_BINARY_BITWISE_XOR:
      binary_instruction->data.instruction_binary.binary_op = ASM_BINARY_BITWISE_XOR;
      break;
    case IR_BINARY_BITWISE_LEFT_SHIFT:
      binary_instruction->data.instruction_binary.binary_op = ASM_BINARY_BITWISE_LEFT_SHIFT;
      break;
    case IR_BINARY_BITWISE_RIGHT_SHIFT:
      binary_instruction->data.instruction_binary.binary_op = ASM_BINARY_BITWISE_RIGHT_SHIFT;
      break;
    default:
      fprintf(stderr, "ERROR - Assembler: Operator type not found for binary operation");
      exit(1);
      break;
  }

  asm_add_instruction_to_function(asm_function, mov_instruction);
  asm_add_instruction_to_function(asm_function, binary_instruction);
}

void asm_instruction_unary_not(AsmNode *asm_function, IRNode *ir_unary_not_instruction, Arena *asm_arena, DeclarationSymbolTable *declaration_symbol_table) {
  AsmNode *source = asm_operand(ir_unary_not_instruction->data.unary.source, asm_arena);
  AsmNode *destination_node = asm_operand(ir_unary_not_instruction->data.unary.destination, asm_arena);

  AsmType source_type = convert_ir_value_to_asm_type(ir_unary_not_instruction->data.unary.source, declaration_symbol_table);
  AsmType destination_type = convert_ir_value_to_asm_type(ir_unary_not_instruction->data.unary.destination, declaration_symbol_table);
  
  AsmNode *imm_operand = arena_alloc(asm_arena);
  imm_operand->type = ASM_OPERAND_IMM;
  imm_operand->data.operand_imm.value = 0;

  AsmNode *cmp_instruction = arena_alloc(asm_arena);

  cmp_instruction->type = ASM_INSTRUCTION_CMP;
  cmp_instruction->data.instruction_cmp.operand_1 = imm_operand;
  cmp_instruction->data.instruction_cmp.operand_2 = source;
  cmp_instruction->data.instruction_cmp.assembly_type = source_type;

  asm_add_instruction_to_function(asm_function, cmp_instruction);

  AsmNode *mov_instruction = arena_alloc(asm_arena);
  mov_instruction->type = ASM_INSTRUCTION_MOV;
  mov_instruction->data.instruction_mov.source = imm_operand;
  mov_instruction->data.instruction_mov.destination = destination_node;
  mov_instruction->data.instruction_mov.assembly_type = destination_type;

  asm_add_instruction_to_function(asm_function, mov_instruction);

  AsmNode *set_cc_instruction = arena_alloc(asm_arena);  
  set_cc_instruction->type = ASM_INSTRUCTION_SETCC;
  set_cc_instruction->data.instruction_set_cc.condition_code = ASM_CONDITION_EQUAL;
  set_cc_instruction->data.instruction_set_cc.operand = destination_node;

  asm_add_instruction_to_function(asm_function, set_cc_instruction);
}

void asm_instruction_binary_relational(AsmNode *asm_function, IRNode *ir_relational_instruction, Arena *asm_arena, DeclarationSymbolTable *declaration_symbol_table) {
  AsmNode *source_1 = asm_operand(ir_relational_instruction->data.instruction_binary.source_1, asm_arena);
  AsmNode *source_2 = asm_operand(ir_relational_instruction->data.instruction_binary.source_2, asm_arena);
  AsmNode *destination_node = asm_operand(ir_relational_instruction->data.instruction_binary.destination, asm_arena);

  AsmType source_1_type = convert_ir_value_to_asm_type(ir_relational_instruction->data.instruction_binary.source_1, declaration_symbol_table);
  AsmType destination_type = convert_ir_value_to_asm_type(ir_relational_instruction->data.instruction_binary.destination, declaration_symbol_table);
  
  AsmNode *cmp_instruction = arena_alloc(asm_arena);
  cmp_instruction->type = ASM_INSTRUCTION_CMP;
  cmp_instruction->data.instruction_cmp.operand_1 = source_2;
  cmp_instruction->data.instruction_cmp.operand_2 = source_1;
  cmp_instruction->data.instruction_cmp.assembly_type = source_1_type;
  
  asm_add_instruction_to_function(asm_function, cmp_instruction);

  AsmNode *imm_operand = arena_alloc(asm_arena);
  imm_operand->type = ASM_OPERAND_IMM;
  imm_operand->data.operand_imm.value = 0;

  AsmNode *mov_instruction = arena_alloc(asm_arena);
  mov_instruction->type = ASM_INSTRUCTION_MOV;
  mov_instruction->data.instruction_mov.source = imm_operand;
  mov_instruction->data.instruction_mov.destination = destination_node;
  mov_instruction->data.instruction_mov.assembly_type = destination_type;

  asm_add_instruction_to_function(asm_function, mov_instruction);

  AsmConditionCode relational_op;

  switch (ir_relational_instruction->data.instruction_binary.op_type) {
    case IR_BINARY_EQUAL:              relational_op = ASM_CONDITION_EQUAL; break;
    case IR_BINARY_NOT_EQUAL:          relational_op = ASM_CONDITION_NOT_EQUAL; break;
    case IR_BINARY_GREATER_THAN:       relational_op = ASM_CONDITION_GREATER; break;
    case IR_BINARY_GREATER_OR_EQUAL:   relational_op = ASM_CONDITION_GREATER_EQUAL; break;
    case IR_BINARY_LESS_THAN:          relational_op = ASM_CONDITION_LESS; break;
    case IR_BINARY_LESS_OR_EQUAL:      relational_op = ASM_CONDITION_LESS_EQUAL; break;      
    default:
      fprintf(stderr, "Binary Relational OP type %d is not found", ir_relational_instruction->data.instruction_binary.op_type);
      exit(1);
      break;
  }

  AsmNode *set_cc_instruction = arena_alloc(asm_arena);
  set_cc_instruction->type = ASM_INSTRUCTION_SETCC;  
  set_cc_instruction->data.instruction_set_cc.condition_code = relational_op;
  set_cc_instruction->data.instruction_set_cc.operand = destination_node;

  asm_add_instruction_to_function(asm_function, set_cc_instruction);
}

void asm_instruction_binary_division(AsmNode *asm_function, const IRNode *ir_binary_instruction, Arena *asm_arena, DeclarationSymbolTable *declaration_symbol_table) {
  AsmNode *source_1 = asm_operand(ir_binary_instruction->data.instruction_binary.source_1, asm_arena);
  AsmNode *source_2 = asm_operand(ir_binary_instruction->data.instruction_binary.source_2, asm_arena);
  AsmNode *destination_node = asm_operand(ir_binary_instruction->data.instruction_binary.destination, asm_arena);

  AsmType source_1_type = convert_ir_value_to_asm_type(ir_binary_instruction->data.instruction_binary.source_1, declaration_symbol_table);
  
  AsmNode *mov_instruction_1 = arena_alloc(asm_arena);
  mov_instruction_1->type = ASM_INSTRUCTION_MOV;
  mov_instruction_1->data.instruction_mov.source = source_1;
  mov_instruction_1->data.instruction_mov.assembly_type = source_1_type;

  AsmNode *mov_destination_1 = arena_alloc(asm_arena);
  mov_destination_1->type = ASM_OPERAND_REGISTER;
  mov_destination_1->data.operand_register.op_register = ASM_REGISTER_AX;  

  mov_instruction_1->data.instruction_mov.destination = mov_destination_1;

  asm_add_instruction_to_function(asm_function, mov_instruction_1);

  AsmNode *cdq_instruction = arena_alloc(asm_arena);
  cdq_instruction->type = ASM_INSTRUCTION_CDQ;
  cdq_instruction->data.instruction_cdq.assembly_type = source_1_type;

  asm_add_instruction_to_function(asm_function, cdq_instruction);
  
  AsmNode *idiv_instruction = arena_alloc(asm_arena);
  idiv_instruction->type = ASM_INSTRUCTION_IDIV;
  idiv_instruction->data.instruction_idiv.operand = source_2;
  idiv_instruction->data.instruction_idiv.assembly_type = source_1_type;

  asm_add_instruction_to_function(asm_function, idiv_instruction);

  AsmNode *mov_instruction_2 = arena_alloc(asm_arena);
  mov_instruction_2->type = ASM_INSTRUCTION_MOV;
  mov_instruction_2->data.instruction_mov.destination = destination_node;
  mov_instruction_2->data.instruction_mov.assembly_type = source_1_type;

  AsmNode *mov_destination_2 = arena_alloc(asm_arena);
  mov_destination_2->type = ASM_OPERAND_REGISTER;
  
  if (ir_binary_instruction->data.instruction_binary.op_type == IR_BINARY_DIVIDE) {
    mov_destination_2->data.operand_register.op_register = ASM_REGISTER_AX;
  } else {
    //IR_BINARY_REMAINDER
    mov_destination_2->data.operand_register.op_register = ASM_REGISTER_DX;
  }

  mov_instruction_2->data.instruction_mov.source = mov_destination_2;

  asm_add_instruction_to_function(asm_function, mov_instruction_2);
}
 
void asm_instruction_unary(AsmNode *asm_function, IRNode *ir_unary_instruction, Arena *asm_arena, DeclarationSymbolTable *declaration_symbol_table) {
  AsmNode *source_node = asm_operand(ir_unary_instruction->data.unary.source, asm_arena);
  AsmNode *destination_node = asm_operand(ir_unary_instruction->data.unary.destination, asm_arena);

  AsmType source_type = convert_ir_value_to_asm_type(ir_unary_instruction->data.unary.source, declaration_symbol_table);

  AsmNode *mov_node = arena_alloc(asm_arena);

  mov_node->type = ASM_INSTRUCTION_MOV;
  mov_node->data.instruction_mov.source = source_node;
  mov_node->data.instruction_mov.destination = destination_node;
  mov_node->data.instruction_mov.assembly_type = source_type;

  asm_add_instruction_to_function(asm_function, mov_node);
  
  AsmNode *unary_instruction = arena_alloc(asm_arena);
  unary_instruction->type = ASM_INSTRUCTION_UNARY;
  unary_instruction->data.instruction_unary.assembly_type = source_type;

  if (ir_unary_instruction->data.unary.op_type == IR_UNARY_NEGATE) {
    unary_instruction->data.instruction_unary.unary_op = ASM_UNARY_NEG;
  } else {
    unary_instruction->data.instruction_unary.unary_op = ASM_UNARY_NOT;
  }
  
  unary_instruction->data.instruction_unary.operand = destination_node;

  asm_add_instruction_to_function(asm_function, unary_instruction);
}

void asm_instruction_return(AsmNode *asm_function, IRNode *ir_return_instruction, Arena *asm_arena, DeclarationSymbolTable *declaration_symbol_table) {
  //Function calls were being duplicated without this check.
  if (ir_return_instruction->data.instruction_ret.value->type != IR_INSTRUCTION_FUNCTION_CALL) {
    AsmNode *source_node = asm_operand(ir_return_instruction->data.instruction_ret.value, asm_arena);

    AsmNode *destination_node = arena_alloc(asm_arena);
    destination_node->type = ASM_OPERAND_REGISTER;
    destination_node->data.operand_register.op_register = ASM_REGISTER_AX;  

    AsmNode *mov_node = arena_alloc(asm_arena);
    mov_node->type = ASM_INSTRUCTION_MOV;
    mov_node->data.instruction_mov.source = source_node;
    mov_node->data.instruction_mov.destination = destination_node;

    //@NOTE: Need to add these so that the convert function doesn't fail. However, need to investigate on whether at this point we do something for assembly_type's when the operand is a pseudo register
    if (source_node->type != ASM_OPERAND_PSEUDO_REGISTER) {
      AsmType source_type = convert_ir_value_to_asm_type(ir_return_instruction->data.instruction_ret.value, declaration_symbol_table);
      mov_node->data.instruction_mov.assembly_type = source_type;
    }

    asm_add_instruction_to_function(asm_function, mov_node);
  }

  AsmNode *ret_node = arena_alloc(asm_arena);
  ret_node->type = ASM_INSTRUCTION_RET;

  asm_add_instruction_to_function(asm_function, ret_node);
}

void asm_instruction_function_call(AsmNode *asm_function, IRNode *ir_function_call_instruction, Arena *asm_arena, DeclarationSymbolTable *declaration_symbol_table) {
  //As per the System V ABI (Application Binary Interface), the first 6 arguments of a function call will be loaded into the following 'arg_registers' as ordered in the array. After that, any additional arguments will be added to the stack in reverse order to be processed in the order of how they are called.
  AsmRegisterType arg_registers[] = { ASM_REGISTER_DI, ASM_REGISTER_SI, ASM_REGISTER_DX, ASM_REGISTER_CX, ASM_REGISTER_R8, ASM_REGISTER_R9 };
  int arg_count = ir_function_call_instruction->data.instruction_function_call.arg_count;
  int stack_padding = 0;
   
  //Adjust the stack alignment when there are stack allocated arguments and it's an odd alignment
  if (arg_count > 6 && arg_count % 2 != 0) {
    stack_padding = 8;
    asm_instruction_allocate_rsp_stack(asm_function, stack_padding, asm_arena);    
  }

  int stack_arg_count = 0;

  for (int i = 0; i < arg_count; i++) {
    AsmNode *arg = asm_operand(&ir_function_call_instruction->data.instruction_function_call.args[i], asm_arena);

    if (i < 6) {
      AsmNode *mov_instruction = arena_alloc(asm_arena);
      mov_instruction->type = ASM_INSTRUCTION_MOV;
      mov_instruction->data.instruction_mov.source = arg;

      AsmNode *destination = arena_alloc(asm_arena);
      destination->type = ASM_OPERAND_REGISTER;
      destination->data.operand_register.op_register = arg_registers[i];      

      mov_instruction->data.instruction_mov.destination = destination;

      asm_add_instruction_to_function(asm_function, mov_instruction);
    } else {
      if (arg->type == ASM_OPERAND_PSEUDO_REGISTER || arg->type == ASM_OPERAND_IMM || is_instruction_quadword(arg)) {
        AsmNode *push_instruction = arena_alloc(asm_arena);
        push_instruction->type = ASM_INSTRUCTION_PUSH;  
        push_instruction->data.instruction_push.operand = arg;

        asm_add_instruction_to_function(asm_function, push_instruction);
      } else {
        stack_arg_count++;
        
        AsmNode *mov_instruction = arena_alloc(asm_arena);
        mov_instruction->type = ASM_INSTRUCTION_MOV;  
        mov_instruction->data.instruction_mov.source = arg;
        mov_instruction->data.instruction_mov.assembly_type = ASM_TYPE_LONGWORD;

        asm_add_instruction_to_function(asm_function, mov_instruction);

        AsmNode *dest_register = arena_alloc(asm_arena);
        dest_register->type = ASM_OPERAND_REGISTER;
        dest_register->data.operand_register.op_register = ASM_REGISTER_AX;

        mov_instruction->data.instruction_mov.destination = dest_register;

        AsmNode *push_instruction = arena_alloc(asm_arena);
        push_instruction->type = ASM_INSTRUCTION_PUSH;
        push_instruction->data.instruction_push.operand = dest_register;

        asm_add_instruction_to_function(asm_function, push_instruction);        
      }

      continue;
    }
  }  

  AsmNode *call_instruction = arena_alloc(asm_arena);
  call_instruction->type = ASM_INSTRUCTION_CALL;
  call_instruction->data.instruction_call.identifier = ir_function_call_instruction->data.instruction_function_call.identifier;

  asm_add_instruction_to_function(asm_function, call_instruction);        

  //Adjust stack pointer
  int bytes_to_remove = 8 * stack_arg_count + stack_padding;

  if (bytes_to_remove != 0) {
    AsmNode *imm_operand = arena_alloc(asm_arena);
    imm_operand->type = ASM_OPERAND_IMM;
    imm_operand->data.operand_imm.value = bytes_to_remove;

    AsmNode *register_operand = arena_alloc(asm_arena);
    register_operand->type = ASM_OPERAND_REGISTER;
    register_operand->data.operand_register.op_register = ASM_REGISTER_SP;

    AsmNode *deallocate_stack_binary_instruction = arena_alloc(asm_arena);
    deallocate_stack_binary_instruction->type = ASM_INSTRUCTION_BINARY;
    deallocate_stack_binary_instruction->data.instruction_binary.assembly_type = ASM_TYPE_QUADWORD;
    deallocate_stack_binary_instruction->data.instruction_binary.binary_op = ASM_BINARY_ADD;
    deallocate_stack_binary_instruction->data.instruction_binary.operand_1 = imm_operand;
    deallocate_stack_binary_instruction->data.instruction_binary.operand_2 = register_operand;

    asm_add_instruction_to_function(asm_function, deallocate_stack_binary_instruction);        
  }

  //retrieve return value 
  AsmNode *assembly_destination = asm_operand(ir_function_call_instruction->data.instruction_function_call.destination, asm_arena);

  AsmNode *dest_register = arena_alloc(asm_arena);
  dest_register->type = ASM_OPERAND_REGISTER;
  dest_register->data.operand_register.op_register = ASM_REGISTER_AX;
  
  AsmNode *mov_instruction = arena_alloc(asm_arena);
  mov_instruction->type = ASM_INSTRUCTION_MOV;  
  mov_instruction->data.instruction_mov.source = dest_register;
  mov_instruction->data.instruction_mov.destination = assembly_destination;

  //@NOTE: Need to add these so that the convert function doesn't fail. However, need to investigate on whether at this point we do something for assembly_type's when the operand is a pseudo register
  if (assembly_destination->type != ASM_OPERAND_PSEUDO_REGISTER) {
    AsmType destination_type = convert_ir_value_to_asm_type(ir_function_call_instruction->data.instruction_function_call.destination, declaration_symbol_table);
    mov_instruction->data.instruction_mov.assembly_type = destination_type;
  }
  
  asm_add_instruction_to_function(asm_function, mov_instruction);  
}

void asm_instruction_sign_extend(AsmNode *asm_function, IRNode *ir_sign_extend_instruction, Arena *asm_arena) {
  AsmNode *movsx_instruction = arena_alloc(asm_arena);
  movsx_instruction->type = ASM_INSTRUCTION_MOVSX;
  movsx_instruction->data.instruction_movsx.source = asm_operand(ir_sign_extend_instruction->data.instruction_sign_extend.source, asm_arena); 
  movsx_instruction->data.instruction_movsx.destination = asm_operand(ir_sign_extend_instruction->data.instruction_sign_extend.destination, asm_arena); 

  asm_add_instruction_to_function(asm_function, movsx_instruction);
}

void asm_instruction_truncate(AsmNode *asm_function, IRNode *ir_truncate_instruction, Arena *asm_arena) {
  AsmNode *mov_instruction = arena_alloc(asm_arena);
  mov_instruction->type = ASM_INSTRUCTION_MOV;
  mov_instruction->data.instruction_mov.assembly_type = ASM_TYPE_LONGWORD;
  mov_instruction->data.instruction_mov.source = asm_operand(ir_truncate_instruction->data.instruction_truncate.source, asm_arena); 
  mov_instruction->data.instruction_mov.destination = asm_operand(ir_truncate_instruction->data.instruction_truncate.destination, asm_arena); 

  asm_add_instruction_to_function(asm_function, mov_instruction);
}

void asm_add_instruction_to_function(AsmNode *function, AsmNode *instruction) {
  asm_add_to_node_pointer(instruction, function->data.function.instruction_pointers);
  function->data.function.instruction_count++;
}

AsmNode* asm_operand(IRNode *ir_operand, Arena *asm_arena) {
  AsmNode *asm_operand = arena_alloc(asm_arena);

  switch (ir_operand->type) {
    case IR_VALUE_CONSTANT:
      asm_operand->type = ASM_OPERAND_IMM;

      switch (ir_operand->data.value_constant.type) {
        case IR_TYPE_INT:  asm_operand->data.operand_imm.value = ir_operand->data.value_constant.value.int_value; break;
        //TODO: Assigning long to int here. Don't think that's right. Read top of pg 266
        case IR_TYPE_LONG: asm_operand->data.operand_imm.value = ir_operand->data.value_constant.value.long_value; break;         
      }
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

void print_assembly(AsmNode *node) {
  switch(node->type) {    
    case ASM_PROGRAM:
      printf("Program \n");

      for (int i = 0; i < node->data.program.top_level_count; i++) {
        print_assembly(node->data.program.top_level_pointers->asm_pointers[i]);
      }

      printf("\n");
      break;
    case ASM_FUNCTION:
      printf("Function: %s -> Instruction count: %d\n", node->data.function.name, node->data.function.instruction_count);

      for (int i = 0; i < node->data.function.instruction_count; i++) {
        print_assembly(node->data.function.instruction_pointers->asm_pointers[i]);
      }      
      break;
    case ASM_STATIC_VARIABLE:
      printf("Static Variable %s\n", node->data.static_variable.identifier);
      break;
    case ASM_INSTRUCTION_MOV:
      printf("MOV -> ");
      printf("Src( ");
      print_assembly(node->data.instruction_mov.source);
      printf(") Dest( ");
      print_assembly(node->data.instruction_mov.destination);
      printf(")\n");
      break;
    case ASM_INSTRUCTION_MOVSX:
      printf("MOVSX -> ");
      printf("Src( ");
      print_assembly(node->data.instruction_movsx.source);
      printf(") Dest( ");
      print_assembly(node->data.instruction_movsx.destination);
      printf(")\n");
      break;
    case ASM_INSTRUCTION_RET:
      printf("RET -> \n");
      break;
    case ASM_INSTRUCTION_UNARY:
      printf("UNARY -> Operator( ");
      switch (node->data.instruction_unary.unary_op) {
        case ASM_UNARY_NEG: printf("NEG )"); break;
        case ASM_UNARY_NOT: printf("NOT )"); break;
      }

      printf(" Operand( ");      
      print_assembly(node->data.instruction_unary.operand);
      printf(")\n");
      break;
    case ASM_INSTRUCTION_BINARY:
      switch (node->data.instruction_binary.binary_op) {
        case ASM_BINARY_ADD:                  printf("ADD -> "); break;
        case ASM_BINARY_SUB:                  printf("SUB -> "); break;
        case ASM_BINARY_MULT:                 printf("MUL -> "); break;
        case ASM_BINARY_BITWISE_AND:          printf("AND -> "); break;
        case ASM_BINARY_BITWISE_OR:           printf("OR -> "); break;
        case ASM_BINARY_BITWISE_XOR:          printf("XOR -> "); break;
        case ASM_BINARY_BITWISE_LEFT_SHIFT:   printf("SHL -> "); break;
        case ASM_BINARY_BITWISE_RIGHT_SHIFT:  printf("SHR -> "); break;
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
    case ASM_INSTRUCTION_JMP:
      printf("JMP -> Identifier( %s )\n", node->data.instruction_jmp.identifier);      
      break;
    case ASM_INSTRUCTION_JMPCC:
      printf("JMPCC -> Identifier( %s ), Condition(  ", node->data.instruction_jmp_cc.identifier);
      switch (node->data.instruction_jmp_cc.condition_code) {
        case ASM_CONDITION_EQUAL:          printf("Equal"); break;
        case ASM_CONDITION_NOT_EQUAL:      printf("Not Equal"); break;
        case ASM_CONDITION_GREATER:        printf("Greater"); break;
        case ASM_CONDITION_GREATER_EQUAL:  printf("Greater or Equal"); break;
        case ASM_CONDITION_LESS:           printf("Less"); break;
        case ASM_CONDITION_LESS_EQUAL:     printf("Less or Equal"); break;
      }
      printf(" )\n");
      break;
    case ASM_INSTRUCTION_SETCC:
      printf("SETCC -> Operand( ");
      print_assembly(node->data.instruction_set_cc.operand);
      printf(") Condition( ");
      switch (node->data.instruction_set_cc.condition_code) {
        case ASM_CONDITION_EQUAL:          printf("Equal"); break;
        case ASM_CONDITION_NOT_EQUAL:      printf("Not Equal"); break;
        case ASM_CONDITION_GREATER:        printf("Greater"); break;
        case ASM_CONDITION_GREATER_EQUAL:  printf("Greater or Equal"); break;
        case ASM_CONDITION_LESS:           printf("Less"); break;
        case ASM_CONDITION_LESS_EQUAL:     printf("Less or Equal"); break;
      }
      printf(" )\n");
      break;
    case ASM_INSTRUCTION_CMP:
      printf("CMP -> Operand( "); 
      print_assembly(node->data.instruction_cmp.operand_1);
      printf("), Operand( ");
      print_assembly(node->data.instruction_cmp.operand_2);
      printf(")\n");
      break;
    case ASM_INSTRUCTION_LABEL:
      printf("LABEL -> %s\n", node->data.instruction_label.identifier);
      break;
    case ASM_OPERAND_REGISTER:
      printf("Register %d ", node->data.operand_register.op_register);
      break;
    case ASM_OPERAND_PSEUDO_REGISTER:
      printf("Pseudo Register %s ", node->data.operand_pseudo_register.identifier);
      break;
    case ASM_OPERAND_IMM:
      printf("IMM %d ", node->data.operand_imm.value);
      break;
    case ASM_OPERAND_STACK:
      printf("Stack %d ", node->data.operand_stack.address);
      break;
    case ASM_OPERAND_DATA:
      printf("Data %s ", node->data.operand_data.identifier);
      break;
    case ASM_INSTRUCTION_PUSH:
      printf("Push ->");
      print_assembly(node->data.instruction_push.operand);
      printf("\n");
      break;
    case ASM_INSTRUCTION_CALL:
      printf("Call -> %s\n", node->data.instruction_call.identifier);
      break;
    default:
      fprintf(stderr, "ERROR - Assembler: No print debug option for '%d' asm node type\n", node->type);
      exit(1);
      break;
  }
}

void asm_add_to_node_pointer(AsmNode *asm_node, AsmNodePointers *asm_node_pointer) {
  if (asm_node_pointer == NULL) {
    return;
  }
  
  if (asm_node_pointer->count == asm_node_pointer->capacity) {
    int new_size = asm_node_pointer->capacity == 0 ? NODE_POINTER_CAPACITY : asm_node_pointer->capacity * 2;

    AsmNode **realloc_pointers = realloc(asm_node_pointer->asm_pointers, new_size * sizeof(AsmNode**));

    asm_node_pointer->capacity = new_size;
    asm_node_pointer->asm_pointers = realloc_pointers;
  } 

  asm_node_pointer->asm_pointers[asm_node_pointer->count] = asm_node;
  asm_node_pointer->count++;
}

void asm_init_node_pointer(AsmNodePointers *asm_node_pointer) {
  if (asm_node_pointer == NULL) {
    return;
  }
  
  asm_node_pointer->capacity = 0;
  asm_node_pointer->count = 0;
  asm_node_pointer->asm_pointers = NULL;
}

static AsmType convert_ir_value_to_asm_type(IRNode *ir_node, DeclarationSymbolTable *declaration_symbol_table) {
  switch (ir_node->type) {
    case IR_VALUE_CONSTANT:
        switch (ir_node->data.value_constant.type) {
          case IR_TYPE_INT: return ASM_TYPE_LONGWORD;
          case IR_TYPE_LONG: return ASM_TYPE_QUADWORD;
        }
      break;
    case IR_VALUE_VAR: {
      //TODO: Add some error checking
      HashTableEntry *variable_hash_entry = hash_table_get_entry(declaration_symbol_table->symbol_table, ir_node->data.value_var.identifier);
     
      DeclarationSymbol *declaration_symbol = variable_hash_entry->value->structure;

      switch (declaration_symbol->data.variable_symbol->value_type) {
        case DECLARATION_SYMBOL_TYPE_INT:  return ASM_TYPE_LONGWORD;
        case DECLARATION_SYMBOL_TYPE_LONG: return ASM_TYPE_QUADWORD;
      }

      fprintf(stderr, "ERROR - Assembler: Invalid IR Node type '%d' when attempting to convert to ASM Type", ir_node->type);
      exit(1);
      break;
    }
    default:
      fprintf(stderr, "ERROR - Assembler: Invalid IR Node type '%d' when attempting to convert to ASM Type", ir_node->type);
      exit(1);
  }
}

static bool is_instruction_quadword(AsmNode *instruction) {
  switch (instruction->type) {
    case ASM_INSTRUCTION_MOV: return instruction->data.instruction_mov.assembly_type == ASM_TYPE_QUADWORD;
    case ASM_INSTRUCTION_UNARY: return instruction->data.instruction_unary.assembly_type == ASM_TYPE_QUADWORD;
    case ASM_INSTRUCTION_BINARY: return instruction->data.instruction_binary.assembly_type == ASM_TYPE_QUADWORD;
    case ASM_INSTRUCTION_CMP: return instruction->data.instruction_cmp.assembly_type == ASM_TYPE_QUADWORD;
    case ASM_INSTRUCTION_IDIV: return instruction->data.instruction_idiv.assembly_type == ASM_TYPE_QUADWORD;
    case ASM_INSTRUCTION_CDQ: return instruction->data.instruction_cdq.assembly_type == ASM_TYPE_QUADWORD;
    default:
      return false;
  }
}


void backend_symbol_table_init(AsmBackendSymbolTable *backend_symbol_table) {
  HashTable *symbol_table = malloc(sizeof(HashTable));
  hash_table_init(symbol_table);

  Arena *backend_symbol_arena = malloc(sizeof(Arena));
  arena_init(backend_symbol_arena, sizeof(AsmBackendSymbol), sizeof(AsmBackendSymbol) * 1000, true);

  backend_symbol_table->symbol_arena = backend_symbol_arena;
  backend_symbol_table->symbol_table = symbol_table;  
}

void backend_symbol_table_free(AsmBackendSymbolTable *backend_symbol_table) {
  arena_free(backend_symbol_table->symbol_arena);
  free(backend_symbol_table->symbol_table);
}

static void convert_declaration_table_to_backend_table(DeclarationSymbolTable *declaration_symbol_table, AsmBackendSymbolTable *backend_symbol_table) {
  for (int i = 0; i < declaration_symbol_table->symbol_table->capacity; i++) {
    if (&declaration_symbol_table->symbol_table->entries[i] == NULL || declaration_symbol_table->symbol_table->entries[i].key == NULL) {
      continue;
    } 
    
    HashTableEntry *declaration_symbol_entry = hash_table_get_entry(declaration_symbol_table->symbol_table, declaration_symbol_table->symbol_table->entries[i].key);
    
    AsmBackendSymbol *asm_backend_symbol = arena_alloc(backend_symbol_table->symbol_arena);   
    DeclarationSymbol *declaration_symbol = declaration_symbol_entry->value->structure;    

    if (declaration_symbol->symbol_type == DECLARATION_SYMBOL_VARIABLE) {
      asm_backend_symbol->type = ASM_SYMBOL_OBJECT_ENTRY;

      switch (declaration_symbol->data.variable_symbol->value_type) {
        case DECLARATION_SYMBOL_TYPE_INT:   asm_backend_symbol->data.object_entry.assembly_type = ASM_TYPE_LONGWORD; break;
        case DECLARATION_SYMBOL_TYPE_LONG:  asm_backend_symbol->data.object_entry.assembly_type= ASM_TYPE_QUADWORD; break;
        default:
          fprintf(stderr, "ERROR - ASSEMBLER: Could not resolve declaration symbol type when attempting to convert to backend assembly type");
          exit(1);
      }

      asm_backend_symbol->data.object_entry.is_static = !declaration_symbol->data.variable_symbol->is_automatic_storage_duration;      
    } else {
      asm_backend_symbol->type = ASM_SYMBOL_FUNCTION_ENTRY;
      asm_backend_symbol->data.function_entry.is_defined = declaration_symbol->data.function_symbol->is_defined;
    }
    
    HashValue *hash_value = malloc(sizeof(HashValue));
    hash_value->type = HASH_STRUCT;
    hash_value->structure = asm_backend_symbol;

    HashTableEntry *hash_entry = malloc(sizeof(HashTableEntry));
    hash_entry->key = declaration_symbol_entry->key;
    hash_entry->value = hash_value;

    hash_table_add_entry(backend_symbol_table->symbol_table, hash_entry);    
  }
}

void backend_symbol_table_print(AsmBackendSymbolTable *backend_symbol_table) {
  printf("Backend Symbol Table\n");
  
  for (int i = 0; i < backend_symbol_table->symbol_table->capacity; i++) {
    if (backend_symbol_table->symbol_table->entries[i].key == NULL) {
      continue;
    }
    
    printf("index: %d\tkey: %s \t", i, backend_symbol_table->symbol_table->entries[i].key);    
    
    HashValue *hash_value = backend_symbol_table->symbol_table->entries[i].value;
    AsmBackendSymbol *symbol = hash_value->structure;

    if (symbol->type == ASM_SYMBOL_OBJECT_ENTRY) {
      printf("type: Object Entry\t");
      printf("assembly_type: ");

      switch (symbol->data.object_entry.assembly_type) {
        case ASM_TYPE_QUADWORD:    printf("Quadword\t"); break;
        case ASM_TYPE_LONGWORD:    printf("Longword\t"); break;
      }      

      printf("is_static: %d\n", symbol->data.object_entry.is_static);
    } else {
      printf("type: Function\t");
      printf("is_defined: %d\n", symbol->data.function_entry.is_defined);
    }
  }
}

static int round_stack_offset(int stack_offset) {
  //%RBP is always 16 byte aligned. Round stack offset of the stack frame to the next multiple of 16 makes it easier to maintain the correct stack alignment during function calls. Alignment is required by the System V ABI.
  return ((stack_offset + 15) / 16) * 16;
}
