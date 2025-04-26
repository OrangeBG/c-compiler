#ifndef INTERMEDIATE_REP
#define INTERMEDIATE_REP

#include "parser.h"

typedef struct IRNode IRNode;

typedef enum {
  IR_PROGRAM,
  IR_FUNCTION,
  IR_INSTRUCTION_RET,
  IR_INSTRUCTION_UNARY,
  IR_VALUE_CONSTANT,
  IR_VALUE_VAR
} IRNodeType;

typedef enum {
  IR_UNARY_COMPLEMENT,
  IR_UNARY_NEGATE
} IRUnaryOpType;

typedef struct IRNode {
 IRNodeType type;
 union {
  struct IRProgram { struct IRNode *function; } program;
  struct IRFunction { char *identifier; int instruction_count; int instruction_capacity; IRNode *instructions; } function;
  struct IRInstructionReturn { struct IRNode *value; } instruction_ret;
  struct IRInstructionUnary { IRUnaryOpType op_type; IRNode *source; IRNode *destination; } unary;
  struct IRValueConstant { int value; } value_constant;
  struct IRValueVar { char *identifier; } value_var;
 } data; 
} IRNode;

IRNode* generate_intermediate_rep(AstNode *ast_node);
void print_immediate_ret(IRNode *ir_node);

#endif
