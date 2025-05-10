#include <stdint.h>
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
AstNode*   ast_expression(Parser *parser, int min_precedence);
AstNode*   ast_factor(Parser *parser);
Token*     current_token(Parser *parser);
Token*     previous_token(Parser *parser);
TokenType  peek_next_token(Parser *parser); 
char*      ast_identifier(Parser *parser);
void       ast_expect(Parser *parser, TokenType expected_type);
void       print_whitespace(int count); 
bool       end_of_file(Parser *parser);
bool       is_binary_operator_token(Parser *parser);
int        get_precedence(TokenType token_type);

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
      print_ast(node->data.return_statement.expression, ++whitespace);
      printf("\n");
      print_whitespace(whitespace);
      printf(")");
      break;
    case AST_FACTOR_CONSTANT:
      print_whitespace(whitespace);
      printf("Constant(%d)", node->data.constant_factor.value);
      break;
    case AST_FACTOR_UNARY:
      print_whitespace(whitespace);
      printf("Unary(");
      if (node->data.unary_factor.op_type == AST_UNARY_COMPLEMENT) {
        printf("Complement(\n");
      } else {
        printf("Negate(\n");
      }
      print_ast(node->data.unary_factor.factor, ++whitespace);
      printf("))");
      break;
    case AST_EXPRESSION_BINARY:
      print_whitespace(whitespace);
      printf("Binary(\n");
      print_ast(node->data.binary_expression.left_expression, ++whitespace);
  
      switch (node->data.binary_expression.op_type) {
        case AST_BINARY_ADD:
          printf(" + ");
          break;
        case AST_BINARY_SUBTRACT:
          printf(" - ");
          break;
        case AST_BINARY_DIVIDE:
          printf(" / ");
          break;
        case AST_BINARY_MULTIPLY:
          printf(" * ");
          break;
        case AST_BINARY_REMAINDER:
          printf(" %% ");
          break;
        case AST_BINARY_BITWISE_AND:
          printf(" & ");
          break; 
        case AST_BINARY_BITWISE_OR:
          printf(" | ");
          break; 
        case AST_BINARY_BITWISE_XOR:
          printf(" ^ ");
          break; 
      }
    
      print_ast(node->data.binary_expression.right_expression, 0);
      printf(")");
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

TokenType peek_next_token(Parser *parser) {
  if (current_token(parser)->type == TOKEN_EOF) {
    return TOKEN_EOF;
  }

  return parser->tokens[parser->current_token_index + 1].type;
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
  
  AstNode *expression = ast_expression(parser, 0);
  AstNode *return_node = malloc(sizeof(AstNode));
    
  return_node->type = AST_STATEMENT_RETURN;
  return_node->data.return_statement.expression = expression;

  ast_expect(parser, TOKEN_SEMICOLON);

  return return_node;
}

AstNode* ast_expression(Parser *parser, int min_precedence) {
  AstNode *left = ast_factor(parser);

  TokenType next_token = current_token(parser)->type;
  while ((next_token == TOKEN_PLUS || next_token == TOKEN_NEGATION || next_token == TOKEN_PERCENT || next_token == TOKEN_ASTERISK || next_token == TOKEN_FORWARD_SLASH || next_token == TOKEN_BITWISE_AND || next_token == TOKEN_BITWISE_XOR || next_token == TOKEN_BITWISE_OR) && get_precedence(next_token) >= min_precedence) {
    parser-> current_token_index++;

    AstNode *right = ast_expression(parser, get_precedence(next_token) + 1);

    AstNode *binary_expression = malloc(sizeof(AstNode));
    binary_expression->type = AST_EXPRESSION_BINARY;

    binary_expression->data.binary_expression.left_expression = left;
    binary_expression->data.binary_expression.right_expression = right;

    if (next_token == TOKEN_PLUS) {
      binary_expression->data.binary_expression.op_type = AST_BINARY_ADD;
    } else if (next_token == TOKEN_NEGATION) {
      binary_expression->data.binary_expression.op_type = AST_BINARY_SUBTRACT;
    } else if (next_token == TOKEN_ASTERISK) {
      binary_expression->data.binary_expression.op_type = AST_BINARY_MULTIPLY;
    } else if (next_token == TOKEN_FORWARD_SLASH) {
      binary_expression->data.binary_expression.op_type = AST_BINARY_DIVIDE;
    } else if (next_token == TOKEN_PERCENT) {
      binary_expression->data.binary_expression.op_type = AST_BINARY_REMAINDER;
    } else if (next_token == TOKEN_BITWISE_AND) {
      binary_expression->data.binary_expression.op_type = AST_BINARY_BITWISE_AND;
    } else if (next_token == TOKEN_BITWISE_OR) {
      binary_expression->data.binary_expression.op_type = AST_BINARY_BITWISE_OR;
    } else {
      binary_expression->data.binary_expression.op_type = AST_BINARY_BITWISE_XOR;
    }

    left = binary_expression;
    next_token = current_token(parser)->type;
  } 

  return left;
}

//TODO: Function is hard to read
AstNode* ast_factor(Parser *parser) {
 if (end_of_file(parser)) {
    fprintf(stderr, "ERROR - Parser: Incomplete expression (line %d)\n", previous_token(parser)->line);
    exit(1);
  }
  
  if (current_token(parser)->type == TOKEN_CONSTANT_INT) {
    ast_expect(parser, TOKEN_CONSTANT_INT); 

    AstNode *constant = malloc(sizeof(AstNode));
    constant->type = AST_FACTOR_CONSTANT;
    //TODO: Only supports up to '9'
    constant->data.constant_factor.value = (int)(parser->file[previous_token(parser)->start_index] - 48);   

    return constant;
  } else if (current_token(parser)->type == TOKEN_NEGATION || current_token(parser)->type == TOKEN_BITWISE_NOT) {
    UnaryOpType op_type = current_token(parser)->type == TOKEN_NEGATION ? AST_UNARY_NEGATE : AST_UNARY_COMPLEMENT;    
    parser->current_token_index++;

    AstNode *unary_value_expression = ast_factor(parser);

    AstNode *unary = malloc(sizeof(AstNode));    
    unary->type = AST_FACTOR_UNARY;
    unary->data.unary_factor.op_type = op_type;
    unary->data.unary_factor.factor = unary_value_expression;

    return unary;
  } else if (current_token(parser)->type == TOKEN_OPEN_PAREN) {
    parser->current_token_index++;

    AstNode *expression = ast_expression(parser, 0);
        
    ast_expect(parser, TOKEN_CLOSE_PAREN);

    return expression;
  }    

  fprintf(stderr, "ERROR - Parser: Failed to parse factor for '%s' token (line %d)\n", TokenTypeStr[current_token(parser)->type], current_token(parser)->line);
  exit(1);
}

int get_precedence(TokenType token_type) {
  switch (token_type) {
    case TOKEN_ASTERISK:
    case TOKEN_FORWARD_SLASH:
    case TOKEN_PERCENT:
      return 50;
      break;
    case TOKEN_PLUS:
    case TOKEN_NEGATION:
      return 45;
      break;
    case TOKEN_BITWISE_AND:
      return 35;
      break;
    case TOKEN_BITWISE_XOR:
      return 30;
      break;
    case TOKEN_BITWISE_OR:
      return 25;
      break;      
    default: {
      // return 0;
      fprintf(stderr, "ERROR - Parser: Token '%s 'does not have a supported operator precendence", TokenTypeStr[token_type]);
      exit(1);
    }
  }
}

