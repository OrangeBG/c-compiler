#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include "../include/parser.h"
#include "../include/arena.h"
#include "../include/lexer.h"

#define ADD_WHITESPACE (whitespace + 5)
#define POINTER_ARENA_INIT_CAPACITY 8
#define FUNCTION_IDENTIFIER_INIT_CAPACITY 4
#define FUNCTION_PARAMETER_TYPE_INIT_CAPACITY 4
#define NODE_POINTER_CAPACITY 8
#define BASE_TEN 10

typedef struct Parser {
  int token_count;
  int current_token_index;
  int current_loop_label_id;
  Token *tokens;
  char* file;
  Arena* node_arena;
} Parser;

typedef struct Specifier {
  StorageClassType storage_class_type;
  Types specifier_type;
  bool specifier_type_found;
} Specifier;
 
static void       parse_program(Parser *parser, AstNode *program_node);
static void       parse_declaration(Parser *parser, AstNode *declaration_node); 
static void       parse_function_declaration(Parser *parser, AstNode *function_node, StorageClassType storage_class_type, Types return_type_specifier);
static void       parse_variable_declaration(Parser *parser, AstNode *variable_node, StorageClassType storage_class_type, Types variable_type_specifier);
static void       parse_block(Parser *parser, AstNode *block_node);
static void       parse_statement(Parser *parser, AstNode **statement_node);
static void       parse_statement_null(Parser *parser, AstNode *statement_node);
static void       parse_statement_return(Parser *parser, AstNode *statement_node); 
static void       parse_statement_if(Parser *parser, AstNode *statement_node); 
static void       parse_statement_goto(Parser *parser, AstNode *statement_node); 
static void       parse_statement_break(Parser *parser, AstNode *break_statement_node); 
static void       parse_statement_continue(Parser *parser, AstNode *continue_statement_node); 
static void       parse_statement_while(Parser *parser, AstNode *while_statement_node); 
static void       parse_statement_do(Parser *parser, AstNode *do_statement_node); 
static void       parse_statement_for(Parser *parser, AstNode *for_statement_node); 
static void       parse_statement_compound_statement(Parser *parser, AstNode *compound_statement_node); 
static void       parse_expression(Parser *parser, AstNode **expression_node, int min_precedence);
static void       parse_expression_postfix(Parser *parser, AstNode *postfix_expression, AstNode *left_expression,  TokenType postfix_token);
static void       parse_expression_assignment(Parser *parser, AstNode *assignment_expression, AstNode *left_factor, TokenType assignment_token); 
static void       parse_expression_conditional(Parser *parser, AstNode *conditional_expression_node, AstNode *left_expression, TokenType conditional_token); 
static void       parse_expression_binary(Parser *parser, AstNode **binary_expression_node, AstNode *left_expression, TokenType op_type);
static void       parse_factor(Parser *parser, AstNode *factor_node);
static void       parse_factor_constant(Parser *parser, AstNode *factor_node, TokenType constant_type);
static void       parse_factor_unary(Parser *parser, AstNode *factor_node); 
static void       parse_factor_prefix_expression(Parser *parser, AstNode *factor_node); 
static void       parse_factor_parenthetical_expression(Parser *parser, AstNode *factor_node); 
static void       parse_factor_cast_expression(Parser *parser, AstNode *factor_node); 
static void       parse_factor_goto_label(Parser *parser, AstNode *factor_node); 
static void       parse_factor_variable_expression(Parser *parser, AstNode *factor_node, char *label_identifier);
static void       parse_factor_function_call(Parser *parser, AstNode *factor_node, char *identifier); 
static Specifier  parse_specifier(Parser *parser, bool error_if_storage_class_found);
static Token*     current_token(const Parser *parser);
static Token*     previous_token(const Parser *parser);
static TokenType  peek_next_token(const Parser *parser); 
static char*      get_identifier(Parser *parser);
static void       expect(Parser *parser, TokenType expected_type);
static void       print_whitespace(int count); 
static void       add_to_node_pointer(AstNode *node, NodePointer *node_pointer); 
static void       init_node_pointer(NodePointer *node_pointer); 
static bool       end_of_file(const Parser *parser);
static bool       is_type_identifier_token(TokenType token_type);
static int        get_precedence(TokenType token_type);
static void       add_function_parameter_identifier(char *identifier, AstNode *function_declaration_node);   
static void       add_function_parameter_type(AstNode *function_parameter_type, AstNode *function_type);   

Arena* parse_ast(Token *tokens, int token_count, char *file) {  
  Arena *parser_arena = malloc(sizeof(Arena));
  //TODO: Hardcoded capacity
  arena_init(parser_arena, sizeof(AstNode), sizeof(AstNode) * 1000, false);
  
  Parser parser = {
    .token_count = token_count,
    .current_token_index = 0,
    .tokens = tokens,
    .file = file,
    .current_loop_label_id = 0,
    .node_arena = parser_arena
  };
  
  AstNode *program_node = arena_alloc(parser.node_arena);
  parse_program(&parser, program_node);

  expect(&parser, TOKEN_EOF);

  if (token_count > parser.current_token_index) {
    fprintf(stderr, "ERROR - Parser: Identifier declared outside of program scope (line %d)\n", parser.tokens[parser.current_token_index].line);
    exit(1);
  }

  return parser_arena;
}

