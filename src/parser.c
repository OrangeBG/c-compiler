#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <sys/types.h>
#include "../include/parser.h"

#define BLOCK_STARTING_ALLOCATION 8

typedef struct Parser {
  int token_count;
  int current_token_index;
  Token *tokens;
  char* file;
} Parser;
 
AstNode*   ast_program(Parser *parser);
AstNode*   ast_function(Parser *parser);
void       ast_statement(Parser *parser, AstNode *function);
void       ast_declaration(Parser *parser, AstNode *function);
AstNode*   ast_expression(Parser *parser, int min_precedence);
AstNode*   ast_factor(Parser *parser);
Token*     current_token(Parser *parser);
Token*     previous_token(Parser *parser);
TokenType  peek_next_token(Parser *parser); 
char*      ast_identifier(Parser *parser);
void       ast_expect(Parser *parser, TokenType expected_type);
void       print_whitespace(int count); 
void       add_to_function_block(AstNode *function, AstNode *expr_or_stmt);
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

      for (int i = 0; i < node->data.function.block_count; i++) {
        print_ast(&node->data.function.blocks[i], ++whitespace);
        printf("\n");
      }   

      print_whitespace(whitespace);
      printf(")\n)");      
      break;
    case AST_DECLARATION:
      print_whitespace(whitespace);
      printf("Declaration(%s,\n", node->data.declaration.identifier);

      if (node->data.declaration.has_expression) {
        print_ast(node->data.declaration.expression, ++whitespace);
      }

      print_whitespace(whitespace);
      printf(")\n");
      
      break;
    case AST_STATEMENT_RETURN:
      print_whitespace(whitespace);
      printf("Return(\n");
      print_ast(node->data.return_statement.expression, ++whitespace);
      printf("\n");
      print_whitespace(whitespace);
      printf(")");
      break;
    case AST_STATEMENT_NULL:
      print_whitespace(whitespace);
      printf("Null()\n");
      break;
    case AST_STATEMENT_EXPRESSION:
      break;
    case AST_EXPRESSION_CONSTANT:
      print_whitespace(whitespace);
      printf("Constant(%d)", node->data.constant_expression.value);
      break;
    case AST_EXPRESSION_UNARY:
      print_whitespace(whitespace);
      printf("Unary(");
      if (node->data.unary_expression.op_type == AST_UNARY_COMPLEMENT) {
        printf("Complement(\n");
      } else {
        printf("Negate(\n");
      }
      print_ast(node->data.unary_expression.expression, ++whitespace);
      printf("))");
      break;
    case AST_EXPRESSION_BINARY:
      print_whitespace(whitespace);
      printf("Binary(\n");
      print_ast(node->data.binary_expression.left_expression, ++whitespace);
  
      switch (node->data.binary_expression.op_type) {
        case AST_BINARY_ADD:                  printf(" + "); break;
        case AST_BINARY_SUBTRACT:             printf(" - "); break;
        case AST_BINARY_DIVIDE:               printf(" / "); break;
        case AST_BINARY_MULTIPLY:             printf(" * "); break;
        case AST_BINARY_REMAINDER:            printf(" %% "); break;
        case AST_BINARY_BITWISE_AND:          printf(" & "); break; 
        case AST_BINARY_BITWISE_OR:           printf(" | "); break; 
        case AST_BINARY_BITWISE_XOR:          printf(" ^ "); break; 
        case AST_BINARY_BITWISE_LEFT_SHIFT:   printf(" << "); break;
        case AST_BINARY_BITWISE_RIGHT_SHIFT:  printf(" >> "); break;
        case AST_BINARY_AND:                  printf(" && "); break;
        case AST_BINARY_OR:                   printf(" || "); break;
        case AST_BINARY_GREATER_THAN:         printf(" > "); break;
        case AST_BINARY_GREATER_OR_EQUAL:     printf(" >= "); break;
        case AST_BINARY_LESS_THAN:            printf(" < "); break;
        case AST_BINARY_LESS_OR_EQUAL:        printf(" <= "); break;
        case AST_BINARY_EQUAL:                printf(" == "); break;
        case AST_BINARY_NOT_EQUAL:            printf(" != "); break;
      }
    
      print_ast(node->data.binary_expression.right_expression, 0);
      printf(")");
      break;
      case AST_EXPRESSION_VARIABLE:
        print_whitespace(whitespace);
        printf("Variable(%s)", node->data.variable_expression.identifier);
        break;
      case AST_EXPRESSION_ASSIGNMENT:
        print_whitespace(whitespace);
        printf("Assignment(Left(");
        print_ast(node->data.assignement_expression.left_expression, 0);
        printf("), Right(");
        print_ast(node->data.assignement_expression.right_expression, 0);
        printf(")\n");
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

