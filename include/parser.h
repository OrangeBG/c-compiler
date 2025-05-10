#ifndef PARSER
#define PARSER

#include "../include/lexer.h"

typedef struct AstNode AstNode;

typedef enum {
  AST_PROGRAM,
  AST_FUNCTION,
  AST_STATEMENT_RETURN,
  AST_EXPRESSION_BINARY,
  AST_FACTOR_CONSTANT,
  AST_FACTOR_UNARY
} NodeType;

typedef enum {
  AST_UNARY_COMPLEMENT,
  AST_UNARY_NEGATE
} UnaryOpType;

typedef enum {
  AST_BINARY_ADD,
  AST_BINARY_SUBTRACT,
  AST_BINARY_MULTIPLY,
  AST_BINARY_DIVIDE,
  AST_BINARY_REMAINDER,
  AST_BINARY_BITWISE_AND,
  AST_BINARY_BITWISE_OR,
  AST_BINARY_BITWISE_XOR
} BinaryOpType;

typedef struct AstNode {
  NodeType type;
  union {
    struct Program { struct AstNode *function; } program;
    struct Function { char* name; struct AstNode *statement; } function;
    struct ReturnStatement { struct AstNode* expression; } return_statement;
    struct BinaryExpression { BinaryOpType op_type; struct AstNode *left_expression; struct AstNode *right_expression; } binary_expression;
    struct ConstantFactor { int value; } constant_factor;
    struct UnaryFactor { UnaryOpType op_type; struct AstNode *factor; } unary_factor;
  } data;
} AstNode;

AstNode* parse_ast(Token *tokens, int token_count, char *file);   
void print_ast(AstNode *node, int level);

#endif