void print_ast(const AstNode *node, int whitespace) {
  switch(node->type){
    case AST_PROGRAM:  
      printf("Program (\n");
      for (int i = 0; i < node->data.program.declaration_count; i++) {
        AstNode *function = node->data.program.declaration_ptrs->node_pointers[i];
        print_ast(function, ADD_WHITESPACE);
      }
      printf(")\n");
      break;
    case AST_VARIABLE_DECLARATION:
      print_whitespace(whitespace);
      printf("Variable Declaration (id = \"%s\" ", node->data.variable_declaration.name);

      switch (node->data.variable_declaration.storage_class_type) {
        case AST_STORAGE_CLASS_NONE: printf("storage class = \"None\""); break;
        case AST_STORAGE_CLASS_EXTERN : printf("storage class = \"Extern\""); break;
        case AST_STORAGE_CLASS_STATIC : printf("storage class = \"Static\""); break;        
      }
      
      printf(", type = ");
      print_ast(node->data.variable_declaration.type, 0);

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
        print_whitespace(ADD_WHITESPACE);
        printf("Param( name = %s\n", node->data.function_declaration.parameter_identifiers[i]);

        print_ast(&node->data.function_declaration.function_type->data.type.function_param_types[i], ADD_WHITESPACE + 5);
        
        print_whitespace(ADD_WHITESPACE);
        printf(")\n");
      }

      if (node->data.function_declaration.body_block != NULL) {
        print_whitespace(whitespace);
        printf("body=\n");
        print_ast(node->data.function_declaration.body_block, ADD_WHITESPACE);
      }

      print_whitespace(whitespace);
      printf(")\n");      
      break;
    case AST_TYPE:
      print_whitespace(whitespace);
      printf("Type(");

      switch (node->data.type.type) {
        case TYPE_VOID:     printf("void"); break;
        case TYPE_INT:      printf("int"); break;
        case TYPE_UINT:     printf("uint"); break;
        case TYPE_LONG:     printf("long"); break;
        case TYPE_ULONG:    printf("ulong"); break;
        case TYPE_DOUBLE:   printf("double"); break;
        case TYPE_FUNCTION: printf("function"); break;
        default:
          fprintf(stderr, "ERROR - Parser: Could not find AST Type '%d' when printing\n", node->data.type.type);
          exit(1);
      }
      printf(")\n");      
      break;
    case AST_BLOCK:
      print_whitespace(whitespace);
      printf("Block (\n");
      for (int i = 0; i < node->data.block.block_count; i++) {
        AstNode *block_item = node->data.block.block_ptrs->node_pointers[i];
        print_ast(block_item, ADD_WHITESPACE);
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
     // case AST_STATEMENT_EXPRESSION:
     //  print_whitespace(whitespace);
     //  printf("Expression Statement(\n");
     //  print_ast(node->data.expression, ADD_WHITESPACE);
     //  printf(")\n");
     //  break;
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
    case AST_STATEMENT_COMPOUND:
      print_ast(node->data.compound_statement.block, whitespace);
      break;
    case AST_EXPRESSION_CONSTANT:
      print_whitespace(whitespace);

      switch (node->data.constant_expression.constant_type) {
        case AST_CONSTANT_TYPE_INT:
          printf("Constant(Int (%d))\n", node->data.constant_expression.int_value);
          break;
        case AST_CONSTANT_TYPE_UINT:
          printf("Constant(UInt (%d))\n", node->data.constant_expression.uint_value);
          break;
        case AST_CONSTANT_TYPE_LONG:
          printf("Constant(Long(%ld))\n", node->data.constant_expression.long_value);
          break;
        case AST_CONSTANT_TYPE_ULONG:
          printf("Constant(ULong(%ld))\n", node->data.constant_expression.ulong_value);
          break;
        case AST_CONSTANT_TYPE_DOUBLE:
          printf("Constant(Double(%f))\n", node->data.constant_expression.double_value);
          break;
        default:
          fprintf(stderr, "ERROR - Parser: Could not find constant type when printing\n");
          exit(1);
          break;
      }
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
      print_whitespace(ADD_WHITESPACE);
      printf("True Expression = \n");
      print_ast(node->data.conditional_expression.true_expression, ADD_WHITESPACE + 5);
      print_whitespace(ADD_WHITESPACE);
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
        printf("Function Call(name= '%s' args=\n", node->data.function_call_expression.identfier);

        for (int i = 0; i < node->data.function_call_expression.argument_count; i++) {
          AstNode *argument = node->data.function_call_expression.argument_ptrs->node_pointers[i];
          print_ast(argument, ADD_WHITESPACE);
        }

        print_whitespace(whitespace);
        printf(")\n");
        break;
      }
      case AST_EXPRESSION_CAST: {
        print_whitespace(whitespace);
        printf("Cast(type=");

        switch (node->data.cast_expression.target_type->data.type.type) {
          case TYPE_INT:    printf("int\n"); break;
          case TYPE_UINT:   printf("uint\n"); break;
          case TYPE_LONG:   printf("long\n"); break;
          case TYPE_ULONG:  printf("ulong\n"); break;
          case TYPE_DOUBLE: printf("double\n"); break;
          default:            
            printf("ERROR - Parser: Unsupported cast node type to print %d\n", node->type);
        }

        print_ast(node->data.cast_expression.expression, ADD_WHITESPACE);        

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

//TODO: Maybe make into macro?
static void print_whitespace(int count) {
  printf("%*s", count, "");
}

static Token* current_token(const Parser *parser) {
  return &parser->tokens[parser->current_token_index];
}

static Token* previous_token(const Parser *parser) {
  return &parser->tokens[parser->current_token_index - 1];
}

static TokenType peek_next_token(const Parser *parser) {
  if (current_token(parser)->type == TOKEN_EOF) {
    return TOKEN_EOF;
  }

  return parser->tokens[parser->current_token_index + 1].type;
}

static bool end_of_file(const Parser *parser) {
  return parser->tokens[parser->current_token_index].type == TOKEN_EOF;
}

static void add_to_node_pointer(AstNode *node, NodePointer *node_pointer) {
  if (node_pointer == NULL) {
    return;
  }
  
  if (node_pointer->count == node_pointer->capacity) {
    int new_size = node_pointer->capacity == 0 ? NODE_POINTER_CAPACITY : node_pointer->capacity * 2;

    AstNode **realloc_pointers = realloc(node_pointer->node_pointers, new_size * sizeof(AstNode**));

    node_pointer->capacity = new_size;
    node_pointer->node_pointers = realloc_pointers;
  } 

  node_pointer->node_pointers[node_pointer->count] = node;
  node_pointer->count++;
}

static void init_node_pointer(NodePointer *node_pointer) {
  if (node_pointer == NULL) {
    return;
  }
  
  node_pointer->capacity = 0;
  node_pointer->count = 0;
  node_pointer->node_pointers = NULL;
} 

static void expect(Parser *parser, TokenType expected_type) {
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

static void parse_program(Parser *parser, AstNode *program_node) {
  program_node->type = AST_PROGRAM;
  program_node->data.program.declaration_count = 0;

  NodePointer *declaration_pointers = malloc(sizeof(NodePointer));
  init_node_pointer(declaration_pointers);

  program_node->data.program.declaration_ptrs = declaration_pointers;
  
  while (current_token(parser)->type != TOKEN_EOF) {
    AstNode *declaration_node = arena_alloc(parser->node_arena);
    parse_declaration(parser, declaration_node);

    program_node->data.program.declaration_count++;
    add_to_node_pointer(declaration_node, declaration_pointers);
  } 
}

static void parse_declaration(Parser *parser, AstNode *declaration_node) {
  Specifier specifier = parse_specifier(parser, false);

  //Variable Declaration -> int c; or int c = 0; 
  if (peek_next_token(parser) == TOKEN_EQUAL || peek_next_token(parser) == TOKEN_SEMICOLON) {
    parse_variable_declaration(parser, declaration_node, specifier.storage_class_type, specifier.specifier_type);
    return;
  }

  parse_function_declaration(parser, declaration_node, specifier.storage_class_type, specifier.specifier_type);
}

static void parse_function_declaration(Parser *parser, AstNode *function_node, StorageClassType storage_class_type, Types return_type_specifier) {
  function_node->data.function_declaration.parameter_count = 0;
  function_node->data.function_declaration.parameter_identifier_capacity = 0;
  function_node->data.function_declaration.parameter_identifiers = NULL;
  function_node->data.function_declaration.storage_class_type = storage_class_type;

  AstNode *return_type_node = arena_alloc(parser->node_arena);
  return_type_node->type = AST_TYPE;
  return_type_node->data.type.type = return_type_specifier;

  AstNode *function_type = arena_alloc(parser->node_arena);
  function_type->type = AST_TYPE;
  function_type->data.type.type = TYPE_FUNCTION;
  function_type->data.type.function_return_type = return_type_node;
  function_type->data.type.function_param_type_count = 0;

  function_node->data.function_declaration.function_type = function_type;
  
  char *id_name = get_identifier(parser);  

  function_node->data.function_declaration.name = id_name;

  expect(parser, TOKEN_OPEN_PAREN);

  AstNode *parameter_type = arena_alloc(parser->node_arena);
  parameter_type->type = AST_TYPE;

  Specifier parameter_specifier = parse_specifier(parser, true);

  parameter_type->data.type.type = parameter_specifier.specifier_type;

  if (parameter_specifier.specifier_type != TYPE_VOID) {
    char *identifier = get_identifier(parser);
    add_function_parameter_identifier(identifier, function_node);
  }
  
  add_function_parameter_type(parameter_type, function_type);

  while(current_token(parser)->type == TOKEN_COMMA) {
    expect(parser, TOKEN_COMMA);

    AstNode *next_parameter_type = arena_alloc(parser->node_arena);
    next_parameter_type->type = AST_TYPE;
    
    Specifier next_parameter_specifier = parse_specifier(parser, true);
    next_parameter_type->data.type.type = next_parameter_specifier.specifier_type;

    if (next_parameter_specifier.specifier_type != TYPE_VOID) {
      char *identifier = get_identifier(parser);
      add_function_parameter_identifier(identifier, function_node);
    }

    add_function_parameter_type(next_parameter_type, function_type);
  }
  
  expect(parser, TOKEN_CLOSE_PAREN);

  function_node->type = AST_FUNCTION_DECLARATION;
  function_node->data.function_declaration.name = id_name;

  //If semicolon is found, then it is considered a function definition
  if (current_token(parser)->type == TOKEN_SEMICOLON) {
    expect(parser, TOKEN_SEMICOLON);
    return;
  }
  
  AstNode *block_node = arena_alloc(parser->node_arena);
  function_node->data.function_declaration.body_block = block_node;
  parse_block(parser, block_node);
}

static void parse_variable_declaration(Parser *parser, AstNode *variable_node, StorageClassType storage_class_type, Types variable_type_specifier) {
  char *identifier = get_identifier(parser);

  variable_node->type = AST_VARIABLE_DECLARATION;
  variable_node->data.variable_declaration.name = identifier;
  variable_node->data.variable_declaration.storage_class_type = storage_class_type;

  AstNode *variable_type_node = arena_alloc(parser->node_arena);
  variable_type_node->type = AST_TYPE;
  variable_type_node->data.type.type = variable_type_specifier;

  variable_node->data.variable_declaration.type = variable_type_node;

  if (current_token(parser)->type == TOKEN_EQUAL) {
    //TODO: Fix as ast_identifier eats the token but we need it to feed into ast_expression();
    parser->current_token_index--;

    AstNode *expression_node = arena_alloc(parser->node_arena);
    parse_expression(parser, &expression_node, 0);

    variable_node->data.variable_declaration.has_expression = true;
    variable_node->data.variable_declaration.init_expression = expression_node;
  }

  expect(parser, TOKEN_SEMICOLON);
}

static void parse_block(Parser *parser, AstNode *block_node) {
  expect(parser, TOKEN_OPEN_BRACE);

  block_node->type = AST_BLOCK;
  block_node->data.block.block_count = 0;

  NodePointer *block_item_pointers = malloc(sizeof(NodePointer));
  init_node_pointer(block_item_pointers);
  block_node->data.block.block_ptrs = block_item_pointers;

  //TODO: While(true) loop seems dangerous if no close brace is supplied
  while(true) {
    if (current_token(parser)->type == TOKEN_CLOSE_BRACE) {
      expect(parser, TOKEN_CLOSE_BRACE);
      return;
    }

    //TODO: This if check looks like it's going to grow larger as we add more types. Look to see if there is a better way to check declarations from statements
    if (current_token(parser)->type == TOKEN_INT || current_token(parser)->type == TOKEN_LONG || current_token(parser)->type == TOKEN_DOUBLE || current_token(parser)->type == TOKEN_UNSIGNED || current_token(parser)->type == TOKEN_SIGNED || current_token(parser)->type == TOKEN_EXTERN || current_token(parser)->type == TOKEN_STATIC) {
      AstNode *declaration_node = arena_alloc(parser->node_arena);
      parse_declaration(parser, declaration_node);
      block_node->data.block.block_count++;
      add_to_node_pointer(declaration_node, block_item_pointers);
    } else {
      AstNode *statement_node = arena_alloc(parser->node_arena);
      parse_statement(parser, &statement_node);
      block_node->data.block.block_count++;
      add_to_node_pointer(statement_node, block_item_pointers);
    }
  }
}

static void parse_statement_compound_statement(Parser *parser, AstNode *compound_statement_node) {
  AstNode *block_node = arena_alloc(parser->node_arena);

  compound_statement_node->type = AST_STATEMENT_COMPOUND;
  compound_statement_node->data.compound_statement.block = block_node;

  parse_block(parser, block_node);
}

static char* get_identifier(Parser *parser) {
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

static void parse_statement(Parser *parser, AstNode **statement_node) { 
  if (end_of_file(parser)) {
    fprintf(stderr, "ERROR - Parser: Incomplete statement (line %d)\n", previous_token(parser)->line);
    exit(1);
  }

  switch (current_token(parser)->type) {
    case TOKEN_OPEN_BRACE: parse_statement_compound_statement(parser, *statement_node); break;
    case TOKEN_SEMICOLON:  parse_statement_null(parser, *statement_node); break;
    case TOKEN_RETURN:     parse_statement_return(parser, *statement_node); break;
    case TOKEN_IF:         parse_statement_if(parser, *statement_node); break;
    case TOKEN_GOTO:       parse_statement_goto(parser, *statement_node); break;
    case TOKEN_BREAK:      parse_statement_break(parser, *statement_node); break;
    case TOKEN_CONTINUE:   parse_statement_continue(parser, *statement_node); break;
    case TOKEN_WHILE:      parse_statement_while(parser, *statement_node); break;
    case TOKEN_DO:         parse_statement_do(parser, *statement_node); break;
    case TOKEN_FOR:        parse_statement_for(parser, *statement_node); break;
    default: {
      if (current_token(parser)->type == TOKEN_IDENTIFIER && peek_next_token(parser) == TOKEN_COLON) {
        parse_factor_goto_label(parser, *statement_node);

        if (current_token(parser)->type == TOKEN_CLOSE_BRACE) {
          //TODO: This is not a requirement for C23
          fprintf(stderr, "ERROR - Parser: Label must have a following statement (line %d)\n", current_token(parser)->line);
          exit(1);
        }
        break;
      }
      
      parse_expression(parser, statement_node, 0);  

      //TODO: See if we add this in ast_expression() instead of doing this goto label check
      if ((*statement_node)->type != AST_STATEMENT_GOTO_LABEL) {
        expect(parser, TOKEN_SEMICOLON);
      }
      break;
    }
  }
}

static void parse_statement_null(Parser *parser, AstNode *statement_node) {
  expect(parser, TOKEN_SEMICOLON);
  statement_node->type = AST_STATEMENT_NULL;
}

static void parse_statement_return(Parser *parser, AstNode *statement_node) {
  expect(parser, TOKEN_RETURN);

  AstNode *expression = arena_alloc(parser->node_arena);
  parse_expression(parser, &expression, 0);
  
  statement_node->type = AST_STATEMENT_RETURN;
  statement_node->data.return_statement.expression = expression;

  expect(parser, TOKEN_SEMICOLON);
}

static void parse_statement_if(Parser *parser, AstNode *if_statement_node) {
  expect(parser, TOKEN_IF);
  expect(parser, TOKEN_OPEN_PAREN);

  AstNode *condition_expression = arena_alloc(parser->node_arena);
  parse_expression(parser, &condition_expression, 0);

  expect(parser, TOKEN_CLOSE_PAREN);


  AstNode *statement = arena_alloc(parser->node_arena);
  parse_statement(parser, &statement);

  if_statement_node->type = AST_STATEMENT_IF;
  if_statement_node->data.if_statement.condition_expression = condition_expression;
  if_statement_node->data.if_statement.then_statement = statement;

  if (current_token(parser)->type != TOKEN_ELSE) {
    return;
  }

  expect(parser, TOKEN_ELSE);
  AstNode *else_statement = arena_alloc(parser->node_arena);
  parse_statement(parser, &else_statement);

  if_statement_node->data.if_statement.else_statement = else_statement;
}

static void parse_statement_goto(Parser *parser, AstNode *goto_statement_node) {
  expect(parser, TOKEN_GOTO);

  char *goto_label = get_identifier(parser);

  goto_statement_node->type = AST_STATEMENT_GOTO;
  goto_statement_node->data.goto_statement.label = goto_label;

  expect(parser, TOKEN_SEMICOLON);
}

static void parse_statement_break(Parser *parser, AstNode *break_statement_node) {
  expect(parser, TOKEN_BREAK);
  expect(parser, TOKEN_SEMICOLON);

  break_statement_node->type = AST_STATEMENT_BREAK;
}
  
static void parse_statement_continue(Parser *parser, AstNode *continue_statement_node) {
  expect(parser, TOKEN_CONTINUE);
  expect(parser, TOKEN_SEMICOLON);

  continue_statement_node->type = AST_STATEMENT_CONTINUE;
}

static void parse_statement_while(Parser *parser, AstNode *while_statement_node) {
  expect(parser, TOKEN_WHILE);
  expect(parser, TOKEN_OPEN_PAREN);

  AstNode *condition_expression = arena_alloc(parser->node_arena);
  parse_expression(parser, &condition_expression, 0);
  
  expect(parser, TOKEN_CLOSE_PAREN);

  AstNode *statements = arena_alloc(parser->node_arena);
  parse_statement(parser, &statements);

  while_statement_node->type = AST_STATEMENT_WHILE;
  while_statement_node->data.while_statement.condition = condition_expression;
  while_statement_node->data.while_statement.statement_body = statements;
}

static void parse_statement_do(Parser *parser, AstNode *do_statement_node) {
  expect(parser, TOKEN_DO);

  AstNode *statements = arena_alloc(parser->node_arena);
  parse_statement(parser, &statements);
  
  expect(parser, TOKEN_WHILE);
  expect(parser, TOKEN_OPEN_PAREN);

  AstNode *condition_expression = arena_alloc(parser->node_arena);
  parse_expression(parser, &condition_expression, 0);
  
  expect(parser, TOKEN_CLOSE_PAREN);
  expect(parser, TOKEN_SEMICOLON);

  do_statement_node->type = AST_STATEMENT_DO_WHILE;
  do_statement_node->data.do_while_statement.condition = condition_expression;
  do_statement_node->data.do_while_statement.statement_body = statements;
}

static void parse_statement_for(Parser *parser, AstNode *for_statement_node) {
  expect(parser, TOKEN_FOR);
  expect(parser, TOKEN_OPEN_PAREN);

  for_statement_node->type = AST_STATEMENT_FOR;

  AstNode *dec_or_exp = arena_alloc(parser->node_arena);

  Specifier type_specifier = parse_specifier(parser, true);

  if (type_specifier.specifier_type_found) {
    parse_variable_declaration(parser, dec_or_exp, AST_STORAGE_CLASS_NONE, type_specifier.specifier_type);
  } else if (current_token(parser)->type == TOKEN_SEMICOLON) {
    expect(parser, TOKEN_SEMICOLON);    
    dec_or_exp = NULL;
  } else {
    parse_expression(parser, &dec_or_exp, 0);
    //TODO: Weird we do this for expressions but are handled in ast_declaration()
    expect(parser, TOKEN_SEMICOLON);
  }

  for_statement_node->data.for_statement.for_loop_init = dec_or_exp;

  if (current_token(parser)->type != TOKEN_SEMICOLON) {
    AstNode *for_condition = arena_alloc(parser->node_arena);
    parse_expression(parser, &for_condition, 0);
    for_statement_node->data.for_statement.condition_expression = for_condition;
  }

  expect(parser, TOKEN_SEMICOLON);

  if (current_token(parser)->type != TOKEN_SEMICOLON && current_token(parser)->type != TOKEN_CLOSE_PAREN) {
    AstNode *post_expression = arena_alloc(parser->node_arena);
    parse_expression(parser, &post_expression, 0);
    for_statement_node->data.for_statement.post_expression = post_expression;
  }

  expect(parser, TOKEN_CLOSE_PAREN);

  AstNode *for_statements = arena_alloc(parser->node_arena);
  parse_statement(parser, &for_statements);

  for_statement_node->data.for_statement.statement_body = for_statements;    
}

static void parse_expression(Parser *parser, AstNode **expression_node, int min_precedence) {
  parse_factor(parser, *expression_node);

  TokenType next_token = current_token(parser)->type;

  while (get_precedence(next_token) >= min_precedence) {
    switch(next_token) {
      case TOKEN_INCREMENT:
      case TOKEN_DECREMENT: {
        AstNode *postfix_expression = arena_alloc(parser->node_arena);
        parse_expression_postfix(parser, postfix_expression, *expression_node, next_token);
          *expression_node = postfix_expression;
        break;
      }
      case TOKEN_EQUAL: {
        AstNode *assignment_expression = arena_alloc(parser->node_arena);
        parse_expression_assignment(parser, assignment_expression, *expression_node, next_token);
          *expression_node = assignment_expression;
        break;
      }
      case TOKEN_QUESTION_MARK: {
        AstNode *conditional_expression = arena_alloc(parser->node_arena);
        parse_expression_conditional(parser, conditional_expression, *expression_node, next_token);
          *expression_node = conditional_expression;
        break;
      }
      default: {
        AstNode *binary_expression = arena_alloc(parser->node_arena);
        parse_expression_binary(parser, &binary_expression, *expression_node, next_token);
        *expression_node = binary_expression;
        break;
      }
    }
    next_token = current_token(parser)->type;
  } 
}

static void parse_expression_postfix(Parser *parser, AstNode *postfix_expression, AstNode *left_expression,  TokenType postfix_token) {
  parser-> current_token_index++;

  if (postfix_token == TOKEN_INCREMENT) {
    postfix_expression->type = AST_EXPRESSION_POSTFIX_INCREMENT;
  } else {
    postfix_expression->type = AST_EXPRESSION_POSTFIX_DECREMENT;
  }

  AstNode *postfix_assignment = arena_alloc(parser->node_arena);
  postfix_assignment->type = AST_EXPRESSION_ASSIGNMENT;
  postfix_assignment->data.assignement_expression.left_expression = left_expression;

  AstNode *postfix_constant = arena_alloc(parser->node_arena);
  postfix_constant->type = AST_EXPRESSION_CONSTANT;
  //TODO: Look into why I'm doing this
  postfix_constant->data.constant_expression.int_value = 1;
  postfix_constant->data.constant_expression.expression_type = NULL;
  
  AstNode *postfix_binary = arena_alloc(parser->node_arena);
  postfix_binary->type = AST_EXPRESSION_BINARY;
  
  if (postfix_token == TOKEN_INCREMENT) {
    postfix_binary->data.binary_expression.op_type = AST_BINARY_ADD;
  } else {
    postfix_binary->data.binary_expression.op_type = AST_BINARY_SUBTRACT;
  }
  
  postfix_binary->data.binary_expression.left_expression = left_expression;  
  postfix_binary->data.binary_expression.right_expression = postfix_constant;
  postfix_binary->data.binary_expression.expression_type = NULL;
  postfix_assignment->data.assignement_expression.right_expression = postfix_binary;
  postfix_expression->data.increment_decrement_expression.expression = postfix_assignment;
}

static void parse_expression_assignment(Parser *parser, AstNode *assignment_expression, AstNode *left_factor, TokenType assignment_token) {
  //right-associative assignment
  expect(parser, TOKEN_EQUAL);

  AstNode *right = arena_alloc(parser->node_arena);
  parse_expression(parser, &right, get_precedence(assignment_token));

  assignment_expression->type = AST_EXPRESSION_ASSIGNMENT;
  assignment_expression->data.assignement_expression.left_expression = left_factor;
  assignment_expression->data.assignement_expression.right_expression = right;
  assignment_expression->data.assignement_expression.expression_type = NULL;
}

// TODO: conditional_token may always be question mark. If so, remove param and assign get_precedence in the function
// to TOKEN_QUESTION_MARK
static void parse_expression_conditional(Parser *parser, AstNode *conditional_expression_node, AstNode *left_expression, TokenType conditional_token) {
  expect(parser, TOKEN_QUESTION_MARK);

  AstNode *middle = arena_alloc(parser->node_arena);
  parse_expression(parser, &middle, 0);

  expect(parser, TOKEN_COLON);

  AstNode *right = arena_alloc(parser->node_arena);
  parse_expression(parser, &right, get_precedence(conditional_token));

  conditional_expression_node->type = AST_EXPRESSION_CONDITIONAL;
  conditional_expression_node->data.conditional_expression.condition = left_expression;
  conditional_expression_node->data.conditional_expression.true_expression = middle;
  conditional_expression_node->data.conditional_expression.false_expression = right;
  conditional_expression_node->data.conditional_expression.expression_type = NULL;
}

static void parse_expression_binary(Parser *parser, AstNode **binary_expression, AstNode *left_expression, TokenType op_type) {
  parser-> current_token_index++;

  AstNode *right = arena_alloc(parser->node_arena);
  parse_expression(parser, &right, get_precedence(op_type) + 1);

  AstNode *binary_expression_pointer = *binary_expression;
  binary_expression_pointer->type = AST_EXPRESSION_BINARY;
  binary_expression_pointer->data.binary_expression.left_expression = left_expression;
  binary_expression_pointer->data.binary_expression.right_expression = right;
  binary_expression_pointer->data.binary_expression.expression_type = NULL;
 
  switch (op_type) {
    case TOKEN_PLUS:                        binary_expression_pointer->data.binary_expression.op_type = AST_BINARY_ADD; break;
    case TOKEN_NEGATION:                    binary_expression_pointer->data.binary_expression.op_type = AST_BINARY_SUBTRACT; break;
    case TOKEN_ASTERISK:                    binary_expression_pointer->data.binary_expression.op_type = AST_BINARY_MULTIPLY; break;
    case TOKEN_FORWARD_SLASH:               binary_expression_pointer->data.binary_expression.op_type = AST_BINARY_DIVIDE; break;
    case TOKEN_PERCENT:                     binary_expression_pointer->data.binary_expression.op_type = AST_BINARY_REMAINDER; break;
    case TOKEN_BITWISE_AND:                 binary_expression_pointer->data.binary_expression.op_type = AST_BINARY_BITWISE_AND; break;
    case TOKEN_BITWISE_OR:                  binary_expression_pointer->data.binary_expression.op_type = AST_BINARY_BITWISE_OR; break;
    case TOKEN_BITWISE_XOR:                 binary_expression_pointer->data.binary_expression.op_type = AST_BINARY_BITWISE_XOR; break;
    case TOKEN_BITWISE_LEFT_SHIFT:          binary_expression_pointer->data.binary_expression.op_type = AST_BINARY_BITWISE_LEFT_SHIFT; break;
    case TOKEN_BITWISE_RIGHT_SHIFT:         binary_expression_pointer->data.binary_expression.op_type = AST_BINARY_BITWISE_RIGHT_SHIFT; break;
    case TOKEN_RELATIONAL_LESS_THAN:        binary_expression_pointer->data.binary_expression.op_type = AST_BINARY_LESS_THAN; break;
    case TOKEN_RELATIONAL_LESS_OR_EQUAL:    binary_expression_pointer->data.binary_expression.op_type = AST_BINARY_LESS_OR_EQUAL; break;
    case TOKEN_RELATIONAL_GREATER_THAN:     binary_expression_pointer->data.binary_expression.op_type = AST_BINARY_GREATER_THAN; break;
    case TOKEN_RELATIONAL_GREATER_OR_EQUAL: binary_expression_pointer->data.binary_expression.op_type = AST_BINARY_GREATER_OR_EQUAL; break;
    case TOKEN_RELATIONAL_EQUAL:            binary_expression_pointer->data.binary_expression.op_type = AST_BINARY_EQUAL; break;
    case TOKEN_RELATIONAL_NOT_EQUAL:        binary_expression_pointer->data.binary_expression.op_type = AST_BINARY_NOT_EQUAL; break;
    case TOKEN_LOGICAL_AND:                 binary_expression_pointer->data.binary_expression.op_type = AST_BINARY_AND; break;
    case TOKEN_LOGICAL_OR:                  binary_expression_pointer->data.binary_expression.op_type = AST_BINARY_OR; break;
    case TOKEN_PLUS_EQUAL:                  binary_expression_pointer->data.binary_expression.op_type = AST_BINARY_ADD; break;
    case TOKEN_NEGATION_EQUAL:              binary_expression_pointer->data.binary_expression.op_type = AST_BINARY_SUBTRACT; break;
    case TOKEN_ASTERISK_EQUAL:              binary_expression_pointer->data.binary_expression.op_type = AST_BINARY_MULTIPLY; break;
    case TOKEN_FORWARD_SLASH_EQUAL:         binary_expression_pointer->data.binary_expression.op_type = AST_BINARY_DIVIDE; break;
    case TOKEN_PERCENT_EQUAL:               binary_expression_pointer->data.binary_expression.op_type = AST_BINARY_REMAINDER; break;
    case TOKEN_BITWISE_AND_EQUAL:           binary_expression_pointer->data.binary_expression.op_type = AST_BINARY_BITWISE_AND; break;
    case TOKEN_BITWISE_OR_EQUAL:            binary_expression_pointer->data.binary_expression.op_type = AST_BINARY_BITWISE_OR; break;
    case TOKEN_BITWISE_XOR_EQUAL:           binary_expression_pointer->data.binary_expression.op_type = AST_BINARY_BITWISE_XOR; break;
    case TOKEN_BITWISE_RIGHT_SHIFT_EQUAL:   binary_expression_pointer->data.binary_expression.op_type = AST_BINARY_BITWISE_RIGHT_SHIFT; break;
    case TOKEN_BITWISE_LEFT_SHIFT_EQUAL:    binary_expression_pointer->data.binary_expression.op_type = AST_BINARY_BITWISE_LEFT_SHIFT; break;
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
      //TODO: Arena Alloc restructure: This needs to be tested to make sure it works
      AstNode *assignment_expression = arena_alloc(parser->node_arena);
      assignment_expression->type = AST_EXPRESSION_ASSIGNMENT;
      assignment_expression->data.assignement_expression.left_expression = left_expression;
      assignment_expression->data.assignement_expression.right_expression = binary_expression_pointer;
      assignment_expression->data.assignement_expression.expression_type = NULL;
      *binary_expression = assignment_expression;
      return;
    }
    default:
     return;
  }
}

static void parse_factor(Parser *parser, AstNode *factor_node) {
 if (end_of_file(parser)) {
    fprintf(stderr, "ERROR - Parser: Incomplete expression (line %d)\n", previous_token(parser)->line);
    exit(1);
  }

  switch(current_token(parser)->type) {
    case TOKEN_CONSTANT_INT:
    case TOKEN_CONSTANT_LONG:
    case TOKEN_CONSTANT_FLOAT:
    case TOKEN_CONSTANT_UNSIGNED_INT:
    case TOKEN_CONSTANT_UNSIGNED_LONG:
      parse_factor_constant(parser, factor_node, current_token(parser)->type); break;
    case TOKEN_NEGATION:
    case TOKEN_BITWISE_NOT:
    case TOKEN_LOGICAL_NOT:
      parse_factor_unary(parser, factor_node); break;
    case TOKEN_INCREMENT:
    case TOKEN_DECREMENT:
      parse_factor_prefix_expression(parser, factor_node); break;
    case TOKEN_OPEN_PAREN: {
      if (is_type_identifier_token(peek_next_token(parser))) {
        parse_factor_cast_expression(parser, factor_node); 
        break;
      }

      parse_factor_parenthetical_expression(parser, factor_node);
      break;
    }
    case TOKEN_IDENTIFIER: {    
      char *identifier = get_identifier(parser);

      switch(current_token(parser)->type) {
        case TOKEN_OPEN_PAREN: parse_factor_function_call(parser, factor_node, identifier); break;
        default:               parse_factor_variable_expression(parser, factor_node, identifier); break;
      }      
      break;
    }
    default:
      fprintf(stderr, "ERROR - Parser: Failed to parse factor for '%s' token (line %d)\n", TokenTypeStr[current_token(parser)->type], current_token(parser)->line);
      exit(1);    
  }
}

static void parse_factor_constant(Parser *parser, AstNode *factor_node, TokenType constant_type) {
  expect(parser, constant_type); 

  factor_node->type = AST_EXPRESSION_CONSTANT;

  char slice[previous_token(parser)->end_index - previous_token(parser)->start_index]; 
  strncpy(slice, parser->file + previous_token(parser)->start_index, (previous_token(parser)->end_index - previous_token(parser)->start_index) + 1);

  char *end_ptr;
  //@BUG: Does not support float values
  long constant_value = strtol(slice, &end_ptr, BASE_TEN);

  if (constant_value < LONG_MIN || constant_value > LONG_MAX) {
    fprintf(stderr, "ERROR - Parser: Out of bounds int/long constant '%ld'\n", constant_value);
    exit(1);
  }

  AstNode *expression_type = arena_alloc(parser->node_arena);
  expression_type->type = AST_TYPE;

  if (constant_type == TOKEN_CONSTANT_INT && constant_value > INT_MIN && constant_value < INT_MAX) {
    factor_node->data.constant_expression.constant_type = AST_CONSTANT_TYPE_INT;
    factor_node->data.constant_expression.int_value = (int)constant_value;

    expression_type->data.type.type = TYPE_INT;  
    
    factor_node->data.constant_expression.expression_type = expression_type;

    return;
  }

  //Floating points constants can't go out of range since a double supports positive and negative infinity
  if (constant_type == TOKEN_CONSTANT_FLOAT) {
    factor_node->data.constant_expression.constant_type = AST_CONSTANT_TYPE_DOUBLE;
    factor_node->data.constant_expression.double_value = (double)constant_value;

    expression_type->data.type.type = TYPE_DOUBLE;  
    
    factor_node->data.constant_expression.expression_type = expression_type;

    return;
  }

  if (constant_type == TOKEN_CONSTANT_UNSIGNED_INT && constant_value >= 0  && constant_value < UINT_MAX) {
    factor_node->data.constant_expression.constant_type = AST_CONSTANT_TYPE_UINT;
    factor_node->data.constant_expression.uint_value = (unsigned int)constant_value;

    expression_type->data.type.type = TYPE_UINT;  
    
    factor_node->data.constant_expression.expression_type = expression_type;

    return;
  }

  if (constant_type == TOKEN_CONSTANT_UNSIGNED_LONG && constant_value >= 0  && constant_value < ULONG_MAX) {
    factor_node->data.constant_expression.constant_type = AST_CONSTANT_TYPE_ULONG;
    factor_node->data.constant_expression.ulong_value = (unsigned long)constant_value;

    expression_type->data.type.type = TYPE_ULONG;  
    
    factor_node->data.constant_expression.expression_type = expression_type;

    return;
  }

  factor_node->data.constant_expression.constant_type = AST_CONSTANT_TYPE_LONG;
  factor_node->data.constant_expression.long_value = constant_value;

  expression_type->data.type.type = TYPE_LONG;  
    
  factor_node->data.constant_expression.expression_type = expression_type;
}

static void parse_factor_unary(Parser *parser, AstNode *factor_node) {
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
  }
  
  parser->current_token_index++;

  AstNode *unary_value_expression_node = arena_alloc(parser->node_arena);
  parse_factor(parser, unary_value_expression_node);

  factor_node->type = AST_EXPRESSION_UNARY;
  factor_node->data.unary_expression.op_type = op_type;  
  factor_node->data.unary_expression.expression = unary_value_expression_node;
  factor_node->data.unary_expression.expression_type = NULL;
}

static void parse_factor_prefix_expression(Parser *parser, AstNode *factor_node) {
  AstNode *prefix_expression = arena_alloc(parser->node_arena);

  if (current_token(parser)->type == TOKEN_INCREMENT) {
    expect(parser, TOKEN_INCREMENT);
    prefix_expression->type = AST_EXPRESSION_PREFIX_INCREMENT;
  } else {
    expect(parser, TOKEN_DECREMENT);
    prefix_expression->type = AST_EXPRESSION_PREFIX_DECREMENT;
  }

  AstNode *left = arena_alloc(parser->node_arena);

  parse_expression(parser, &left, 0);

  factor_node->type = AST_EXPRESSION_ASSIGNMENT;
  factor_node->data.assignement_expression.left_expression = left;

  AstNode *postfix_constant = arena_alloc(parser->node_arena);
  postfix_constant->type = AST_EXPRESSION_CONSTANT;
  //TODO: Look into why I'm doing this
  postfix_constant->data.constant_expression.int_value = 1;
  postfix_constant->data.constant_expression.expression_type = NULL;

  AstNode *postfix_binary = arena_alloc(parser->node_arena);
  postfix_binary->type = AST_EXPRESSION_BINARY;

  if (prefix_expression->type == AST_EXPRESSION_PREFIX_INCREMENT) {
    postfix_binary->data.binary_expression.op_type = AST_BINARY_ADD;
  } else {
    postfix_binary->data.binary_expression.op_type = AST_BINARY_SUBTRACT;
  }

  postfix_binary->data.binary_expression.left_expression = left;
  postfix_binary->data.binary_expression.right_expression = postfix_constant;
  postfix_binary->data.binary_expression.expression_type = NULL;

  factor_node->data.assignement_expression.right_expression = postfix_binary;

  prefix_expression->data.increment_decrement_expression.expression = factor_node;
}

static void parse_factor_parenthetical_expression(Parser *parser, AstNode *factor_node) {
  expect(parser, TOKEN_OPEN_PAREN);
  parse_expression(parser, &factor_node, 0);    
  expect(parser, TOKEN_CLOSE_PAREN);
}

static void parse_factor_cast_expression(Parser *parser, AstNode *factor_node) {
  expect(parser, TOKEN_OPEN_PAREN);

  AstNode *type_node = arena_alloc(parser->node_arena);
  type_node->type = AST_TYPE;

  Specifier specifier = parse_specifier(parser, true);
  type_node->data.type.type = specifier.specifier_type; 

  expect(parser, TOKEN_CLOSE_PAREN);

  AstNode *expression_node = arena_alloc(parser->node_arena);
  parse_factor(parser, expression_node);
  
  factor_node->type = AST_EXPRESSION_CAST;
  factor_node->data.cast_expression.target_type = type_node;  
  factor_node->data.cast_expression.expression = expression_node;
  factor_node->data.cast_expression.expression_type = NULL;
}

static void parse_factor_goto_label(Parser *parser, AstNode *factor_node) {
  char *label_identifier = get_identifier(parser);

  expect(parser, TOKEN_COLON);
  factor_node->type = AST_STATEMENT_GOTO_LABEL;
  factor_node->data.goto_label_statement.label = label_identifier;
}

static void parse_factor_variable_expression(Parser *parser, AstNode *factor_node, char *label_identifier) {
  factor_node->type = AST_EXPRESSION_VARIABLE;
  factor_node->data.variable_expression.identifier = label_identifier;
}

static void parse_factor_function_call(Parser *parser, AstNode *factor_node, char *identifier) {
  expect(parser, TOKEN_OPEN_PAREN);

  factor_node->type = AST_EXPRESSION_FUNCTION_CALL;
  factor_node->data.function_call_expression.identfier = identifier;
  factor_node->data.function_call_expression.argument_count = 0;
  factor_node->data.function_call_expression.expression_type = NULL;
  
  NodePointer *argument_pointers = malloc(sizeof(NodePointer));
  init_node_pointer(argument_pointers);
  factor_node->data.function_call_expression.argument_ptrs = argument_pointers;

  if (current_token(parser)->type == TOKEN_CLOSE_PAREN) {
    expect(parser, TOKEN_CLOSE_PAREN);
    return;
  }  

  AstNode *expression_node = arena_alloc(parser->node_arena);
  parse_expression(parser, &expression_node, 0);
  add_to_node_pointer(expression_node, argument_pointers);
  factor_node->data.function_call_expression.argument_count++;

  while (current_token(parser)->type == TOKEN_COMMA) {
    expect(parser, TOKEN_COMMA);
    AstNode *next_expression_node = arena_alloc(parser->node_arena);
    parse_expression(parser, &next_expression_node, 0);
    add_to_node_pointer(next_expression_node, argument_pointers);
    factor_node->data.function_call_expression.argument_count++;
  }

  expect(parser, TOKEN_CLOSE_PAREN);
} 

static Specifier parse_specifier(Parser *parser, bool error_if_storage_class_found) {
  Specifier specifier;

  if (current_token(parser)->type == TOKEN_VOID) {
    expect(parser, TOKEN_VOID);

    specifier.specifier_type = TYPE_VOID;
    specifier.storage_class_type = AST_STORAGE_CLASS_NONE;

    return specifier;
  }

  TokenType type_specifiers[4];
  int type_specifier_count = 0;
  int unsigned_count = 0;
  int signed_count = 0;

  specifier.storage_class_type = AST_STORAGE_CLASS_NONE;

  while(true) {
    switch (current_token(parser)->type) {
      case TOKEN_STATIC:
        if (error_if_storage_class_found) {
          fprintf(stderr, "ERROR - Parser: Declared type cannot contain 'static' storage class (line %d)\n", current_token(parser)->line);
          exit(1);
        }

        if (specifier.storage_class_type != AST_STORAGE_CLASS_NONE) {
          fprintf(stderr, "ERROR - Parser: Declared 'static' cannot be included (line %d)\n", current_token(parser)->line);
          exit(1);
        }
        
        specifier.storage_class_type = AST_STORAGE_CLASS_STATIC;
        parser->current_token_index++;
        continue;
      case TOKEN_EXTERN:
        if (error_if_storage_class_found) {
          fprintf(stderr, "ERROR - Parser: Declared type cannot contain 'extern' storage class (line %d)\n", current_token(parser)->line);
          exit(1);
        }

        if (specifier.storage_class_type != AST_STORAGE_CLASS_NONE) {
          fprintf(stderr, "ERROR - Parser: Declared 'static' cannot be included (line %d)\n", current_token(parser)->line);
          exit(1);
        }

        specifier.storage_class_type = AST_STORAGE_CLASS_EXTERN;
        parser->current_token_index++;
        continue;
      case TOKEN_UNSIGNED:
      case TOKEN_SIGNED:
      case TOKEN_INT:
      case TOKEN_LONG:
      case TOKEN_DOUBLE: {
        if (current_token(parser)->type == TOKEN_UNSIGNED) {
          unsigned_count++;
        } else if (current_token(parser)->type == TOKEN_SIGNED) {
          signed_count++;
        }
        
        type_specifiers[type_specifier_count] = current_token(parser)->type;
        type_specifier_count++;
        parser->current_token_index++;
        continue;
      }
    }

    break;    
  }

  specifier.specifier_type_found = type_specifier_count == 0 ? false : true;

  if (unsigned_count >= 1 && signed_count > 0) {
    fprintf(stderr, "ERROR - Parser: Unsigned type contains invalid specifier. Line %d\n", current_token(parser)->line);
    exit(1);
  }

  if (signed_count >= 1 && unsigned_count > 0) {
    fprintf(stderr, "ERROR - Parser: Signed type contains invalid specifier. Line %d\n", current_token(parser)->line);
    exit(1);
  }

  for (int i = 0; i < type_specifier_count; i++) {
    if (type_specifiers[i] == TOKEN_DOUBLE) {
      if (type_specifier_count > 1) {
        fprintf(stderr, "ERROR - Parser: Double cannot contain another type specifier. Line %d\n", current_token(parser)->line);
        exit(1);
      } else {
        specifier.specifier_type = TYPE_DOUBLE;
        return specifier;
      }
    }
    
    if (type_specifiers[i] == TOKEN_LONG) {
      if (unsigned_count) {
        specifier.specifier_type = TYPE_ULONG;
      } else {
        specifier.specifier_type = TYPE_LONG;
      }
      
      return specifier;
    }

    if (type_specifiers[i] == TOKEN_INT ) {
      if (unsigned_count) {
        specifier.specifier_type = TYPE_UINT;
      } else {
        specifier.specifier_type = TYPE_INT;
      }
    }
  }

  if (unsigned_count) {
    specifier.specifier_type = TYPE_UINT;
  } else {
    specifier.specifier_type = TYPE_INT;
  }

  return specifier;
}

static int get_precedence(TokenType token_type) {
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
      return -1;
    }
  }
}

