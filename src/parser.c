#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <sys/types.h>
#include "../include/parser.h"
#include "../include/arena.h"

#define BLOCK_STARTING_ALLOCATION 8
#define FUNCTION_PARAM_STARTING_ALLOCATION 8
#define FUNCTION_CALL_STARTING_ALLOCATION 8
#define PROGRAM_FUNCTION_ALLOCATION 8
#define ADD_WHITESPACE whitespace + 5

typedef struct Parser {
  int token_count;
  int current_token_index;
  int current_loop_label_id;
  Token *tokens;
  char* file;
  Arena* node_arena;
} Parser;
 
void       ast_program(Parser *parser, AstNode *program_node);
void       ast_function_declaration(Parser *parser, AstNode *function_node);
void       ast_variable_declaration(Parser *parser, AstNode *variable_node);
void       ast_block(Parser *parser, AstNode *block_node);
void       ast_declaration(Parser *parser, AstNode *declaration_node); 
AstNode*   ast_parse_statement(Parser *parser);
AstNode*   ast_parse_statement_null(Parser *parser);
AstNode*   ast_parse_statement_return(Parser *parser); 
AstNode*   ast_parse_statement_if(Parser *parser); 
AstNode*   ast_parse_statement_goto(Parser *parser); 
AstNode*   ast_parse_statement_break(Parser *parser); 
AstNode*   ast_parse_statement_continue(Parser *parser); 
AstNode*   ast_parse_statement_while(Parser *parser); 
AstNode*   ast_parse_statement_do(Parser *parser); 
AstNode*   ast_parse_statement_for(Parser *parser); 
void       ast_parse_expression(Parser *parser, AstNode *expression_node, int min_precedence);
AstNode*   ast_parse_expression_postfix(Parser *parser, AstNode *left_expression,  TokenType postfix_token);
AstNode*   ast_parse_expression_assignment(Parser *parser, AstNode *left_factor, TokenType assignment_token); 
AstNode*   ast_parse_expression_conditional(Parser *parser, AstNode *left_expression, TokenType conditional_token); 
AstNode*   ast_parse_expression_binary(Parser *parser, AstNode *left_expression, TokenType op_type);
void       ast_parse_factor(Parser *parser, AstNode *factor_node);
void       ast_parse_factor_constant(Parser *parser, AstNode *factor_node);
void       ast_parse_factor_unary(Parser *parser, AstNode *factor_node); 
void       ast_parse_factor_prefix_expression(Parser *parser, AstNode *factor_node); 
AstNode*   ast_parse_factor_parenthetical_expression(Parser *parser); 
AstNode*   ast_parse_factor_goto_label(Parser *parser, char *label_identifier); 
AstNode*   ast_parse_factor_variable_expression(Parser *parser, char *label_identifier);
AstNode*   ast_parse_factor_function_call(Parser *parser, char *identifier); 
Token*     current_token(Parser *parser);
Token*     previous_token(Parser *parser);
TokenType  peek_next_token(Parser *parser); 
char*      ast_identifier(Parser *parser);
void       ast_expect(Parser *parser, TokenType expected_type);
void       print_whitespace(int count); 
void       add_to_block(AstNode *function, AstNode *expr_or_stmt);
// void       add_to_function_params(AstNode *function_declaration, AstNode *parameter); 
void       add_to_function_call(AstNode *function_call, AstNode *expression); 
void       add_to_function_to_program(AstNode *program, AstNode *function_declaration); 
bool       end_of_file(Parser *parser);
bool       is_binary_operator_token(Parser *parser);
int        get_precedence(TokenType token_type);

AstNode* parse_ast(Token *tokens, int token_count, char *file) {  
  Arena *parser_arena = malloc(sizeof(Arena));
  //TODO: Hardcoded capacity
  arena_init(parser_arena, sizeof(AstNode), 1000);
  
  Parser parser = {
    .token_count = token_count,
    .current_token_index = 0,
    .tokens = tokens,
    .file = file,
    .current_loop_label_id = 0,
    .node_arena = parser_arena
  };
  
  AstNode *program_node = arena_alloc(parser.node_arena);
  ast_program(&parser, program_node);

  ast_expect(&parser, TOKEN_EOF);

  if (token_count > parser.current_token_index) {
    fprintf(stderr, "ERROR - Parser: Identifier declared outside of program scope (line %d)\n", parser.tokens[parser.current_token_index].line);
    exit(1);
  }

  return program_node;
}

