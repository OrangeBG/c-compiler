#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <sys/types.h>
#include "../include/parser.h"
#include "../include/stack.h"

#define BLOCK_STARTING_ALLOCATION 8

typedef struct Parser {
  int token_count;
  int current_token_index;
  Token *tokens;
  char* file;
  int current_loop_label_id;
  Stack loop_label_stack;
} Parser;
 
AstNode*   ast_program(Parser *parser);
AstNode*   ast_function(Parser *parser);
AstNode*   ast_block(Parser *parser);
AstNode*   ast_statement(Parser *parser);
void       ast_declaration(Parser *parser, AstNode *function);
AstNode*   ast_expression(Parser *parser, int min_precedence);
AstNode*   ast_factor(Parser *parser);
Token*     current_token(Parser *parser);
Token*     previous_token(Parser *parser);
TokenType  peek_next_token(Parser *parser); 
char*      ast_identifier(Parser *parser);
void       ast_expect(Parser *parser, TokenType expected_type);
void       print_whitespace(int count); 
void       add_to_block(AstNode *function, AstNode *expr_or_stmt);
bool       end_of_file(Parser *parser);
bool       is_binary_operator_token(Parser *parser);
int        get_precedence(TokenType token_type);

AstNode* parse_ast(Token *tokens, int token_count, char *file) {  
  Stack loop_label_stack;
  stack_init(&loop_label_stack, 128);

  Parser parser = {
    .token_count = token_count,
    .current_token_index = 0,
    .tokens = tokens,
    .file = file,
    .current_loop_label_id = 0,
    .loop_label_stack = loop_label_stack
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

      print_ast(node->data.function.block, ++whitespace);

      print_whitespace(whitespace);
      printf(")\n");      
      break;
    case AST_BLOCK:
      print_whitespace(whitespace);
      printf("START BLOCK\n");
      for (int i = 0; i < node->data.block.block_count; i++) {
        print_ast(&node->data.block.block_items[i], ++whitespace);
        printf("\n");
      }   
      print_whitespace(whitespace);
      printf("END BLOCK\n");
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
      print_whitespace(whitespace);
      printf("Expression Statement(");
      print_ast(node->data.expression_statement.expression, 0);
      printf(")\n");
      break;
    case AST_STATEMENT_IF:      
      print_whitespace(whitespace);
      printf("If( Condition( ");
      print_ast(node->data.if_statement.condition_expression, 0);
      printf(") Then( ");
      print_ast(node->data.if_statement.then_statement, 0);
      printf(")");

      if (node->data.if_statement.else_statement != NULL) {
        printf(" Else( ");
        print_ast(node->data.if_statement.else_statement, 0);
        printf(")");
      }
      printf(")\n");
      break;
    case AST_STATEMENT_WHILE:
      print_whitespace(whitespace);
      printf("While (\n");
      print_whitespace(whitespace + 1);
      printf("Condition (\n");
      print_ast(node->data.while_statement.condition, whitespace + 1);
      print_whitespace(whitespace + 1);
      printf(")\n");
      print_whitespace(whitespace + 1);
      printf("Body (\n");
      print_ast(node->data.while_statement.statement_body, whitespace + 1);
      print_whitespace(whitespace + 1);
      printf(")\n");
      print_whitespace(whitespace);
      printf(")\n");
      break;
    case AST_EXPRESSION_CONSTANT:
      print_whitespace(whitespace);
      printf("Constant(%d)", node->data.constant_expression.value);
      break;
    case AST_EXPRESSION_POSTFIX_INCREMENT:
      print_whitespace(whitespace);
      printf("Postfix Increment(");
      print_ast(node->data.increment_decrement_expression.expression, 0);
      printf(")");
      break;
    case AST_EXPRESSION_POSTFIX_DECREMENT:
      print_whitespace(whitespace);
      printf("Postfix Decrement(");
      print_ast(node->data.increment_decrement_expression.expression, 0);
      printf(")");
      break;
    case AST_EXPRESSION_PREFIX_INCREMENT:
      print_whitespace(whitespace);
      printf("Prefix Increment(");
      print_ast(node->data.increment_decrement_expression.expression, 0);
      printf(")");
      break;
    case AST_EXPRESSION_PREFIX_DECREMENT:
      print_whitespace(whitespace);
      printf("Prefix Decrement(");
      print_ast(node->data.increment_decrement_expression.expression, 0);
      printf(")");
      break;
    case AST_EXPRESSION_CONDITIONAL:
      print_whitespace(whitespace);

      printf("Conditional(\n");
      int indent = whitespace++;
      print_whitespace(indent);
      printf("Condition(\n");
      print_ast(node->data.conditional_expression.condition, 0);
      printf(") True Exp(");
      print_ast(node->data.conditional_expression.true_expression, 0);
      printf(") False Exp(");
      print_ast(node->data.conditional_expression.false_expression, 0);
      printf(")\n");      
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
      case AST_EXPRESSION_ASSIGNMENT: {
        int indentation = whitespace += 1;
        print_whitespace(whitespace);
        printf("Assignment(\n");
        print_whitespace(indentation);
        printf("Left(\n");
        print_ast(node->data.assignement_expression.left_expression, ++indentation);
        printf(")\n");

        print_whitespace(indentation);
        printf("Right(\n");
        print_ast(node->data.assignement_expression.right_expression, ++indentation);
        printf(")\n");
        print_whitespace(indentation);
        printf(")\n");
        break;
      }
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

void add_to_block(AstNode *block, AstNode *expr_or_stmt) {
  int current_count = block->data.block.block_count;
  int current_capacity = block->data.block.block_capacity;

  if (current_count == current_capacity) {
    int new_size = current_capacity == 0 ? BLOCK_STARTING_ALLOCATION : current_capacity * 2;

    AstNode *blocks = realloc(block->data.block.block_items, new_size * sizeof(AstNode));

    block->data.block.block_capacity = new_size;
    block->data.block.block_items = blocks;
  } 

  block->data.block.block_items[block->data.block.block_count] = *expr_or_stmt;
  block->data.block.block_count++;
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

  AstNode *function = malloc(sizeof(AstNode)); 
  function->type = AST_FUNCTION;
  function->data.function.name = id_name;
  function->data.function.block = ast_block(parser);

  return function;
}

AstNode* ast_block(Parser *parser) {
  ast_expect(parser, TOKEN_OPEN_BRACE);

  AstNode *block = malloc(sizeof(AstNode));
  block->type = AST_BLOCK;
  block->data.block.block_count = 0;
  block->data.block.block_capacity = 0;
  block->data.block.block_items = NULL;

  //TODO: While(true) loop seems dangerous if no close brace is supplied
  while(true) {
    if (current_token(parser)->type == TOKEN_CLOSE_BRACE) {
      ast_expect(parser, TOKEN_CLOSE_BRACE);
      return block;
    }

    if (current_token(parser)->type == TOKEN_INT) {
      ast_declaration(parser, block);
    } else {
      AstNode *statement = ast_statement(parser);
      add_to_block(block, statement);
    }
  }

  ast_expect(parser, TOKEN_CLOSE_BRACE);
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

void ast_declaration(Parser *parser, AstNode *block) {
  ast_expect(parser, TOKEN_INT);

  char *identifier = ast_identifier(parser);

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
  add_to_block(block, declaration);
}

AstNode *ast_statement(Parser *parser) { 
  if (end_of_file(parser)) {
    fprintf(stderr, "ERROR - Parser: Incomplete statement (line %d)\n", previous_token(parser)->line);
    exit(1);
  }

  if (current_token(parser)->type == TOKEN_SEMICOLON) {
    ast_expect(parser, TOKEN_SEMICOLON);
    AstNode *null_statement = malloc(sizeof(AstNode));
    null_statement->type = AST_STATEMENT_NULL;
    return null_statement;
  }

  if (current_token(parser)->type == TOKEN_RETURN) {
    ast_expect(parser, TOKEN_RETURN);
  
    AstNode *expression = ast_expression(parser, 0);
    AstNode *return_node = malloc(sizeof(AstNode));
    
    return_node->type = AST_STATEMENT_RETURN;
    return_node->data.return_statement.expression = expression;

    ast_expect(parser, TOKEN_SEMICOLON);
    return return_node;
  }

  if (current_token(parser)->type == TOKEN_IF) {
    ast_expect(parser, TOKEN_IF);
    ast_expect(parser, TOKEN_OPEN_PAREN);

    AstNode *condition_expression = ast_expression(parser, 0);

    ast_expect(parser, TOKEN_CLOSE_PAREN);

    AstNode *statement = ast_statement(parser);

    AstNode *if_statement = malloc(sizeof(AstNode));
    if_statement->type = AST_STATEMENT_IF;
    if_statement->data.if_statement.condition_expression = condition_expression;
    if_statement->data.if_statement.then_statement = statement;

    if (current_token(parser)->type != TOKEN_ELSE) {
      return if_statement;
    }

    ast_expect(parser, TOKEN_ELSE);
    AstNode *else_statement = ast_statement(parser);

    if_statement->data.if_statement.else_statement = else_statement;

    return if_statement;
  }

  if (current_token(parser)->type == TOKEN_OPEN_BRACE) {
    AstNode *block = ast_block(parser);
    return block;
  }

  if (current_token(parser)->type == TOKEN_GOTO) {
    ast_expect(parser, TOKEN_GOTO);

    char *goto_label = ast_identifier(parser);

    AstNode *goto_statement = malloc(sizeof(AstNode));
    goto_statement->type = AST_STATEMENT_GOTO;
    goto_statement->data.goto_statement.label = goto_label;

    ast_expect(parser, TOKEN_SEMICOLON);

    return goto_statement;
  }

  if (current_token(parser)->type == TOKEN_BREAK) {
    ast_expect(parser, TOKEN_BREAK);
    ast_expect(parser, TOKEN_SEMICOLON);

    StackValue *current_loop_label = stack_top(&parser->loop_label_stack);

    if (current_loop_label == NULL) {
      fprintf(stderr, "ERROR: Parser - Null loop label value for 'break'. Line %d", parser->tokens[parser->current_token_index].line);
      exit(1);
    }

    AstNode *break_statement = malloc(sizeof(AstNode));
    break_statement->type = AST_STATEMENT_BREAK;
    break_statement->data.break_statement.label_id = current_loop_label->data.integer;

    return break_statement;
  }

  if (current_token(parser)->type == TOKEN_CONTINUE) {
    ast_expect(parser, TOKEN_CONTINUE);
    ast_expect(parser, TOKEN_SEMICOLON);

    StackValue *current_loop_label = stack_top(&parser->loop_label_stack);

    if (current_loop_label == NULL) {
      fprintf(stderr, "ERROR: Parser - Null loop label value for 'continue'. Line %d", parser->tokens[parser->current_token_index].line);
      exit(1);
    }

    AstNode *continue_statement = malloc(sizeof(AstNode));
    continue_statement->type = AST_STATEMENT_CONTINUE;
    continue_statement->data.continue_statement.label_id = current_loop_label->data.integer;

    return continue_statement;
  }

  if (current_token(parser)->type == TOKEN_WHILE) {
    Stack *loop_label_stack = &parser->loop_label_stack;
    StackValue loop_stack_value = {
      .type = STACK_INT,
      .data.integer = parser->current_loop_label_id++
    };
    stack_push(loop_label_stack, loop_stack_value);
    
    ast_expect(parser, TOKEN_WHILE);
    ast_expect(parser, TOKEN_OPEN_PAREN);

    AstNode *condition_expression = ast_expression(parser, 0);
    
    ast_expect(parser, TOKEN_CLOSE_PAREN);

    AstNode *statements = ast_statement(parser);

    AstNode *while_statement = malloc(sizeof(AstNode));
    while_statement->type = AST_STATEMENT_WHILE;
    while_statement->data.while_statement.condition = condition_expression;
    while_statement->data.while_statement.statement_body = statements;
    while_statement->data.while_statement.label_id = loop_stack_value.data.integer;

    stack_pop(&parser->loop_label_stack);

    return while_statement;
  }

  if (current_token(parser)->type == TOKEN_DO) {
    Stack *loop_label_stack = &parser->loop_label_stack;
    StackValue loop_stack_value = {
      .type = STACK_INT,
      .data.integer = parser->current_loop_label_id++
    };
    stack_push(loop_label_stack, loop_stack_value);

    ast_expect(parser, TOKEN_DO);

    AstNode *statements = ast_statement(parser);
    
    ast_expect(parser, TOKEN_WHILE);
    ast_expect(parser, TOKEN_OPEN_PAREN);

    AstNode *condition_expression = ast_expression(parser, 0);
    
    ast_expect(parser, TOKEN_CLOSE_PAREN);
    ast_expect(parser, TOKEN_SEMICOLON);

    AstNode *do_statement = malloc(sizeof(AstNode));
    do_statement->type = AST_STATEMENT_DO_WHILE;
    do_statement->data.do_while_statement.condition = condition_expression;
    do_statement->data.do_while_statement.statement_body = statements;
    do_statement->data.do_while_statement.label_id = loop_stack_value.data.integer;

    stack_pop(&parser->loop_label_stack);

    return do_statement;
  }

  if (current_token(parser)->type == TOKEN_FOR) {
    Stack *loop_label_stack = &parser->loop_label_stack;
    StackValue loop_stack_value = {
      .type = STACK_INT,
      .data.integer = parser->current_loop_label_id++
    };
    stack_push(loop_label_stack, loop_stack_value);

    ast_expect(parser, TOKEN_FOR);
    ast_expect(parser, TOKEN_OPEN_PAREN);

    AstNode *for_loop_statement = malloc(sizeof(AstNode));
    for_loop_statement->type = AST_STATEMENT_FOR;

    AstNode *stmt_or_exp = ast_statement(parser);

    if (current_token(parser)->type != TOKEN_SEMICOLON) {
      AstNode *for_condition = ast_expression(parser, 0);
      for_loop_statement->data.for_statement.condition_expression = for_condition;
    }

    ast_expect(parser, TOKEN_SEMICOLON);

    if (current_token(parser)->type != TOKEN_SEMICOLON) {
      AstNode *post_expression = ast_expression(parser, 0);
      for_loop_statement->data.for_statement.post_expression = post_expression;
    }

    ast_expect(parser, TOKEN_CLOSE_PAREN);

    AstNode *for_statements = ast_statement(parser);

    for_loop_statement->data.for_statement.statement_body = for_statements;    
    for_loop_statement->data.for_statement.label_id = loop_stack_value.data.integer;

    stack_pop(&parser->loop_label_stack);

    return for_loop_statement;
  }
  
  AstNode *expression = ast_expression(parser, 0);  

  //TODO: See if we add this in ast_expression() instead of doing this goto label check
  if (expression->type != AST_STATEMENT_GOTO_LABEL) {
    ast_expect(parser, TOKEN_SEMICOLON);
  }
  return expression;
}

AstNode* ast_expression(Parser *parser, int min_precedence) {
  AstNode *left = ast_factor(parser);

  TokenType next_token = current_token(parser)->type;

  //TODO: May be easier to check outliers rather than what is being done here
  while ((next_token == TOKEN_PLUS || next_token == TOKEN_NEGATION || next_token == TOKEN_PERCENT || next_token == TOKEN_ASTERISK || next_token == TOKEN_FORWARD_SLASH || next_token == TOKEN_BITWISE_AND || next_token == TOKEN_BITWISE_XOR || next_token == TOKEN_BITWISE_OR || next_token == TOKEN_BITWISE_LEFT_SHIFT || next_token == TOKEN_BITWISE_RIGHT_SHIFT || next_token == TOKEN_RELATIONAL_LESS_THAN || next_token == TOKEN_RELATIONAL_LESS_OR_EQUAL || next_token == TOKEN_RELATIONAL_GREATER_THAN || next_token == TOKEN_RELATIONAL_GREATER_OR_EQUAL || next_token == TOKEN_RELATIONAL_EQUAL || next_token == TOKEN_RELATIONAL_NOT_EQUAL || next_token == TOKEN_LOGICAL_AND || next_token == TOKEN_LOGICAL_OR || next_token == TOKEN_EQUAL || next_token == TOKEN_PLUS_EQUAL || next_token == TOKEN_NEGATION_EQUAL || next_token == TOKEN_ASTERISK_EQUAL || next_token == TOKEN_FORWARD_SLASH_EQUAL || next_token == TOKEN_PERCENT_EQUAL || next_token == TOKEN_BITWISE_AND_EQUAL || next_token == TOKEN_BITWISE_OR_EQUAL || next_token == TOKEN_BITWISE_XOR_EQUAL || next_token == TOKEN_BITWISE_LEFT_SHIFT_EQUAL || next_token == TOKEN_BITWISE_RIGHT_SHIFT_EQUAL || next_token == TOKEN_INCREMENT || next_token == TOKEN_DECREMENT || next_token == TOKEN_QUESTION_MARK) && get_precedence(next_token) >= min_precedence) {

    if (next_token == TOKEN_INCREMENT || next_token == TOKEN_DECREMENT) {
      parser-> current_token_index++;

      AstNode *postfix_expression = malloc(sizeof(AstNode));

      if (next_token == TOKEN_INCREMENT) {
        postfix_expression->type = AST_EXPRESSION_POSTFIX_INCREMENT;
      } else {
        postfix_expression->type = AST_EXPRESSION_POSTFIX_DECREMENT;
      }

      //TODO: We should validate that 'left' is an identifier. May do in semantic analysis

      AstNode *postfix_assignment = malloc(sizeof(AstNode));
      postfix_assignment->type = AST_EXPRESSION_ASSIGNMENT;
      postfix_assignment->data.assignement_expression.left_expression = left;

      AstNode *postfix_constant = malloc(sizeof(AstNode));
      postfix_constant->type = AST_EXPRESSION_CONSTANT;
      postfix_constant->data.constant_expression.value = 1;
      
      AstNode *postfix_binary = malloc(sizeof(AstNode));
      postfix_binary->type = AST_EXPRESSION_BINARY;
      
      if (next_token == TOKEN_INCREMENT) {
        postfix_binary->data.binary_expression.op_type = AST_BINARY_ADD;
      } else {
        postfix_binary->data.binary_expression.op_type = AST_BINARY_SUBTRACT;
      }

      postfix_binary->data.binary_expression.left_expression = left;
      postfix_binary->data.binary_expression.right_expression = postfix_constant;

      postfix_assignment->data.assignement_expression.right_expression = postfix_binary;

      postfix_expression->data.increment_decrement_expression.expression = postfix_assignment;
      
      return postfix_expression;
    }
    
    if (next_token == TOKEN_EQUAL) {
      //right-associative assignment
      ast_expect(parser, TOKEN_EQUAL);

      // AstNode *right = ast_expression(parser, get_precedence(next_token) + 1);
      AstNode *right = ast_expression(parser, get_precedence(peek_next_token(parser)));
      AstNode *assignment_expression= malloc(sizeof(AstNode));

      assignment_expression->type = AST_EXPRESSION_ASSIGNMENT;
      assignment_expression->data.assignement_expression.left_expression = left;
      assignment_expression->data.assignement_expression.right_expression = right;

      left = assignment_expression;

      return left;
    } 

    if (next_token == TOKEN_QUESTION_MARK) {
      ast_expect(parser, TOKEN_QUESTION_MARK);

      AstNode *middle = ast_expression(parser, 0);

      ast_expect(parser, TOKEN_COLON);

      AstNode *right = ast_expression(parser, get_precedence(next_token));

      AstNode *conditional = malloc(sizeof(AstNode));
      conditional->type = AST_EXPRESSION_CONDITIONAL;
      conditional->data.conditional_expression.condition = left;
      conditional->data.conditional_expression.true_expression = middle;
      conditional->data.conditional_expression.false_expression = right;

      return conditional;
    }

    if (next_token == TOKEN_PLUS_EQUAL || next_token == TOKEN_NEGATION_EQUAL || next_token == TOKEN_ASTERISK_EQUAL || next_token == TOKEN_FORWARD_SLASH_EQUAL || next_token  == TOKEN_PERCENT_EQUAL || next_token == TOKEN_BITWISE_AND_EQUAL || next_token == TOKEN_BITWISE_OR_EQUAL || next_token == TOKEN_BITWISE_XOR_EQUAL || next_token == TOKEN_BITWISE_LEFT_SHIFT_EQUAL || next_token == TOKEN_BITWISE_RIGHT_SHIFT_EQUAL) {

      parser->current_token_index++;

      AstNode *right = ast_expression(parser, get_precedence(next_token));

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

    char slice[previous_token(parser)->end_index - previous_token(parser)->start_index]; 
    strncpy(slice, parser->file + previous_token(parser)->start_index, (previous_token(parser)->end_index - previous_token(parser)->start_index) + 1);
    
    int constant_value = atoi(slice);
    constant->data.constant_expression.value = constant_value;

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
  } else if (current_token(parser)->type == TOKEN_INCREMENT || current_token(parser)->type == TOKEN_DECREMENT) {
      AstNode *prefix_expression = malloc(sizeof(AstNode));

      if (current_token(parser)->type == TOKEN_INCREMENT) {
        ast_expect(parser, TOKEN_INCREMENT);
        prefix_expression->type = AST_EXPRESSION_PREFIX_INCREMENT;
      } else {
        ast_expect(parser, TOKEN_DECREMENT);
        prefix_expression->type = AST_EXPRESSION_PREFIX_DECREMENT;
      }

      AstNode *left = ast_expression(parser, 0);
      
      AstNode *prefix_assignment = malloc(sizeof(AstNode));
      prefix_assignment->type = AST_EXPRESSION_ASSIGNMENT;
      prefix_assignment->data.assignement_expression.left_expression = left;

      AstNode *postfix_constant = malloc(sizeof(AstNode));
      postfix_constant->type = AST_EXPRESSION_CONSTANT;
      postfix_constant->data.constant_expression.value = 1;
      
      AstNode *postfix_binary = malloc(sizeof(AstNode));
      postfix_binary->type = AST_EXPRESSION_BINARY;
      
      if (prefix_expression->type == AST_EXPRESSION_PREFIX_INCREMENT) {
        postfix_binary->data.binary_expression.op_type = AST_BINARY_ADD;
      } else {
        postfix_binary->data.binary_expression.op_type = AST_BINARY_SUBTRACT;
      }

      postfix_binary->data.binary_expression.left_expression = left;
      postfix_binary->data.binary_expression.right_expression = postfix_constant;

      prefix_assignment->data.assignement_expression.right_expression = postfix_binary;

      prefix_expression->data.increment_decrement_expression.expression = prefix_assignment;
      
      return prefix_assignment;

  } else if (current_token(parser)->type == TOKEN_OPEN_PAREN) {
    parser->current_token_index++;

    AstNode *expression = ast_expression(parser, 0);
        
    ast_expect(parser, TOKEN_CLOSE_PAREN);

    return expression;
  } else if (current_token(parser)->type == TOKEN_IDENTIFIER) {
    char *identifier = ast_identifier(parser);

    if (current_token(parser)->type == TOKEN_COLON) {
      ast_expect(parser, TOKEN_COLON);
      AstNode *goto_label_node = malloc(sizeof(AstNode));
      goto_label_node->type = AST_STATEMENT_GOTO_LABEL;
      goto_label_node->data.goto_label_statement.label = identifier;

      return goto_label_node;
    }

    AstNode *identifier_node = malloc(sizeof(AstNode));
    identifier_node->type = AST_EXPRESSION_VARIABLE;
    identifier_node->data.variable_expression.identifier = identifier;

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
    case TOKEN_QUESTION_MARK:
      return 3;
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
      // fprintf(stderr, "ERROR - Parser: Token '%s 'does not have a supported operator precendence\n", TokenTypeStr[token_type]);
      // exit(1);
      return -1;
    }
  }
}

