#ifndef PARSER
#define PARSER

#include "../include/lexer.h"
#include <stdbool.h>

typedef struct AstNode AstNode;

typedef enum {
  AST_PROGRAM,
  AST_FUNCTION,
  AST_STATEMENT_RETURN,
  AST_STATEMENT_EXPRESSION,
  AST_STATEMENT_NULL,
  AST_DECLARATION,
  AST_EXPRESSION_BINARY,
  AST_EXPRESSION_CONSTANT,
  AST_EXPRESSION_UNARY,
  AST_EXPRESSION_VARIABLE,
  AST_EXPRESSION_ASSIGNMENT
} NodeType;

typedef enum {
  AST_UNARY_COMPLEMENT,
  AST_UNARY_NEGATE,
  AST_UNARY_NOT
} UnaryOpType;

typedef enum {
  AST_BINARY_ADD,
  AST_BINARY_AND,
  AST_BINARY_OR,
  AST_BINARY_EQUAL,
  AST_BINARY_NOT_EQUAL,
  AST_BINARY_LESS_THAN,
  AST_BINARY_LESS_OR_EQUAL,
  AST_BINARY_GREATER_THAN,
  AST_BINARY_GREATER_OR_EQUAL,
  AST_BINARY_SUBTRACT,
  AST_BINARY_MULTIPLY,
  AST_BINARY_DIVIDE,
  AST_BINARY_REMAINDER,
  AST_BINARY_BITWISE_AND,
  AST_BINARY_BITWISE_OR,
  AST_BINARY_BITWISE_XOR,
  AST_BINARY_BITWISE_LEFT_SHIFT,
  AST_BINARY_BITWISE_RIGHT_SHIFT
} BinaryOpType;

typedef enum {
  AST_BLOCK_STATEMENT,
  AST_BLOCK_DECLARATION
} AstBlockType;

typedef struct AstNode {
  NodeType type;
  union {
    struct Program { struct AstNode *function; } program;
    struct Function { char* name; AstNode* blocks; int block_count; int block_capacity; } function;
    struct Block { AstBlockType type; AstNode *block_item; } block;
    struct Declaration { char* identifier; bool has_expression; AstNode* expression; } declaration;
    struct ReturnStatement { struct AstNode* expression; } return_statement;
    struct ExpressionStatement { struct AstNode* expression; } expression_statement;
    struct ConstantExpression { int value; } constant_expression;
    struct VariableExpression { char* identifier; } variable_expression;
    struct UnaryExpression { UnaryOpType op_type; struct AstNode *expression; } unary_expression;
    struct BinaryExpression { BinaryOpType op_type; struct AstNode *left_expression; struct AstNode *right_expression; } binary_expression;
    struct AssignmentExpression { AstNode *left_expression; AstNode *right_expression; } assignement_expression;
  } data;
} AstNode;

AstNode* parse_ast(Token *tokens, int token_count, char *file);   
void print_ast(AstNode *node, int level);

#endif