void print_ast(AstNode *node, int whitespace) {
  switch(node->type){
    case AST_PROGRAM:  
      printf("Program (\n");
      for (int i = 0; i < node->data.program.function_count; i++) {
        print_ast(&node->data.program.function_declarations[i], ADD_WHITESPACE);
      }
      printf(")\n");
      break;
    case AST_VARIABLE_DECLARATION:
      print_whitespace(whitespace);
      printf("Variable Declaration (id = \"%s\"\n", node->data.variable_declaration.name);

      if (node->data.variable_declaration.has_expression) {
        print_ast(node->data.variable_declaration.init_expression, ADD_WHITESPACE);
      }

      print_whitespace(whitespace);
      printf(")\n");      
      break;
    case AST_FUNCTION_DECLARATION:
      print_whitespace(whitespace);
      printf("Function Declaration (name = \"%s\"\n", node->data.function_declaration.name);

      for (int i = 0; i < node->data.function_declaration.parameter_count; i++) {
        print_ast(&node->data.function_declaration.parameters[i], ADD_WHITESPACE);
      }

      if (node->data.function_declaration.body_block != NULL) {
        print_whitespace(whitespace);
        printf("body=\n");
        print_ast(node->data.function_declaration.body_block, ADD_WHITESPACE);
      }

      print_whitespace(whitespace);
      printf(")\n");      
      break;
    case AST_FUNCTION_PARAMETER:
      print_whitespace(whitespace);
      // printf("Function Param (type = %d id = %s)\n", node->data.function_parameters.type, node->data.function_parameters.name);
      printf("Function Param (type = ");

      switch (node->data.function_parameters.type) {
        case AST_PARAMETER_VOID:
          printf("void");
          break;
        case AST_PARAMETER_INT:
          printf("int");
          break;
      }

      if (node->data.function_parameters.type != AST_PARAMETER_VOID) {
        printf(" id = %s", node->data.function_parameters.name);
      }      

      printf(")\n");
      break;      
    case AST_BLOCK:
      print_whitespace(whitespace);
      printf("Block (\n");
      for (int i = 0; i < node->data.block.block_count; i++) {
        print_ast(&node->data.block.block_items[i], ADD_WHITESPACE);
      }   
      print_whitespace(whitespace);
      printf(")\n");
      break;
    case AST_STATEMENT_GOTO:
      print_whitespace(whitespace);
      printf("Goto (%s)\n", node->data.goto_statement.label);
      break;      
    case AST_STATEMENT_GOTO_LABEL:
      print_whitespace(whitespace);
      printf("Goto Label(%s)\n", node->data.goto_label_statement.label);
      break;      
    case AST_STATEMENT_BREAK:
      print_whitespace(whitespace);
      printf("Break(id = %d)\n", node->data.break_statement.label_id);
      break;
    case AST_STATEMENT_CONTINUE:
      print_whitespace(whitespace);
      printf("Continue(id = %d)\n", node->data.continue_statement.label_id);
      break;
    case AST_STATEMENT_RETURN:
      print_whitespace(whitespace);
      printf("Return(\n");
      print_ast(node->data.return_statement.expression, ADD_WHITESPACE);
      print_whitespace(whitespace);
      printf(")\n");
      break;
    case AST_STATEMENT_NULL:
      print_whitespace(whitespace);
      printf("Null()\n");
      break;
     case AST_STATEMENT_EXPRESSION:
      print_whitespace(whitespace);
      printf("Expression Statement(\n");
      print_ast(node->data.expression_statement.expression, ADD_WHITESPACE);
      printf(")\n");
      break;
    case AST_STATEMENT_IF:      
      print_whitespace(whitespace);
      printf("If (\n");
      print_ast(node->data.if_statement.condition_expression, ADD_WHITESPACE);
      print_whitespace(whitespace);
      printf(")\n ");
      print_whitespace(whitespace);
      printf("Then(\n");
      print_ast(node->data.if_statement.then_statement, ADD_WHITESPACE);
      print_whitespace(whitespace);
      printf(")\n");

      if (node->data.if_statement.else_statement != NULL) {
        print_whitespace(whitespace);
        printf("Else(\n");
        print_ast(node->data.if_statement.else_statement, ADD_WHITESPACE);
        print_whitespace(whitespace);
        printf(")\n");
      }
      break;
    case AST_STATEMENT_WHILE:
      print_whitespace(whitespace);
      printf("While (\n");
      print_whitespace(ADD_WHITESPACE);
      printf("Id = %d\n", node->data.while_statement.label_id);
      print_whitespace(ADD_WHITESPACE);
      printf("Condition =\n");
      print_ast(node->data.while_statement.condition, ADD_WHITESPACE + 5);
      print_whitespace(ADD_WHITESPACE);
      printf("Statements =\n");
      print_ast(node->data.while_statement.statement_body, ADD_WHITESPACE + 5);
      print_whitespace(whitespace);
      printf(")\n");
      break;
    case AST_STATEMENT_DO_WHILE:
      print_whitespace(whitespace);
      printf("Do (\n");
      print_whitespace(ADD_WHITESPACE);
      printf("Id = %d\n", node->data.do_while_statement.label_id);
      print_whitespace(ADD_WHITESPACE);
      printf("Statements = \n");
      print_ast(node->data.do_while_statement.statement_body, ADD_WHITESPACE + 5);
      print_whitespace(ADD_WHITESPACE);
      printf("Condition = \n");
      print_ast(node->data.do_while_statement.condition, ADD_WHITESPACE + 5);
      print_whitespace(whitespace);
      printf(")\n");
      break;
    case AST_STATEMENT_FOR:
      print_whitespace(whitespace);
      printf("For (\n");
      print_whitespace(ADD_WHITESPACE);
      printf("Id = %d\n", node->data.for_statement.label_id);

      if (node->data.for_statement.for_loop_init != NULL) {
        print_whitespace(ADD_WHITESPACE);
        printf("Init = \n");
        print_ast(node->data.for_statement.for_loop_init, ADD_WHITESPACE + 5);
      }

      if (node->data.for_statement.condition_expression != NULL) {
        print_whitespace(ADD_WHITESPACE);
        printf("Condition = \n");
        print_ast(node->data.for_statement.condition_expression, ADD_WHITESPACE + 5);
      }

      if (node->data.for_statement.post_expression != NULL) {
        print_whitespace(ADD_WHITESPACE);
        printf("Post = \n");
        print_ast(node->data.for_statement.post_expression, ADD_WHITESPACE + 5);
      }

      print_whitespace(whitespace);
      printf(")\n");      
      break;
    case AST_EXPRESSION_CONSTANT:
      print_whitespace(whitespace);
      printf("Constant(%d)\n", node->data.constant_expression.value);
      break;
    case AST_EXPRESSION_POSTFIX_INCREMENT:
      print_whitespace(whitespace);
      printf("Postfix Increment(\n");
      print_ast(node->data.increment_decrement_expression.expression, ADD_WHITESPACE);
      print_whitespace(whitespace);
      printf(")\n");
      break;
    case AST_EXPRESSION_POSTFIX_DECREMENT:
      print_whitespace(whitespace);
      printf("Postfix Decrement(\n");
      print_ast(node->data.increment_decrement_expression.expression, ADD_WHITESPACE);
      print_whitespace(whitespace);
      printf(")\n");
      break;
    case AST_EXPRESSION_PREFIX_INCREMENT:
      print_whitespace(whitespace);
      printf("Prefix Increment(\n");
      print_ast(node->data.increment_decrement_expression.expression, ADD_WHITESPACE);
      print_whitespace(whitespace);
      printf(")\n");
      break;
    case AST_EXPRESSION_PREFIX_DECREMENT:
      print_whitespace(whitespace);
      printf("Prefix Decrement(\n");
      print_ast(node->data.increment_decrement_expression.expression, ADD_WHITESPACE);
      print_whitespace(whitespace);
      printf(")\n");
      break;
    case AST_EXPRESSION_CONDITIONAL:
      print_whitespace(whitespace);
      printf("Conditional(\n");
      print_whitespace(ADD_WHITESPACE);
      printf("Condition = \n");
      print_ast(node->data.conditional_expression.condition, ADD_WHITESPACE + 5);
      printf("True Expression = \n");
      print_ast(node->data.conditional_expression.true_expression, ADD_WHITESPACE + 5);
      printf("False Expression = \n");
      print_ast(node->data.conditional_expression.false_expression, ADD_WHITESPACE + 5);
      print_whitespace(whitespace);
      printf(")\n");      
      break;
    case AST_EXPRESSION_UNARY:
      print_whitespace(whitespace);
      printf("Unary (type = ");

      switch (node->data.unary_expression.op_type) {
        case AST_UNARY_COMPLEMENT: printf("Complement"); break;
        case AST_UNARY_NEGATE: printf("Negate"); break;
        case AST_UNARY_NOT: printf("Not"); break;
        case AST_UNARY_PREFIX_INCREMENT: printf("Prefix Increment"); break;
        case AST_UNARY_PREFIX_DECREMENT: printf("Prefix Decrement"); break;
      }
      printf("\n");      
      print_ast(node->data.unary_expression.expression, ADD_WHITESPACE);
      print_whitespace(whitespace);
      printf(")\n");
      break;
    case AST_EXPRESSION_BINARY:
      print_whitespace(whitespace);
      printf("Binary( op type = ");
      switch (node->data.binary_expression.op_type) {
        case AST_BINARY_ADD:                  printf("\"+\""); break;
        case AST_BINARY_SUBTRACT:             printf("\"-\""); break;
        case AST_BINARY_DIVIDE:               printf("\"/\""); break;
        case AST_BINARY_MULTIPLY:             printf("\"*\""); break;
        case AST_BINARY_REMAINDER:            printf("\"%%\""); break;
        case AST_BINARY_BITWISE_AND:          printf("\"&\""); break; 
        case AST_BINARY_BITWISE_OR:           printf("\"|\""); break; 
        case AST_BINARY_BITWISE_XOR:          printf("\"^\""); break; 
        case AST_BINARY_BITWISE_LEFT_SHIFT:   printf("\"<<\""); break;
        case AST_BINARY_BITWISE_RIGHT_SHIFT:  printf("\">>\""); break;
        case AST_BINARY_AND:                  printf("\"&&\""); break;
        case AST_BINARY_OR:                   printf("\"||\""); break;
        case AST_BINARY_GREATER_THAN:         printf("\">\""); break;
        case AST_BINARY_GREATER_OR_EQUAL:     printf("\">=\""); break;
        case AST_BINARY_LESS_THAN:            printf("\"<\""); break;
        case AST_BINARY_LESS_OR_EQUAL:        printf("\"<=\""); break;
        case AST_BINARY_EQUAL:                printf("\"==\""); break;
        case AST_BINARY_NOT_EQUAL:            printf("\"!=\""); break;
      }
      printf("\n");    
      print_whitespace(ADD_WHITESPACE);
      printf("Left = \n");
      print_ast(node->data.binary_expression.left_expression, ADD_WHITESPACE + 5);
      print_whitespace(ADD_WHITESPACE);
      printf("Right = \n");    
      print_ast(node->data.binary_expression.right_expression, ADD_WHITESPACE + 5);
      print_whitespace(whitespace);
      printf(")\n");
      break;
      case AST_EXPRESSION_VARIABLE:
        print_whitespace(whitespace);
        printf("Variable(%s)\n", node->data.variable_expression.identifier);
        break;
      case AST_EXPRESSION_ASSIGNMENT: {
        print_whitespace(whitespace);
        printf("Assignment(\n");
        print_whitespace(ADD_WHITESPACE);
        printf("Left = \n");
        print_ast(node->data.assignement_expression.left_expression, ADD_WHITESPACE + 5);

        print_whitespace(ADD_WHITESPACE);
        printf("Right = \n");
        print_ast(node->data.assignement_expression.right_expression, ADD_WHITESPACE + 5);
        print_whitespace(whitespace);
        printf(")\n");
        break;
      }
      case AST_EXPRESSION_FUNCTION_CALL: {
        print_whitespace(whitespace);
        printf("Function Call(args=\n");

        for (int i = 0; i < node->data.function_call_expression.argument_count; i++) {
          print_ast(&node->data.function_call_expression.arguments[i], ADD_WHITESPACE);
        }

        print_whitespace(whitespace);
        printf(")\n");
        break;
      }
      default: {
        printf("ERROR - Parser: Missing ast node type for printing: %d\n", node->type);
        exit(1);
    }
  }    
}

