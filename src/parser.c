#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "../include/parser.h"

typedef struct Parser {
  int token_count;
  int current_token_index;
  Token *tokens;
  char* file;
} Parser;
 
AstNode*   ast_program(Parser *parser);
AstNode*   ast_function(Parser *parser);
AstNode*   ast_statement(Parser *parser);
AstNode*   ast_expression(Parser *parser);
Token*     current_token(Parser *parser);
Token*     previous_token(Parser *parser);
TokenType  peek_next_token(Parser *parser); 
char*      ast_identifier(Parser *parser);
void       ast_expect(Parser *parser, TokenType expected_type);
void       print_whitespace(int count); 
bool       end_of_file(Parser *parser);

AstNode* parse_ast(Token *tokens, int token_count, char *file) {  
  Parser parser = {
    .token_count = token_count,
    .current_token_index = 0,
    .tokens = tokens,
    .file = file
  };
  
  AstNode *ret_program = ast_program(&parser);

  ast_expect(&parser, TOKEN_EOF);

  if (token_count > parser.current_token_index) {
    fprintf(stderr, "ERROR - Parser: Identifier declared outside of program scope (line %d)\n", parser.tokens[parser.current_token_index].line);
    exit(1);
  }

  return ret_program;
}

void print_ast(AstNode *node, int whitespace) {

  switch(node->type){
    case AST_PROGRAM:  
      printf("Program (\n");
      print_ast(node->data.program.function, ++whitespace);
      printf(")\n");
      break;
    case AST_FUNCTION:
      print_whitespace(whitespace);
      printf("Function (name=\"%s\", body =\n", node->data.function.name);
      print_ast(node->data.function.statement, ++whitespace);
      printf("\n");
      print_whitespace(whitespace);
      printf(")\n)");      
      break;
    case AST_STATEMENT_RETURN:
      print_whitespace(whitespace);
      printf("Return(\n");
      print_ast(node->data.return_stmt.expression, ++whitespace);
      printf("\n");
      print_whitespace(whitespace);
      printf(")");
      break;
    case AST_EXPRESSION_CONSTANT:
      print_whitespace(whitespace);
      printf("Constant(%d)", node->data.constant.value);
      break;
    case AST_EXPRESSION_UNARY:
      print_whitespace(whitespace);
      printf("Unary(");
      if (node->data.unary.op_type == AST_UNARY_COMPLEMENT) {
        printf("Complement(\n");
      } else {
        printf("Negate(\n");
      }
      print_ast(node->data.unary.expression, ++whitespace);
      printf("))");
      break;
  }    
}

void print_whitespace(int count) {
  for (int i = 0; i < count;i++) {
    printf(" ");
  }
}

Token* current_token(Parser *parser) {
  return &parser->tokens[parser->current_token_index];
}

Token* previous_token(Parser *parser) {
  return &parser->tokens[parser->current_token_index - 1];
}

bool end_of_file(Parser *parser) {
  return parser->tokens[parser->current_token_index].type == TOKEN_EOF;
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
  AstNode *program = malloc(sizeof(AstNode));
  AstNode *function = ast_function(parser);

  program->type = AST_PROGRAM;
  program->data.program.function = function;
  
  return program;
}

AstNode* ast_function(Parser *parser) {
  ast_expect(parser, TOKEN_INT);

  char *id_name = ast_identifier(parser);  

  ast_expect(parser, TOKEN_OPEN_PAREN);
  ast_expect(parser, TOKEN_VOID);
  ast_expect(parser, TOKEN_CLOSE_PAREN);
  ast_expect(parser, TOKEN_OPEN_BRACE);

  AstNode *stmt = ast_statement(parser);
  AstNode *function = malloc(sizeof(AstNode));
  
  function->type = AST_FUNCTION;
  function->data.function.name = id_name;
  function->data.function.statement = stmt;

  ast_expect(parser, TOKEN_CLOSE_BRACE);

  return function;
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
  if (end_of_file(parser)) {
    fprintf(stderr, "ERROR - Parser: Incomplete statement (line %d)\n", previous_token(parser)->line);
    exit(1);
  }

  ast_expect(parser, TOKEN_RETURN);
  
  AstNode *expression = ast_expression(parser);
  AstNode *return_node = malloc(sizeof(AstNode));
    
  return_node->type = AST_STATEMENT_RETURN;
  return_node->data.return_stmt.expression = expression;

  ast_expect(parser, TOKEN_SEMICOLON);

  return return_node;
}

//TODO: Function is hard to read
AstNode* ast_expression(Parser *parser) {
  if (end_of_file(parser)) {
    fprintf(stderr, "ERROR - Parser: Incomplete expression (line %d)\n", previous_token(parser)->line);
    exit(1);
  }
  
  if (current_token(parser)->type == TOKEN_CONSTANT_INT) {
    ast_expect(parser, TOKEN_CONSTANT_INT); 

    AstNode *constant = malloc(sizeof(AstNode));
    constant->type = AST_EXPRESSION_CONSTANT;
    //TODO: Only supports up to '9'
    constant->data.constant.value = (int)(parser->file[previous_token(parser)->start_index] - 48);   

    return constant;
  } else if (current_token(parser)->type == TOKEN_NEGATION || current_token(parser)->type == TOKEN_BITWISE_NOT) {
    UnaryOpType op_type = current_token(parser)->type == TOKEN_NEGATION ? AST_UNARY_NEGATE : AST_UNARY_COMPLEMENT;    
    parser->current_token_index++;

    AstNode *unary_value_expression = ast_expression(parser);

    AstNode *unary = malloc(sizeof(AstNode));    
    unary->type = AST_EXPRESSION_UNARY;
    unary->data.unary.op_type = op_type;
    unary->data.unary.expression = unary_value_expression;

    return unary;
  } else if (current_token(parser)->type == TOKEN_OPEN_PAREN) {
    parser->current_token_index++;

    AstNode *expression = ast_expression(parser);
        
    ast_expect(parser, TOKEN_CLOSE_PAREN);

    return expression;
  }    

  fprintf(stderr, "ERROR - Parser: Failed to parse expression for '%s' token (line %d)\n", TokenTypeStr[current_token(parser)->type], current_token(parser)->line);
  exit(1);
}