void add_to_function_block(AstNode *function, AstNode *expr_or_stmt) {
  int current_count = function->data.function.block_count;
  int current_capacity = function->data.function.block_capacity;

  if (current_count == current_capacity) {
    int new_size = current_capacity == 0 ? BLOCK_STARTING_ALLOCATION : current_capacity * 2;

    AstNode *blocks = realloc(function->data.function.blocks, new_size * sizeof(AstNode));

    function->data.function.block_capacity = new_size;
    function->data.function.blocks = blocks;
  } 

  function->data.function.blocks[function->data.function.block_count] = *expr_or_stmt;
  function->data.function.block_count++;
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


  AstNode *function = malloc(sizeof(AstNode)); 
  function->type = AST_FUNCTION;
  function->data.function.name = id_name;
  function->data.function.block_count = 0;
  function->data.function.block_capacity = 0;
  function->data.function.blocks = NULL;

  while(true) {
    if (current_token(parser)->type == TOKEN_CLOSE_BRACE) {
      ast_expect(parser, TOKEN_CLOSE_BRACE);
      return function;
    }

    if (current_token(parser)->type == TOKEN_INT) {
      ast_declaration(parser, function);
    } else {
      ast_statement(parser, function);
    }
  }
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

void ast_declaration(Parser *parser, AstNode *function) {
  ast_expect(parser, TOKEN_INT);

  char *identifier = ast_identifier(parser);
  // ast_expect(parser, TOKEN_IDENTIFIER);

  AstNode *declaration = malloc(sizeof(AstNode));
  declaration->type = AST_DECLARATION;
  declaration->data.declaration.identifier = identifier;

  if (current_token(parser)->type == TOKEN_EQUAL) {
    //TODO: Fix as ast_identifier eats the token but we need it to feed into ast_expression();
    parser->current_token_index--;
    AstNode *expression = ast_expression(parser, 0);

    declaration->data.declaration.has_expression = true;
    declaration->data.declaration.expression = expression;
  }

  ast_expect(parser, TOKEN_SEMICOLON);
  add_to_function_block(function, declaration);
}

void ast_statement(Parser *parser, AstNode *function) { 
  //TODO: May need to wrap each statement type into a parent Statement node
  // AstNode *statement = malloc(sizeof(AstNode));
  // statement->type = AST_STATEMENT

  if (end_of_file(parser)) {
    fprintf(stderr, "ERROR - Parser: Incomplete statement (line %d)\n", previous_token(parser)->line);
    exit(1);
  }

  if (current_token(parser)->type == TOKEN_SEMICOLON) {
    ast_expect(parser, TOKEN_SEMICOLON);
    return;
  }

  if (current_token(parser)->type == TOKEN_RETURN) {
    ast_expect(parser, TOKEN_RETURN);
  
    AstNode *expression = ast_expression(parser, 0);
    AstNode *return_node = malloc(sizeof(AstNode));
    
    return_node->type = AST_STATEMENT_RETURN;
    return_node->data.return_statement.expression = expression;

    ast_expect(parser, TOKEN_SEMICOLON);
    add_to_function_block(function, return_node);
    return;
  }

  AstNode *expression = ast_expression(parser, 0);
  
  add_to_function_block(function, expression);
  
}

AstNode* ast_expression(Parser *parser, int min_precedence) {
  AstNode *left = ast_factor(parser);

  TokenType next_token = current_token(parser)->type;

  //TODO: May be easier to check outliers rather than what is being done here
  while ((next_token == TOKEN_PLUS || next_token == TOKEN_NEGATION || next_token == TOKEN_PERCENT || next_token == TOKEN_ASTERISK || next_token == TOKEN_FORWARD_SLASH || next_token == TOKEN_BITWISE_AND || next_token == TOKEN_BITWISE_XOR || next_token == TOKEN_BITWISE_OR || next_token == TOKEN_BITWISE_LEFT_SHIFT || next_token == TOKEN_BITWISE_RIGHT_SHIFT || next_token == TOKEN_RELATIONAL_LESS_THAN || next_token == TOKEN_RELATIONAL_LESS_OR_EQUAL || next_token == TOKEN_RELATIONAL_GREATER_THAN || next_token == TOKEN_RELATIONAL_GREATER_OR_EQUAL || next_token == TOKEN_RELATIONAL_EQUAL || next_token == TOKEN_RELATIONAL_NOT_EQUAL || next_token == TOKEN_LOGICAL_AND || next_token == TOKEN_LOGICAL_OR || next_token == TOKEN_EQUAL || next_token == TOKEN_PLUS_EQUAL || next_token == TOKEN_NEGATION_EQUAL || next_token == TOKEN_ASTERISK_EQUAL || next_token == TOKEN_FORWARD_SLASH_EQUAL || next_token == TOKEN_PERCENT_EQUAL || next_token == TOKEN_BITWISE_AND_EQUAL || next_token == TOKEN_BITWISE_OR_EQUAL || next_token == TOKEN_BITWISE_XOR_EQUAL || next_token == TOKEN_BITWISE_LEFT_SHIFT_EQUAL || next_token == TOKEN_BITWISE_RIGHT_SHIFT_EQUAL) && get_precedence(next_token) >= min_precedence) {

    if (next_token == TOKEN_EQUAL) {
      //right-associative assignment      
      parser-> current_token_index++;

      AstNode *right = ast_expression(parser, get_precedence(next_token) + 1);
      AstNode *assignment_expression= malloc(sizeof(AstNode));

      assignment_expression->type = AST_EXPRESSION_ASSIGNMENT;
      assignment_expression->data.assignement_expression.left_expression = left;
      assignment_expression->data.assignement_expression.right_expression = right;

      left = assignment_expression;

      return left;
    } 

    if (next_token == TOKEN_PLUS_EQUAL || next_token == TOKEN_NEGATION_EQUAL || next_token == TOKEN_ASTERISK_EQUAL || next_token == TOKEN_FORWARD_SLASH_EQUAL || next_token  == TOKEN_PERCENT_EQUAL || next_token == TOKEN_BITWISE_AND_EQUAL || next_token == TOKEN_BITWISE_OR_EQUAL || next_token == TOKEN_BITWISE_XOR_EQUAL || next_token == TOKEN_BITWISE_LEFT_SHIFT_EQUAL || next_token == TOKEN_BITWISE_RIGHT_SHIFT_EQUAL) {

      parser->current_token_index++;

      AstNode *right = ast_expression(parser, get_precedence(next_token) + 1);

      AstNode *binary = malloc(sizeof(AstNode));
      binary->type = AST_EXPRESSION_BINARY;

      switch(next_token) {
        case TOKEN_PLUS_EQUAL:          binary->data.binary_expression.op_type = AST_BINARY_ADD; break;
        case TOKEN_NEGATION_EQUAL:      binary->data.binary_expression.op_type = AST_BINARY_SUBTRACT; break;
        case TOKEN_ASTERISK_EQUAL:      binary->data.binary_expression.op_type = AST_BINARY_MULTIPLY; break;
        case TOKEN_FORWARD_SLASH_EQUAL: binary->data.binary_expression.op_type = AST_BINARY_DIVIDE; break;
        case TOKEN_PERCENT_EQUAL:       binary->data.binary_expression.op_type = AST_BINARY_REMAINDER; break;
        case TOKEN_BITWISE_AND_EQUAL:   binary->data.binary_expression.op_type = AST_BINARY_BITWISE_AND; break;
        case TOKEN_BITWISE_OR_EQUAL:   binary->data.binary_expression.op_type = AST_BINARY_BITWISE_OR; break;
        case TOKEN_BITWISE_XOR_EQUAL:   binary->data.binary_expression.op_type = AST_BINARY_BITWISE_XOR; break;
        case TOKEN_BITWISE_RIGHT_SHIFT_EQUAL:   binary->data.binary_expression.op_type = AST_BINARY_BITWISE_RIGHT_SHIFT; break;
        case TOKEN_BITWISE_LEFT_SHIFT_EQUAL:   binary->data.binary_expression.op_type = AST_BINARY_BITWISE_LEFT_SHIFT; break;
        default:
          fprintf(stderr, "ERROR - Parser: Compound assignment type not found '%d'\n", next_token);
          exit(1);
          break;
      }
      
      binary->data.binary_expression.left_expression = left;
      binary->data.binary_expression.right_expression = right;
      
      AstNode *assignment_expression= malloc(sizeof(AstNode));

      assignment_expression->type = AST_EXPRESSION_ASSIGNMENT;
      assignment_expression->data.assignement_expression.left_expression = left;
      assignment_expression->data.assignement_expression.right_expression = binary;

      left = assignment_expression;

      return left;
    }

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
    } else if (next_token == TOKEN_BITWISE_XOR) {
      binary_expression->data.binary_expression.op_type = AST_BINARY_BITWISE_XOR;
    } else if (next_token == TOKEN_BITWISE_LEFT_SHIFT) {
      binary_expression->data.binary_expression.op_type = AST_BINARY_BITWISE_LEFT_SHIFT;
    } else if (next_token == TOKEN_BITWISE_RIGHT_SHIFT) {
      binary_expression->data.binary_expression.op_type = AST_BINARY_BITWISE_RIGHT_SHIFT;
    } else if (next_token == TOKEN_RELATIONAL_LESS_THAN) {
      binary_expression->data.binary_expression.op_type = AST_BINARY_LESS_THAN;
    } else if (next_token == TOKEN_RELATIONAL_LESS_OR_EQUAL) {
      binary_expression->data.binary_expression.op_type = AST_BINARY_LESS_OR_EQUAL;
    } else if (next_token == TOKEN_RELATIONAL_GREATER_THAN) {
      binary_expression->data.binary_expression.op_type = AST_BINARY_GREATER_THAN;
    } else if (next_token == TOKEN_RELATIONAL_GREATER_OR_EQUAL) {
      binary_expression->data.binary_expression.op_type = AST_BINARY_GREATER_OR_EQUAL;
    } else if (next_token == TOKEN_RELATIONAL_EQUAL) {
      binary_expression->data.binary_expression.op_type = AST_BINARY_EQUAL;
    } else if (next_token == TOKEN_RELATIONAL_NOT_EQUAL) {
      binary_expression->data.binary_expression.op_type = AST_BINARY_NOT_EQUAL;
    } else if (next_token == TOKEN_LOGICAL_AND) {
      binary_expression->data.binary_expression.op_type = AST_BINARY_AND;
    } else if (next_token == TOKEN_LOGICAL_OR) {
      binary_expression->data.binary_expression.op_type = AST_BINARY_OR;
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
    constant->type = AST_EXPRESSION_CONSTANT;
    //TODO: Only supports up to '9'
    constant->data.constant_expression.value = (int)(parser->file[previous_token(parser)->start_index] - 48);   

    return constant;
  } else if (current_token(parser)->type == TOKEN_NEGATION || current_token(parser)->type == TOKEN_BITWISE_NOT || current_token(parser)->type == TOKEN_LOGICAL_NOT) {

    UnaryOpType op_type; 
    switch(current_token(parser)->type) {
      case TOKEN_NEGATION:
        op_type = AST_UNARY_NEGATE;
        break;
      case TOKEN_BITWISE_NOT:
        op_type = AST_UNARY_COMPLEMENT;
        break;
      case TOKEN_LOGICAL_NOT:
        op_type = AST_UNARY_NOT;
        break;
      default:
        fprintf(stderr, "ERROR - Parser: Unary token type not found for ast_factor()");
        exit(1);
        break;
    }
    
    parser->current_token_index++;

    AstNode *unary_value_expression = ast_factor(parser);

    AstNode *unary = malloc(sizeof(AstNode));    
    unary->type = AST_EXPRESSION_UNARY;
    unary->data.unary_expression.op_type = op_type;
    unary->data.unary_expression.expression = unary_value_expression;

    return unary;
  } else if (current_token(parser)->type == TOKEN_OPEN_PAREN) {
    parser->current_token_index++;

    AstNode *expression = ast_expression(parser, 0);
        
    ast_expect(parser, TOKEN_CLOSE_PAREN);

    return expression;
  } else if (current_token(parser)->type == TOKEN_IDENTIFIER) {
    AstNode *identifier_node = malloc(sizeof(AstNode));
    identifier_node->type = AST_EXPRESSION_VARIABLE;
    identifier_node->data.variable_expression.identifier = ast_identifier(parser);

    return identifier_node;
  }

  fprintf(stderr, "ERROR - Parser: Failed to parse factor for '%s' token (line %d)\n", TokenTypeStr[current_token(parser)->type], current_token(parser)->line);
  exit(1);
}

int get_precedence(TokenType token_type) {
  switch (token_type) {
    case TOKEN_INCREMENT:
    case TOKEN_DECREMENT:
      return 14;
    case TOKEN_ASTERISK:
    case TOKEN_FORWARD_SLASH:
    case TOKEN_PERCENT:
      return 13;
    case TOKEN_PLUS:
    case TOKEN_NEGATION:
      return 12;
    case TOKEN_BITWISE_LEFT_SHIFT:
    case TOKEN_BITWISE_RIGHT_SHIFT:
      return 11;
    case TOKEN_RELATIONAL_GREATER_THAN:
    case TOKEN_RELATIONAL_GREATER_OR_EQUAL:
    case TOKEN_RELATIONAL_LESS_THAN:
    case TOKEN_RELATIONAL_LESS_OR_EQUAL:
      return 10;
    case TOKEN_RELATIONAL_EQUAL:
    case TOKEN_RELATIONAL_NOT_EQUAL:
      return 9;
    case TOKEN_BITWISE_AND:
      return 8;
    case TOKEN_BITWISE_XOR:
      return 7;
    case TOKEN_BITWISE_OR:
      return 6;
    case TOKEN_LOGICAL_AND:
      return 5;
    case TOKEN_LOGICAL_OR:
      return 4;
    case TOKEN_EQUAL:
    case TOKEN_PLUS_EQUAL:
    case TOKEN_NEGATION_EQUAL:
    case TOKEN_ASTERISK_EQUAL:
    case TOKEN_FORWARD_SLASH_EQUAL:
    case TOKEN_PERCENT_EQUAL:
    case TOKEN_BITWISE_AND_EQUAL:
    case TOKEN_BITWISE_XOR_EQUAL:
    case TOKEN_BITWISE_OR_EQUAL:
    case TOKEN_BITWISE_LEFT_SHIFT_EQUAL:
    case TOKEN_BITWISE_RIGHT_SHIFT_EQUAL:
      return 2;
    default: {
      fprintf(stderr, "ERROR - Parser: Token '%s 'does not have a supported operator precendence", TokenTypeStr[token_type]);
      exit(1);
    }
  }
}