void print_whitespace(int count) {
  printf("%*s", count, "");
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

// void add_to_block(AstNode *block, AstNode *expr_or_stmt) {
//   int current_count = block->data.block.block_count;
//   int current_capacity = block->data.block.block_capacity;

//   if (current_count == current_capacity) {
//     int new_size = current_capacity == 0 ? BLOCK_STARTING_ALLOCATION : current_capacity * 2;

//     AstNode *blocks = realloc(block->data.block.block_items, new_size * sizeof(AstNode));

//     block->data.block.block_capacity = new_size;
//     block->data.block.block_items = blocks;
//   } 

//   block->data.block.block_items[block->data.block.block_count] = *expr_or_stmt;
//   block->data.block.block_count++;
// }

// void add_to_function_params(AstNode *function_declaration, AstNode *parameter) { 
//   int current_count = function_declaration->data.function_declaration.parameter_count;
//   int current_capacity = function_declaration->data.function_declaration.parameter_capacity;

//   if (current_count == current_capacity) {
//     int new_size = current_capacity == 0 ? FUNCTION_PARAM_STARTING_ALLOCATION: current_capacity * 2;

//     AstNode *parameters = realloc(function_declaration->data.function_declaration.parameters, new_size * sizeof(AstNode));

//     function_declaration->data.function_declaration.parameter_capacity = new_size;
//     function_declaration->data.function_declaration.parameters = parameters;
//   } 

//   function_declaration->data.function_declaration.parameters[function_declaration->data.function_declaration.parameter_count] = *parameter;
//   function_declaration->data.function_declaration.parameter_count++;
// }

void add_to_function_call(AstNode *function_call, AstNode *expression) {
  int current_count = function_call->data.function_call_expression.argument_count;
  int current_capacity = function_call->data.function_call_expression.argument_capacity;

  if (current_count == current_capacity) {
    int new_size = current_capacity == 0 ? FUNCTION_CALL_STARTING_ALLOCATION: current_capacity * 2;

    AstNode *arguments = realloc(function_call->data.function_call_expression.arguments, new_size * sizeof(AstNode));

    function_call->data.function_call_expression.argument_capacity = new_size;
    function_call->data.function_call_expression.arguments = arguments;
  } 

  function_call->data.function_call_expression.arguments[function_call->data.function_call_expression.argument_count] = *expression;
  function_call->data.function_call_expression.argument_count++;
} 

void add_to_function_to_program(AstNode *program, AstNode *function_declaration) {
  int current_count = program->data.program.function_count;
  int current_capacity = program->data.program.function_capacity;

  if (current_count == current_capacity) {
    int new_size = current_capacity == 0 ? PROGRAM_FUNCTION_ALLOCATION : current_capacity * 2;

    AstNode *arguments = realloc(program->data.program.function_declarations, new_size * sizeof(AstNode));

    program->data.program.function_capacity = new_size;
    program->data.program.function_declarations = arguments;
  } 

  program->data.program.function_declarations[program->data.program.function_count] = *function_declaration;
  program->data.program.function_count++;
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

void ast_program(Parser *parser, AstNode *program_node) {
  program_node->type = AST_PROGRAM;
  program_node->data.program.function_capacity = 0;
  program_node->data.program.function_count = 0;
  program_node->data.program.function_declarations = NULL;
  
  while (current_token(parser)->type != TOKEN_EOF) {
    AstNode *function_node = arena_alloc(parser->node_arena);
    ast_function_declaration(parser, function_node);
    //TODO: Check if necessary
    // add_to_function_to_program(program_node, function);
  } 
}

void ast_declaration(Parser *parser, AstNode *declaration_node) {
  //Variable Declaration -> int c; or int c = 0; 
  if (parser->tokens[parser->current_token_index + 2].type == TOKEN_EQUAL || parser->tokens[parser->current_token_index + 2].type == TOKEN_SEMICOLON) {
    ast_variable_declaration(parser);
  }


  AstNode *function_node = arena_alloc(parser->node_arena);
  ast_function_declaration(parser, function_node);
  return;
}

void ast_function_declaration(Parser *parser, AstNode *function_node) {
  function_node->data.function_declaration.parameter_count = 0;
  
  ast_expect(parser, TOKEN_INT);

  char *id_name = ast_identifier(parser);  

  ast_expect(parser, TOKEN_OPEN_PAREN);

  AstNode *parameter = arena_alloc(parser->node_arena);
  parameter->type = AST_FUNCTION_PARAMETER;

  switch (current_token(parser)->type) {
    case TOKEN_VOID:
      ast_expect(parser, TOKEN_VOID);
      parameter->data.function_parameters.type = AST_PARAMETER_VOID;
      break;
    case TOKEN_INT: {
      ast_expect(parser, TOKEN_INT);
      parameter->data.function_parameters.type = AST_PARAMETER_INT;
      parameter->data.function_parameters.name = ast_identifier(parser); 
      break;
    }
    default: {
      fprintf(stderr, "ERROR - Parser: Unsupported parameter type %d", current_token(parser)->type);
      exit(1);
    }
  }

  function_node->data.function_declaration.parameter_count++;
  // add_to_function_params(function_node, parameter);

  while(current_token(parser)->type == TOKEN_COMMA) {
    ast_expect(parser, TOKEN_COMMA);

    AstNode *next_parameter = arena_alloc(parser->node_arena);
    next_parameter->type = AST_FUNCTION_PARAMETER;
    
    switch (current_token(parser)->type) {
      case TOKEN_VOID:
        ast_expect(parser, TOKEN_VOID);
        next_parameter->data.function_parameters.type = AST_PARAMETER_VOID;
        break;
      case TOKEN_INT: {
        ast_expect(parser, TOKEN_INT);
        next_parameter->data.function_parameters.type = AST_PARAMETER_INT;
        next_parameter->data.function_parameters.name = ast_identifier(parser); 
        break;
      }
      default: {
        fprintf(stderr, "ERROR - Parser: Unsupported parameter type %d", current_token(parser)->type);
        exit(1);
      }    
    }

    // add_to_function_params(function_node, next_parameter);
    function_node->data.function_declaration.parameter_count++;
  }
  
  ast_expect(parser, TOKEN_CLOSE_PAREN);

  function_node->type = AST_FUNCTION_DECLARATION;
  function_node->data.function_declaration.name = id_name;

  //If semi-colon is found, then it is considered a function definition
  if (current_token(parser)->type == TOKEN_SEMICOLON) {
    ast_expect(parser, TOKEN_SEMICOLON);
    return;
  }
  
  AstNode *block_node = arena_alloc(parser->node_arena);
  ast_block(parser, block_node);
}

void ast_variable_declaration(Parser *parser, AstNode *variable_node) {
  ast_expect(parser, TOKEN_INT);

  char *identifier = ast_identifier(parser);

  variable_node->type = AST_VARIABLE_DECLARATION;
  variable_node->data.variable_declaration.name = identifier;

  if (current_token(parser)->type == TOKEN_EQUAL) {
    //TODO: Fix as ast_identifier eats the token but we need it to feed into ast_expression();
    parser->current_token_index--;

    AstNode *expression_node = arena_alloc(parser->node_arena);
    ast_parse_expression(parser, expression_node, 0);

    variable_node->data.variable_declaration.has_expression = true;
    variable_node->data.variable_declaration.init_expression = expression_node;
  }

  ast_expect(parser, TOKEN_SEMICOLON);
}

void ast_block(Parser *parser, AstNode *block_node) {
  ast_expect(parser, TOKEN_OPEN_BRACE);

  block_node->type = AST_BLOCK;
  block_node->data.block.block_count = 0;
  block_node->data.block.block_capacity = 0;
  block_node->data.block.block_items = NULL;

  //TODO: While(true) loop seems dangerous if no close brace is supplied
  while(true) {
    if (current_token(parser)->type == TOKEN_CLOSE_BRACE) {
      ast_expect(parser, TOKEN_CLOSE_BRACE);
      return;
    }

    if (current_token(parser)->type == TOKEN_INT) {
      AstNode *declaration_node = arena_alloc(parser->node_arena);
      ast_declaration(parser, declaration_node);
      block_node->data.block.block_count++;
      // add_to_block(block, declaration);
    } else {
      AstNode *statement = ast_parse_statement(parser);
      block_node->data.block.block_count++;
      // add_to_block(block, statement);
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

AstNode *ast_parse_statement(Parser *parser) { 
  if (end_of_file(parser)) {
    fprintf(stderr, "ERROR - Parser: Incomplete statement (line %d)\n", previous_token(parser)->line);
    exit(1);
  }

  switch (current_token(parser)->type) {
    case TOKEN_OPEN_BRACE: return ast_block(parser);
    case TOKEN_SEMICOLON:  return ast_parse_statement_null(parser);
    case TOKEN_RETURN:     return ast_parse_statement_return(parser);
    case TOKEN_IF:         return ast_parse_statement_if(parser);
    case TOKEN_GOTO:       return ast_parse_statement_goto(parser);
    case TOKEN_BREAK:      return ast_parse_statement_break(parser);
    case TOKEN_CONTINUE:   return ast_parse_statement_continue(parser); 
    case TOKEN_WHILE:      return ast_parse_statement_while(parser);
    case TOKEN_DO:         return ast_parse_statement_do(parser);
    case TOKEN_FOR:        return ast_parse_statement_for(parser);
    default: {
      AstNode *expression = ast_parse_expression(parser, 0);  

      //TODO: See if we add this in ast_expression() instead of doing this goto label check
      if (expression->type != AST_STATEMENT_GOTO_LABEL) {
        ast_expect(parser, TOKEN_SEMICOLON);
      }
      return expression;
    }
  }
}

AstNode* ast_parse_statement_null(Parser *parser) {
  ast_expect(parser, TOKEN_SEMICOLON);
  AstNode *null_statement = malloc(sizeof(AstNode));
  null_statement->type = AST_STATEMENT_NULL;
  return null_statement;
}

AstNode* ast_parse_statement_return(Parser *parser) {
  ast_expect(parser, TOKEN_RETURN);

  AstNode *expression = ast_parse_expression(parser, 0);
  AstNode *return_node = malloc(sizeof(AstNode));
  
  return_node->type = AST_STATEMENT_RETURN;
  return_node->data.return_statement.expression = expression;

  ast_expect(parser, TOKEN_SEMICOLON);
  return return_node;
}

AstNode* ast_parse_statement_if(Parser *parser) {
  ast_expect(parser, TOKEN_IF);
  ast_expect(parser, TOKEN_OPEN_PAREN);

  AstNode *condition_expression = ast_parse_expression(parser, 0);

  ast_expect(parser, TOKEN_CLOSE_PAREN);

  AstNode *statement = ast_parse_statement(parser);

  AstNode *if_statement = malloc(sizeof(AstNode));
  if_statement->type = AST_STATEMENT_IF;
  if_statement->data.if_statement.condition_expression = condition_expression;
  if_statement->data.if_statement.then_statement = statement;

  if (current_token(parser)->type != TOKEN_ELSE) {
    return if_statement;
  }

  ast_expect(parser, TOKEN_ELSE);
  AstNode *else_statement = ast_parse_statement(parser);

  if_statement->data.if_statement.else_statement = else_statement;

  return if_statement;
}

AstNode* ast_parse_statement_goto(Parser *parser) {
  ast_expect(parser, TOKEN_GOTO);

  char *goto_label = ast_identifier(parser);

  AstNode *goto_statement = malloc(sizeof(AstNode));
  goto_statement->type = AST_STATEMENT_GOTO;
  goto_statement->data.goto_statement.label = goto_label;

  ast_expect(parser, TOKEN_SEMICOLON);

  return goto_statement;
}

AstNode* ast_parse_statement_break(Parser *parser) {
  ast_expect(parser, TOKEN_BREAK);
  ast_expect(parser, TOKEN_SEMICOLON);

  AstNode *break_statement = malloc(sizeof(AstNode));
  break_statement->type = AST_STATEMENT_BREAK;

  return break_statement;
}
  
AstNode* ast_parse_statement_continue(Parser *parser) {
    ast_expect(parser, TOKEN_CONTINUE);
    ast_expect(parser, TOKEN_SEMICOLON);

    AstNode *continue_statement = malloc(sizeof(AstNode));
    continue_statement->type = AST_STATEMENT_CONTINUE;

    return continue_statement;
}

AstNode* ast_parse_statement_while(Parser *parser) {
  ast_expect(parser, TOKEN_WHILE);
  ast_expect(parser, TOKEN_OPEN_PAREN);

  AstNode *condition_expression = ast_parse_expression(parser, 0);
  
  ast_expect(parser, TOKEN_CLOSE_PAREN);

  AstNode *statements = ast_parse_statement(parser);

  AstNode *while_statement = malloc(sizeof(AstNode));
  while_statement->type = AST_STATEMENT_WHILE;
  while_statement->data.while_statement.condition = condition_expression;
  while_statement->data.while_statement.statement_body = statements;

  return while_statement;
}

AstNode* ast_parse_statement_do(Parser *parser) {
  ast_expect(parser, TOKEN_DO);

  AstNode *statements = ast_parse_statement(parser);
  
  ast_expect(parser, TOKEN_WHILE);
  ast_expect(parser, TOKEN_OPEN_PAREN);

  AstNode *condition_expression = ast_parse_expression(parser, 0);
  
  ast_expect(parser, TOKEN_CLOSE_PAREN);
  ast_expect(parser, TOKEN_SEMICOLON);

  AstNode *do_statement = malloc(sizeof(AstNode));
  do_statement->type = AST_STATEMENT_DO_WHILE;
  do_statement->data.do_while_statement.condition = condition_expression;
  do_statement->data.do_while_statement.statement_body = statements;

  return do_statement;
}

AstNode* ast_parse_statement_for(Parser *parser) {
  ast_expect(parser, TOKEN_FOR);
  ast_expect(parser, TOKEN_OPEN_PAREN);

  AstNode *for_loop_statement = malloc(sizeof(AstNode));
  for_loop_statement->type = AST_STATEMENT_FOR;

  AstNode *dec_or_exp;

  //TODO: This will not work when we introduce declaration types other than 'int'
  if (current_token(parser)->type == TOKEN_SEMICOLON) {
    ast_expect(parser, TOKEN_SEMICOLON);    
    dec_or_exp = NULL;
  } else if (current_token(parser)->type == TOKEN_INT) {
    dec_or_exp = ast_variable_declaration(parser);
  } else {
    dec_or_exp = ast_parse_expression(parser, 0);
    //TODO: Weird we do this for expressions but are handled in ast_declaration()
    ast_expect(parser, TOKEN_SEMICOLON);
  }

  for_loop_statement->data.for_statement.for_loop_init = dec_or_exp;

  if (current_token(parser)->type != TOKEN_SEMICOLON) {
    AstNode *for_condition = ast_parse_expression(parser, 0);
    for_loop_statement->data.for_statement.condition_expression = for_condition;
  }

  ast_expect(parser, TOKEN_SEMICOLON);

  if (current_token(parser)->type != TOKEN_SEMICOLON && current_token(parser)->type != TOKEN_CLOSE_PAREN) {
    AstNode *post_expression = ast_parse_expression(parser, 0);
    for_loop_statement->data.for_statement.post_expression = post_expression;
  }

  ast_expect(parser, TOKEN_CLOSE_PAREN);

  AstNode *for_statements = ast_parse_statement(parser);

  for_loop_statement->data.for_statement.statement_body = for_statements;    

  return for_loop_statement;
}

void ast_parse_expression(Parser *parser, AstNode *expression_node, int min_precedence) {
  ast_parse_factor(parser, expression_node);

  TokenType next_token = current_token(parser)->type;

  while (get_precedence(next_token) >= min_precedence) {
    switch(next_token) {
      case TOKEN_INCREMENT:
      case TOKEN_DECREMENT:
        ast_parse_expression_postfix(parser, expression_node, next_token);        
        break;
      case TOKEN_EQUAL:
        ast_parse_expression_assignment(parser, expression_node, next_token);        
        break;
      case TOKEN_QUESTION_MARK:
        ast_parse_expression_conditional(parser, expression_node, next_token);
        break;
      default: {
        ast_parse_expression_binary(parser, expression_node, next_token);
        break;
      }
    }
    next_token = current_token(parser)->type;
  } 
}

AstNode* ast_parse_expression_postfix(Parser *parser, AstNode *left_expression,  TokenType postfix_token) {
  parser-> current_token_index++;

  AstNode *postfix_expression = malloc(sizeof(AstNode));

  if (postfix_token == TOKEN_INCREMENT) {
    postfix_expression->type = AST_EXPRESSION_POSTFIX_INCREMENT;
  } else {
    postfix_expression->type = AST_EXPRESSION_POSTFIX_DECREMENT;
  }

  //TODO: We should validate that 'left' is an identifier. May do in semantic analysis

  AstNode *postfix_assignment = malloc(sizeof(AstNode));
  postfix_assignment->type = AST_EXPRESSION_ASSIGNMENT;
  postfix_assignment->data.assignement_expression.left_expression = left_expression;

  AstNode *postfix_constant = malloc(sizeof(AstNode));
  postfix_constant->type = AST_EXPRESSION_CONSTANT;
  postfix_constant->data.constant_expression.value = 1;
  
  AstNode *postfix_binary = malloc(sizeof(AstNode));
  postfix_binary->type = AST_EXPRESSION_BINARY;
  
  if (postfix_token == TOKEN_INCREMENT) {
    postfix_binary->data.binary_expression.op_type = AST_BINARY_ADD;
  } else {
    postfix_binary->data.binary_expression.op_type = AST_BINARY_SUBTRACT;
  }

  postfix_binary->data.binary_expression.left_expression = left_expression;
  postfix_binary->data.binary_expression.right_expression = postfix_constant;
  postfix_assignment->data.assignement_expression.right_expression = postfix_binary;
  postfix_expression->data.increment_decrement_expression.expression = postfix_assignment;
  
  return postfix_expression;
}

AstNode* ast_parse_expression_assignment(Parser *parser, AstNode *left_factor, TokenType assignment_token) {
  //right-associative assignment
  ast_expect(parser, TOKEN_EQUAL);

  //TODO: @Test - This was previously:
  // AstNode *right = ast_expression(parser, get_precedence(peek_next_token(parser)));
  // That doesn't make sense since we consume the '=' token and not use the precedence of the next token
  AstNode *right = ast_parse_expression(parser, get_precedence(assignment_token));
  AstNode *assignment_expression= malloc(sizeof(AstNode));

  assignment_expression->type = AST_EXPRESSION_ASSIGNMENT;
  assignment_expression->data.assignement_expression.left_expression = left_factor;
  assignment_expression->data.assignement_expression.right_expression = right;

  return assignment_expression;
}

// TODO: conditional_token may always be question mark. If so, remove param and assign get_precedence in the function
// to TOKEN_QUESTION_MARK
AstNode* ast_parse_expression_conditional(Parser *parser, AstNode *left_expression, TokenType conditional_token) {
  ast_expect(parser, TOKEN_QUESTION_MARK);

  AstNode *middle = ast_parse_expression(parser, 0);

  ast_expect(parser, TOKEN_COLON);

  AstNode *right = ast_parse_expression(parser, get_precedence(conditional_token));

  AstNode *conditional = malloc(sizeof(AstNode));
  conditional->type = AST_EXPRESSION_CONDITIONAL;
  conditional->data.conditional_expression.condition = left_expression;
  conditional->data.conditional_expression.true_expression = middle;
  conditional->data.conditional_expression.false_expression = right;

  return conditional;
}

AstNode* ast_parse_expression_binary(Parser *parser, AstNode *left_expression, TokenType op_type) {
  parser-> current_token_index++;

  AstNode *right = ast_parse_expression(parser, get_precedence(op_type) + 1);

  AstNode *binary_expression = malloc(sizeof(AstNode));
  binary_expression->type = AST_EXPRESSION_BINARY;

  binary_expression->data.binary_expression.left_expression = left_expression;
  binary_expression->data.binary_expression.right_expression = right;
 
  switch (op_type) {
    case TOKEN_PLUS:                        binary_expression->data.binary_expression.op_type = AST_BINARY_ADD; break;
    case TOKEN_NEGATION:                    binary_expression->data.binary_expression.op_type = AST_BINARY_SUBTRACT; break;
    case TOKEN_ASTERISK:                    binary_expression->data.binary_expression.op_type = AST_BINARY_MULTIPLY; break;
    case TOKEN_FORWARD_SLASH:               binary_expression->data.binary_expression.op_type = AST_BINARY_DIVIDE; break;
    case TOKEN_PERCENT:                     binary_expression->data.binary_expression.op_type = AST_BINARY_REMAINDER; break;
    case TOKEN_BITWISE_AND:                 binary_expression->data.binary_expression.op_type = AST_BINARY_BITWISE_AND; break;
    case TOKEN_BITWISE_OR:                  binary_expression->data.binary_expression.op_type = AST_BINARY_BITWISE_OR; break;
    case TOKEN_BITWISE_XOR:                 binary_expression->data.binary_expression.op_type = AST_BINARY_BITWISE_XOR; break;
    case TOKEN_BITWISE_LEFT_SHIFT:          binary_expression->data.binary_expression.op_type = AST_BINARY_BITWISE_LEFT_SHIFT; break;
    case TOKEN_BITWISE_RIGHT_SHIFT:         binary_expression->data.binary_expression.op_type = AST_BINARY_BITWISE_RIGHT_SHIFT; break;
    case TOKEN_RELATIONAL_LESS_THAN:        binary_expression->data.binary_expression.op_type = AST_BINARY_LESS_THAN; break;
    case TOKEN_RELATIONAL_LESS_OR_EQUAL:    binary_expression->data.binary_expression.op_type = AST_BINARY_LESS_OR_EQUAL; break;
    case TOKEN_RELATIONAL_GREATER_THAN:     binary_expression->data.binary_expression.op_type = AST_BINARY_GREATER_THAN; break;
    case TOKEN_RELATIONAL_GREATER_OR_EQUAL: binary_expression->data.binary_expression.op_type = AST_BINARY_GREATER_OR_EQUAL; break;
    case TOKEN_RELATIONAL_EQUAL:            binary_expression->data.binary_expression.op_type = AST_BINARY_EQUAL; break;
    case TOKEN_RELATIONAL_NOT_EQUAL:        binary_expression->data.binary_expression.op_type = AST_BINARY_NOT_EQUAL; break;
    case TOKEN_LOGICAL_AND:                 binary_expression->data.binary_expression.op_type = AST_BINARY_AND; break;
    case TOKEN_LOGICAL_OR:                  binary_expression->data.binary_expression.op_type = AST_BINARY_OR; break;
    case TOKEN_PLUS_EQUAL:                  binary_expression->data.binary_expression.op_type = AST_BINARY_ADD; break;
    case TOKEN_NEGATION_EQUAL:              binary_expression->data.binary_expression.op_type = AST_BINARY_SUBTRACT; break;
    case TOKEN_ASTERISK_EQUAL:              binary_expression->data.binary_expression.op_type = AST_BINARY_MULTIPLY; break;
    case TOKEN_FORWARD_SLASH_EQUAL:         binary_expression->data.binary_expression.op_type = AST_BINARY_DIVIDE; break;
    case TOKEN_PERCENT_EQUAL:               binary_expression->data.binary_expression.op_type = AST_BINARY_REMAINDER; break;
    case TOKEN_BITWISE_AND_EQUAL:           binary_expression->data.binary_expression.op_type = AST_BINARY_BITWISE_AND; break;
    case TOKEN_BITWISE_OR_EQUAL:            binary_expression->data.binary_expression.op_type = AST_BINARY_BITWISE_OR; break;
    case TOKEN_BITWISE_XOR_EQUAL:           binary_expression->data.binary_expression.op_type = AST_BINARY_BITWISE_XOR; break;
    case TOKEN_BITWISE_RIGHT_SHIFT_EQUAL:   binary_expression->data.binary_expression.op_type = AST_BINARY_BITWISE_RIGHT_SHIFT; break;
    case TOKEN_BITWISE_LEFT_SHIFT_EQUAL:    binary_expression->data.binary_expression.op_type = AST_BINARY_BITWISE_LEFT_SHIFT; break;
    default:
      fprintf(stderr, "ERROR - Parser: Expected Binary op token, found %d", op_type);
      exit(1);
  }

  switch (op_type) {
    case TOKEN_PLUS_EQUAL:          
    case TOKEN_NEGATION_EQUAL:      
    case TOKEN_ASTERISK_EQUAL:      
    case TOKEN_FORWARD_SLASH_EQUAL: 
    case TOKEN_PERCENT_EQUAL:       
    case TOKEN_BITWISE_AND_EQUAL:   
    case TOKEN_BITWISE_OR_EQUAL:   
    case TOKEN_BITWISE_XOR_EQUAL:   
    case TOKEN_BITWISE_RIGHT_SHIFT_EQUAL:   
    case TOKEN_BITWISE_LEFT_SHIFT_EQUAL: {  
      AstNode *assignment_expression = malloc(sizeof(AstNode));
      assignment_expression->type = AST_EXPRESSION_ASSIGNMENT;
      assignment_expression->data.assignement_expression.left_expression = left_expression;
      assignment_expression->data.assignement_expression.right_expression = binary_expression;
      return assignment_expression;
    }
    default:
     return binary_expression;
  }
}

void ast_parse_factor(Parser *parser, AstNode *factor_node) {
 if (end_of_file(parser)) {
    fprintf(stderr, "ERROR - Parser: Incomplete expression (line %d)\n", previous_token(parser)->line);
    exit(1);
  }

  switch(current_token(parser)->type) {
    case TOKEN_CONSTANT_INT:
      ast_parse_factor_constant(parser, factor_node);
    case TOKEN_NEGATION:
    case TOKEN_BITWISE_NOT:
    case TOKEN_LOGICAL_NOT:
      ast_parse_factor_unary(parser, factor_node);
    case TOKEN_INCREMENT:
    case TOKEN_DECREMENT:
      ast_parse_factor_prefix_expression(parser, factor_node);
    case TOKEN_OPEN_PAREN:
      return ast_parse_factor_parenthetical_expression(parser);
    case TOKEN_IDENTIFIER: {    
      char *identifier = ast_identifier(parser);

      switch(current_token(parser)->type) {
        case TOKEN_COLON:
          return ast_parse_factor_goto_label(parser, identifier);
        case TOKEN_OPEN_PAREN:
          return ast_parse_factor_function_call(parser, identifier);   
        default:
          return ast_parse_factor_variable_expression(parser, identifier);
      }      
    }
    default:
      fprintf(stderr, "ERROR - Parser: Failed to parse factor for '%s' token (line %d)\n", TokenTypeStr[current_token(parser)->type], current_token(parser)->line);
      exit(1);    
  }
}

void ast_parse_factor_constant(Parser *parser, AstNode *factor_node) {
  ast_expect(parser, TOKEN_CONSTANT_INT); 

  factor_node->type = AST_EXPRESSION_CONSTANT;

  char slice[previous_token(parser)->end_index - previous_token(parser)->start_index]; 
  strncpy(slice, parser->file + previous_token(parser)->start_index, (previous_token(parser)->end_index - previous_token(parser)->start_index) + 1);
  
  int constant_value = atoi(slice);
  factor_node->data.constant_expression.value = constant_value;
}

void ast_parse_factor_unary(Parser *parser, AstNode *factor_node) {
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

  AstNode *unary_value_expression_node = arena_alloc(parser->node_arena);
  ast_parse_factor(parser, unary_value_expression_node);

  factor_node->type = AST_EXPRESSION_UNARY;
  factor_node->data.unary_expression.op_type = op_type;
  factor_node->data.unary_expression.expression = unary_value_expression_node;
}

void ast_parse_factor_prefix_expression(Parser *parser, AstNode *factor_node) {
  AstNode *prefix_expression = malloc(sizeof(AstNode));

  if (current_token(parser)->type == TOKEN_INCREMENT) {
    ast_expect(parser, TOKEN_INCREMENT);
    prefix_expression->type = AST_EXPRESSION_PREFIX_INCREMENT;
  } else {
    ast_expect(parser, TOKEN_DECREMENT);
    prefix_expression->type = AST_EXPRESSION_PREFIX_DECREMENT;
  }

  AstNode *left = ast_parse_expression(parser, 0);

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
}

AstNode* ast_parse_factor_parenthetical_expression(Parser *parser) {
  parser->current_token_index++;

  AstNode *expression = ast_parse_expression(parser, 0);
    
  ast_expect(parser, TOKEN_CLOSE_PAREN);

  return expression;
}
 
AstNode* ast_parse_factor_goto_label(Parser *parser, char *label_identifier) {
  ast_expect(parser, TOKEN_COLON);
  AstNode *goto_label_node = malloc(sizeof(AstNode));
  goto_label_node->type = AST_STATEMENT_GOTO_LABEL;
  goto_label_node->data.goto_label_statement.label = label_identifier;

  return goto_label_node;
}

AstNode* ast_parse_factor_variable_expression(Parser *parser, char *label_identifier) {
  AstNode *identifier_node = malloc(sizeof(AstNode));
  identifier_node->type = AST_EXPRESSION_VARIABLE;
  identifier_node->data.variable_expression.identifier = label_identifier;

  return identifier_node;
}

AstNode* ast_parse_factor_function_call(Parser *parser, char *identifier) {
  ast_expect(parser, TOKEN_OPEN_PAREN);

  AstNode *function_call_node = malloc(sizeof(AstNode));
  function_call_node->type = AST_EXPRESSION_FUNCTION_CALL;
  function_call_node->data.function_call_expression.identfier = identifier;
  function_call_node->data.function_call_expression.argument_count = 0;
  function_call_node->data.function_call_expression.argument_capacity = 0;
  function_call_node->data.function_call_expression.arguments = NULL;

  if (current_token(parser)->type == TOKEN_CLOSE_PAREN) {
    ast_expect(parser, TOKEN_CLOSE_PAREN);
    return function_call_node;
  }  

  AstNode *expression = ast_parse_expression(parser, 0);
  add_to_function_call(function_call_node, expression);

  while (current_token(parser)->type == TOKEN_COMMA) {
    ast_expect(parser, TOKEN_COMMA);
    expression = ast_parse_expression(parser, 0);
    add_to_function_call(function_call_node, expression);
  }

  ast_expect(parser, TOKEN_CLOSE_PAREN);

  return function_call_node;
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

