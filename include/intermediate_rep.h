#ifndef INTERMEDIATE_REP
#define INTERMEDIATE_REP

#include "hash_table.h"
#include "parser.h"

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
  IR_INSTRUCTION_TRUNCATE,
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


typedef enum {
  IR_TYPE_INT,
  IR_TYPE_LONG,
} IRType;

typedef struct {
  int capacity;
  int count;
  IRNode **node_pointers;
} IRNodePointer;

typedef struct IRNode {
 IRNodeType type;
 union {
  struct IRProgram { IRNodePointer *top_level_ptrs; int top_level_count; } program;
  struct IRFunction { char *identifier; bool is_global; char *params; int param_count; int instruction_count; IRNodePointer *instruction_ptrs; } function;
  struct IRStaticVariable { char *identifier; bool is_global; IRType type; union { int int_value; long long_value; } initial_value; } static_variable;
  struct IRInstructionReturn { struct IRNode *value; } instruction_ret;
  struct IRInstructionUnary { IRUnaryOpType op_type; IRNode *source; IRNode *destination; } unary;
  struct IRInstructionBinary { IRBinaryOpType op_type; IRNode *source_1; IRNode *source_2; IRNode *destination; } instruction_binary;
  struct IRInstructionCopy { struct IRNode *source; struct IRNode *destination; } instruction_copy;
  struct IRInstructionJump { char *target; } instruction_jump;
  struct IRInstructionJumpIfZero { IRNode *condition; char *target; } instruction_jump_if_zero;
  struct IRInstructionJumpIfNotZero { IRNode *condition; char *target; } instruction_jump_if_not_zero;
  struct IRInstructionLabel { char *identifier; } instruction_label;
  struct IRInstructionSignExtend { IRNode *source; IRNode *destination; } instruction_sign_extend;
  struct IRInstructionTruncate { IRNode *source; IRNode *destination; } instruction_truncate;
  struct IRValueConstant { IRType type; union { int int_value; long long_value; } value; } value_constant;
  struct IRValueVar { char *identifier; } value_var;
  struct IRFunctionCall { char *identifier; IRNode *args; int arg_count; int arg_capacity; IRNode *destination; } instruction_function_call;
 } data; 
} IRNode;

IRNode* generate_intermediate_rep(AstNode *ast_node, HashTable *declaration_symbols);
void print_intermediate_ret(IRNode *ir_node);

#endif