static bool is_type_identifier_token(TokenType token_type) {
  switch(token_type) {
    case TOKEN_INT:
    case TOKEN_LONG:
    case TOKEN_DOUBLE:
    case TOKEN_UNSIGNED:
    case TOKEN_SIGNED:
      return true;
    default:
      return false;
  }
}

static void add_function_parameter_identifier(char *identifier, AstNode *function_declaration_node) {  
  if (function_declaration_node->data.function_declaration.parameter_count == function_declaration_node->data.function_declaration.parameter_identifier_capacity) {
    int size = function_declaration_node->data.function_declaration.parameter_identifier_capacity == 0 ? FUNCTION_IDENTIFIER_INIT_CAPACITY : function_declaration_node->data.function_declaration.parameter_identifier_capacity * 2;
    function_declaration_node->data.function_declaration.parameter_identifier_capacity = size;
    function_declaration_node->data.function_declaration.parameter_identifiers = realloc(function_declaration_node->data.function_declaration.parameter_identifiers, size * sizeof(char*));
  }

  function_declaration_node->data.function_declaration.parameter_identifiers[function_declaration_node->data.function_declaration.parameter_count] = identifier;
  function_declaration_node->data.function_declaration.parameter_count++;
}

static void add_function_parameter_type(AstNode *function_parameter_type, AstNode *function_type) {
  if (function_type->data.type.function_param_type_count == function_type->data.type.function_param_type_capacity) {
    int size = function_type->data.type.function_param_type_capacity == 0 ? FUNCTION_PARAMETER_TYPE_INIT_CAPACITY : function_type->data.type.function_param_type_capacity * 2;
    function_type->data.type.function_param_type_capacity = size;
    function_type->data.type.function_param_types = realloc(function_type->data.type.function_param_types, size * sizeof(AstNode));
  }

  function_type->data.type.function_param_types[function_type->data.type.function_param_type_count] = *function_parameter_type;
  function_type->data.type.function_param_type_count++;
}   

