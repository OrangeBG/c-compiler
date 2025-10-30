#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>
#include "../include/assembly.h"
#include "../include/hash_table.h"
#include "../include/arena.h"
#include "../include/declaration_symbol.h"
#include "../include/intermediate_rep.h"
#include "../include/types.h"

#define NODE_POINTER_CAPACITY 8
#define ALIGNMENT_QUADWORD 8
#define ALIGNMENT_LONGWORD 4

typedef enum {
  INSTRUCTION_FIXED,
  INSTRUCTION_NOT_FIXED
} ResolveType;

typedef struct {
  DeclarationSymbolTable *declaration_symbol_table;
  AsmNodePointers *top_level_declarations;
  AsmNodePointers *static_constants;
  Arena *asm_arena;
  AsmNode *register_ax;
  AsmNode *register_cx;
  AsmNode *register_dx;
  AsmNode *register_di;
  AsmNode *register_si;
  AsmNode *register_r8;
  AsmNode *register_r9;
  AsmNode *register_r10;
  AsmNode *register_r11;
  AsmNode *register_sp;
  AsmNode *register_xmm0;
  AsmNode *register_xmm1;
  AsmNode *register_xmm2;
  AsmNode *register_xmm3;
  AsmNode *register_xmm4;
  AsmNode *register_xmm5;
  AsmNode *register_xmm6;
  AsmNode *register_xmm7;
  AsmNode *register_xmm14;    
  AsmNode *register_xmm15;
} Assembly;

static void         emit_ir_function(IRNode *ir_function, AsmNode *asm_function, Assembly *assembly);
static void         emit_static_variable(IRNode *ir_static_variable, AsmNode *asm_static_variable);
static AsmNode*     emit_static_constant(double source_double, int alignment, Assembly *assembly);  
static void         emit_ir_instruction_return(AsmNode *asm_function, IRNode *ir_return_instruction, Assembly *assembly);
static void         emit_ir_instruction_unary(AsmNode *asm_function, IRNode *ir_unary_instruction, Assembly *assembly); 
static void         emit_ir_instruction_unary_negation_double(AsmNode *asm_function, IRNode *ir_unary_instruction, Assembly *assembly); 
static void         emit_ir_instruction_unary_not_integer(AsmNode *asm_function, IRNode *ir_unary_not_instruction, Assembly *assembly); 
static void         emit_ir_instruction_unary_not_double(AsmNode *asm_function, IRNode *ir_unary_not_instruction, Assembly *assembly); 
static void         emit_ir_instruction_binary(AsmNode *asm_function, IRNode *ir_binary_instruction, Assembly *assembly); 
static void         emit_ir_instruction_binary_relational(AsmNode *asm_function, IRNode *ir_relational_instruction, Assembly *assembly); 
static void         emit_ir_instruction_binary_signed_division(AsmNode *asm_function, const IRNode *ir_binary_instruction, Assembly *assembly); 
static void         emit_ir_instruction_binary_unsigned_division(AsmNode *asm_function, const IRNode *ir_binary_instruction, Assembly *assembly); 
static void         emit_ir_instruction_allocate_rsp_stack(AsmNode *asm_function, int bytes, Assembly *assembly); 
static void         emit_ir_instruction_jump(AsmNode *asm_function, IRNode *ir_jump_instruction, Assembly *assembly); 
static void         emit_ir_instruction_jump_if_zero_integer(AsmNode *asm_function, IRNode *ir_jump_if_zero_instruction, AsmType asm_source_type, Assembly *assembly); 
static void         emit_ir_instruction_jump_if_zero_double(AsmNode *asm_function, IRNode *ir_jump_if_zero_instruction, Assembly *assembly); 
static void         emit_ir_instruction_jump_if_not_zero_integer(AsmNode *asm_function, IRNode *ir_jump_if_not_zero_instruction, AsmType asm_source_type, Assembly *assembly); 
static void         emit_ir_instruction_jump_if_not_zero_double(AsmNode *asm_function, IRNode *ir_jump_if_not_zero_instruction, Assembly *assembly);   
static void         emit_ir_instruction_copy(AsmNode *asm_function, IRNode *ir_copy_instruction, Assembly *assembly);
static void         emit_ir_instruction_label(AsmNode *asm_function, IRNode *ir_label_instruction, Assembly *assembly); 
static void         emit_ir_instruction_function_call(AsmNode *asm_function, IRNode *ir_function_call_instruction, Assembly *assembly);
static void         emit_ir_instruction_sign_extend(AsmNode *asm_function, IRNode *ir_sign_extend_instruction, Assembly *assembly); 
static void         emit_ir_instruction_zero_extend(AsmNode *asm_function, IRNode *ir_zero_extend_instruction, Assembly *assembly);
static void         emit_ir_instruction_truncate(AsmNode *asm_function, IRNode *ir_truncate_instruction, Assembly *assembly); 
static void         emit_ir_instruction_cvtsi2sd(AsmNode *asm_function, IRNode *ir_int_to_double_instruction, Assembly *assembly); 
static void         emit_ir_instruction_cvttsd2si(AsmNode *asm_function, IRNode *ir_int_to_double_instruction, Assembly *assembly);
static void         emit_ir_instruction_uint_to_double(AsmNode *asm_function, IRNode *ir_uint_to_double_instruction, Assembly *assembly); 
static void         emit_ir_instruction_ulong_to_double(AsmNode *asm_function, IRNode *ir_ulong_to_double_instruction, Assembly *assembly); 
static void         emit_ir_instruction_double_to_uint(AsmNode *asm_function, IRNode *ir_double_to_uint_instruction, Assembly *assembly); 
static void         emit_ir_instruction_double_to_ulong(AsmNode *asm_function, IRNode *ir_double_to_ulong_instruction, Assembly *assembly); 
static AsmNode*     create_register(AsmRegisterType register_type, Assembly *assembly);
static AsmNode*     create_imm_operand(long value, Assembly *assembly);
static void         emit_asm_mov_instruction(AsmNode *function, AsmNode *source_node, AsmNode *destination_node, AsmType type, Assembly *assembly);
static void         emit_asm_mov_zero_extend_instruction(AsmNode *function, AsmNode *source_node, AsmNode *destination_node, Assembly *assembly);
static void         emit_asm_cmp_instruction(AsmNode *function, AsmNode *operand_1, AsmNode *operand_2, AsmType type, Assembly *assembly);
static void         emit_asm_binary_instruction(AsmNode *function, AsmNode *operand_1, AsmNode *operand_2, AsmBinaryOpType op_type, AsmType assembly_type, Assembly *assembly); 
static void         emit_asm_div_instruction(AsmNode *function, AsmNode *operand, AsmType type, Assembly *assembly);
static void         emit_asm_idiv_instruction(AsmNode *function, AsmNode *operand, AsmType type, Assembly *assembly);
static void         emit_asm_push_instruction(AsmNode *function, AsmNode *operand, Assembly *assembly);
static void         emit_asm_unary_instruction(AsmNode *function, AsmNode *operand, AsmUnaryOpType op_type, AsmType assembly_type, Assembly *assembly); 
static void         emit_asm_label_instruction(AsmNode *function, char *identifier, Assembly *assembly);
static void         emit_asm_cvttsd2si_instruction(AsmNode *function, AsmNode *source_node, AsmNode *destination_node, AsmType type, Assembly *assembly);
static void         emit_asm_cvtsi2sd_instruction(AsmNode *function, AsmNode *source_node, AsmNode *destination_node, AsmType type, Assembly *assembly); 
static void         emit_asm_jmp_instruction(AsmNode *function, char *identifier, Assembly *assembly); 
static void         emit_asm_jmpcc_instruction(AsmNode *function, AsmConditionCode condition_code, char *identifier, Assembly *assembly); 
static void         emit_asm_setcc_instruction(AsmNode *function, AsmConditionCode condition_code, AsmNode *operand, Assembly *assembly); 
static void         add_instruction_to_function(AsmNode *function, AsmNode *instruction); 
static void         add_to_node_pointer(AsmNode *asm_node, AsmNodePointers *asm_node_pointer);
static Assembly*    init_assembly(DeclarationSymbolTable *declaration_symbol_table);
static void         init_node_pointer(AsmNodePointers *asm_node_pointer);
static void         pseudo_register_pass(AsmNode *asm_function, AsmBackendSymbolTable *backend_symbol_table, int *stack_offset); 
static void         replace_pseudo_register(AsmNode *instruction, AsmType instruction_type, HashTable *stack_location_table, AsmBackendSymbolTable *backend_symbol_table, int *stack_offset); 
static AsmNode*     create_operand(IRNode *ir_operand, Assembly *assembly);
static AsmNode*     resolve_instructions(AsmNode *function, Assembly *assembly); 
static ResolveType  resolve_idiv_instruction(AsmNode *function, AsmNode *idiv_instruction, Assembly *assembly);
static ResolveType  resolve_div_instruction(AsmNode *function, AsmNode *div_instruction, Assembly *assembly); 
static ResolveType  resolve_mov_instruction(AsmNode *function, AsmNode *instruction, Assembly *assembly); 
static ResolveType  resolve_cmp_instruction(AsmNode *function, AsmNode *instruction, Assembly *assembly); 
static ResolveType  resolve_binary_add_sub_instruction(AsmNode *function, AsmNode *instruction, Assembly *assembly); 
static ResolveType  resolve_binary_mul_instruction(AsmNode *function, AsmNode *instruction, Assembly *assembly); 
static ResolveType  resolve_binary_double_instructions(AsmNode *function, AsmNode *instruction, Assembly *assembly); 
static ResolveType  resolve_movsx_instruction(AsmNode *function, AsmNode *movsx_instruction, Assembly *assembly); 
static ResolveType  resolve_mov_zero_extend_instruction(AsmNode *function, AsmNode *mov_zero_extend_instruction, Assembly *assembly);
static ResolveType  resolve_large_imm_operand(AsmNode *function, AsmNode *instruction, Assembly *assembly); 
static ResolveType  resolve_cvttsd2si_instruction(AsmNode *function, AsmNode *instruction, Assembly *assembly); 
static ResolveType  resolve_cvtsi2sd_instruction(AsmNode *function, AsmNode *cvtsi2sd_instruction, Assembly *assembly); 
static AsmType      convert_ir_value_to_asm_type(IRNode *ir_node, DeclarationSymbolTable *declaration_symbol_table); 
static AsmType      convert_type_to_asm_type(Types type); 
static Types        get_ir_node_type(IRNode *ir_node, DeclarationSymbolTable *declaration_symbol_table); 
static void         convert_declaration_table_to_backend_table(DeclarationSymbolTable *declaration_symbol_table, AsmBackendSymbolTable *backend_symbol_table); 
static int          round_stack_offset(int stack_offset); 
static bool         is_signed_ir_value_node(IRNode *ir_node, DeclarationSymbolTable *declaration_symbol_table);
static AsmType      get_instruction_type(AsmNode *instruction); 
static void         print_assembly_type(AsmType type); 

