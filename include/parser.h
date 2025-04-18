#ifndef PARSER
#define PARSER

#include "../include/lexer.h"

//TODO: AstNode is a circular reference to other Ast types. See if there are better ways to structure this.
typedef struct AstNode Node;

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
  AST_CONSTANT,
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

typedef struct AstConstant {
  Value *value;
} AstConstant;

typedef struct AstReturn {
  struct AstNode *return_node;
} AstReturn;

typedef struct AstNode {
  AstNodeType type;
  union {
    AstConstant *ast_constant;
    AstReturn *ast_return;
    AstFunction *ast_function;
    AstProgram *ast_program;
  };
} AstNode;

AstNode* parse(Token *token, char *file);
void print_ast(AstNode *node, int level); 

#endif
