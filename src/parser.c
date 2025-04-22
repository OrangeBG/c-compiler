#include <stdio.h>
#include <stdlib.h>
#include "../include/parser.h"

typedef struct Parser {
  int token_count;
  int current_token_index;
  Token *tokens;
  char* file;
} Parser;
 
AstNode* ast_program(Parser *parser);
AstNode* ast_function(Parser *parser);
AstNode* ast_statement(Parser *parser);
AstNode* ast_expression(Parser *parser);
char* ast_identifier(Parser *parser);
void ast_expect(Parser *parser, TokenType expected_type);
Token* current_token(Parser *parser);
Token* previous_token(Parser *parser);

AstNode* parse(Token *tokens, int token_count, char *file) {  
  Parser parser = {
    .token_count = token_count,
    .current_token_index = 0,
    .tokens = tokens,
    .file = file
  };
  
  AstNode *ret_program = ast_program(&parser);

  if (token_count > parser.current_token_index) {
    fprintf(stderr, "ERROR - Parser: Identifier declared outside of program scope (line %d)", parser.tokens[parser.current_token_index].line);
    exit(1);
  }

  return ret_program;
}

void print_ast(AstNode *node, int level) {
  for (int i = 0; i < level; i++) {
    printf("  ");
  }

  switch(node->type){
    case AST_PROGRAM:  
      printf("Program (\n");
      print_ast(node->ast_program->function, ++level);
      break;
    case AST_EXPRESSION:
      switch (node->ast_expression->type) {
        case EXPRESSION_CONSTANT:
          printf("Constant(%d)\n", node->ast_expression->constant->value->integer);
          return;
          break;
        case EXPRESSION_UNARY:
          //TODO: Add here
          break;
      }
    case AST_FUNCTION:
      printf("Function (name=\"%s\", body =\n", node->ast_function->name);
      print_ast(node->ast_function->statement, ++level);
      break;
    case AST_RETURN:
      printf("Return(\n");
      print_ast(node->ast_return->return_node, ++level);
      break;
  }    

  for (int i = 0; i < level; i++) {
    printf("  ");
  }

  printf(")\n");
}

Token* current_token(Parser *parser) {
  return &parser->tokens[parser->current_token_index];
}

Token* previous_token(Parser *parser) {
  return &parser->tokens[parser->current_token_index - 1];
}

void ast_expect(Parser *parser, TokenType expected_type) {
  if (parser->current_token_index == parser->token_count) {
    fprintf(stderr, "ERROR - Parser: Expected %s (line %d)\n", TokenTypeStr[expected_type], previous_token(parser)->line);
    exit(1);
  }

  if (current_token(parser)->type == expected_type) {
    parser->current_token_index++;
    return;
  } 

  fprintf(stderr, "ERROR - Parser: Expected %s, but found %s (line %d)\n", TokenTypeStr[expected_type],TokenTypeStr[current_token(parser)->type], current_token(parser)->line);
  exit(1);
}

AstNode* ast_program(Parser *parser) {
  AstNode *func = ast_function(parser);

  AstProgram *program = malloc(sizeof(AstProgram));
  program->function = func;

  AstNode *program_node = malloc(sizeof(AstNode));

  program_node->type = AST_PROGRAM;
  program_node->ast_program = program;

  return program_node;
}

AstNode* ast_function(Parser *parser) {
  ast_expect(parser, TOKEN_INT);

  char *id_name = ast_identifier(parser);  

  ast_expect(parser, TOKEN_OPEN_PAREN);
  ast_expect(parser, TOKEN_VOID);
  ast_expect(parser, TOKEN_CLOSE_PAREN);
  ast_expect(parser, TOKEN_OPEN_BRACE);

  AstNode *stmt = ast_statement(parser);
  AstFunction *function = malloc(sizeof(AstFunction));
  
  function->name = id_name;
  function->statement = stmt;

  ast_expect(parser, TOKEN_CLOSE_BRACE);

  AstNode *function_node = malloc(sizeof(AstNode)); 
  function_node->type = AST_FUNCTION;
  function_node->ast_function = function;

  return function_node;
}

char* ast_identifier(Parser *parser) {
  int start = current_token(parser)->start_index;
  int end = current_token(parser)->end_index;

  if (parser->file[start] >= 48 && parser->file[start] <= 57) {
    printf("ERROR - Parser: Identifier cannot start with a number (line %d)\n", current_token(parser)->line);
    exit(1);
  }

  //+2 -> One for the Null operator, one for the index
  char* ret_val = malloc((end - start) + 2);
  int ret_val_idx = 0;

  for (int i = start; i <= end; i++) {
    ret_val[ret_val_idx] = parser->file[i];
    ret_val_idx++;    
  }

  ret_val[ret_val_idx] = '\0';
  
  parser->current_token_index++;

  return ret_val;
}

AstNode* ast_statement(Parser *parser) {
  ast_expect(parser, TOKEN_RETURN);
  AstNode *constant_node = ast_expression(parser);

  AstReturn *return_node = malloc(sizeof(AstReturn));
  return_node->return_node = constant_node;

  AstNode *statement_node = malloc(sizeof(AstNode)); 
  statement_node->type = AST_RETURN;
  statement_node->ast_return = return_node;  
  
  ast_expect(parser, TOKEN_SEMICOLON);

  return statement_node;
}

AstNode* ast_expression(Parser *parser) {
  if (current_token(parser)->type == TOKEN_CONSTANT_INT) {
    ast_expect(parser, TOKEN_CONSTANT_INT); 

    Value *value = malloc(sizeof(Value));

    value->type = INTEGER;
    value->integer = (int)(parser->file[previous_token(parser)->start_index] - 48);   
  
    AstConstant *constant_node = malloc(sizeof(AstConstant));
    constant_node->value = value;

    AstExpression *expression_node = malloc(sizeof(AstExpression));
    expression_node->type = EXPRESSION_CONSTANT;
    expression_node->constant = constant_node;

    AstNode *node = malloc(sizeof(AstNode));
    node->type = AST_EXPRESSION;
    node->ast_expression = expression_node;

    return node;
  }
}
