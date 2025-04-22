#ifndef PARSER
#define PARSER

#include "../include/lexer.h"

typedef struct AstNode AstNode;
typedef struct AstExpression AstExpression; 

typedef enum {
  INTEGER
} ValueType;

typedef struct Value {
  ValueType type;
  union {
    int integer;
  };
} Value;

typedef enum {
  AST_EXPRESSION,
  AST_RETURN,
  AST_FUNCTION,
  AST_PROGRAM
} AstNodeType;

typedef struct AstProgram {
  struct AstNode *function;
} AstProgram;

typedef struct AstFunction {
  char *name;
  struct AstNode *statement;  
} AstFunction;

typedef struct AstReturn {
  struct AstNode *return_node;
} AstReturn;

typedef enum ExpressionType {
  EXPRESSION_CONSTANT,
  EXPRESSION_UNARY
} ExpressionType;

typedef struct AstConstant {
  Value *value;
} AstConstant;

typedef enum UnaryOperatorType {
  UNARY_OP_COMPLEMENT,
  UNARY_OP_NEGATE
} UnaryOperatorType;

typedef struct AstUnary {
  UnaryOperatorType type;  
  AstExpression *expression;
} AstUnary;

typedef struct AstExpression {
  ExpressionType type;
  union {
    AstConstant *constant;
    AstUnary *unary;
  };
} AstExpression;

typedef struct AstNode {
  AstNodeType type;
  union {
    AstExpression *ast_expression;
    AstReturn *ast_return;
    AstFunction *ast_function;
    AstProgram *ast_program;
  };
} AstNode;

AstNode* parse(Token *tokens, int token_count, char *file);
void print_ast(AstNode *node, int level); 

#endif