AsmNode* generate_assembly(IRNode *ir_nodes, DeclarationSymbolTable *declaration_symbol_table, AsmBackendSymbolTable *backend_symbol_table) {   
  Assembly *assembly = init_assembly(declaration_symbol_table);

  AsmNode *program = arena_alloc(assembly->asm_arena);
  program->type = ASM_PROGRAM;
  program->data.program.top_level_count = 0;
  program->data.program.top_level_pointers = assembly->top_level_declarations;
  program->data.program.static_constant_pointers = assembly->static_constants;

  for (int i = 0; i < ir_nodes->data.program.top_level_count; i++) {
    AsmNode *declaration = arena_alloc(assembly->asm_arena);
    add_to_node_pointer(declaration, assembly->top_level_declarations);
    
    if (ir_nodes->data.program.top_level_ptrs->node_pointers[i]->type == IR_FUNCTION) {
      emit_ir_function(ir_nodes->data.program.top_level_ptrs->node_pointers[i], declaration, assembly);
    } else {
      emit_static_variable(ir_nodes->data.program.top_level_ptrs->node_pointers[i], declaration);
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
    pseudo_register_pass(top_level_node, backend_symbol_table, &stack_offset);
    
    if (top_level_node->data.function.instruction_pointers->asm_pointers[0]->type != ASM_INSTRUCTION_BINARY) {
      fprintf(stderr, "ERROR - Assembler: First instruction is not Binary Instruction for the '%s' function\n", program->data.program.top_level_pointers->asm_pointers[i]->data.function.name);
      exit(1);
    } 

    stack_offset = round_stack_offset(stack_offset);

    top_level_node->data.function.instruction_pointers->asm_pointers[0]->data.instruction_binary.operand_1->data.operand_imm.value = stack_offset;    

    AsmNode *new_function = resolve_instructions(top_level_node, assembly);

    program->data.program.top_level_pointers->asm_pointers[i] = new_function;
  }
  
  return program;
}

static Assembly* init_assembly(DeclarationSymbolTable *declaration_symbol_table) {
  Arena *asm_arena = malloc(sizeof(Arena));
  //TODO: Hardcoded capacity
  arena_init(asm_arena, sizeof(AsmNode), sizeof(AsmNode) * 1000, true);

  AsmNodePointers *top_level_declarations = malloc(sizeof(AsmNodePointers));
  init_node_pointer(top_level_declarations);  

  AsmNodePointers *static_constant_node_pointers = malloc(sizeof(AsmNodePointers));
  init_node_pointer(static_constant_node_pointers);  

  Assembly *assembly = malloc(sizeof(Assembly));
  assembly->asm_arena = asm_arena;
  assembly->declaration_symbol_table = declaration_symbol_table;
  assembly->top_level_declarations = top_level_declarations;
  assembly->static_constants = static_constant_node_pointers;

  assembly->register_ax = create_register(ASM_REGISTER_AX, assembly);
  assembly->register_cx = create_register(ASM_REGISTER_CX, assembly);
  assembly->register_dx = create_register(ASM_REGISTER_DX, assembly);
  assembly->register_di = create_register(ASM_REGISTER_DI, assembly);
  assembly->register_si = create_register(ASM_REGISTER_SI, assembly);
  assembly->register_r8 = create_register(ASM_REGISTER_R8, assembly);
  assembly->register_r9 = create_register(ASM_REGISTER_R9, assembly);
  assembly->register_r10 = create_register(ASM_REGISTER_R10, assembly);
  assembly->register_r11 = create_register(ASM_REGISTER_R11, assembly);
  assembly->register_sp = create_register(ASM_REGISTER_SP, assembly);
  assembly->register_xmm0 = create_register(ASM_REGISTER_XMM0, assembly);
  assembly->register_xmm1 = create_register(ASM_REGISTER_XMM1, assembly);
  assembly->register_xmm2 = create_register(ASM_REGISTER_XMM2, assembly);
  assembly->register_xmm3 = create_register(ASM_REGISTER_XMM3, assembly);
  assembly->register_xmm4 = create_register(ASM_REGISTER_XMM4, assembly);
  assembly->register_xmm5 = create_register(ASM_REGISTER_XMM5, assembly);
  assembly->register_xmm6 = create_register(ASM_REGISTER_XMM6, assembly);
  assembly->register_xmm7 = create_register(ASM_REGISTER_XMM7, assembly);
  assembly->register_xmm14 = create_register(ASM_REGISTER_XMM14, assembly);    
  assembly->register_xmm15 = create_register(ASM_REGISTER_XMM15, assembly);

  return assembly;
}

static AsmNode* resolve_instructions(AsmNode *function, Assembly *assembly) {
  AsmNodePointers *new_instructions = malloc(sizeof(AsmNodePointers));
  init_node_pointer(new_instructions);
  
  AsmNode *new_function = arena_alloc(assembly->asm_arena);
  new_function->type = ASM_FUNCTION;
  new_function->data.function.name = function->data.function.name;
  new_function->data.function.is_global = function->data.function.is_global;
  new_function->data.function.instruction_count = 0;
  new_function->data.function.instruction_pointers = new_instructions;
  
  AsmNodePointers *instruction_ptr = function->data.function.instruction_pointers;

  for (int i = 0; i < function->data.function.instruction_count; i++) {
    AsmNodeType instruction_type = instruction_ptr->asm_pointers[i]->type;
    AsmNode *instruction = instruction_ptr->asm_pointers[i];

    ResolveType resolve_type = INSTRUCTION_NOT_FIXED;

    switch (instruction_type) {
      case ASM_INSTRUCTION_MOV:
        resolve_type = resolve_mov_instruction(new_function, instruction, assembly);
        break;
      case ASM_INSTRUCTION_MOVSX:        
        resolve_type = resolve_movsx_instruction(new_function, instruction, assembly); 
        break;
      case ASM_INSTRUCTION_MOV_ZERO_EXTEND:        
        resolve_type = resolve_mov_zero_extend_instruction(new_function, instruction, assembly);
        break;
      case ASM_INSTRUCTION_CMP:
        resolve_type = resolve_cmp_instruction(new_function, instruction, assembly);
        break;
      case ASM_INSTRUCTION_BINARY:
        if (instruction->data.instruction_binary.assembly_type == ASM_TYPE_DOUBLE) {
          resolve_type = resolve_binary_double_instructions(new_function, instruction, assembly);
        }

        AsmBinaryOpType op_type = instruction->data.instruction_binary.binary_op;

        if (op_type == ASM_BINARY_ADD || op_type == ASM_BINARY_SUB || op_type == ASM_BINARY_BITWISE_AND || op_type == ASM_BINARY_BITWISE_OR) {
          resolve_type = resolve_binary_add_sub_instruction(new_function, instruction, assembly);
        } else if (instruction->data.instruction_binary.binary_op == ASM_BINARY_MULT) {
          resolve_type = resolve_binary_mul_instruction(new_function, instruction, assembly);
        }
        break;
      case ASM_INSTRUCTION_IDIV:
        resolve_type = resolve_idiv_instruction(new_function, instruction, assembly);
        break;
      case ASM_INSTRUCTION_DIV:
        resolve_type = resolve_div_instruction(new_function, instruction, assembly);
        break;      
      case ASM_INSTRUCTION_CVTTSD2SI:
        resolve_type = resolve_cvttsd2si_instruction(new_function, instruction, assembly);        
        break;
      case ASM_INSTRUCTION_CVTSI2SD:
        resolve_type = resolve_cvtsi2sd_instruction(new_function, instruction, assembly);
        break;
    }

    if (resolve_type == INSTRUCTION_FIXED) {
      continue;
    }

    resolve_large_imm_operand(new_function, instruction, assembly);

    AsmNode *new_instruction = arena_alloc(assembly->asm_arena); 
    new_instruction->type = instruction->type;
    new_instruction->data = instruction->data;

    add_instruction_to_function(new_function, new_instruction);
  }

  return new_function;
}

static ResolveType resolve_large_imm_operand(AsmNode *function, AsmNode *instruction, Assembly *assembly) {
  //The quadword versions of binary arithmetic instructions (addq, imulq, and subq) can’t handle immediate values that don’t fit into an int,
  //and neither can cmpq or pushq. If the source of any of these instructions is a constant outside the range of int, we’ll need to copy it into R10 before we can use it.
  if (instruction->type == ASM_INSTRUCTION_BINARY &&
      instruction->data.instruction_binary.assembly_type == ASM_TYPE_QUADWORD &&
      instruction->data.instruction_binary.operand_1->type == ASM_OPERAND_IMM &&
      (instruction->data.instruction_binary.operand_1->data.operand_imm.value > INT_MAX || instruction->data.instruction_binary.operand_1->data.operand_imm.value < INT_MIN )) {
    emit_asm_mov_instruction(function, instruction->data.instruction_binary.operand_1, assembly->register_r10, ASM_TYPE_QUADWORD, assembly);
    emit_asm_mov_instruction(function, assembly->register_r10, instruction->data.instruction_binary.operand_2, ASM_TYPE_QUADWORD, assembly);

    return INSTRUCTION_FIXED;
  }

  if (instruction->type == ASM_INSTRUCTION_CMP &&
      instruction->data.instruction_cmp.assembly_type == ASM_TYPE_QUADWORD &&
      instruction->data.instruction_cmp.operand_1->type == ASM_OPERAND_IMM &&
      (instruction->data.instruction_cmp.operand_1->data.operand_imm.value > INT_MAX || instruction->data.instruction_cmp.operand_1->data.operand_imm.value < INT_MIN )) {
    emit_asm_mov_instruction(function, instruction->data.instruction_cmp.operand_1, assembly->register_r10, ASM_TYPE_QUADWORD, assembly);
    emit_asm_mov_instruction(function, assembly->register_r10, instruction->data.instruction_cmp.operand_2, ASM_TYPE_QUADWORD, assembly);

    return INSTRUCTION_FIXED;
  }

  if (instruction->type == ASM_INSTRUCTION_PUSH &&
      instruction->data.instruction_push.operand->type == ASM_OPERAND_IMM &&
      (instruction->data.instruction_push.operand->data.operand_imm.value > INT_MAX || instruction->data.instruction_push.operand->data.operand_imm.value < INT_MIN )) {
    emit_asm_mov_instruction(function, instruction->data.instruction_push.operand, assembly->register_r10, ASM_TYPE_QUADWORD, assembly);
    emit_asm_mov_instruction(function, assembly->register_r10, instruction->data.instruction_push.operand, ASM_TYPE_QUADWORD, assembly);

    return INSTRUCTION_FIXED;
  }

  return INSTRUCTION_NOT_FIXED;
}

static ResolveType resolve_movsx_instruction(AsmNode *function, AsmNode *movsx_instruction, Assembly *assembly) {
  //MOVSX instructions cannot have a memory address as a destination or an immediate value as a source
  if (movsx_instruction->data.instruction_movsx.source->type != ASM_OPERAND_IMM && movsx_instruction->data.instruction_movsx.destination->type != ASM_OPERAND_STACK) {
    return INSTRUCTION_NOT_FIXED;
  }

  AsmNode *new_movsx = arena_alloc(assembly->asm_arena);
  new_movsx->type = ASM_INSTRUCTION_MOVSX;

  if (movsx_instruction->data.instruction_movsx.source->type == ASM_OPERAND_IMM) {
    emit_asm_mov_instruction(function, movsx_instruction->data.instruction_movsx.source, assembly->register_r10, ASM_TYPE_LONGWORD, assembly);    
    new_movsx->data.instruction_movsx.source = assembly->register_r10;
  } else {
    new_movsx->data.instruction_movsx.source = movsx_instruction->data.instruction_movsx.source;
  }

  if (movsx_instruction->data.instruction_movsx.destination->type == ASM_OPERAND_STACK) {
    new_movsx->data.instruction_movsx.destination = assembly->register_r11;
    add_instruction_to_function(function, new_movsx);

    emit_asm_mov_instruction(function, assembly->register_r11, movsx_instruction->data.instruction_movsx.destination, ASM_TYPE_QUADWORD, assembly);
  } else {
    new_movsx->data.instruction_movsx.destination = movsx_instruction->data.instruction_movsx.destination;
    add_instruction_to_function(function, new_movsx);
  }

  return INSTRUCTION_FIXED;
}

static ResolveType resolve_idiv_instruction(AsmNode *function, AsmNode *idiv_instruction, Assembly *assembly) {
  //IDIV instructions need to be copied into a scratch buffer if the operand is a constant
  if (idiv_instruction->data.instruction_idiv.operand->type != ASM_OPERAND_IMM) {
    return INSTRUCTION_NOT_FIXED;
  }

  emit_asm_mov_instruction(function, idiv_instruction->data.instruction_idiv.operand, assembly->register_r10, idiv_instruction->data.instruction_idiv.assembly_type, assembly);
  emit_asm_idiv_instruction(function, assembly->register_r10, idiv_instruction->data.instruction_idiv.assembly_type, assembly);

  return INSTRUCTION_FIXED;
}

static ResolveType resolve_div_instruction(AsmNode *function, AsmNode *div_instruction, Assembly *assembly) {
  //DIV instructions need to be copied into a scratch buffer if the operand is a constant
  if (div_instruction->data.instruction_div.operand->type != ASM_OPERAND_IMM) {
    return INSTRUCTION_NOT_FIXED;
  }

  emit_asm_mov_instruction(function, div_instruction->data.instruction_div.operand, assembly->register_r10, div_instruction->data.instruction_div.assembly_type, assembly);
  emit_asm_div_instruction(function, assembly->register_r10, div_instruction->data.instruction_div.assembly_type, assembly);

  return INSTRUCTION_FIXED;
}

static ResolveType resolve_cvttsd2si_instruction(AsmNode *function, AsmNode *cvttsd2si_instruction, Assembly *assembly) {
  if (cvttsd2si_instruction->data.instruction_cvttsd2si.destination_operand->type == ASM_OPERAND_REGISTER) {
    return INSTRUCTION_NOT_FIXED;
  }

  emit_asm_cvttsd2si_instruction(function, cvttsd2si_instruction->data.instruction_cvttsd2si.source_operand, assembly->register_r11, ASM_TYPE_QUADWORD, assembly);
  emit_asm_mov_instruction(function, assembly->register_r11, cvttsd2si_instruction->data.instruction_cvttsd2si.destination_operand, ASM_TYPE_QUADWORD, assembly);
  
  return INSTRUCTION_FIXED;
}

static ResolveType resolve_cvtsi2sd_instruction(AsmNode *function, AsmNode *cvtsi2sd_instruction, Assembly *assembly) {
  if (cvtsi2sd_instruction->data.instruction_cvtsi2sd.source_operand->type != ASM_OPERAND_IMM && cvtsi2sd_instruction->data.instruction_cvtsi2sd.destination_operand->type == ASM_OPERAND_REGISTER) {
    return INSTRUCTION_NOT_FIXED;
  }

  AsmNode *source_node = cvtsi2sd_instruction->data.instruction_cvtsi2sd.source_operand;

  if (source_node->type == ASM_OPERAND_IMM) {
    emit_asm_mov_instruction(function, cvtsi2sd_instruction->data.instruction_cvtsi2sd.source_operand, assembly->register_r10, ASM_TYPE_LONGWORD, assembly);
  }

  if (cvtsi2sd_instruction->data.instruction_cvtsi2sd.destination_operand->type != ASM_OPERAND_REGISTER) {
    emit_asm_cvtsi2sd_instruction(function, source_node, assembly->register_xmm15, ASM_TYPE_LONGWORD, assembly);
    emit_asm_mov_instruction(function, assembly->register_xmm15, cvtsi2sd_instruction->data.instruction_cvtsi2sd.destination_operand, ASM_TYPE_DOUBLE, assembly);
  } else {
    emit_asm_cvtsi2sd_instruction(function, source_node, cvtsi2sd_instruction->data.instruction_cvtsi2sd.destination_operand, ASM_TYPE_LONGWORD, assembly);
  }

  return INSTRUCTION_FIXED;
}

static ResolveType resolve_mov_zero_extend_instruction(AsmNode *function, AsmNode *mov_zero_extend_instruction, Assembly *assembly) {
  if (mov_zero_extend_instruction->data.instruction_mov_zero_extend.destination->type == ASM_OPERAND_REGISTER) {
    //TODO: No assembly type
    AsmNode *mov_instruction = arena_alloc(assembly->asm_arena);
    mov_instruction->type = ASM_INSTRUCTION_MOV;
    mov_instruction->data.instruction_mov.source = mov_zero_extend_instruction->data.instruction_mov_zero_extend.source;
    mov_instruction->data.instruction_mov.destination = mov_zero_extend_instruction->data.instruction_mov_zero_extend.destination;
    
    add_instruction_to_function(function, mov_instruction);

    return INSTRUCTION_FIXED;
  }
  
  if (mov_zero_extend_instruction->data.instruction_mov_zero_extend.destination->type == ASM_OPERAND_IMM) {
    emit_asm_mov_instruction(function, mov_zero_extend_instruction->data.instruction_mov_zero_extend.source, assembly->register_r11, ASM_TYPE_LONGWORD, assembly);
    emit_asm_mov_instruction(function, assembly->register_r11, mov_zero_extend_instruction->data.instruction_mov_zero_extend.destination, ASM_TYPE_QUADWORD, assembly);

    return INSTRUCTION_FIXED;
  }

  return INSTRUCTION_NOT_FIXED;
}

static ResolveType resolve_binary_mul_instruction(AsmNode *function, AsmNode *instruction, Assembly *assembly) {
  //MUL instructions cannot use a memory address as its destination
  if (instruction->data.instruction_binary.operand_2->type != ASM_OPERAND_STACK) {
    return INSTRUCTION_NOT_FIXED;
  }
  
  AsmNode *destination;

  if (instruction->data.instruction_binary.assembly_type == ASM_TYPE_DOUBLE) {
    destination = assembly->register_xmm15;
  } else {
    destination = assembly->register_r11;
  }

  emit_asm_mov_instruction(function, instruction->data.instruction_binary.operand_2, destination, instruction->data.instruction_binary.assembly_type, assembly);
  emit_asm_binary_instruction(function, instruction->data.instruction_binary.operand_1, destination, ASM_BINARY_MULT, instruction->data.instruction_binary.assembly_type, assembly);
  emit_asm_mov_instruction(function, destination, instruction->data.instruction_binary.operand_2, instruction->data.instruction_binary.assembly_type, assembly);

  return INSTRUCTION_FIXED;
}

static ResolveType resolve_binary_add_sub_instruction(AsmNode *function, AsmNode *instruction, Assembly *assembly) {
  //ADD and SUB instructions cannot have both a source and destination as memory addresses
  if (instruction->data.instruction_binary.operand_1->type != ASM_OPERAND_STACK || instruction->data.instruction_binary.operand_2->type != ASM_OPERAND_STACK) {
    return INSTRUCTION_NOT_FIXED;
  }

  AsmNode *destination;

  if (instruction->data.instruction_binary.assembly_type == ASM_TYPE_DOUBLE) {
    destination = assembly->register_xmm15;    
  } else {
    destination = assembly->register_r10;
  }

  emit_asm_mov_instruction(function, instruction->data.instruction_binary.operand_1, destination, instruction->data.instruction_binary.assembly_type, assembly);
  emit_asm_binary_instruction(function, destination, instruction->data.instruction_binary.operand_2, instruction->data.instruction_binary.binary_op, instruction->data.instruction_binary.assembly_type, assembly);

  return INSTRUCTION_FIXED;
}

static ResolveType resolve_binary_double_instructions(AsmNode *function, AsmNode *instruction, Assembly *assembly) {
  if (instruction->data.instruction_binary.operand_2->type == ASM_OPERAND_REGISTER) {
    return INSTRUCTION_NOT_FIXED;
  }

  AsmBinaryOpType op_type = instruction->data.instruction_binary.binary_op;

  if (op_type != ASM_BINARY_ADD && op_type != ASM_BINARY_SUB && op_type != ASM_BINARY_MULT && op_type != ASM_BINARY_DIV_DOUBLE && op_type != ASM_BINARY_BITWISE_XOR) {
    return INSTRUCTION_NOT_FIXED;
  }

  emit_asm_mov_instruction(function, instruction->data.instruction_binary.operand_2, assembly->register_xmm15, ASM_TYPE_DOUBLE, assembly);
  emit_asm_binary_instruction(function, instruction->data.instruction_binary.operand_1, assembly->register_xmm15, instruction->data.instruction_binary.binary_op, instruction->data.instruction_binary.assembly_type, assembly);

  return INSTRUCTION_FIXED;
}

static ResolveType resolve_cmp_instruction(AsmNode *function, AsmNode *instruction, Assembly *assembly) {
  //CMP instructions cannot have both a source and destination as memory addresses
  if (instruction->data.instruction_cmp.assembly_type != ASM_TYPE_DOUBLE && instruction->data.instruction_cmp.operand_1->type == ASM_OPERAND_STACK && instruction->data.instruction_cmp.operand_2->type == ASM_OPERAND_STACK) {
    emit_asm_mov_instruction(function, instruction->data.instruction_cmp.operand_1, assembly->register_r10, instruction->data.instruction_cmp.assembly_type, assembly);
    emit_asm_cmp_instruction(function, assembly->register_r10, instruction->data.instruction_cmp.operand_2, instruction->data.instruction_cmp.assembly_type, assembly);

    return INSTRUCTION_FIXED;
  }

  //CMP instructions cannot have a constant as the second operand.
  //TODO: Investigate if this is also needed for sub, add, and imul instructions
  if (instruction->data.instruction_cmp.assembly_type != ASM_TYPE_DOUBLE && instruction->data.instruction_cmp.operand_2->type == ASM_OPERAND_IMM) {
    emit_asm_mov_instruction(function, instruction->data.instruction_cmp.operand_2, assembly->register_r11, instruction->data.instruction_cmp.assembly_type, assembly);
    emit_asm_cmp_instruction(function, assembly->register_ax, assembly->register_r11, instruction->data.instruction_cmp.assembly_type, assembly);

    return INSTRUCTION_FIXED;
  }

  if (instruction->data.instruction_cmp.assembly_type == ASM_TYPE_DOUBLE && instruction->data.instruction_cmp.operand_2->type != ASM_OPERAND_REGISTER) {
    emit_asm_mov_instruction(function, instruction->data.instruction_cmp.operand_2, assembly->register_xmm15, ASM_TYPE_DOUBLE, assembly);
    emit_asm_cmp_instruction(function, instruction->data.instruction_cmp.operand_1, assembly->register_xmm15, ASM_TYPE_DOUBLE, assembly);

    return INSTRUCTION_FIXED;
  }

  return INSTRUCTION_NOT_FIXED;
}

static ResolveType resolve_mov_instruction(AsmNode *function, AsmNode *instruction, Assembly *assembly) {
    //MOV and MOVSD instructions cannot have both a source and destination as memory addresses
    if ((instruction->data.instruction_mov.destination->type != ASM_OPERAND_STACK || instruction->data.instruction_mov.source->type != ASM_OPERAND_STACK) &&
        (instruction->data.instruction_mov.destination->type != ASM_OPERAND_STACK || instruction->data.instruction_mov.source->type != ASM_OPERAND_DATA) &&
        (instruction->data.instruction_mov.destination->type != ASM_OPERAND_DATA || instruction->data.instruction_mov.source->type != ASM_OPERAND_IMM) &&
        (instruction->data.instruction_mov.destination->type != ASM_OPERAND_DATA || instruction->data.instruction_mov.source->type != ASM_OPERAND_STACK)) {
      return INSTRUCTION_NOT_FIXED;
    }

    AsmNode *new_destination;

    if (instruction->data.instruction_mov.assembly_type == ASM_TYPE_DOUBLE) {
      new_destination = assembly->register_xmm15;
    } else {
      new_destination = assembly->register_r10;
    } 

    emit_asm_mov_instruction(function, instruction->data.instruction_mov.source, new_destination, instruction->data.instruction_mov.assembly_type, assembly);

    AsmNode *new_source;

    if (instruction->data.instruction_mov.assembly_type == ASM_TYPE_DOUBLE) {
      new_source = assembly->register_xmm15;
    } else {
      new_source = assembly->register_r10;
    } 

    emit_asm_mov_instruction(function, new_source, instruction->data.instruction_mov.destination, instruction->data.instruction_mov.assembly_type, assembly);

    return INSTRUCTION_FIXED;
}

static void pseudo_register_pass(AsmNode *asm_function, AsmBackendSymbolTable *backend_symbol_table, int *stack_offset) {
  HashTable stack_location_table;
  hash_table_init(&stack_location_table);
  
  for (int i = 0; i < asm_function->data.function.instruction_count; i++) {
    AsmNode *instruction = asm_function->data.function.instruction_pointers->asm_pointers[i];

    switch(instruction->type) {
      case ASM_INSTRUCTION_MOV:        
        if (instruction->data.instruction_mov.source->type == ASM_OPERAND_PSEUDO_REGISTER) {
         replace_pseudo_register(instruction->data.instruction_mov.source, instruction->data.instruction_mov.assembly_type, &stack_location_table, backend_symbol_table, stack_offset);
        }

        if (instruction->data.instruction_mov.destination->type == ASM_OPERAND_PSEUDO_REGISTER) {
         replace_pseudo_register(instruction->data.instruction_mov.destination, instruction->data.instruction_mov.assembly_type, &stack_location_table, backend_symbol_table, stack_offset);        
        }
        break;
      case ASM_INSTRUCTION_MOVSX:
        if (instruction->data.instruction_movsx.source->type == ASM_OPERAND_PSEUDO_REGISTER) {
         replace_pseudo_register(instruction->data.instruction_movsx.source, ASM_TYPE_LONGWORD, &stack_location_table, backend_symbol_table, stack_offset);
        }

        if (instruction->data.instruction_movsx.destination->type == ASM_OPERAND_PSEUDO_REGISTER) {
         replace_pseudo_register(instruction->data.instruction_movsx.destination, ASM_TYPE_LONGWORD, &stack_location_table, backend_symbol_table, stack_offset);
        }
        break;
      case ASM_INSTRUCTION_MOV_ZERO_EXTEND:
        if (instruction->data.instruction_mov_zero_extend.source->type == ASM_OPERAND_PSEUDO_REGISTER) {
         replace_pseudo_register(instruction->data.instruction_mov_zero_extend.source, ASM_TYPE_LONGWORD, &stack_location_table, backend_symbol_table, stack_offset);
        }

        if (instruction->data.instruction_mov_zero_extend.destination->type == ASM_OPERAND_PSEUDO_REGISTER) {
         replace_pseudo_register(instruction->data.instruction_mov_zero_extend.destination, ASM_TYPE_LONGWORD, &stack_location_table, backend_symbol_table, stack_offset);
        }
        break;
      case ASM_INSTRUCTION_UNARY:
        if (instruction->data.instruction_unary.operand->type == ASM_OPERAND_PSEUDO_REGISTER) {
         replace_pseudo_register(instruction->data.instruction_unary.operand, instruction->data.instruction_unary.assembly_type, &stack_location_table, backend_symbol_table, stack_offset);        
        }
        break;
      case ASM_INSTRUCTION_BINARY:
        if (instruction->data.instruction_binary.operand_1->type == ASM_OPERAND_PSEUDO_REGISTER) {
         replace_pseudo_register(instruction->data.instruction_binary.operand_1, instruction->data.instruction_binary.assembly_type, &stack_location_table, backend_symbol_table, stack_offset);        
        }

        if (instruction->data.instruction_binary.operand_2->type == ASM_OPERAND_PSEUDO_REGISTER) {
         replace_pseudo_register(instruction->data.instruction_binary.operand_2, instruction->data.instruction_binary.assembly_type, &stack_location_table, backend_symbol_table, stack_offset);        
        }
        break;
      case ASM_INSTRUCTION_IDIV:
        if (instruction->data.instruction_idiv.operand->type == ASM_OPERAND_PSEUDO_REGISTER) {
         replace_pseudo_register(instruction->data.instruction_idiv.operand, instruction->data.instruction_idiv.assembly_type, &stack_location_table, backend_symbol_table, stack_offset);        
        }
        break;
      case ASM_INSTRUCTION_DIV:
        if (instruction->data.instruction_div.operand->type == ASM_OPERAND_PSEUDO_REGISTER) {
         replace_pseudo_register(instruction->data.instruction_div.operand, instruction->data.instruction_div.assembly_type, &stack_location_table, backend_symbol_table, stack_offset);        
        }
        break;
      case ASM_INSTRUCTION_CMP:
        if (instruction->data.instruction_cmp.operand_1->type == ASM_OPERAND_PSEUDO_REGISTER) {
          replace_pseudo_register(instruction->data.instruction_cmp.operand_1, instruction->data.instruction_cmp.assembly_type, &stack_location_table, backend_symbol_table, stack_offset);
        }        

        if (instruction->data.instruction_cmp.operand_2->type == ASM_OPERAND_PSEUDO_REGISTER) {
          replace_pseudo_register(instruction->data.instruction_cmp.operand_2, instruction->data.instruction_cmp.assembly_type, &stack_location_table, backend_symbol_table, stack_offset);
        }        
        break;
      case ASM_INSTRUCTION_PUSH:
        if (instruction->data.instruction_push.operand->type == ASM_OPERAND_PSEUDO_REGISTER) {
          replace_pseudo_register(instruction->data.instruction_push.operand, ASM_TYPE_LONGWORD, &stack_location_table, backend_symbol_table, stack_offset);
        }
        break;
      case ASM_INSTRUCTION_CVTSI2SD:
        if (instruction->data.instruction_cvtsi2sd.source_operand->type == ASM_OPERAND_PSEUDO_REGISTER) {
          replace_pseudo_register(instruction->data.instruction_cvtsi2sd.source_operand, instruction->data.instruction_cvtsi2sd.source_assembly_type, &stack_location_table, backend_symbol_table, stack_offset);
        }

        if (instruction->data.instruction_cvtsi2sd.destination_operand->type == ASM_OPERAND_PSEUDO_REGISTER) {
          replace_pseudo_register(instruction->data.instruction_cvtsi2sd.destination_operand, instruction->data.instruction_cvtsi2sd.source_assembly_type, &stack_location_table, backend_symbol_table, stack_offset);
        }
        break;
      case ASM_INSTRUCTION_CVTTSD2SI:
        if (instruction->data.instruction_cvttsd2si.source_operand->type == ASM_OPERAND_PSEUDO_REGISTER) {
          replace_pseudo_register(instruction->data.instruction_cvttsd2si.source_operand, instruction->data.instruction_cvttsd2si.destination_assembly_type, &stack_location_table, backend_symbol_table, stack_offset);
        }

        if (instruction->data.instruction_cvttsd2si.destination_operand->type == ASM_OPERAND_PSEUDO_REGISTER) {
          replace_pseudo_register(instruction->data.instruction_cvttsd2si.destination_operand, instruction->data.instruction_cvttsd2si.destination_assembly_type, &stack_location_table, backend_symbol_table, stack_offset);
        }
        break;
      default:
        break;
    }
  }
}

static void replace_pseudo_register(AsmNode *pseudo_register, AsmType instruction_type, HashTable *stack_location_table, AsmBackendSymbolTable *backend_symbol_table, int *stack_offset) {
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
      if (instruction_type == ASM_TYPE_QUADWORD || instruction_type == ASM_TYPE_DOUBLE) {
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

static void emit_ir_function(IRNode *ir_function, AsmNode *asm_function, Assembly *assembly) {
  asm_function->type = ASM_FUNCTION;
  asm_function->data.function.name = ir_function->data.function.identifier;
  asm_function->data.function.is_global = ir_function->data.function.is_global;
  
  AsmNodePointers *asm_pointers = malloc(sizeof(AsmNodePointers));
  init_node_pointer(asm_pointers);
  asm_function->data.function.instruction_count = 0;
  asm_function->data.function.instruction_pointers = asm_pointers;

  //Adds the Allocate Stack instruction, but will allocate the stack offset value of the instruction in another pass after building the assembly nodes
  emit_ir_instruction_allocate_rsp_stack(asm_function, 0, assembly);

  //Parameter instructions
  int stack_offset = 16;
  int general_register_count = 0;
  int floating_point_register_count = 0;

  for (int i = 0; i < ir_function->data.function.parameter_count; i++) {    
    HashTableEntry *parameter_variable_symbol_entry = hash_table_get_entry(assembly->declaration_symbol_table->symbol_table, ir_function->data.function.parameter_identifiers[i]);

    if (parameter_variable_symbol_entry == NULL || parameter_variable_symbol_entry->key == NULL) {
      fprintf(stderr, "ERROR: Assembler - Could not find '%s' function parameter identifier in symbol table", ir_function->data.function.identifier);
      exit(1);
    }

    Types parameter_type = ((DeclarationSymbol*)(parameter_variable_symbol_entry->value->structure))->data.variable_symbol->value_type;
    
    AsmNode *source_operand = arena_alloc(assembly->asm_arena);

    if (parameter_type != TYPE_DOUBLE && general_register_count < 6) {
      switch (general_register_count) {
        case 0:   source_operand->data.operand_register.op_register = ASM_REGISTER_DI; break;
        case 1:   source_operand->data.operand_register.op_register = ASM_REGISTER_SI; break;
        case 2:   source_operand->data.operand_register.op_register = ASM_REGISTER_DX; break;
        case 3:   source_operand->data.operand_register.op_register = ASM_REGISTER_CX; break;
        case 4:   source_operand->data.operand_register.op_register = ASM_REGISTER_R8; break;
        case 5:   source_operand->data.operand_register.op_register = ASM_REGISTER_R9; break;
      }
    
      source_operand->type = ASM_OPERAND_REGISTER;
      general_register_count++;
    } else if (parameter_type == TYPE_DOUBLE && floating_point_register_count < 8) {
      switch (floating_point_register_count) {
        case 0:   source_operand->data.operand_register.op_register = ASM_REGISTER_XMM0; break;
        case 1:   source_operand->data.operand_register.op_register = ASM_REGISTER_XMM1; break;
        case 2:   source_operand->data.operand_register.op_register = ASM_REGISTER_XMM2; break;
        case 3:   source_operand->data.operand_register.op_register = ASM_REGISTER_XMM3; break;
        case 4:   source_operand->data.operand_register.op_register = ASM_REGISTER_XMM4; break;
        case 5:   source_operand->data.operand_register.op_register = ASM_REGISTER_XMM5; break;
        case 6:   source_operand->data.operand_register.op_register = ASM_REGISTER_XMM6; break;
        case 7:   source_operand->data.operand_register.op_register = ASM_REGISTER_XMM7; break;
      }
    
      source_operand->type = ASM_OPERAND_REGISTER;
      floating_point_register_count++;
    } else {
      source_operand->data.operand_stack.address = stack_offset;
      source_operand->type = ASM_OPERAND_STACK;
      stack_offset += 8;
    }
    
    AsmNode *destination_pseudo_register = arena_alloc(assembly->asm_arena);
    destination_pseudo_register->type = ASM_OPERAND_PSEUDO_REGISTER;
    destination_pseudo_register->data.operand_pseudo_register.identifier = ir_function->data.function.parameter_identifiers[i];

    AsmType assembly_type = convert_type_to_asm_type(parameter_type); 

    emit_asm_mov_instruction(asm_function, source_operand, destination_pseudo_register, assembly_type, assembly);
  }

  //Function block instructions
  for (int i = 0; i < ir_function->data.function.instruction_count; i++) {
    IRNode *current_ir_node = ir_function->data.function.instruction_ptrs->node_pointers[i];
    switch (current_ir_node->type) {
      case IR_INSTRUCTION_RET:
        emit_ir_instruction_return(asm_function, current_ir_node, assembly);
        break;
      case IR_INSTRUCTION_UNARY: {
        AsmType source_type = convert_ir_value_to_asm_type(current_ir_node->data.instruction_unary.source, assembly->declaration_symbol_table);

        if (current_ir_node->data.instruction_unary.op_type != IR_UNARY_NOT) {
          if (current_ir_node->data.instruction_unary.op_type != IR_UNARY_NEGATE && source_type == ASM_TYPE_DOUBLE) {
            emit_ir_instruction_unary_negation_double(asm_function, current_ir_node, assembly); 
            continue;
          }

          emit_ir_instruction_unary(asm_function, current_ir_node, assembly);
          continue;
        }

        if (source_type == ASM_TYPE_DOUBLE) {
          emit_ir_instruction_unary_not_double(asm_function, current_ir_node, assembly);
          continue;
        } 

        emit_ir_instruction_unary_not_integer(asm_function, current_ir_node, assembly);
        break;
      }
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
            emit_ir_instruction_binary(asm_function, current_ir_node, assembly);
            break;
          case IR_BINARY_EQUAL:
          case IR_BINARY_NOT_EQUAL:
          case IR_BINARY_GREATER_THAN:
          case IR_BINARY_GREATER_OR_EQUAL:
          case IR_BINARY_LESS_THAN:
          case IR_BINARY_LESS_OR_EQUAL:
            emit_ir_instruction_binary_relational(asm_function, current_ir_node, assembly);
            break;
          case IR_BINARY_DIVIDE: {
            AsmType source_1_type = convert_ir_value_to_asm_type(current_ir_node->data.instruction_binary.source_1, assembly->declaration_symbol_table);
            AsmType source_2_type = convert_ir_value_to_asm_type(current_ir_node->data.instruction_binary.source_2, assembly->declaration_symbol_table);

            if (source_1_type == ASM_TYPE_DOUBLE && source_2_type == ASM_TYPE_DOUBLE) {
              emit_ir_instruction_binary(asm_function, current_ir_node, assembly);
            } else {
              if (is_signed_ir_value_node(current_ir_node->data.instruction_binary.destination, assembly->declaration_symbol_table)) {
                emit_ir_instruction_binary_signed_division(asm_function, current_ir_node, assembly);            
              } else {
                emit_ir_instruction_binary_unsigned_division(asm_function, current_ir_node, assembly);
              }
            }             
            break;
          }
          case IR_BINARY_REMAINDER:
            if (is_signed_ir_value_node(current_ir_node->data.instruction_binary.destination, assembly->declaration_symbol_table)) {
              emit_ir_instruction_binary_signed_division(asm_function, current_ir_node, assembly);            
            } else {
              emit_ir_instruction_binary_unsigned_division(asm_function, current_ir_node, assembly);
            }
            break;
        }
          break;
        case IR_INSTRUCTION_JUMP:
          emit_ir_instruction_jump(asm_function, current_ir_node, assembly);
          break;
        case IR_INSTRUCTION_JUMP_IF_ZERO: {
          AsmType source_type = convert_ir_value_to_asm_type(current_ir_node->data.instruction_jump_if_zero.condition, assembly->declaration_symbol_table);

          if (source_type == ASM_TYPE_DOUBLE) {
            emit_ir_instruction_jump_if_zero_double(asm_function, current_ir_node, assembly);
          } else {
            emit_ir_instruction_jump_if_zero_integer(asm_function, current_ir_node, source_type, assembly);
          }
          break;
        }
        case IR_INSTRUCTION_JUMP_IF_NOT_ZERO: {
          AsmType source_type = convert_ir_value_to_asm_type(current_ir_node->data.instruction_jump_if_not_zero.condition, assembly->declaration_symbol_table);

          if (source_type == ASM_TYPE_DOUBLE) {
            emit_ir_instruction_jump_if_not_zero_double(asm_function, current_ir_node, assembly);
          } else {
            emit_ir_instruction_jump_if_not_zero_integer(asm_function, current_ir_node, source_type, assembly);
          }
          break;
        }
        case IR_INSTRUCTION_COPY:
          emit_ir_instruction_copy(asm_function, current_ir_node, assembly);
          break;
        case IR_INSTRUCTION_LABEL:
          emit_ir_instruction_label(asm_function, current_ir_node, assembly);
          break;
        case IR_INSTRUCTION_FUNCTION_CALL:
          emit_ir_instruction_function_call(asm_function, current_ir_node, assembly);
          break;
        case IR_INSTRUCTION_SIGN_EXTEND:
          emit_ir_instruction_sign_extend(asm_function, current_ir_node, assembly);
          break;
        case IR_INSTRUCTION_ZERO_EXTEND:
          emit_ir_instruction_zero_extend(asm_function, current_ir_node, assembly);
          break;
        case IR_INSTRUCTION_TRUNCATE:
          emit_ir_instruction_truncate(asm_function, current_ir_node, assembly);
          break;
        case IR_INSTRUCTION_INT_TO_DOUBLE:
          emit_ir_instruction_cvtsi2sd(asm_function, current_ir_node, assembly);
          break;
        case IR_INSTRUCTION_DOUBLE_TO_INT:
          emit_ir_instruction_cvttsd2si(asm_function, current_ir_node, assembly);
          break;
        case IR_INSTRUCTION_UINT_TO_DOUBLE: {
          Types type = get_ir_node_type(current_ir_node->data.instruction_uint_to_double.source, assembly->declaration_symbol_table);

          if (type == TYPE_UINT) {
            emit_ir_instruction_uint_to_double(asm_function, current_ir_node, assembly);
          } else {
            emit_ir_instruction_ulong_to_double(asm_function, current_ir_node, assembly); 
          }
          
          break;
        }
        case IR_INSTRUCTION_DOUBLE_TO_UINT: {
          Types type = get_ir_node_type(current_ir_node->data.instruction_double_to_uint.source, assembly->declaration_symbol_table);

          if (type == TYPE_UINT) {
            emit_ir_instruction_double_to_uint(asm_function, current_ir_node, assembly);
          } else {
            emit_ir_instruction_double_to_ulong(asm_function, current_ir_node, assembly);
          }
          break;
        }
      default:
        fprintf(stderr, "ERROR - Assembler: Could not resolve instruction type in asm_function\n");
        exit(1);
    }
  }
}

static void emit_static_variable(IRNode *ir_static_variable, AsmNode *asm_static_variable) {
  asm_static_variable->type = ASM_STATIC_VARIABLE;
  asm_static_variable->data.static_variable.identifier = ir_static_variable->data.static_variable.identifier;
  asm_static_variable->data.static_variable.static_variable_symbol = ir_static_variable->data.static_variable.static_variable_symbol;
  asm_static_variable->data.static_variable.is_global = ir_static_variable->data.static_variable.is_global;

  switch (ir_static_variable->data.static_variable.static_variable_symbol->value_type) {
    case TYPE_INT:
    case TYPE_UINT:
      asm_static_variable->data.static_variable.alignment = ALIGNMENT_LONGWORD;
      break;
    case TYPE_LONG:
    case TYPE_ULONG:
    case TYPE_DOUBLE:
      asm_static_variable->data.static_variable.alignment = ALIGNMENT_QUADWORD;
      break;
    default:
      fprintf(stderr, "ERROR: Assembler - Could not assign alignment value to static variable '%s'\n", ir_static_variable->data.static_variable.identifier);
      exit(1);
  }  
}

static AsmNode* emit_static_constant(double source_double, int alignment, Assembly *assembly) {  
  static int constant_label_counter = 0;

  char *constant_label= malloc(64);
  snprintf(constant_label, 64, "static_constant.%d", constant_label_counter); 
 
  //If we've created a static constant with the same value and alignment, reuse and return the same pointer
  for (int i = 0; i < assembly->top_level_declarations->count; i++) {
    if (assembly->top_level_declarations->asm_pointers[i]->type != ASM_STATIC_CONSTANT) {
      continue;
    }

    if (assembly->top_level_declarations->asm_pointers[i]->data.static_constant.alignment != ALIGNMENT_QUADWORD) {
      continue;
    }

    double ir_double = source_double;
    double top_level_double = assembly->top_level_declarations->asm_pointers[i]->data.static_constant.static_init->static_initial_value.double_value; 

    //0.0 and -0.0 should be treated independantly. A new top level entry should be made for both if they are both declared
    //TODO: Look into de-duplicating the data alloc that happend here and below when a top level declaration is new
    if (ir_double == 0.0 && top_level_double == 0.0 && signbit(ir_double) == signbit(top_level_double)) {
      AsmNode *data = arena_alloc(assembly->asm_arena);
      data->type = ASM_OPERAND_DATA;
      data->data.operand_data.identifier = assembly->top_level_declarations->asm_pointers[i]->data.static_constant.identifier;

      return data;
    } else if (ir_double == top_level_double) {
      AsmNode *data = arena_alloc(assembly->asm_arena);
      data->type = ASM_OPERAND_DATA;
      data->data.operand_data.identifier = assembly->top_level_declarations->asm_pointers[i]->data.static_constant.identifier;

      return data;
    }    
  }

  constant_label_counter++;

  InitialValue initial_value = { .double_value = source_double };  

  add_static_variable_declaration_symbol(assembly->declaration_symbol_table, TYPE_DOUBLE, initial_value, constant_label, true, INITIAL_VALUE_INITIALIZED);  

  HashTableEntry *entry = hash_table_get_entry(assembly->declaration_symbol_table->symbol_table, constant_label);

  if (entry == NULL || entry->key == NULL) {
    fprintf(stderr, "ERROR - Assembly: Could not find static constant label '%s' in symbol declaration table\n", constant_label);
    exit(1);
  }
  
  DeclarationSymbol *symbol = entry->value->structure;

  AsmNode *static_constant = arena_alloc(assembly->asm_arena);
  static_constant->type = ASM_STATIC_CONSTANT;
  static_constant->data.static_constant.alignment = alignment;
  static_constant->data.static_constant.identifier = constant_label;
  static_constant->data.static_constant.static_init = symbol->data.variable_symbol;

  add_to_node_pointer(static_constant, assembly->top_level_declarations);  
  add_to_node_pointer(static_constant, assembly->static_constants);
  
  AsmNode *data_operand = arena_alloc(assembly->asm_arena);
  data_operand->type = ASM_OPERAND_DATA;
  data_operand->data.operand_data.identifier = constant_label;

  return data_operand;
}

static void emit_ir_instruction_allocate_rsp_stack(AsmNode *asm_function, int bytes, Assembly *assembly) {
  AsmNode *imm_operand = create_imm_operand(bytes, assembly);
  emit_asm_binary_instruction(asm_function, imm_operand, assembly->register_sp, ASM_BINARY_SUB, ASM_TYPE_QUADWORD, assembly);
}

static void emit_ir_instruction_label(AsmNode *asm_function, IRNode *ir_label_instruction, Assembly *assembly) {
  AsmNode *label = arena_alloc(assembly->asm_arena);
  label->type = ASM_INSTRUCTION_LABEL;
  label->data.instruction_label.identifier = ir_label_instruction->data.instruction_label.identifier;

  add_instruction_to_function(asm_function, label);
}

static void emit_ir_instruction_copy(AsmNode *asm_function, IRNode *ir_copy_instruction, Assembly *assembly) {
  AsmNode *source = create_operand(ir_copy_instruction->data.instruction_copy.source, assembly);
  AsmNode *destination = create_operand(ir_copy_instruction->data.instruction_copy.destination, assembly);

  AsmType source_type = convert_ir_value_to_asm_type(ir_copy_instruction->data.instruction_copy.source, assembly->declaration_symbol_table);

  emit_asm_mov_instruction(asm_function, source, destination, source_type, assembly);
}

static void emit_ir_instruction_jump(AsmNode *asm_function, IRNode *ir_jump_instruction, Assembly *assembly) {
  AsmNode *jmp_instruction = arena_alloc(assembly->asm_arena);
  jmp_instruction->type = ASM_INSTRUCTION_JMP;
  jmp_instruction->data.instruction_jmp.identifier = ir_jump_instruction->data.instruction_jump.target;

  add_instruction_to_function(asm_function, jmp_instruction); 
}

static void emit_ir_instruction_jump_if_zero_integer(AsmNode *asm_function, IRNode *ir_jump_if_zero_instruction, AsmType asm_source_type, Assembly *assembly) {
  AsmNode *imm = create_imm_operand(0, assembly);
  AsmNode *condition = create_operand(ir_jump_if_zero_instruction->data.instruction_jump_if_zero.condition, assembly);

  emit_asm_cmp_instruction(asm_function, imm, condition, asm_source_type, assembly);
  emit_asm_jmpcc_instruction(asm_function, ASM_CONDITION_EQUAL, ir_jump_if_zero_instruction->data.instruction_jump_if_zero.target, assembly);
}

static void emit_ir_instruction_jump_if_zero_double(AsmNode *asm_function, IRNode *ir_jump_if_zero_instruction, Assembly *assembly) {
  //TODO: Using XMM14 and 15 since 1-7 are reserved for loading function arguments in System V ABI. Confirm that using this is okay.

  emit_asm_binary_instruction(asm_function, assembly->register_xmm14, assembly->register_xmm14, ASM_BINARY_BITWISE_XOR, ASM_TYPE_DOUBLE, assembly);

  AsmNode *condition = create_operand(ir_jump_if_zero_instruction->data.instruction_jump_if_zero.condition, assembly);

  emit_asm_cmp_instruction(asm_function, condition, assembly->register_xmm14, ASM_TYPE_DOUBLE, assembly);
  emit_asm_jmpcc_instruction(asm_function, ASM_CONDITION_EQUAL, ir_jump_if_zero_instruction->data.instruction_jump_if_zero.target, assembly);
}

static void emit_ir_instruction_jump_if_not_zero_integer(AsmNode *asm_function, IRNode *ir_jump_if_not_zero_instruction, AsmType source_asm_type, Assembly *assembly) {
  AsmNode *imm = create_imm_operand(0, assembly);
  AsmNode *condition = create_operand(ir_jump_if_not_zero_instruction->data.instruction_jump_if_not_zero.condition, assembly);

  emit_asm_cmp_instruction(asm_function, imm, condition, source_asm_type, assembly);
  emit_asm_jmpcc_instruction(asm_function, ASM_CONDITION_NOT_EQUAL, ir_jump_if_not_zero_instruction->data.instruction_jump_if_not_zero.target, assembly);
}

static void emit_ir_instruction_jump_if_not_zero_double(AsmNode *asm_function, IRNode *ir_jump_if_not_zero_instruction, Assembly *assembly) {  
  //TODO: Using XMM14 and 15 since 1-7 are reserved for loading function arguments in System V ABI. Confirm that using this is okay.
  emit_asm_binary_instruction(asm_function, assembly->register_xmm14, assembly->register_xmm14, ASM_BINARY_BITWISE_XOR, ASM_TYPE_DOUBLE, assembly);

  AsmNode *condition = create_operand(ir_jump_if_not_zero_instruction->data.instruction_jump_if_zero.condition, assembly);

  emit_asm_cmp_instruction(asm_function, condition, assembly->register_xmm14, ASM_TYPE_DOUBLE, assembly);
  emit_asm_jmpcc_instruction(asm_function, ASM_CONDITION_NOT_EQUAL, ir_jump_if_not_zero_instruction->data.instruction_jump_if_zero.target, assembly);
}

static void emit_ir_instruction_binary(AsmNode *asm_function, IRNode *ir_binary_instruction, Assembly *assembly) {
  AsmNode *source_1 = create_operand(ir_binary_instruction->data.instruction_binary.source_1, assembly);
  AsmNode *source_2 = create_operand(ir_binary_instruction->data.instruction_binary.source_2, assembly);
  AsmNode *destination_node = create_operand(ir_binary_instruction->data.instruction_binary.destination, assembly);

  AsmType source_1_type = convert_ir_value_to_asm_type(ir_binary_instruction->data.instruction_binary.source_1, assembly->declaration_symbol_table);

  emit_asm_mov_instruction(asm_function, source_1, destination_node, source_1_type, assembly);

  AsmBinaryOpType binary_op; 

  switch (ir_binary_instruction->data.instruction_binary.op_type) {
    case IR_BINARY_ADD:                 binary_op = ASM_BINARY_ADD; break;
    case IR_BINARY_SUBTRACT:            binary_op = ASM_BINARY_SUB; break;
    case IR_BINARY_MULTIPLY:            binary_op = ASM_BINARY_MULT; break;
    case IR_BINARY_BITWISE_AND:         binary_op = ASM_BINARY_BITWISE_AND; break;
    case IR_BINARY_BITWISE_OR:          binary_op = ASM_BINARY_BITWISE_OR; break;
    case IR_BINARY_BITWISE_XOR:         binary_op = ASM_BINARY_BITWISE_XOR; break;
    case IR_BINARY_BITWISE_LEFT_SHIFT:  binary_op = ASM_BINARY_BITWISE_LEFT_SHIFT; break;
    case IR_BINARY_BITWISE_RIGHT_SHIFT: binary_op = ASM_BINARY_BITWISE_RIGHT_SHIFT; break;
    case IR_BINARY_DIVIDE:              binary_op = ASM_BINARY_DIV_DOUBLE; break;      
    default:
      fprintf(stderr, "ERROR - Assembler: Operator type not found for binary operation\n");
      exit(1);
      break;
  }

  emit_asm_binary_instruction(asm_function, source_2, destination_node, binary_op, source_1_type, assembly);  
}

static void emit_ir_instruction_unary_not_integer(AsmNode *asm_function, IRNode *ir_unary_not_instruction, Assembly *assembly) {
  AsmNode *source = create_operand(ir_unary_not_instruction->data.instruction_unary.source, assembly);
  AsmNode *destination_node = create_operand(ir_unary_not_instruction->data.instruction_unary.destination, assembly);

  AsmType source_type = convert_ir_value_to_asm_type(ir_unary_not_instruction->data.instruction_unary.source, assembly->declaration_symbol_table);
  AsmType destination_type = convert_ir_value_to_asm_type(ir_unary_not_instruction->data.instruction_unary.destination, assembly->declaration_symbol_table);
  
  AsmNode *imm_operand = create_imm_operand(0, assembly);

  emit_asm_cmp_instruction(asm_function, imm_operand, source, source_type, assembly);
  emit_asm_mov_instruction(asm_function, imm_operand, destination_node, destination_type, assembly);
  emit_asm_setcc_instruction(asm_function, ASM_CONDITION_EQUAL, destination_node, assembly);
}

static void emit_ir_instruction_unary_not_double(AsmNode *asm_function, IRNode *ir_unary_not_instruction, Assembly *assembly) {
  AsmNode *source = create_operand(ir_unary_not_instruction->data.instruction_unary.source, assembly);
  AsmNode *destination_node = create_operand(ir_unary_not_instruction->data.instruction_unary.destination, assembly);
  AsmType destination_type = convert_ir_value_to_asm_type(ir_unary_not_instruction->data.instruction_unary.destination, assembly->declaration_symbol_table);

  //TODO: Using XMM14 and 15 since 1-7 are reserved for loading function arguments in System V ABI. Confirm that using this is okay.
  emit_asm_binary_instruction(asm_function, assembly->register_xmm14, assembly->register_xmm14, ASM_BINARY_BITWISE_XOR, ASM_TYPE_DOUBLE, assembly);  
  emit_asm_cmp_instruction(asm_function, source, assembly->register_xmm14, ASM_TYPE_DOUBLE, assembly);

  AsmNode *imm = create_imm_operand(0, assembly);

  emit_asm_mov_instruction(asm_function, imm, destination_node, destination_type, assembly);
  emit_asm_setcc_instruction(asm_function, ASM_CONDITION_EQUAL, destination_node, assembly);
}

static void emit_ir_instruction_binary_relational(AsmNode *asm_function, IRNode *ir_relational_instruction, Assembly *assembly) {
  AsmNode *source_1 = create_operand(ir_relational_instruction->data.instruction_binary.source_1, assembly);
  AsmNode *source_2 = create_operand(ir_relational_instruction->data.instruction_binary.source_2, assembly);
  AsmNode *destination_node = create_operand(ir_relational_instruction->data.instruction_binary.destination, assembly);

  AsmType source_1_type = convert_ir_value_to_asm_type(ir_relational_instruction->data.instruction_binary.source_1, assembly->declaration_symbol_table);
  AsmType destination_type = convert_ir_value_to_asm_type(ir_relational_instruction->data.instruction_binary.destination, assembly->declaration_symbol_table);
  
  emit_asm_cmp_instruction(asm_function, source_2, source_1, source_1_type, assembly);

  AsmNode *imm_operand = create_imm_operand(0, assembly);

  emit_asm_mov_instruction(asm_function, imm_operand, destination_node, destination_type, assembly);

  AsmConditionCode relational_op;

  bool is_signed_condition = is_signed_ir_value_node(ir_relational_instruction->data.instruction_binary.destination, assembly->declaration_symbol_table);

  switch (ir_relational_instruction->data.instruction_binary.op_type) {
    case IR_BINARY_EQUAL:              relational_op = ASM_CONDITION_EQUAL; break;
    case IR_BINARY_NOT_EQUAL:          relational_op = ASM_CONDITION_NOT_EQUAL; break;
    case IR_BINARY_GREATER_THAN:       relational_op = is_signed_condition ? ASM_CONDITION_GREATER : ASM_CONDITION_ABOVE; break;
    case IR_BINARY_GREATER_OR_EQUAL:   relational_op = is_signed_condition ? ASM_CONDITION_GREATER_EQUAL : ASM_CONDITION_ABOVE_EQUAL; break;
    case IR_BINARY_LESS_THAN:          relational_op = is_signed_condition ? ASM_CONDITION_LESS : ASM_CONDITION_BELOW; break;
    case IR_BINARY_LESS_OR_EQUAL:      relational_op = is_signed_condition ? ASM_CONDITION_LESS_EQUAL : ASM_CONDITION_BELOW_EQUAL; break;      
    default:
      fprintf(stderr, "Binary Relational OP type %d is not found", ir_relational_instruction->data.instruction_binary.op_type);
      exit(1);
      break;
  }

  emit_asm_setcc_instruction(asm_function, relational_op, destination_node, assembly);
}

static void emit_ir_instruction_binary_signed_division(AsmNode *asm_function, const IRNode *ir_binary_instruction, Assembly *assembly) {
  AsmNode *source_1 = create_operand(ir_binary_instruction->data.instruction_binary.source_1, assembly);
  AsmNode *source_2 = create_operand(ir_binary_instruction->data.instruction_binary.source_2, assembly);
  AsmNode *destination_node = create_operand(ir_binary_instruction->data.instruction_binary.destination, assembly);

  AsmType source_1_type = convert_ir_value_to_asm_type(ir_binary_instruction->data.instruction_binary.source_1, assembly->declaration_symbol_table);

  emit_asm_mov_instruction(asm_function, source_1, assembly->register_ax, source_1_type, assembly);

  AsmNode *cdq_instruction = arena_alloc(assembly->asm_arena);
  cdq_instruction->type = ASM_INSTRUCTION_CDQ;
  cdq_instruction->data.instruction_cdq.assembly_type = source_1_type;

  add_instruction_to_function(asm_function, cdq_instruction);

  emit_asm_idiv_instruction(asm_function, source_2, source_1_type, assembly);

  AsmNode *mov_destination_2;
  
  if (ir_binary_instruction->data.instruction_binary.op_type == IR_BINARY_DIVIDE) {
    mov_destination_2 = assembly->register_ax;
  } else {
    //IR_BINARY_REMAINDER
    mov_destination_2 = assembly->register_dx;
  }

  emit_asm_mov_instruction(asm_function, mov_destination_2, destination_node, source_1_type, assembly);
}
 
static void emit_ir_instruction_binary_unsigned_division(AsmNode *asm_function, const IRNode *ir_binary_instruction, Assembly *assembly) {
  AsmNode *source_1 = create_operand(ir_binary_instruction->data.instruction_binary.source_1, assembly);
  AsmNode *source_2 = create_operand(ir_binary_instruction->data.instruction_binary.source_2, assembly);
  AsmNode *destination_node = create_operand(ir_binary_instruction->data.instruction_binary.destination, assembly);

  AsmType source_1_type = convert_ir_value_to_asm_type(ir_binary_instruction->data.instruction_binary.source_1, assembly->declaration_symbol_table);

  emit_asm_mov_instruction(asm_function, source_1, assembly->register_ax, source_1_type, assembly);

  AsmNode *imm_operand = create_imm_operand(0, assembly);

  emit_asm_mov_instruction(asm_function, imm_operand, assembly->register_dx, source_1_type, assembly);
  emit_asm_div_instruction(asm_function, source_2, source_1_type, assembly);

  AsmNode *mov_destination_2;
  
  if (ir_binary_instruction->data.instruction_binary.op_type == IR_BINARY_DIVIDE) {
    mov_destination_2 = assembly->register_ax;
  } else {
    //IR_BINARY_REMAINDER
    mov_destination_2 = assembly->register_dx;
  }

  emit_asm_mov_instruction(asm_function, mov_destination_2, destination_node, source_1_type, assembly);
}

static void emit_ir_instruction_unary(AsmNode *asm_function, IRNode *ir_unary_instruction, Assembly *assembly) {
  AsmNode *source_node = create_operand(ir_unary_instruction->data.instruction_unary.source, assembly);
  AsmNode *destination_node = create_operand(ir_unary_instruction->data.instruction_unary.destination, assembly);

  AsmType source_type = convert_ir_value_to_asm_type(ir_unary_instruction->data.instruction_unary.source, assembly->declaration_symbol_table);

  emit_asm_mov_instruction(asm_function, source_node, destination_node, source_type, assembly);
  
  AsmUnaryOpType op_type;

  if (ir_unary_instruction->data.instruction_unary.op_type == IR_UNARY_NEGATE) {
    op_type = ASM_UNARY_NEG;
  } else {
    op_type = ASM_UNARY_NOT;
  }

  emit_asm_unary_instruction(asm_function, destination_node, op_type, source_type, assembly);
}

static void emit_ir_instruction_unary_negation_double(AsmNode *asm_function, IRNode *ir_unary_instruction, Assembly *assembly) {
  AsmNode *source_node = create_operand(ir_unary_instruction->data.instruction_unary.source, assembly);
  AsmNode *destination_node = create_operand(ir_unary_instruction->data.instruction_unary.destination, assembly);

  emit_asm_mov_instruction(asm_function, source_node, destination_node, ASM_TYPE_DOUBLE, assembly);

  AsmNode *data_node = emit_static_constant(-0.0, 16, assembly); 

  emit_asm_binary_instruction(asm_function, data_node, source_node, ASM_BINARY_BITWISE_XOR, ASM_TYPE_DOUBLE, assembly);
}

static void emit_ir_instruction_return(AsmNode *asm_function, IRNode *ir_return_instruction, Assembly *assembly) {
  //Function calls were being duplicated without this check.
  if (ir_return_instruction->data.instruction_ret.value->type != IR_INSTRUCTION_FUNCTION_CALL) {
    AsmNode *source_node = create_operand(ir_return_instruction->data.instruction_ret.value, assembly);
    AsmType source_type = convert_ir_value_to_asm_type(ir_return_instruction->data.instruction_ret.value, assembly->declaration_symbol_table);

    AsmNode *destination_node;

    if (source_type == ASM_TYPE_DOUBLE) {
      destination_node = assembly->register_xmm0;  
    } else {
      destination_node = assembly->register_ax;  
    }

    emit_asm_mov_instruction(asm_function, source_node, destination_node, source_type, assembly);
  }

  AsmNode *ret_node = arena_alloc(assembly->asm_arena);
  ret_node->type = ASM_INSTRUCTION_RET;

  add_instruction_to_function(asm_function, ret_node);
}

static void emit_ir_instruction_function_call(AsmNode *asm_function, IRNode *ir_function_call_instruction, Assembly *assembly) {
  //As per the System V ABI (Application Binary Interface), the first 6 arguments of a function call will be loaded into the following 'arg_registers' as ordered in the array. After that, any additional arguments will be added to the stack in reverse order to be processed in the order of how they are called.
  AsmRegisterType arg_general_registers[] = { ASM_REGISTER_DI, ASM_REGISTER_SI, ASM_REGISTER_DX, ASM_REGISTER_CX, ASM_REGISTER_R8, ASM_REGISTER_R9 };
  AsmRegisterType arg_floating_point_registers[] = { ASM_REGISTER_XMM0, ASM_REGISTER_XMM1, ASM_REGISTER_XMM2, ASM_REGISTER_XMM3, ASM_REGISTER_XMM4, ASM_REGISTER_XMM5, ASM_REGISTER_XMM6, ASM_REGISTER_XMM7 }; 
 
  int arg_count = ir_function_call_instruction->data.instruction_function_call.arg_count;
  int stack_padding = 0;
   
  //Adjust the stack alignment when there are stack allocated arguments and it's an odd alignment
  if (arg_count > 6 && arg_count % 2 != 0) {
    stack_padding = 8;
    emit_ir_instruction_allocate_rsp_stack(asm_function, stack_padding, assembly);    
  }

  int general_arg_count = 0;
  int floating_point_arg_count = 0;
  int stack_arg_count = 0;

  for (int i = 0; i < arg_count; i++) {
    AsmNode *arg = create_operand(&ir_function_call_instruction->data.instruction_function_call.args[i], assembly);
    Types node_type = get_ir_node_type(&ir_function_call_instruction->data.instruction_function_call.args[i], assembly->declaration_symbol_table);

    if ((node_type != TYPE_DOUBLE && general_arg_count < 6) || (node_type == TYPE_DOUBLE && floating_point_arg_count < 8)) {
      AsmType asm_type = convert_ir_value_to_asm_type(&ir_function_call_instruction->data.instruction_function_call.args[i], assembly->declaration_symbol_table);

      //TODO: Need to use the new registers in Assembly rather than creating them here
      AsmNode *destination = arena_alloc(assembly->asm_arena);
      destination->type = ASM_OPERAND_REGISTER;

      if (node_type == TYPE_DOUBLE) {
        destination->data.operand_register.op_register = arg_floating_point_registers[i];      
        floating_point_arg_count++;
      } else {
        destination->data.operand_register.op_register = arg_general_registers[i];      
        general_arg_count++;
      }

      emit_asm_mov_instruction(asm_function, arg, destination, asm_type, assembly);
    } else {
      if (arg->type == ASM_OPERAND_PSEUDO_REGISTER || arg->type == ASM_OPERAND_IMM || get_instruction_type(arg) == ASM_TYPE_QUADWORD || get_instruction_type(arg) == ASM_TYPE_DOUBLE) {
        emit_asm_push_instruction(asm_function, arg, assembly);
      } else {
        stack_arg_count++;

        AsmType mov_type = convert_ir_value_to_asm_type(&ir_function_call_instruction->data.instruction_function_call.args[i], assembly->declaration_symbol_table);       

        emit_asm_mov_instruction(asm_function, arg, assembly->register_ax, mov_type, assembly);
        emit_asm_push_instruction(asm_function, assembly->register_r10, assembly);
      }

      continue;
    }
  }  

  AsmNode *call_instruction = arena_alloc(assembly->asm_arena);
  call_instruction->type = ASM_INSTRUCTION_CALL;
  call_instruction->data.instruction_call.identifier = ir_function_call_instruction->data.instruction_function_call.identifier;

  add_instruction_to_function(asm_function, call_instruction);        

  //Adjust stack pointer
  int bytes_to_remove = 8 * stack_arg_count + stack_padding;

  if (bytes_to_remove != 0) {
    AsmNode *imm_operand = create_imm_operand(bytes_to_remove, assembly);
    emit_asm_binary_instruction(asm_function, imm_operand, assembly->register_sp, ASM_BINARY_ADD, ASM_TYPE_QUADWORD, assembly);
  }

  //retrieve return value 
  AsmNode *assembly_destination = create_operand(ir_function_call_instruction->data.instruction_function_call.destination, assembly);

  //TODO: @Cleanup - This needs a clean up. Do a conversion function from declaration symbol to asm type. Check to see if entry is found or else throw exception. Don't assume that found entry is a function symbol
  // AsmType return_type = get_instruction_type(assembly_destination);

  HashTableEntry *entry = hash_table_get_entry(assembly->declaration_symbol_table->symbol_table, ir_function_call_instruction->data.instruction_function_call.identifier);
  DeclarationSymbol *declaration_symbol = entry->value->structure;
  
  AsmType return_type = convert_type_to_asm_type(declaration_symbol->data.function_symbol->value_type);

  AsmNode *dest_register;
   
  if (return_type == ASM_TYPE_DOUBLE) {
    dest_register = assembly->register_xmm0; 
  } else {
    dest_register = assembly->register_ax;
  }
  
  AsmType destination_type = convert_ir_value_to_asm_type(ir_function_call_instruction->data.instruction_function_call.destination, assembly->declaration_symbol_table);

  emit_asm_mov_instruction(asm_function, dest_register, assembly_destination, destination_type, assembly); 
}

static void emit_ir_instruction_sign_extend(AsmNode *asm_function, IRNode *ir_sign_extend_instruction, Assembly *assembly) {
  AsmNode *movsx_instruction = arena_alloc(assembly->asm_arena);
  movsx_instruction->type = ASM_INSTRUCTION_MOVSX;
  movsx_instruction->data.instruction_movsx.source = create_operand(ir_sign_extend_instruction->data.instruction_sign_extend.source, assembly); 
  movsx_instruction->data.instruction_movsx.destination = create_operand(ir_sign_extend_instruction->data.instruction_sign_extend.destination, assembly); 

  add_instruction_to_function(asm_function, movsx_instruction);
}

static void emit_ir_instruction_zero_extend(AsmNode *asm_function, IRNode *ir_zero_extend_instruction, Assembly *assembly) {
  AsmNode *source_node = create_operand(ir_zero_extend_instruction->data.instruction_sign_extend.source, assembly);
  AsmNode *destination_node = create_operand(ir_zero_extend_instruction->data.instruction_sign_extend.destination, assembly);

  emit_asm_mov_zero_extend_instruction(asm_function, source_node, destination_node, assembly);
}

static void emit_ir_instruction_truncate(AsmNode *asm_function, IRNode *ir_truncate_instruction, Assembly *assembly) {
  AsmNode *source = create_operand(ir_truncate_instruction->data.instruction_truncate.source, assembly); 
  AsmNode *destination = create_operand(ir_truncate_instruction->data.instruction_truncate.destination, assembly);   

  emit_asm_mov_instruction(asm_function, source, destination, ASM_TYPE_LONGWORD, assembly);
}

static void add_instruction_to_function(AsmNode *function, AsmNode *instruction) {
  add_to_node_pointer(instruction, function->data.function.instruction_pointers);
  function->data.function.instruction_count++;
}

static void emit_ir_instruction_cvtsi2sd(AsmNode *asm_function, IRNode *ir_int_to_double_instruction, Assembly *assembly) {
  AsmNode *source_node = create_operand(ir_int_to_double_instruction->data.instruction_int_to_double.source, assembly);
  AsmNode *destination_node = create_operand(ir_int_to_double_instruction->data.instruction_int_to_double.destination, assembly);
  AsmType source_type = convert_ir_value_to_asm_type(ir_int_to_double_instruction->data.instruction_int_to_double.source, assembly->declaration_symbol_table);

  emit_asm_cvtsi2sd_instruction(asm_function, source_node, destination_node, source_type, assembly);
}

static void emit_ir_instruction_cvttsd2si(AsmNode *asm_function, IRNode *ir_int_to_double_instruction, Assembly *assembly) {
  AsmNode *source_node = create_operand(ir_int_to_double_instruction->data.instruction_double_to_int.source, assembly);
  AsmNode *destination_node = create_operand(ir_int_to_double_instruction->data.instruction_double_to_int.destination, assembly);
  AsmType destination_type = convert_ir_value_to_asm_type(ir_int_to_double_instruction->data.instruction_double_to_int.destination, assembly->declaration_symbol_table);

  emit_asm_cvttsd2si_instruction(asm_function, source_node, destination_node, destination_type, assembly);
}

static void emit_ir_instruction_uint_to_double(AsmNode *asm_function, IRNode *ir_uint_to_double_instruction, Assembly *assembly) {
  AsmNode *source_node = create_operand(ir_uint_to_double_instruction->data.instruction_uint_to_double.source, assembly);
  AsmNode *destination_node = create_operand(ir_uint_to_double_instruction->data.instruction_uint_to_double.destination, assembly);

  emit_asm_mov_zero_extend_instruction(asm_function, source_node, assembly->register_ax, assembly);
  emit_asm_cvtsi2sd_instruction(asm_function, assembly->register_ax, destination_node, ASM_TYPE_QUADWORD, assembly);
}

static void emit_ir_instruction_ulong_to_double(AsmNode *asm_function, IRNode *ir_ulong_to_double_instruction, Assembly *assembly) {
  AsmNode *source_node = create_operand(ir_ulong_to_double_instruction->data.instruction_uint_to_double.source, assembly);
  AsmNode *destination_node = create_operand(ir_ulong_to_double_instruction->data.instruction_uint_to_double.destination, assembly);
  AsmType source_type = get_instruction_type(source_node);
  AsmNode *imm_0 = create_imm_operand(0, assembly);  

  emit_asm_cmp_instruction(asm_function, imm_0, source_node, source_type, assembly);

  static int label1 = 0;

  char *label_1_name = malloc(32);
  snprintf(label_1_name, 32, "%ULongToDbl_CC.d", label1++); 

  emit_asm_jmpcc_instruction(asm_function, ASM_CONDITION_LESS, label_1_name, assembly);
  emit_asm_cvtsi2sd_instruction(asm_function, source_node, destination_node, ASM_TYPE_QUADWORD, assembly);

  static int label2 = 0;

  char *label_2_name = malloc(32);
  snprintf(label_2_name, 32, "%ULongToDbl.d", label2++); 

  emit_asm_jmp_instruction(asm_function, label_2_name, assembly);
  emit_asm_label_instruction(asm_function, label_1_name, assembly);
  emit_asm_mov_instruction(asm_function, source_node, assembly->register_ax, ASM_TYPE_QUADWORD, assembly);
  emit_asm_mov_instruction(asm_function, assembly->register_ax, assembly->register_cx, ASM_TYPE_QUADWORD, assembly);
  emit_asm_unary_instruction(asm_function, assembly->register_cx, ASM_UNARY_SHR, ASM_TYPE_QUADWORD, assembly);

  AsmNode *imm_1 = create_imm_operand(1, assembly);

  emit_asm_binary_instruction(asm_function, imm_1, assembly->register_ax, ASM_BINARY_BITWISE_AND, ASM_TYPE_QUADWORD, assembly);  
  emit_asm_binary_instruction(asm_function, assembly->register_ax, assembly->register_cx, ASM_BINARY_BITWISE_OR, ASM_TYPE_QUADWORD, assembly);
  emit_asm_cvtsi2sd_instruction(asm_function, assembly->register_ax, destination_node, ASM_TYPE_QUADWORD, assembly);
  emit_asm_binary_instruction(asm_function, destination_node, destination_node, ASM_BINARY_ADD, ASM_TYPE_DOUBLE, assembly);
  emit_asm_label_instruction(asm_function, label_2_name, assembly);
}

static void emit_ir_instruction_double_to_uint(AsmNode *asm_function, IRNode *ir_double_to_uint_instruction, Assembly *assembly) {
  AsmNode *source_node = create_operand(ir_double_to_uint_instruction->data.instruction_double_to_uint.source, assembly);
  AsmNode *destination_node = create_operand(ir_double_to_uint_instruction->data.instruction_double_to_uint.destination, assembly);

  emit_asm_cvttsd2si_instruction(asm_function, source_node, assembly->register_ax, ASM_TYPE_QUADWORD, assembly);
  emit_asm_mov_instruction(asm_function, assembly->register_ax, destination_node, ASM_TYPE_LONGWORD, assembly);
}

static void emit_ir_instruction_double_to_ulong(AsmNode *asm_function, IRNode *ir_double_to_ulong_instruction, Assembly *assembly) {
  AsmNode *source_node = create_operand(ir_double_to_ulong_instruction->data.instruction_uint_to_double.source, assembly);
  AsmNode *destination_node = create_operand(ir_double_to_ulong_instruction->data.instruction_uint_to_double.destination, assembly);

  HashTableEntry *entry = hash_table_get_entry(assembly->declaration_symbol_table->symbol_table, ".MAX_LONG");

  if (entry == NULL || entry->key == NULL) {
    InitialValue max_long_init = { .long_value = LONG_MAX };
    add_static_variable_declaration_symbol(assembly->declaration_symbol_table, TYPE_LONG, max_long_init, ".MAX_LONG", true, INITIAL_VALUE_INITIALIZED);       
  }

  AsmNode *upper_bound_data = emit_static_constant(9223372036854775808.0, 8, assembly);

  emit_asm_cmp_instruction(asm_function, upper_bound_data, source_node, ASM_TYPE_DOUBLE, assembly);

  static int label_1_index = 0;

  char *label_1_name = malloc(32);
  snprintf(label_1_name, 32, "%d.DblToULong_CC.d", label_1_index++); 

  //TODO: This may need to be just 'above' since upper bound data is not 'max long + 1', only 'max long'
  emit_asm_jmpcc_instruction(asm_function, ASM_CONDITION_ABOVE_EQUAL, label_1_name, assembly);
  emit_asm_cvttsd2si_instruction(asm_function, source_node, destination_node, ASM_TYPE_QUADWORD, assembly);
    
  static int label_2_index = 0;

  char *label_2_name = malloc(32);
  snprintf(label_2_name, 32, "%d.DblToULong_end.d", label_2_index++); 

  emit_asm_jmp_instruction(asm_function, label_2_name, assembly);
  emit_asm_label_instruction(asm_function, label_1_name, assembly);
  emit_asm_mov_instruction(asm_function, source_node, assembly->register_ax, ASM_TYPE_DOUBLE, assembly);
  emit_asm_binary_instruction(asm_function, upper_bound_data, assembly->register_ax, ASM_BINARY_SUB, ASM_TYPE_DOUBLE, assembly);
  emit_asm_cvttsd2si_instruction(asm_function, assembly->register_ax, destination_node, ASM_TYPE_QUADWORD, assembly);

  AsmNode *imm = create_imm_operand(LONG_MAX, assembly);

  emit_asm_mov_instruction(asm_function, imm, assembly->register_ax, ASM_TYPE_QUADWORD, assembly);
  emit_asm_binary_instruction(asm_function, assembly->register_ax, destination_node, ASM_BINARY_ADD, ASM_TYPE_QUADWORD, assembly);
  emit_asm_label_instruction(asm_function, label_2_name, assembly);
}

static AsmNode* create_register(AsmRegisterType register_type, Assembly *assembly) {
  AsmNode *register_node = arena_alloc(assembly->asm_arena);
  register_node->type = ASM_OPERAND_REGISTER;
  register_node->data.operand_register.op_register = register_type;

  return register_node;
}

static AsmNode* create_imm_operand(long value, Assembly *assembly) {
  AsmNode *imm = arena_alloc(assembly->asm_arena);

  imm->type = ASM_OPERAND_IMM;
  imm->data.operand_imm.value = value;

  return imm;
}

static void emit_asm_mov_instruction(AsmNode *function, AsmNode *source_node, AsmNode *destination_node, AsmType type, Assembly *assembly) {
  AsmNode *mov_node = arena_alloc(assembly->asm_arena);

  mov_node->type = ASM_INSTRUCTION_MOV;
  mov_node->data.instruction_mov.source = source_node;
  mov_node->data.instruction_mov.destination = destination_node;
  mov_node->data.instruction_mov.assembly_type = type;

  add_instruction_to_function(function, mov_node);
}

static void emit_asm_mov_zero_extend_instruction(AsmNode *function, AsmNode *source_node, AsmNode *destination_node, Assembly *assembly) {
  AsmNode *mov_zero_extend_instruction = arena_alloc(assembly->asm_arena);

  mov_zero_extend_instruction->type = ASM_INSTRUCTION_MOV_ZERO_EXTEND;
  mov_zero_extend_instruction->data.instruction_mov_zero_extend.source = source_node;
  mov_zero_extend_instruction->data.instruction_mov_zero_extend.destination = destination_node; 

  add_instruction_to_function(function, mov_zero_extend_instruction);
}

static void emit_asm_cmp_instruction(AsmNode *function, AsmNode *operand_1, AsmNode *operand_2, AsmType type, Assembly *assembly) {
  AsmNode *cmp_instruction = arena_alloc(assembly->asm_arena);
  cmp_instruction->type = ASM_INSTRUCTION_CMP;
  cmp_instruction->data.instruction_cmp.operand_1 = operand_1; 
  cmp_instruction->data.instruction_cmp.operand_2 = operand_2;
  cmp_instruction->data.instruction_cmp.assembly_type = type;

  add_instruction_to_function(function, cmp_instruction);
}

static void emit_asm_binary_instruction(AsmNode *function, AsmNode *operand_1, AsmNode *operand_2, AsmBinaryOpType op_type, AsmType assembly_type, Assembly *assembly) {
  AsmNode *binary_instruction = arena_alloc(assembly->asm_arena);

  binary_instruction->type = ASM_INSTRUCTION_BINARY;
  binary_instruction->data.instruction_binary.binary_op = op_type;
  binary_instruction->data.instruction_binary.operand_1 = operand_1;
  binary_instruction->data.instruction_binary.operand_2 = operand_2;
  binary_instruction->data.instruction_binary.assembly_type = assembly_type;

  add_instruction_to_function(function, binary_instruction);
}

static void emit_asm_div_instruction(AsmNode *function, AsmNode *operand, AsmType type, Assembly *assembly) {
  AsmNode *div_instruction = arena_alloc(assembly->asm_arena);

  div_instruction->type = ASM_INSTRUCTION_DIV;
  div_instruction->data.instruction_div.operand = operand;
  div_instruction->data.instruction_div.assembly_type = type;

  add_instruction_to_function(function, div_instruction);
}

static void emit_asm_idiv_instruction(AsmNode *function, AsmNode *operand, AsmType type, Assembly *assembly) {
  AsmNode *idiv_instruction = arena_alloc(assembly->asm_arena);

  idiv_instruction->type = ASM_INSTRUCTION_IDIV;
  idiv_instruction->data.instruction_idiv.operand = operand;
  idiv_instruction->data.instruction_idiv.assembly_type = type;

  add_instruction_to_function(function, idiv_instruction);
}

static void emit_asm_push_instruction(AsmNode *function, AsmNode *operand, Assembly *assembly) {
  AsmNode *push_instruction = arena_alloc(assembly->asm_arena);

  push_instruction->type = ASM_INSTRUCTION_PUSH;  
  push_instruction->data.instruction_push.operand = operand;

  add_instruction_to_function(function, push_instruction);
}

static void emit_asm_unary_instruction(AsmNode *function, AsmNode *operand, AsmUnaryOpType op_type, AsmType assembly_type, Assembly *assembly) {
  AsmNode *unary = arena_alloc(assembly->asm_arena);

  unary->type = ASM_INSTRUCTION_UNARY;
  unary->data.instruction_unary.assembly_type = assembly_type;
  unary->data.instruction_unary.unary_op = op_type;
  unary->data.instruction_unary.operand = operand;

  add_instruction_to_function(function, unary);
}
 
static void emit_asm_label_instruction(AsmNode *function, char *identifier, Assembly *assembly) {
  AsmNode *label = arena_alloc(assembly->asm_arena);
  label->type = ASM_INSTRUCTION_LABEL;
  label->data.instruction_label.identifier = identifier;

  add_instruction_to_function(function, label);
}

static void emit_asm_cvttsd2si_instruction(AsmNode *function, AsmNode *source_node, AsmNode *destination_node, AsmType type, Assembly *assembly) {
  AsmNode *cvttsd2si = arena_alloc(assembly->asm_arena);

  cvttsd2si->type = ASM_INSTRUCTION_CVTTSD2SI;
  cvttsd2si->data.instruction_cvttsd2si.destination_assembly_type = type;
  cvttsd2si->data.instruction_cvttsd2si.source_operand = source_node;
  cvttsd2si->data.instruction_cvttsd2si.destination_operand = destination_node; 

  add_instruction_to_function(function, cvttsd2si);
}

static void emit_asm_cvtsi2sd_instruction(AsmNode *function, AsmNode *source_node, AsmNode *destination_node, AsmType type, Assembly *assembly) {
  AsmNode *cvtsi2sd = arena_alloc(assembly->asm_arena);

  cvtsi2sd->type = ASM_INSTRUCTION_CVTSI2SD;
  cvtsi2sd->data.instruction_cvtsi2sd.source_assembly_type = type;
  cvtsi2sd->data.instruction_cvtsi2sd.source_operand = source_node;
  cvtsi2sd->data.instruction_cvtsi2sd.destination_operand = destination_node; 

  add_instruction_to_function(function, cvtsi2sd);
}

static void emit_asm_jmp_instruction(AsmNode *function, char *identifier, Assembly *assembly) {
  AsmNode *jmp_end = arena_alloc(assembly->asm_arena);

  jmp_end->type = ASM_INSTRUCTION_JMP;
  jmp_end->data.instruction_jmp.identifier = identifier;

  add_instruction_to_function(function, jmp_end);
} 

static void emit_asm_jmpcc_instruction(AsmNode *function, AsmConditionCode condition_code, char *identifier, Assembly *assembly) {
  AsmNode *jmp_instruction = arena_alloc(assembly->asm_arena);

  jmp_instruction->type = ASM_INSTRUCTION_JMPCC;
  jmp_instruction->data.instruction_jmp_cc.condition_code = condition_code;
  jmp_instruction->data.instruction_jmp_cc.identifier = identifier;

  add_instruction_to_function(function, jmp_instruction);
}

static void emit_asm_setcc_instruction(AsmNode *function, AsmConditionCode condition_code, AsmNode *operand, Assembly *assembly) {
  AsmNode *set_cc_instruction = arena_alloc(assembly->asm_arena);  

  set_cc_instruction->type = ASM_INSTRUCTION_SETCC;
  set_cc_instruction->data.instruction_set_cc.condition_code = condition_code;
  set_cc_instruction->data.instruction_set_cc.operand = operand;

  add_instruction_to_function(function, set_cc_instruction);
} 

static AsmNode* create_operand(IRNode *ir_operand, Assembly *assembly) {
  AsmNode *asm_operand = arena_alloc(assembly->asm_arena);

  switch (ir_operand->type) {
    case IR_VALUE_CONSTANT:
      if (ir_operand->data.value_constant.type == TYPE_DOUBLE) {
        return emit_static_constant(ir_operand->data.value_constant.value.double_value, 8, assembly); 
      }
      
      asm_operand->type = ASM_OPERAND_IMM;

      switch (ir_operand->data.value_constant.type) {
        case TYPE_INT:   asm_operand->data.operand_imm.value = ir_operand->data.value_constant.value.int_value; break;
        case TYPE_LONG:  asm_operand->data.operand_imm.value = ir_operand->data.value_constant.value.long_value; break;         
        case TYPE_UINT:  asm_operand->data.operand_imm.value = ir_operand->data.value_constant.value.uint_value; break;
        case TYPE_ULONG: asm_operand->data.operand_imm.value = ir_operand->data.value_constant.value.ulong_value; break;         
        default:
          fprintf(stderr, "ERROR - Assembler: Constant value type %d not found in asm_operand\n", ir_operand->type);
          exit(1);      
      }
      break;
    case IR_VALUE_VAR: {
      // HashTableEntry *variable_hash_entry = hash_table_get_entry(declaration_symbol_table->symbol_table, ir_operand->data.value_var.identifier);
      //
      // if (variable_hash_entry == NULL || variable_hash_entry->key == NULL) {
      //   fprintf(stderr, "ERROR - Assembler: Var value type %s not found in declaration symbol table\n", ir_operand->data.static_variable.identifier);
      //   exit(1);
      // }
      
      //DeclarationSymbol *declaration_symbol = variable_hash_entry->value->structure;

      asm_operand->type = ASM_OPERAND_PSEUDO_REGISTER;
      asm_operand->data.operand_pseudo_register.identifier = ir_operand->data.value_var.identifier;
      break;
    }
    default:
      fprintf(stderr, "ERROR - Assembler: Operand type %d not found in asm_operand\n", ir_operand->type);
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
    case ASM_STATIC_CONSTANT:
      printf("Static Constant %s\n", node->data.static_constant.identifier);
      break;
    case ASM_INSTRUCTION_MOV:
      printf("MOV -> ");
      print_assembly_type(node->data.instruction_mov.assembly_type);
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
      printf("UNARY -> ");
      print_assembly_type(node->data.instruction_unary.assembly_type);
      printf("Operator( ");
      switch (node->data.instruction_unary.unary_op) {
        case ASM_UNARY_NEG: printf("NEG )"); break;
        case ASM_UNARY_NOT: printf("NOT )"); break;
        case ASM_UNARY_SHR: printf("SHR )"); break;
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
        case ASM_BINARY_DIV_DOUBLE:           printf("DBL DIV -> "); break;
        case ASM_BINARY_BITWISE_AND:          printf("AND -> "); break;
        case ASM_BINARY_BITWISE_OR:           printf("OR -> "); break;
        case ASM_BINARY_BITWISE_XOR:          printf("XOR -> "); break;
        case ASM_BINARY_BITWISE_LEFT_SHIFT:   printf("SHL -> "); break;
        case ASM_BINARY_BITWISE_RIGHT_SHIFT:  printf("SHR -> "); break;
      }

      print_assembly_type(node->data.instruction_binary.assembly_type);
      printf("Src( ");
      print_assembly(node->data.instruction_binary.operand_1);
      printf(") Dest(");
      print_assembly(node->data.instruction_binary.operand_2);
      printf(")");
      printf("\n");
      break;
    case ASM_INSTRUCTION_CDQ:
      printf("CDQ Instruction");
      print_assembly_type(node->data.instruction_cdq.assembly_type);
      printf("\n");
      break;
    case ASM_INSTRUCTION_IDIV:
      printf("IDIV Instruction ");
      print_assembly_type(node->data.instruction_idiv.assembly_type);
      printf("\n");
      print_assembly(node->data.instruction_idiv.operand);
      printf("\n");
      break;
    case ASM_INSTRUCTION_DIV:
      printf("DIV Instruction");
      print_assembly_type(node->data.instruction_div.assembly_type);
      printf("\n");
      print_assembly(node->data.instruction_div.operand);
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
        case ASM_CONDITION_ABOVE:          printf("Above"); break;
        case ASM_CONDITION_ABOVE_EQUAL:    printf("Above or Equal"); break;
        case ASM_CONDITION_BELOW:          printf("Below"); break;
        case ASM_CONDITION_BELOW_EQUAL:    printf("Below or Equal"); break;
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
        case ASM_CONDITION_ABOVE:          printf("Above"); break;
        case ASM_CONDITION_ABOVE_EQUAL:    printf("Above or Equal"); break;
        case ASM_CONDITION_BELOW:          printf("Below"); break;
        case ASM_CONDITION_BELOW_EQUAL:    printf("Below or Equal"); break;
      }
      printf(" )\n");
      break;
    case ASM_INSTRUCTION_CMP:
      printf("CMP -> "); 
      print_assembly_type(node->data.instruction_cmp.assembly_type);
      printf("Operand( ");
      print_assembly(node->data.instruction_cmp.operand_1);
      printf("), Operand( ");
      print_assembly(node->data.instruction_cmp.operand_2);
      printf(")\n");
      break;
    case ASM_INSTRUCTION_LABEL:
      printf("LABEL -> %s\n", node->data.instruction_label.identifier);
      break;
    case ASM_INSTRUCTION_CVTTSD2SI:
      printf("CVTTSD2SI -> ");
      print_assembly_type(node->data.instruction_cvttsd2si.destination_assembly_type);
      printf("Operand( ");
      print_assembly(node->data.instruction_cvttsd2si.source_operand);
      printf("), Operand( ");
      print_assembly(node->data.instruction_cvttsd2si.destination_operand);
      printf(")\n");
      break;
    case ASM_INSTRUCTION_CVTSI2SD:
      printf("CVTSI2SD -> ");
      print_assembly_type(node->data.instruction_cvtsi2sd.source_assembly_type);
      printf("Operand( ");
      print_assembly(node->data.instruction_cvtsi2sd.source_operand);
      printf("), Operand( ");
      print_assembly(node->data.instruction_cvtsi2sd.destination_operand);
      printf(")\n");
      break;
    case ASM_OPERAND_REGISTER:
      printf("Register ");

      switch (node->data.operand_register.op_register) {
        case ASM_REGISTER_AX:    printf("AX"); break;
        case ASM_REGISTER_CX:    printf("CX"); break;
        case ASM_REGISTER_DX:    printf("DX"); break;
        case ASM_REGISTER_DI:    printf("DI"); break;
        case ASM_REGISTER_SI:    printf("SI"); break;
        case ASM_REGISTER_R8:    printf("R8"); break;
        case ASM_REGISTER_R9:    printf("R9"); break;
        case ASM_REGISTER_R10:   printf("R10"); break;
        case ASM_REGISTER_R11:   printf("R11"); break;
        case ASM_REGISTER_SP:    printf("SP"); break;
        case ASM_REGISTER_XMM0:  printf("XMM0"); break;
        case ASM_REGISTER_XMM1:  printf("XMM1"); break;
        case ASM_REGISTER_XMM2:  printf("XMM2"); break;
        case ASM_REGISTER_XMM3:  printf("XMM3"); break;
        case ASM_REGISTER_XMM4:  printf("XMM4"); break;
        case ASM_REGISTER_XMM5:  printf("XMM5"); break;
        case ASM_REGISTER_XMM6:  printf("XMM6"); break;
        case ASM_REGISTER_XMM7:  printf("XMM7"); break;
        case ASM_REGISTER_XMM14: printf("XMM14"); break;   
        case ASM_REGISTER_XMM15: printf("XMM15"); break;
      }
      break;
    case ASM_OPERAND_PSEUDO_REGISTER:
      printf("Pseudo Register %s ", node->data.operand_pseudo_register.identifier);
      break;
    case ASM_OPERAND_IMM:
      printf("IMM %ld ", node->data.operand_imm.value);
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

static void print_assembly_type(AsmType type) {
  switch(type) {
    case ASM_TYPE_QUADWORD: printf("Type(Quadword) "); return;
    case ASM_TYPE_LONGWORD: printf("Type(Longword) "); return;
    case ASM_TYPE_DOUBLE:   printf("Type(Double) "); return;
    default:
      fprintf(stderr, "ERROR - Assembler: AsmType '%d' not supported for assembly type printing\n", type);
      exit(1);
  }
}

static void add_to_node_pointer(AsmNode *asm_node, AsmNodePointers *asm_node_pointer) {
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

static void init_node_pointer(AsmNodePointers *asm_node_pointer) {
  if (asm_node_pointer == NULL) {
    return;
  }
  
  asm_node_pointer->capacity = 0;
  asm_node_pointer->count = 0;
  asm_node_pointer->asm_pointers = NULL;
}

static Types get_ir_node_type(IRNode *ir_node, DeclarationSymbolTable *declaration_symbol_table) {
  switch (ir_node->type) {
    case IR_VALUE_CONSTANT: return ir_node->data.value_constant.type;
    case IR_VALUE_VAR: {
      //TODO: Add some error checking
      HashTableEntry *variable_hash_entry = hash_table_get_entry(declaration_symbol_table->symbol_table, ir_node->data.value_var.identifier);
     
      DeclarationSymbol *declaration_symbol = variable_hash_entry->value->structure;

      return declaration_symbol->data.variable_symbol->value_type;
    }
    case IR_INSTRUCTION_FUNCTION_CALL: {
      //TODO: Add some error checking
      HashTableEntry *function_hash_entry = hash_table_get_entry(declaration_symbol_table->symbol_table, ir_node->data.instruction_function_call.identifier);
     
      DeclarationSymbol *declaration_symbol = function_hash_entry->value->structure;

      return declaration_symbol->data.function_symbol->value_type;
      break;
    }
    default:
      fprintf(stderr, "ERROR - Assembler: Invalid IR Node type '%d' when attempting to get node Type\n", ir_node->type);
      exit(1);
  }
}

static AsmType convert_ir_value_to_asm_type(IRNode *ir_node, DeclarationSymbolTable *declaration_symbol_table) {
  switch (ir_node->type) {
    case IR_VALUE_CONSTANT:
      return convert_type_to_asm_type(ir_node->data.value_constant.type);
    case IR_VALUE_VAR: {
      //TODO: Add some error checking
      HashTableEntry *variable_hash_entry = hash_table_get_entry(declaration_symbol_table->symbol_table, ir_node->data.value_var.identifier);
      DeclarationSymbol *declaration_symbol = variable_hash_entry->value->structure;

      return convert_type_to_asm_type(declaration_symbol->data.variable_symbol->value_type);
    }
    default:
      fprintf(stderr, "ERROR - Assembler: Invalid IR Node type '%d' when attempting to convert to ASM Type\n", ir_node->type);
      exit(1);
  }
}

static AsmType convert_type_to_asm_type(Types type) {
  switch (type) {
    case TYPE_INT:
    case TYPE_UINT:
      return ASM_TYPE_LONGWORD;
    case TYPE_LONG:
    case TYPE_ULONG:
      return ASM_TYPE_QUADWORD;
    case TYPE_DOUBLE:
      return ASM_TYPE_DOUBLE;
    default:
      fprintf(stderr, "ERROR - Assembler: Unsupported Type '%d' when attempting to convert to ASM Variable Type\n", type);
      exit(1);
      break;        
  }
}

static AsmType get_instruction_type(AsmNode *instruction) {
  switch (instruction->type) {
    case ASM_INSTRUCTION_MOV: return instruction->data.instruction_mov.assembly_type;
    case ASM_INSTRUCTION_UNARY: return instruction->data.instruction_unary.assembly_type;
    case ASM_INSTRUCTION_BINARY: return instruction->data.instruction_binary.assembly_type;
    case ASM_INSTRUCTION_CMP: return instruction->data.instruction_cmp.assembly_type;
    case ASM_INSTRUCTION_IDIV: return instruction->data.instruction_idiv.assembly_type;
    case ASM_INSTRUCTION_CDQ: return instruction->data.instruction_cdq.assembly_type;
    //case ASM_OPERAND_IMM: return ASM_TYPE_LONGWORD;
    default:
      fprintf(stderr, "ERROR - Assembler: Could not get instruction type for ASM instruction '%d'\n", instruction->type);
      exit(1);
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
      asm_backend_symbol->data.object_entry.assembly_type = convert_type_to_asm_type(declaration_symbol->data.variable_symbol->value_type);

      if (declaration_symbol->data.variable_symbol->value_type == TYPE_DOUBLE) {
        //TODO: Confirm that this is always the case
        asm_backend_symbol->data.object_entry.is_constant = true;
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
        case ASM_TYPE_DOUBLE:      printf("Double\t"); break;
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

static bool is_signed_ir_value_node(IRNode *ir_node, DeclarationSymbolTable *declaration_symbol_table) {
  Types value_type;
  switch (ir_node->type) {
    case IR_VALUE_CONSTANT:
      value_type = ir_node->data.value_constant.type;
      break;
    case IR_VALUE_VAR: {
      HashTableEntry *variable_hash_entry = hash_table_get_entry(declaration_symbol_table->symbol_table, ir_node->data.value_var.identifier);     
      DeclarationSymbol *declaration_symbol = variable_hash_entry->value->structure;
      value_type = declaration_symbol->data.variable_symbol->value_type;
      break;
    }
    case IR_VALUE_STATIC_VAR:
      value_type = ir_node->data.static_variable.static_variable_symbol->value_type;
      break;
    default:
      fprintf(stderr, "ERROR: Assembly - Unsupported IR node type '%d' when attempting to find if IR Value is signed", ir_node->type);
      exit(1);
  }

  switch (value_type) {
    case TYPE_UINT:
    case TYPE_ULONG:
      return false;
    case TYPE_INT:
    case TYPE_LONG:
    case TYPE_DOUBLE:
      return true;
    default:
      fprintf(stderr, "ERROR: Assembly - Unsupported value type '%d' when attempting to find if IR Value is signed", value_type);
      exit(1);
  }  
}
