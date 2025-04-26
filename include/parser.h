#ifndef PARSER
#define PARSER

#include "../include/lexer.h"

typedef struct AstNode AstNode;

typedef enum {
  AST_PROGRAM,
  AST_FUNCTION,
  AST_STATEMENT_RETURN,
  AST_EXPRESSION_CONSTANT,
  AST_EXPRESSION_UNARY
} NodeType;

typedef enum {
  AST_UNARY_COMPLEMENT,
  AST_UNARY_NEGATE
} UnaryOpType;

typedef struct AstNode {
  NodeType type;
  union {
    struct Program { struct AstNode *function; } program;
    struct Function { char* name; struct AstNode *statement; } function;
    struct ReturnStmt { struct AstNode* expression; } return_stmt;
    struct Constant { int value; } constant;
    struct Unary { UnaryOpType op_type; struct AstNode *expression; } unary;
  } data;
} AstNode;

AstNode* parse_ast(Token *tokens, int token_count, char *file);   
void print_ast(AstNode *node, int level);

#endif
