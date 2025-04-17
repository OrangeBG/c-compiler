#include <stdio.h>
#include <stdlib.h>
#include "../include/parser.h"

typedef struct Parser {
  int current_token_index;
  Token *tokens;
  char* file;
} Parser;
 
AstNode program(Parser *parser);
AstNode function(Parser *parser);
char* identifier(Parser *parser);
AstNode statement(Parser *parser);
AstNode expression(Parser *parser);
void expect(Parser *parser, TokenType expected_type);

void parse(Token *tokens, char *file) {  
  Parser parser = {
    .current_token_index = 0,
    .tokens = tokens,
    .file = file
  };
  
  AstNode ret_prgram = program(&parser);
  printf("successfully parsed!\n");
}

void expect(Parser *parser, TokenType expected_type) {
  if (parser->tokens[parser->current_token_index].type == expected_type) {
    parser->current_token_index++;
    return;
  } 

  printf("ERROR - Parser: Expected %s, but got %s", TokenTypeStr[parser->tokens->type], TokenTypeStr[expected_type]);
  exit(1);
}

AstNode program(Parser *parser) {
  AstNode func = function(parser);

  AstProgram program = {
    .function = &func
  };

  AstNode return_node = {
    .type = AST_PROGRAM,
    .ast_program = program
  };

  return return_node;
}

AstNode function(Parser *parser) {
  expect(parser, TOKEN_INT);

  char *id_name = identifier(parser);  

  expect(parser, TOKEN_OPEN_PAREN);
  expect(parser, TOKEN_VOID);
  expect(parser, TOKEN_CLOSE_PAREN);
  expect(parser, TOKEN_OPEN_BRACE);

  AstNode stmt = statement(parser);  

  AstFunction function ={
    .name = id_name,
    .statement = &stmt
  };

  expect(parser, TOKEN_CLOSE_BRACE);

  AstNode return_node = {
    .type = AST_FUNCTION,
    .ast_function = function
  };

  return return_node;
}

char* identifier(Parser *parser) {
  int start = parser->tokens[parser->current_token_index].start_index;
  int end = parser->tokens[parser->current_token_index].end_index;

  //+2 -> One for the Null operator, one for the index
  char* ret_val = malloc((end - start) + 2);
  int ret_val_idx = 0;

  for (int i = start; i <= end; i++) {
    ret_val[ret_val_idx] = parser->file[i];
    ret_val_idx++;    
  }

  ret_val[ret_val_idx] = '\0';
  
  parser->current_token_index++;

  printf("%s\n", ret_val);
  return ret_val;
}

AstNode statement(Parser *parser) {
  expect(parser, TOKEN_RETURN);
  AstNode constant_node = expression(parser);

  AstNode return_node = {
    .type = AST_RETURN,
    .ast_return = &constant_node
  };
  
  expect(parser, TOKEN_SEMICOLON);

  return return_node;
}

AstNode expression(Parser *parser) {
  expect(parser, TOKEN_CONSTANT_INT); 

  Value *value = malloc(sizeof(Value));

  value->type = INTEGER;
  //TODO: '48' converts the ascii value to the int. Look into doing that conversion in the lexer
  value->integer = (int)(parser->file[parser->tokens[parser->current_token_index - 1].start_index] - 48);   

  printf("Expression: %d\n", value->integer);
  
  //TODO: Need to check if we need to malloc the ast's
  AstConstant constantNode = {
    .value = value
  };

  AstNode node = {
    .type = AST_CONSTANT,
    .ast_constant = constantNode
  };

  return node;
}
