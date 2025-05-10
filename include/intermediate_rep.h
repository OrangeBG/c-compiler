#ifndef INTERMEDIATE_REP
#define INTERMEDIATE_REP

#include "parser.h"

typedef struct IRNode IRNode;

typedef enum {
  IR_PROGRAM,
  IR_FUNCTION,
  IR_INSTRUCTION_RET,
  IR_INSTRUCTION_UNARY,
  IR_INSTRUCTION_BINARY,
  IR_VALUE_CONSTANT,
  IR_VALUE_VAR
} IRNodeType;

typedef enum {
  IR_UNARY_COMPLEMENT,
  IR_UNARY_NEGATE
} IRUnaryOpType;

typedef enum {
  IR_BINARY_ADD,
  IR_BINARY_SUBTRACT,
  IR_BINARY_MULTIPLY,
  IR_BINARY_DIVIDE,
  IR_BINARY_REMAINDER,
  IR_BINARY_BITWISE_AND,
  IR_BINARY_BITWISE_OR,
  IR_BINARY_BITWISE_XOR
} IRBinaryOpType;

typedef struct IRNode {
 IRNodeType type;
 union {
  struct IRProgram { struct IRNode *function; } program;
  struct IRFunction { char *identifier; int instruction_count; int instruction_capacity; IRNode *instructions; } function;
  struct IRInstructionReturn { struct IRNode *value; } instruction_ret;
  struct IRInstructionUnary { IRUnaryOpType op_type; IRNode *source; IRNode *destination; } unary;
  struct IRInstructionBinary { IRBinaryOpType op_type; IRNode *source_1; IRNode *source_2; IRNode *destination; } instruction_binary;
  struct IRValueConstant { int value; } value_constant;
  struct IRValueVar { char *identifier; } value_var;
 } data; 
} IRNode;

IRNode* generate_intermediate_rep(AstNode *ast_node);
void print_immediate_ret(IRNode *ir_node);

#endif
