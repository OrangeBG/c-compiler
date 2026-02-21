#ifndef INTERMEDIATE_REP
#define INTERMEDIATE_REP

#include "declaration_symbol.h"
#include "parser.h"
#include "types.h"

typedef struct IRNode IRNode;

typedef enum {
  IR_PROGRAM,
  IR_FUNCTION,
  IR_INSTRUCTION_RET,
  IR_INSTRUCTION_UNARY,
  IR_INSTRUCTION_BINARY,
  IR_INSTRUCTION_COPY,
  IR_INSTRUCTION_JUMP,
  IR_INSTRUCTION_JUMP_IF_ZERO,
  IR_INSTRUCTION_JUMP_IF_NOT_ZERO,
  IR_INSTRUCTION_LABEL,
  IR_INSTRUCTION_FUNCTION_CALL,
  IR_INSTRUCTION_SIGN_EXTEND,
  IR_INSTRUCTION_ZERO_EXTEND,
  IR_INSTRUCTION_TRUNCATE,
  IR_INSTRUCTION_DOUBLE_TO_INT,
  IR_INSTRUCTION_DOUBLE_TO_UINT,
  IR_INSTRUCTION_INT_TO_DOUBLE,
  IR_INSTRUCTION_UINT_TO_DOUBLE,
  IR_INSTRUCTION_GET_ADDRESS,
  IR_INSTRUCTION_LOAD,
  IR_INSTRUCTION_STORE,
  IR_INSTRUCTION_ADD_POINTER,
  IR_INSTRUCTION_COPY_TO_OFFSET,
  IR_VALUE_CONSTANT,
  IR_VALUE_VAR,
  IR_VALUE_STATIC_VAR
} IRNodeType;

typedef enum {
  IR_UNARY_COMPLEMENT,
  IR_UNARY_NEGATE,
  IR_UNARY_NOT
} IRUnaryOpType;

typedef enum {
  IR_BINARY_ADD,
  IR_BINARY_SUBTRACT,
  IR_BINARY_MULTIPLY,
  IR_BINARY_DIVIDE,
  IR_BINARY_REMAINDER,
  IR_BINARY_BITWISE_AND,
  IR_BINARY_BITWISE_OR,
  IR_BINARY_BITWISE_XOR,
  IR_BINARY_BITWISE_LEFT_SHIFT,
  IR_BINARY_BITWISE_RIGHT_SHIFT,
  IR_BINARY_EQUAL,
  IR_BINARY_NOT_EQUAL,
  IR_BINARY_LESS_THAN,
  IR_BINARY_LESS_OR_EQUAL,
  IR_BINARY_GREATER_THAN,
  IR_BINARY_GREATER_OR_EQUAL,
} IRBinaryOpType;

typedef struct {
  int capacity;
  int count;
  IRNode **node_pointers;
} IRNodePointer;

typedef struct IRNode {
 IRNodeType type;
 union {
  struct IRProgram { IRNodePointer *top_level_ptrs; int top_level_count; } program;
  struct IRFunction { char *identifier; bool is_global; char **parameter_identifiers; int parameter_count;int parameter_identifier_capacity; int instruction_count; IRNodePointer *instruction_ptrs; } function;
  struct IRStaticVariable { char *identifier; bool is_global; VariableSymbol *static_variable_symbol; } static_variable;
  struct IRInstructionReturn { struct IRNode *value; } instruction_ret;
  struct IRInstructionUnary { IRUnaryOpType op_type; IRNode *source; IRNode *destination; } instruction_unary;
  struct IRInstructionBinary { IRBinaryOpType op_type; IRNode *source_1; IRNode *source_2; IRNode *destination; } instruction_binary;
  struct IRInstructionCopy { struct IRNode *source; struct IRNode *destination; } instruction_copy;
  struct IRInstructionJump { char *target; } instruction_jump;
  struct IRInstructionJumpIfZero { IRNode *condition; char *target; } instruction_jump_if_zero;
  struct IRInstructionJumpIfNotZero { IRNode *condition; char *target; } instruction_jump_if_not_zero;
  struct IRInstructionLabel { char *identifier; } instruction_label;
  struct IRInstructionSignExtend { IRNode *source; IRNode *destination; } instruction_sign_extend;
  struct IRInstructionZeroExtend { IRNode *source; IRNode *destination; } instruction_zero_extend;
  struct IRInstructionTruncate { IRNode *source; IRNode *destination; } instruction_truncate;
  struct IRInstructionDoubleToInt { IRNode *source; IRNode *destination; } instruction_double_to_int;
  struct IRInstructionDoubleToUInt { IRNode *source; IRNode *destination; } instruction_double_to_uint;
  struct IRInstructionIntToDouble { IRNode *source; IRNode *destination; } instruction_int_to_double;
  struct IRInstructionUIntToDouble { IRNode *source; IRNode *destination; } instruction_uint_to_double;
  struct IRInstructionGetAddress { IRNode *source; IRNode *destination; } instruction_get_address;
  struct IRInstructionLoad { IRNode *source_pointer; IRNode *destination; } instruction_load;
  struct IRInstructionStore { IRNode *source; IRNode *destination_pointer; } instruction_store;
  struct IRInstructionAddPointer { IRNode *pointer; IRNode *index; int scale; IRNode *destination; } instruction_add_pointer;
  struct IRInstructionCopyToOffset { IRNode *source; char *destination_identifier; int offset; } instruction_copy_to_offset;
  struct IRValueConstant { TypeNode *type; union { int int_value; unsigned uint_value; long long_value; unsigned long ulong_value; double double_value; } value; } value_constant;
  struct IRValueVar { char *identifier; } value_var;
  struct IRFunctionCall { char *identifier; IRNode *args; int arg_count; int arg_capacity; IRNode *destination; } instruction_function_call;
 } data; 
} IRNode;

IRNode* generate_intermediate_rep(AstNode *ast_node, DeclarationSymbolTable *declaration_symbol_table);
void print_intermediate_ret(IRNode *ir_node);

#endif
