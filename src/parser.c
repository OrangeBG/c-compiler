#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include "../include/parser.h"
#include "../include/arena.h"
#include "../include/lexer.h"
#include "../include/dynamic_array.h"
#include "../include/error.h"
#include "../include/token.h"

#define ADD_WHITESPACE (whitespace + 5)
#define POINTER_ARENA_INIT_CAPACITY 8
#define FUNCTION_IDENTIFIER_INIT_CAPACITY 4
#define FUNCTION_DECLARATOR_INIT_CAPACITY 4
#define NODE_POINTER_CAPACITY 8
#define BASE_TEN 10

typedef struct {
  int token_count;
  int current_token_index;
  int current_loop_label_id;
  TokenArray *tokens;
  char *file;
  Arena *node_arena;
  Arena *type_arena;
} Parser;

typedef struct {
  StorageClassType storage_class_type;
  Types specifier_type;
  bool specifier_type_found;
} Specifier;

typedef struct Declarator Declarator;

typedef enum {
  DECLARATOR_TYPE_IDENTIFIER,
  DECLARATOR_TYPE_POINTER,
  DECLARATOR_TYPE_ARRAY,
  DECLARATOR_FUNCTION
} DeclaratorType;

typedef struct {
  Types param_type;
  Declarator *declarator;
} DeclaratorParameter; 

typedef struct Declarator {
  DeclaratorType type;
  union {
    struct Identifier { char* identifier; } identifier;
    struct PointerDeclarator { Declarator *declarator; } pointer_declaration;
    struct ArrayDeclarator { Declarator *declarator; unsigned long size; } array_declarator;
    struct FunctionDeclarator { DeclaratorParameter *declarator_parameters; int param_count; int param_capacity; Declarator *declarator; } function_declarator;
  } data;  
} Declarator;

typedef struct {
  char *identifier;
  // AstNode *declaration_type;
  TypeNode *declaration_type;
  int param_identifiers_count;
  int param_identifiers_capacity;
  char **param_identifiers;
} DeclaratorResults; 

typedef enum {
  ABSTRACT_DECLARATOR_POINTER,
  ABSTRACT_ARRAY,
  ABSTRACT_DECLARATOR_BASE
} AbstractDeclaratorType;

typedef struct AbstractDeclarator AbstractDeclarator;

typedef struct AbstractDeclarator {
  AbstractDeclaratorType type;
  union {
    struct AbstractPointer { AbstractDeclarator *abstract_declarator; } abstract_pointer;
    struct AbstractArray { AbstractDeclarator *abstract_declarator; unsigned long size; } abstract_array;
  } data;
} AbstractDeclarator;
 
static void         parse_program(Parser *parser, AstNode *program_node);
static void         parse_declaration(Parser *parser, AstNode *declaration_node); 
static Declarator*  parse_declarator(Parser *parser); 
static Declarator*  parse_direct_declarator(Parser *parser);
static Declarator*  parse_simple_declarator(Parser *parser);
static Declarator*  parse_declarator_suffix(Parser *parser);
static AbstractDeclarator* parse_abstract_declarator(Parser *parser); 
static AbstractDeclarator* parse_direct_abstract_declarator(Parser *parser); 
static void         parse_function_declaration(Parser *parser, AstNode *function_node, StorageClassType storage_class_type, DeclaratorResults *declaration_results);
static void         parse_variable_declaration(Parser *parser, AstNode *variable_node, StorageClassType storage_class_type, DeclaratorResults *declaration_results);
static void         parse_initializer(Parser *parser, AstNode *initializer_node); 
static void         parse_block(Parser *parser, AstNode *block_node);
static void         parse_statement(Parser *parser, AstNode **statement_node);
static void         parse_statement_null(Parser *parser, AstNode *statement_node);
static void         parse_statement_return(Parser *parser, AstNode *statement_node); 
static void         parse_statement_if(Parser *parser, AstNode *statement_node); 
static void         parse_statement_goto(Parser *parser, AstNode *statement_node); 
static void         parse_statement_break(Parser *parser, AstNode *break_statement_node); 
static void         parse_statement_continue(Parser *parser, AstNode *continue_statement_node); 
static void         parse_statement_while(Parser *parser, AstNode *while_statement_node); 
static void         parse_statement_do(Parser *parser, AstNode *do_statement_node); 
static void         parse_statement_for(Parser *parser, AstNode *for_statement_node); 
static void         parse_statement_compound_statement(Parser *parser, AstNode *compound_statement_node); 
static void         parse_expression(Parser *parser, AstNode **expression_node, int min_precedence);
static void         parse_expression_postfix(Parser *parser, AstNode *postfix_expression, AstNode *left_expression,  TokenType postfix_token);
static void         parse_subscript_expression(Parser *parser, AstNode *postfix_node, AstNode *subscript_node);
static void         parse_expression_assignment(Parser *parser, AstNode *assignment_expression, AstNode *left_factor, TokenType assignment_token); 
static void         parse_expression_conditional(Parser *parser, AstNode *conditional_expression_node, AstNode *left_expression, TokenType conditional_token); 
static void         parse_expression_binary(Parser *parser, AstNode **binary_expression_node, AstNode *left_expression, TokenType op_type);
static void         parse_factor(Parser *parser, AstNode **factor_node);
static void         parse_unary_expression(Parser *parser, AstNode **unary_node); 
static void         parse_unary_postfix_expression(Parser *parser, AstNode **postfix_node); 
static void         parse_primary_expression(Parser *parser, AstNode **expression_node); 
static void         parse_factor_constant(Parser *parser, AstNode *factor_node, TokenType constant_type);
static void         parse_factor_unary(Parser *parser, AstNode *factor_node); 
static void         parse_factor_prefix_expression(Parser *parser, AstNode *factor_node); 
static void         parse_factor_parenthetical_expression(Parser *parser, AstNode **factor_node); 
static void         parse_factor_cast_expression(Parser *parser, AstNode *factor_node); 
static void         parse_factor_goto_label(Parser *parser, AstNode *factor_node); 
static void         parse_factor_variable_expression(Parser *parser, AstNode *factor_node, char *label_identifier);
static void         parse_factor_function_call(Parser *parser, AstNode *factor_node, char *identifier); 
static void         parse_factor_address_of(Parser *parser, AstNode *factor_node); 
static void         parse_factor_dereference(Parser *parser, AstNode *factor_node); 
static Specifier    parse_specifier(Parser *parser, bool error_if_storage_class_found);
static Token*       current_token(const Parser *parser);
static Token*       previous_token(const Parser *parser);
static TokenType    peek_next_token(const Parser *parser); 
static char*        get_identifier(Parser *parser);
static void         expect(Parser *parser, TokenType expected_type);
static void         print_whitespace(int count); 
static void         add_to_node_pointer(AstNode *node, NodePointer *node_pointer); 
static void         init_node_pointer(NodePointer *node_pointer); 
static bool         end_of_file(const Parser *parser);
static bool         is_type_identifier_token(TokenType token_type);
static int          get_precedence(TokenType token_type);
static void         add_function_parameter_identifier(char *identifier, AstNode *function_declaration_node);   
static void         add_function_parameter_to_declarator(Declarator *function_declarator, Types param_type, Declarator *param_declarator); 
static void         add_function_parameter_identifier_to_declarator_results(char *identifier, DeclaratorResults *declarator_results);   
static DeclaratorResults* process_declarator(Parser *parser, DeclaratorResults *declaration_results, Declarator *declarator, TypeNode *base_type); 
static TypeNode*    process_abstract_declarator(Parser *parser, AbstractDeclarator *abstract_declarator, TypeNode *base_type);

void parse_ast(ParserResults *results, TokenArray *tokens, int token_count, char *file) {  
  Arena *parser_arena = malloc(sizeof(Arena));
  //TODO: Hardcoded capacity
  arena_init(parser_arena, sizeof(AstNode), sizeof(AstNode) * 1000, false);

  Arena *type_arena = malloc(sizeof(Arena));
  //TODO: Hardcoded capacity
  arena_init(type_arena, sizeof(TypeNode), sizeof(TypeNode) * 1000, false);

  results->ast_node_arena = parser_arena;
  results->type_node_arena = type_arena;
  
  Parser parser = {
    .token_count = token_count,
    .current_token_index = 0,
    .tokens = tokens,
    .file = file,
    .current_loop_label_id = 0,
    .node_arena = parser_arena,
    .type_arena = type_arena
  };
  
  AstNode *program_node = arena_alloc(parser.node_arena);
  parse_program(&parser, program_node);

  expect(&parser, TOKEN_EOF);

  if (token_count > parser.current_token_index) {
    panic("Identifier declared outside of program scope");
  }
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
      printf("Variable Declaration (line = %d, id = \"%s\" ", node->line_number, node->data.declaration_variable.name);

      switch (node->data.declaration_variable.storage_class_type) {
        case AST_STORAGE_CLASS_NONE: printf("storage class = \"None\""); break;
        case AST_STORAGE_CLASS_EXTERN : printf("storage class = \"Extern\""); break;
        case AST_STORAGE_CLASS_STATIC : printf("storage class = \"Static\""); break;        
      }
      
      printf(", type = ");
      print_type_node(node->data.declaration_variable.type);
      printf("\n");

      if (node->data.declaration_variable.has_expression) {
        print_ast(node->data.declaration_variable.init_expression, ADD_WHITESPACE);
      }

      print_whitespace(whitespace);
      printf(")\n");      
      break;
    case AST_FUNCTION_DECLARATION:
      print_whitespace(whitespace);
      printf("Function Declaration (line = %d, name = \"%s\"\n", node->line_number, node->data.declaration_function.name);
      print_whitespace(whitespace);
      printf("return type = ");
      print_type_node(node->data.declaration_function.function_type);
      printf("\n");

      if (node->data.declaration_function.function_type->data.function_type.param_type_count != 0) {
        print_whitespace(whitespace);
        printf("params = ");
        printf("\n");
     
        for (int i = 0; i < node->data.declaration_function.function_type->data.function_type.param_type_count; i++) {
          print_whitespace(ADD_WHITESPACE);
          printf("Param( name = %s, ", node->data.declaration_function.parameter_identifiers[i]);
          print_type_node(&node->data.declaration_function.function_type->data.function_type.param_types[i]);
          printf(")\n");
        }
      }

      if (node->data.declaration_function.body_block != NULL) {
        print_whitespace(whitespace);
        printf("body=\n");
        print_ast(node->data.declaration_function.body_block, ADD_WHITESPACE);
      }

      print_whitespace(whitespace);
      printf(")\n");      
      break;
    case AST_INITIALIZER: {
      if (node->data.initializer.type == AST_INITIALIZER_SINGLE) {
        print_whitespace(whitespace);
        printf("Single Init (line = %d\n", node->line_number);
        print_ast(node->data.initializer.initializer_node.single_init_expression, ADD_WHITESPACE);
        print_whitespace(whitespace);
        printf(")\n");
      } else {
        print_whitespace(whitespace);
        printf("Compound Init (line = %d, count = %d\n", node->line_number, node->data.initializer.initializer_node.compound_initializer->count);
        for (int i = 0; i < node->data.initializer.initializer_node.compound_initializer->count; i++) {
          print_ast(&node->data.initializer.initializer_node.compound_initializer->items[i], ADD_WHITESPACE);
        }
        print_whitespace(whitespace);
        printf(")\n");
      }
      break;
    }
    case AST_BLOCK:
      print_whitespace(whitespace);
      printf("Block (line = %d\n", node->line_number);
      for (int i = 0; i < node->data.block.block_count; i++) {
        AstNode *block_item = node->data.block.block_ptrs->node_pointers[i];
        print_ast(block_item, ADD_WHITESPACE);
      }   
      print_whitespace(whitespace);
      printf(")\n");
      break;
    case AST_STATEMENT_GOTO:
      print_whitespace(whitespace);
      printf("Goto (%s)\n", node->data.statement_goto.label);
      break;      
    case AST_STATEMENT_GOTO_LABEL:
      print_whitespace(whitespace);
      printf("Goto Label(%s)\n", node->data.statement_goto_label.label);
      break;      
    case AST_STATEMENT_BREAK:
      print_whitespace(whitespace);
      printf("Break(id = %d)\n", node->data.statement_break.label_id);
      break;
    case AST_STATEMENT_CONTINUE:
      print_whitespace(whitespace);
      printf("Continue(id = %d)\n", node->data.statement_continue.label_id);
      break;
    case AST_STATEMENT_RETURN:
      print_whitespace(whitespace);
      printf("Return(Line = %d\n", node->line_number);
      print_ast(node->data.statement_return.expression, ADD_WHITESPACE);
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
      printf("If (line = %d\n", node->line_number);
      print_ast(node->data.statement_if.condition_expression, ADD_WHITESPACE);
      print_whitespace(whitespace);
      printf(")\n ");
      print_whitespace(whitespace);
      printf("Then(\n");
      print_ast(node->data.statement_if.then_statement, ADD_WHITESPACE);
      print_whitespace(whitespace);
      printf(")\n");

      if (node->data.statement_if.else_statement != NULL) {
        print_whitespace(whitespace);
        printf("Else(\n");
        print_ast(node->data.statement_if.else_statement, ADD_WHITESPACE);
        print_whitespace(whitespace);
        printf(")\n");
      }
      break;
    case AST_STATEMENT_WHILE:
      print_whitespace(whitespace);
      printf("While (\n");
      print_whitespace(ADD_WHITESPACE);
      printf("Id = %d\n", node->data.statement_while.label_id);
      print_whitespace(ADD_WHITESPACE);
      printf("Condition =\n");
      print_ast(node->data.statement_while.condition, ADD_WHITESPACE + 5);
      print_whitespace(ADD_WHITESPACE);
      printf("Statements =\n");
      print_ast(node->data.statement_while.statement_body, ADD_WHITESPACE + 5);
      print_whitespace(whitespace);
      printf(")\n");
      break;
    case AST_STATEMENT_DO_WHILE:
      print_whitespace(whitespace);
      printf("Do (line = %d\n", node->line_number);
      print_whitespace(ADD_WHITESPACE);
      printf("Id = %d\n", node->data.statement_do_while.label_id);
      print_whitespace(ADD_WHITESPACE);
      printf("Statements = \n");
      print_ast(node->data.statement_do_while.statement_body, ADD_WHITESPACE + 5);
      print_whitespace(ADD_WHITESPACE);
      printf("Condition = \n");
      print_ast(node->data.statement_do_while.condition, ADD_WHITESPACE + 5);
      print_whitespace(whitespace);
      printf(")\n");
      break;
    case AST_STATEMENT_FOR:
      print_whitespace(whitespace);
      printf("For (line = %d\n", node->line_number);
      print_whitespace(ADD_WHITESPACE);
      printf("Id = %d\n", node->data.statement_for.label_id);

      if (node->data.statement_for.for_loop_init != NULL) {
        print_whitespace(ADD_WHITESPACE);
        printf("Init = \n");
        print_ast(node->data.statement_for.for_loop_init, ADD_WHITESPACE + 5);
      }

      if (node->data.statement_for.condition_expression != NULL) {
        print_whitespace(ADD_WHITESPACE);
        printf("Condition = \n");
        print_ast(node->data.statement_for.condition_expression, ADD_WHITESPACE + 5);
      }

      if (node->data.statement_for.post_expression != NULL) {
        print_whitespace(ADD_WHITESPACE);
        printf("Post = \n");
        print_ast(node->data.statement_for.post_expression, ADD_WHITESPACE + 5);
      }

      print_whitespace(ADD_WHITESPACE);
      printf("Statement Body = \n");
      print_ast(node->data.statement_for.statement_body, ADD_WHITESPACE + 5);

      print_whitespace(whitespace);
      printf(")\n");      
      break;
    case AST_STATEMENT_COMPOUND:
      print_ast(node->data.statement_compound.block, whitespace);
      break;
    case AST_EXPRESSION_CONSTANT:
      print_whitespace(whitespace);

      switch (node->data.expression_constant.constant_type) {
        case AST_CONSTANT_TYPE_INT:
          printf("Constant(line = %d, Int (%d))\n",node->line_number, node->data.expression_constant.int_value);
          break;
        case AST_CONSTANT_TYPE_UINT:
          printf("Constant(line = %d, UInt (%d))\n", node->line_number, node->data.expression_constant.uint_value);
          break;
        case AST_CONSTANT_TYPE_LONG:
          printf("Constant(line = %d, Long(%ld))\n", node->line_number, node->data.expression_constant.long_value);
          break;
        case AST_CONSTANT_TYPE_ULONG:
          printf("Constant(line = %d, ULong(%lu))\n", node->line_number, node->data.expression_constant.ulong_value);
          break;
        case AST_CONSTANT_TYPE_DOUBLE:
          printf("Constant(line = %d, Double(%f))\n", node->line_number, node->data.expression_constant.double_value);
          break;
        case AST_CONSTANT_TYPE_CHAR:
          printf("Constant(line = %d, Char(%d))\n", node->line_number, node->data.expression_constant.char_value);
          break;
        case AST_CONSTANT_TYPE_UCHAR:
          printf("Constant(line = %d, UChar(%d))\n", node->line_number, node->data.expression_constant.uchar_value);
          break;
        default:
          panic("Could not find constant type when printing");
      }
      break;
    case AST_EXPRESSION_POSTFIX_INCREMENT:
      print_whitespace(whitespace);
      printf("Postfix Increment(\n");
      print_ast(node->data.expression_increment_decrement.expression, ADD_WHITESPACE);
      print_whitespace(whitespace);
      printf(")\n");
      break;
    case AST_EXPRESSION_POSTFIX_DECREMENT:
      print_whitespace(whitespace);
      printf("Postfix Decrement(\n");
      print_ast(node->data.expression_increment_decrement.expression, ADD_WHITESPACE);
      print_whitespace(whitespace);
      printf(")\n");
      break;
    case AST_EXPRESSION_PREFIX_INCREMENT:
      print_whitespace(whitespace);
      printf("Prefix Increment(\n");
      print_ast(node->data.expression_increment_decrement.expression, ADD_WHITESPACE);
      print_whitespace(whitespace);
      printf(")\n");
      break;
    case AST_EXPRESSION_PREFIX_DECREMENT:
      print_whitespace(whitespace);
      printf("Prefix Decrement(\n");
      print_ast(node->data.expression_increment_decrement.expression, ADD_WHITESPACE);
      print_whitespace(whitespace);
      printf(")\n");
      break;
    case AST_EXPRESSION_CONDITIONAL:
      print_whitespace(whitespace);
      printf("Conditional(\n");
      print_whitespace(ADD_WHITESPACE);
      printf("Condition = \n");
      print_ast(node->data.expression_conditional.condition, ADD_WHITESPACE + 5);
      print_whitespace(ADD_WHITESPACE);
      printf("True Expression = \n");
      print_ast(node->data.expression_conditional.true_expression, ADD_WHITESPACE + 5);
      print_whitespace(ADD_WHITESPACE);
      printf("False Expression = \n");
      print_ast(node->data.expression_conditional.false_expression, ADD_WHITESPACE + 5);
      print_whitespace(whitespace);
      printf(")\n");      
      break;
    case AST_EXPRESSION_UNARY:
      print_whitespace(whitespace);
      printf("Unary (line = %d, type = ", node->line_number);

      switch (node->data.expression_unary.op_type) {
        case AST_UNARY_COMPLEMENT: printf("Complement"); break;
        case AST_UNARY_NEGATE: printf("Negate"); break;
        case AST_UNARY_NOT: printf("Not"); break;
        case AST_UNARY_PREFIX_INCREMENT: printf("Prefix Increment"); break;
        case AST_UNARY_PREFIX_DECREMENT: printf("Prefix Decrement"); break;
      }
      printf("\n");      
      print_ast(node->data.expression_unary.expression, ADD_WHITESPACE);
      print_whitespace(whitespace);
      printf(")\n");
      break;
    case AST_EXPRESSION_BINARY:
      print_whitespace(whitespace);
      printf("Binary(line = %d, op type = ", node->line_number);
      switch (node->data.expression_binary.op_type) {
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
      print_ast(node->data.expression_binary.left_expression, ADD_WHITESPACE + 5);
      print_whitespace(ADD_WHITESPACE);
      printf("Right = \n");    
      print_ast(node->data.expression_binary.right_expression, ADD_WHITESPACE + 5);
      print_whitespace(whitespace);
      printf(")\n");
      break;
      case AST_EXPRESSION_VARIABLE:
        print_whitespace(whitespace);
        printf("Variable(line = %d, %s)\n", node->line_number, node->data.expression_variable.identifier);
        break;
      case AST_EXPRESSION_ASSIGNMENT: {
        print_whitespace(whitespace);
        printf("Assignment(line = %d\n", node->line_number);
        print_whitespace(ADD_WHITESPACE);
        printf("Left = \n");
        print_ast(node->data.expression_assignment.left_expression, ADD_WHITESPACE + 5);

        print_whitespace(ADD_WHITESPACE);
        printf("Right = \n");
        print_ast(node->data.expression_assignment.right_expression, ADD_WHITESPACE + 5);
        print_whitespace(whitespace);
        printf(")\n");
        break;
      }
      case AST_EXPRESSION_FUNCTION_CALL: {
        print_whitespace(whitespace);
        printf("Function Call(line = %d, name = '%s' args =\n", node->line_number, node->data.expression_function_call.identifier);

        for (int i = 0; i < node->data.expression_function_call.argument_count; i++) {
          AstNode *argument = node->data.expression_function_call.argument_ptrs->node_pointers[i];
          print_ast(argument, ADD_WHITESPACE);
        }

        print_whitespace(whitespace);
        printf(")\n");
        break;
      }
      case AST_EXPRESSION_CAST: {
        print_whitespace(whitespace);
        printf("Cast(type=");

        print_type_node(node->data.expression_cast.target_type);
        printf("\n");
        print_ast(node->data.expression_cast.expression, ADD_WHITESPACE);        

        print_whitespace(whitespace);
        printf(")\n");
        break;
      }
      case AST_EXPRESSION_DEREFERENCE:
        print_whitespace(whitespace);
        printf("Dereference(line = %d\n", node->line_number);        
        print_ast(node->data.expression_dereference.expression, ADD_WHITESPACE);
        print_whitespace(whitespace);
        printf(")\n");
        break;
      case AST_EXPRESSION_ADDRESS_OF:
        print_whitespace(whitespace);
        printf("Address Of(line = %d\n", node->line_number);        
        print_ast(node->data.expression_address_of.expression, ADD_WHITESPACE);
        print_whitespace(whitespace);
        printf(")\n");
        break;
      case AST_EXPRESSION_SUBSCRIPT:
        print_whitespace(whitespace);
        printf("Subscript(line = %d\n", node->line_number);        
        print_ast(node->data.expression_subscript.expression_1, ADD_WHITESPACE);
        print_ast(node->data.expression_subscript.expression_2, ADD_WHITESPACE);
        print_whitespace(whitespace);
        printf(")\n");
        break;
      default: {
        panic("Missing ast node type for printing: %d", node->type);
    }
  }    
}

//TODO: Maybe make into macro?
static void print_whitespace(int count) {
  printf("%*s", count, "");
}

static Token* current_token(const Parser *parser) {
  return &parser->tokens->items[parser->current_token_index];
}

static Token* previous_token(const Parser *parser) {
  return &parser->tokens->items[parser->current_token_index - 1];
}

static TokenType peek_next_token(const Parser *parser) {
  if (current_token(parser)->type == TOKEN_EOF) {
    return TOKEN_EOF;
  }

  return parser->tokens->items[parser->current_token_index + 1].type;
}

static bool end_of_file(const Parser *parser) {
  return parser->tokens->items[parser->current_token_index].type == TOKEN_EOF;
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
    input_error_with_line("Expected %s (line %d)", current_token(parser)->line, get_token_name(expected_type), previous_token(parser)->line);
  }

  if (current_token(parser)->type == expected_type) {
    parser->current_token_index++;
    return;
  } 

  input_error_with_line("Expected %s, but found %s",current_token(parser)->line, get_token_name(expected_type), get_token_name(current_token(parser)->type));
}

static void parse_program(Parser *parser, AstNode *program_node) {
  program_node->type = AST_PROGRAM;
  program_node->data.program.declaration_count = 0;
  program_node->line_number = 0;

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

  if (!specifier.specifier_type_found) {
    input_error_with_line("Declaration type not specified", current_token(parser)->line);
  }
  
  Declarator *declarator = parse_declarator(parser);

  if (declarator == NULL) {
    input_error_with_line("Invalid declaration", current_token(parser)->line);
  }

  //TODO: Look into not needing to alloc this type. Can it be derived from specifier?
  TypeNode *base_type = arena_alloc(parser->type_arena);
  base_type->type = specifier.specifier_type;  

  DeclaratorResults results = {
    .param_identifiers = NULL,
    .param_identifiers_count = 0,
    .param_identifiers_capacity = 0
  };

  process_declarator(parser, &results, declarator, base_type);

  if (results.declaration_type->type == TYPE_FUNCTION) {
    parse_function_declaration(parser, declaration_node, specifier.storage_class_type, &results);
  } else {
    parse_variable_declaration(parser, declaration_node, specifier.storage_class_type, &results);
  }
}

static DeclaratorResults* process_declarator(Parser *parser, DeclaratorResults *declaration_results, Declarator *declarator, TypeNode *base_type) {
  switch (declarator->type) {
    case DECLARATOR_TYPE_IDENTIFIER:
      declaration_results->identifier = declarator->data.identifier.identifier;
      declaration_results->declaration_type = base_type;
      break;
    case DECLARATOR_TYPE_POINTER: {
      TypeNode *pointer_type = arena_alloc(parser->type_arena);
      pointer_type->type = TYPE_POINTER;
      pointer_type->data.pointer_type.reference_type = base_type;

      return process_declarator(parser, declaration_results, declarator->data.pointer_declaration.declarator, pointer_type);
    }
    case DECLARATOR_TYPE_ARRAY: {
      TypeNode *array_type = arena_alloc(parser->type_arena);
      array_type->type = TYPE_ARRAY;
      array_type->data.array_type.element_type = base_type;
      array_type->data.array_type.size = declarator->data.array_declarator.size;

      return process_declarator(parser, declaration_results, declarator->data.array_declarator.declarator, array_type);
    }
    case DECLARATOR_FUNCTION:
      switch(declarator->data.function_declarator.declarator->type) {
        case DECLARATOR_TYPE_IDENTIFIER: {
          declaration_results->identifier = declarator->data.function_declarator.declarator->data.identifier.identifier;

          TypeNode *function_type = arena_alloc(parser->type_arena);
          function_type->type = TYPE_FUNCTION;
          function_type->data.function_type.return_type = base_type;

          declaration_results->declaration_type = function_type;

          for (int i = 0; i < declarator->data.function_declarator.param_count; i++) {
            DeclaratorParameter *param = &declarator->data.function_declarator.declarator_parameters[i];
            DeclaratorResults *param_results = malloc(sizeof(DeclaratorResults));

            if (param->param_type == TYPE_VOID)
            {
              break;
            }

            TypeNode *param_type = arena_alloc(parser->type_arena);
            param_type->type = param->param_type;

            param_results = process_declarator(parser, param_results, param->declarator, param_type); 

            add_function_parameter_type(param_results->declaration_type, function_type);
            add_function_parameter_identifier_to_declarator_results(param_results->identifier, declaration_results);
          }

          return declaration_results;
        }
        default:
          fprintf(stderr, "ERROR - Parser: Cannot apply additional type derivations to a function type\n");
          exit(1);
      }
  }

  return declaration_results;
}

static void parse_function_declaration(Parser *parser, AstNode *function_node, StorageClassType storage_class_type, DeclaratorResults *declaration_results) {
  function_node->line_number = current_token(parser)->line;
  function_node->type = AST_FUNCTION_DECLARATION;
  function_node->data.declaration_function.name = declaration_results->identifier; 
  function_node->data.declaration_function.function_type = declaration_results->declaration_type;
  function_node->data.declaration_function.parameter_identifier_capacity = 0;
  function_node->data.declaration_function.parameter_identifiers = NULL;
  function_node->data.declaration_function.storage_class_type = storage_class_type;

  for (int i = 0; i < declaration_results->param_identifiers_count; i++) {
    add_function_parameter_identifier(declaration_results->param_identifiers[i], function_node);
  }

  //If semicolon is found, then it is considered a function definition
  if (current_token(parser)->type == TOKEN_SEMICOLON) {
    expect(parser, TOKEN_SEMICOLON);
    return;
  }
  
  AstNode *block_node = arena_alloc(parser->node_arena);
  function_node->data.declaration_function.body_block = block_node;
  parse_block(parser, block_node);
}

static void parse_variable_declaration(Parser *parser, AstNode *variable_node, StorageClassType storage_class_type, DeclaratorResults *declaration_results) {
  variable_node->line_number = current_token(parser)->line;
  variable_node->type = AST_VARIABLE_DECLARATION;
  variable_node->data.declaration_variable.name = declaration_results->identifier;
  variable_node->data.declaration_variable.storage_class_type = storage_class_type;
  variable_node->data.declaration_variable.type = declaration_results->declaration_type; 

  if (current_token(parser)->type == TOKEN_EQUAL) {
    expect(parser, TOKEN_EQUAL);

    AstNode *initializer_node = arena_alloc(parser->node_arena);
    parse_initializer(parser, initializer_node);
    
    variable_node->data.declaration_variable.has_expression = true;
    variable_node->data.declaration_variable.init_expression = initializer_node;
  }

  expect(parser, TOKEN_SEMICOLON);
}

static void parse_initializer(Parser *parser, AstNode *initializer_node) {
  if (current_token(parser)->type == TOKEN_OPEN_BRACE) {
    expect(parser, TOKEN_OPEN_BRACE);

    initializer_node->line_number = current_token(parser)->line;
    initializer_node->type = AST_INITIALIZER;
    initializer_node->data.initializer.type = AST_INITIALIZER_COMPOUND;

    CompoundInitArray *compound_init_array = malloc(sizeof(CompoundInitArray));
    compound_init_array->count = 0;
    compound_init_array->capacity = 0;
    compound_init_array->items = NULL;

    initializer_node->data.initializer.initializer_node.compound_initializer = compound_init_array;

    AstNode *next_initializer_node = arena_alloc(parser->node_arena);
    parse_initializer(parser, next_initializer_node);

    dynamic_array_add(compound_init_array, *next_initializer_node, COMPOUND_INITIALIZER_CAPACITY);

    //parse_expression(parser, &next_initializer_node, 0);
    //add_compound_initializer(initializer_node, next_initializer_node);

    while(current_token(parser)->type == TOKEN_COMMA) {
      expect(parser, TOKEN_COMMA);

      if (current_token(parser)->type == TOKEN_CLOSE_BRACE) {
        break;
      }

      next_initializer_node = arena_alloc(parser->node_arena);
      parse_initializer(parser, next_initializer_node);      
      dynamic_array_add(compound_init_array, *next_initializer_node, COMPOUND_INITIALIZER_CAPACITY);
    }

    expect(parser, TOKEN_CLOSE_BRACE);
    return;
  }

  AstNode *expression_node = arena_alloc(parser->node_arena);
  parse_expression(parser, &expression_node, 0);

  initializer_node->type = AST_INITIALIZER;
  initializer_node->line_number = current_token(parser)->line;
  initializer_node->data.initializer.type = AST_INITIALIZER_SINGLE;
  initializer_node->data.initializer.initializer_node.single_init_expression = expression_node;
}

static void parse_block(Parser *parser, AstNode *block_node) {
  block_node->line_number = current_token(parser)->line;

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

    //@Debt: This if check looks like it's going to grow larger as we add more types. Look to see if there is a better way to check declarations from statements
    switch (current_token(parser)->type) {
      case TOKEN_INT:
      case TOKEN_LONG:
      case TOKEN_DOUBLE:
      case TOKEN_CHAR:
      case TOKEN_UNSIGNED:
      case TOKEN_SIGNED:
      case TOKEN_EXTERN:
      case TOKEN_STATIC: {
        AstNode *declaration_node = arena_alloc(parser->node_arena);
        parse_declaration(parser, declaration_node);
        block_node->data.block.block_count++;
        add_to_node_pointer(declaration_node, block_item_pointers);
      }        
      default: {
        AstNode *statement_node = arena_alloc(parser->node_arena);
        parse_statement(parser, &statement_node);
        block_node->data.block.block_count++;
        add_to_node_pointer(statement_node, block_item_pointers);
      }
    }    
  }
}

static void parse_statement_compound_statement(Parser *parser, AstNode *compound_statement_node) {
  AstNode *block_node = arena_alloc(parser->node_arena);

  compound_statement_node->type = AST_STATEMENT_COMPOUND;
  compound_statement_node->data.statement_compound.block = block_node;

  parse_block(parser, block_node);
}

static char* get_identifier(Parser *parser) {
  int start = current_token(parser)->start_index;
  int end = current_token(parser)->end_index;

  if (parser->file[start] >= 48 && parser->file[start] <= 57) {
    input_error_with_line("Identifier cannot start with a number", current_token(parser)->line);
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
    panic("Incomplete statement");
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
  statement_node->line_number = current_token(parser)->line;

  expect(parser, TOKEN_RETURN);

  AstNode *expression = arena_alloc(parser->node_arena);
  parse_expression(parser, &expression, 0);
  
  statement_node->type = AST_STATEMENT_RETURN;
  statement_node->data.statement_return.expression = expression;

  expect(parser, TOKEN_SEMICOLON);
}

static void parse_statement_if(Parser *parser, AstNode *if_statement_node) {
  if_statement_node->line_number = current_token(parser)->line;
  
  expect(parser, TOKEN_IF);
  expect(parser, TOKEN_OPEN_PAREN);

  AstNode *condition_expression = arena_alloc(parser->node_arena);
  parse_expression(parser, &condition_expression, 0);

  expect(parser, TOKEN_CLOSE_PAREN);


  AstNode *statement = arena_alloc(parser->node_arena);
  parse_statement(parser, &statement);

  if_statement_node->type = AST_STATEMENT_IF;
  if_statement_node->data.statement_if.condition_expression = condition_expression;
  if_statement_node->data.statement_if.then_statement = statement;

  if (current_token(parser)->type != TOKEN_ELSE) {
    return;
  }

  expect(parser, TOKEN_ELSE);
  AstNode *else_statement = arena_alloc(parser->node_arena);
  parse_statement(parser, &else_statement);

  if_statement_node->data.statement_if.else_statement = else_statement;
}

static void parse_statement_goto(Parser *parser, AstNode *goto_statement_node) {
  goto_statement_node->line_number = current_token(parser)->line;

  expect(parser, TOKEN_GOTO);

  char *goto_label = get_identifier(parser);

  goto_statement_node->type = AST_STATEMENT_GOTO;
  goto_statement_node->data.statement_goto.label = goto_label;

  expect(parser, TOKEN_SEMICOLON);
}

static void parse_statement_break(Parser *parser, AstNode *break_statement_node) {
  break_statement_node->line_number = current_token(parser)->line;

  expect(parser, TOKEN_BREAK);
  expect(parser, TOKEN_SEMICOLON);

  break_statement_node->type = AST_STATEMENT_BREAK;
}
  
static void parse_statement_continue(Parser *parser, AstNode *continue_statement_node) {
  continue_statement_node->line_number = current_token(parser)->line;

  expect(parser, TOKEN_CONTINUE);
  expect(parser, TOKEN_SEMICOLON);

  continue_statement_node->type = AST_STATEMENT_CONTINUE;
}

static void parse_statement_while(Parser *parser, AstNode *while_statement_node) {
  while_statement_node->line_number = current_token(parser)->line;

  expect(parser, TOKEN_WHILE);
  expect(parser, TOKEN_OPEN_PAREN);

  AstNode *condition_expression = arena_alloc(parser->node_arena);
  parse_expression(parser, &condition_expression, 0);
  
  expect(parser, TOKEN_CLOSE_PAREN);

  AstNode *statements = arena_alloc(parser->node_arena);
  parse_statement(parser, &statements);

  while_statement_node->type = AST_STATEMENT_WHILE;
  while_statement_node->data.statement_while.condition = condition_expression;
  while_statement_node->data.statement_while.statement_body = statements;
}

static void parse_statement_do(Parser *parser, AstNode *do_statement_node) {
  do_statement_node->line_number = current_token(parser)->line;
  
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
  do_statement_node->data.statement_do_while.condition = condition_expression;
  do_statement_node->data.statement_do_while.statement_body = statements;
}

static void parse_statement_for(Parser *parser, AstNode *for_statement_node) {
  for_statement_node->line_number = current_token(parser)->line;
  
  expect(parser, TOKEN_FOR);
  expect(parser, TOKEN_OPEN_PAREN);

  for_statement_node->type = AST_STATEMENT_FOR;

  AstNode *dec_or_exp = arena_alloc(parser->node_arena);

  Specifier type_specifier = parse_specifier(parser, true);

  if (type_specifier.specifier_type_found) {
    Declarator *declarator = parse_declarator(parser);
    TypeNode *base_type = arena_alloc(parser->type_arena);
    base_type->type = type_specifier.specifier_type;

    DeclaratorResults results = {
      .param_identifiers = NULL,
      .param_identifiers_count = 0,
      .param_identifiers_capacity = 0
    };

    process_declarator(parser, &results, declarator, base_type);
    parse_variable_declaration(parser, dec_or_exp, AST_STORAGE_CLASS_NONE, &results);
  } else if (current_token(parser)->type == TOKEN_SEMICOLON) {
    expect(parser, TOKEN_SEMICOLON);    
    dec_or_exp = NULL;
  } else {
    parse_expression(parser, &dec_or_exp, 0);
    //TODO: Weird we do this for expressions but are handled in ast_declaration()
    expect(parser, TOKEN_SEMICOLON);
  }

  for_statement_node->data.statement_for.for_loop_init = dec_or_exp;

  if (current_token(parser)->type != TOKEN_SEMICOLON) {
    AstNode *for_condition = arena_alloc(parser->node_arena);
    parse_expression(parser, &for_condition, 0);
    for_statement_node->data.statement_for.condition_expression = for_condition;
  }

  expect(parser, TOKEN_SEMICOLON);

  if (current_token(parser)->type != TOKEN_SEMICOLON && current_token(parser)->type != TOKEN_CLOSE_PAREN) {
    AstNode *post_expression = arena_alloc(parser->node_arena);
    parse_expression(parser, &post_expression, 0);
    for_statement_node->data.statement_for.post_expression = post_expression;
  }

  expect(parser, TOKEN_CLOSE_PAREN);

  AstNode *for_statements = arena_alloc(parser->node_arena);
  parse_statement(parser, &for_statements);

  for_statement_node->data.statement_for.statement_body = for_statements;    
}

static void parse_expression(Parser *parser, AstNode **expression_node, int min_precedence) {
  parse_unary_expression(parser, expression_node);

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
  postfix_assignment->line_number = current_token(parser)->line;
  postfix_assignment->type = AST_EXPRESSION_ASSIGNMENT;
  postfix_assignment->data.expression_assignment.left_expression = left_expression;

  AstNode *postfix_constant = arena_alloc(parser->node_arena);
  postfix_constant->type = AST_EXPRESSION_CONSTANT;
  postfix_constant->line_number = current_token(parser)->line;
  postfix_constant->data.expression_constant.int_value = 1;
  postfix_constant->data.expression_constant.expression_type = NULL;
  
  AstNode *postfix_binary = arena_alloc(parser->node_arena);
  postfix_binary->type = AST_EXPRESSION_BINARY;
  postfix_binary->line_number = current_token(parser)->line;
  
  if (postfix_token == TOKEN_INCREMENT) {
    postfix_binary->data.expression_binary.op_type = AST_BINARY_ADD;
  } else {
    postfix_binary->data.expression_binary.op_type = AST_BINARY_SUBTRACT;
  }
  
  postfix_binary->data.expression_binary.left_expression = left_expression;  
  postfix_binary->data.expression_binary.right_expression = postfix_constant;
  postfix_binary->data.expression_binary.expression_type = NULL;
  postfix_assignment->data.expression_assignment.right_expression = postfix_binary;
  postfix_expression->data.expression_increment_decrement.expression = postfix_assignment;
}

static void parse_expression_assignment(Parser *parser, AstNode *assignment_expression, AstNode *left_factor, TokenType assignment_token) {
  assignment_expression->line_number = current_token(parser)->line;
  //right-associative assignment
  expect(parser, TOKEN_EQUAL);

  AstNode *right = arena_alloc(parser->node_arena);
  parse_expression(parser, &right, get_precedence(assignment_token));

  assignment_expression->type = AST_EXPRESSION_ASSIGNMENT;
  assignment_expression->data.expression_assignment.left_expression = left_factor;
  assignment_expression->data.expression_assignment.right_expression = right;
  assignment_expression->data.expression_assignment.expression_type = NULL;
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
  conditional_expression_node->data.expression_conditional.condition = left_expression;
  conditional_expression_node->data.expression_conditional.true_expression = middle;
  conditional_expression_node->data.expression_conditional.false_expression = right;
  conditional_expression_node->data.expression_conditional.expression_type = NULL;
}

static void parse_expression_binary(Parser *parser, AstNode **binary_expression, AstNode *left_expression, TokenType op_type) {  
  parser-> current_token_index++;

  AstNode *right = arena_alloc(parser->node_arena);
  parse_expression(parser, &right, get_precedence(op_type) + 1);

  AstNode *binary_expression_pointer = *binary_expression;
  binary_expression_pointer->type = AST_EXPRESSION_BINARY;
  binary_expression_pointer->line_number = current_token(parser)->line;
  binary_expression_pointer->data.expression_binary.left_expression = left_expression;
  binary_expression_pointer->data.expression_binary.right_expression = right;
  binary_expression_pointer->data.expression_binary.expression_type = NULL;
 
  switch (op_type) {
    case TOKEN_PLUS:                        binary_expression_pointer->data.expression_binary.op_type = AST_BINARY_ADD; break;
    case TOKEN_NEGATION:                    binary_expression_pointer->data.expression_binary.op_type = AST_BINARY_SUBTRACT; break;
    case TOKEN_ASTERISK:                    binary_expression_pointer->data.expression_binary.op_type = AST_BINARY_MULTIPLY; break;
    case TOKEN_FORWARD_SLASH:               binary_expression_pointer->data.expression_binary.op_type = AST_BINARY_DIVIDE; break;
    case TOKEN_PERCENT:                     binary_expression_pointer->data.expression_binary.op_type = AST_BINARY_REMAINDER; break;
    case TOKEN_BITWISE_AND:                 binary_expression_pointer->data.expression_binary.op_type = AST_BINARY_BITWISE_AND; break;
    case TOKEN_BITWISE_OR:                  binary_expression_pointer->data.expression_binary.op_type = AST_BINARY_BITWISE_OR; break;
    case TOKEN_BITWISE_XOR:                 binary_expression_pointer->data.expression_binary.op_type = AST_BINARY_BITWISE_XOR; break;
    case TOKEN_BITWISE_LEFT_SHIFT:          binary_expression_pointer->data.expression_binary.op_type = AST_BINARY_BITWISE_LEFT_SHIFT; break;
    case TOKEN_BITWISE_RIGHT_SHIFT:         binary_expression_pointer->data.expression_binary.op_type = AST_BINARY_BITWISE_RIGHT_SHIFT; break;
    case TOKEN_RELATIONAL_LESS_THAN:        binary_expression_pointer->data.expression_binary.op_type = AST_BINARY_LESS_THAN; break;
    case TOKEN_RELATIONAL_LESS_OR_EQUAL:    binary_expression_pointer->data.expression_binary.op_type = AST_BINARY_LESS_OR_EQUAL; break;
    case TOKEN_RELATIONAL_GREATER_THAN:     binary_expression_pointer->data.expression_binary.op_type = AST_BINARY_GREATER_THAN; break;
    case TOKEN_RELATIONAL_GREATER_OR_EQUAL: binary_expression_pointer->data.expression_binary.op_type = AST_BINARY_GREATER_OR_EQUAL; break;
    case TOKEN_RELATIONAL_EQUAL:            binary_expression_pointer->data.expression_binary.op_type = AST_BINARY_EQUAL; break;
    case TOKEN_RELATIONAL_NOT_EQUAL:        binary_expression_pointer->data.expression_binary.op_type = AST_BINARY_NOT_EQUAL; break;
    case TOKEN_LOGICAL_AND:                 binary_expression_pointer->data.expression_binary.op_type = AST_BINARY_AND; break;
    case TOKEN_LOGICAL_OR:                  binary_expression_pointer->data.expression_binary.op_type = AST_BINARY_OR; break;
    case TOKEN_PLUS_EQUAL:                  binary_expression_pointer->data.expression_binary.op_type = AST_BINARY_ADD; break;
    case TOKEN_NEGATION_EQUAL:              binary_expression_pointer->data.expression_binary.op_type = AST_BINARY_SUBTRACT; break;
    case TOKEN_ASTERISK_EQUAL:              binary_expression_pointer->data.expression_binary.op_type = AST_BINARY_MULTIPLY; break;
    case TOKEN_FORWARD_SLASH_EQUAL:         binary_expression_pointer->data.expression_binary.op_type = AST_BINARY_DIVIDE; break;
    case TOKEN_PERCENT_EQUAL:               binary_expression_pointer->data.expression_binary.op_type = AST_BINARY_REMAINDER; break;
    case TOKEN_BITWISE_AND_EQUAL:           binary_expression_pointer->data.expression_binary.op_type = AST_BINARY_BITWISE_AND; break;
    case TOKEN_BITWISE_OR_EQUAL:            binary_expression_pointer->data.expression_binary.op_type = AST_BINARY_BITWISE_OR; break;
    case TOKEN_BITWISE_XOR_EQUAL:           binary_expression_pointer->data.expression_binary.op_type = AST_BINARY_BITWISE_XOR; break;
    case TOKEN_BITWISE_RIGHT_SHIFT_EQUAL:   binary_expression_pointer->data.expression_binary.op_type = AST_BINARY_BITWISE_RIGHT_SHIFT; break;
    case TOKEN_BITWISE_LEFT_SHIFT_EQUAL:    binary_expression_pointer->data.expression_binary.op_type = AST_BINARY_BITWISE_LEFT_SHIFT; break;
    default:
      panic("Expected Binary op token, found '%s'", get_token_name(op_type));
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
      assignment_expression->line_number = current_token(parser)->line;
      assignment_expression->data.expression_assignment.left_expression = left_expression;
      assignment_expression->data.expression_assignment.right_expression = binary_expression_pointer;
      assignment_expression->data.expression_assignment.expression_type = NULL;
      *binary_expression = assignment_expression;
      return;
    }
    default:
     return;
  }
}

static void parse_unary_expression(Parser *parser, AstNode **unary_node) {
 if (end_of_file(parser)) {
    panic("Incomplete expression");
  }

  switch(current_token(parser)->type) {
    case TOKEN_NEGATION:
    case TOKEN_BITWISE_NOT:
    case TOKEN_LOGICAL_NOT:
      parse_factor_unary(parser, *unary_node); break;
    case TOKEN_INCREMENT:
    case TOKEN_DECREMENT:
      parse_factor_prefix_expression(parser, *unary_node); break;
    case TOKEN_BITWISE_AND:
      parse_factor_address_of(parser, *unary_node); break;
    case TOKEN_ASTERISK:
      parse_factor_dereference(parser, *unary_node); break;
    case TOKEN_OPEN_PAREN: {
      if (is_type_identifier_token(peek_next_token(parser))) {
        parse_factor_cast_expression(parser, *unary_node); 
        break;
      }

      parse_factor_parenthetical_expression(parser, unary_node);
      break;
    }
    default:
      parse_unary_postfix_expression(parser, unary_node);
      break;
  }  
}

static void parse_unary_postfix_expression(Parser *parser, AstNode **postfix_node) {
  parse_primary_expression(parser, postfix_node);

  if (current_token(parser)->type != TOKEN_OPEN_BRACKET) {
    return;
  }
  
  AstNode *subscript_node = arena_alloc(parser->node_arena);
  subscript_node->type = AST_EXPRESSION_SUBSCRIPT;

  parse_subscript_expression(parser, *postfix_node, subscript_node);

  *postfix_node = subscript_node;
}

static void parse_subscript_expression(Parser *parser, AstNode *postfix_node, AstNode *subscript_node) {
  subscript_node->line_number = current_token(parser)->line;
  
  expect(parser, TOKEN_OPEN_BRACKET);

  AstNode *subscript_expression = arena_alloc(parser->node_arena);
  parse_expression(parser, &subscript_expression, 0);

  expect(parser, TOKEN_CLOSE_BRACKET);

  if (current_token(parser)->type != TOKEN_OPEN_BRACKET) {
    subscript_node->type = AST_EXPRESSION_SUBSCRIPT;
    subscript_node->data.expression_subscript.expression_1 = postfix_node;
    subscript_node->data.expression_subscript.expression_2 = subscript_expression;
    return;
  }

  AstNode *next_subscript_node = arena_alloc(parser->node_arena);
  parse_subscript_expression(parser, postfix_node, next_subscript_node); 

  subscript_node->type = AST_EXPRESSION_SUBSCRIPT;
  subscript_node->data.expression_subscript.expression_1 = next_subscript_node;
  subscript_node->data.expression_subscript.expression_2 = subscript_expression;
}

static void parse_primary_expression(Parser *parser, AstNode **expression_node) {
  switch(current_token(parser)->type) {
    case TOKEN_CONSTANT_INT:
    case TOKEN_CONSTANT_LONG:
    case TOKEN_CONSTANT_FLOAT:
    case TOKEN_CONSTANT_UNSIGNED_INT:
    case TOKEN_CONSTANT_UNSIGNED_LONG:
    case TOKEN_CONSTANT_CHARACTER:
      parse_factor_constant(parser, *expression_node, current_token(parser)->type);
      break;
    case TOKEN_IDENTIFIER: {    
      char *identifier = get_identifier(parser);

      switch(current_token(parser)->type) {
        case TOKEN_OPEN_PAREN: parse_factor_function_call(parser, *expression_node, identifier); break;
        default:               parse_factor_variable_expression(parser, *expression_node, identifier); break;
      }      
      break;
    }
    default:
      panic("Invalid primary expression token '%s'", get_token_name(current_token(parser)->type));
  }
}

static void parse_factor(Parser *parser, AstNode **factor_node) {
 if (end_of_file(parser)) {
    panic("Incomplete expression");
  }

  switch(current_token(parser)->type) {
    case TOKEN_CONSTANT_INT:
    case TOKEN_CONSTANT_LONG:
    case TOKEN_CONSTANT_FLOAT:
    case TOKEN_CONSTANT_UNSIGNED_INT:
    case TOKEN_CONSTANT_UNSIGNED_LONG:
      parse_factor_constant(parser, *factor_node, current_token(parser)->type); break;
    case TOKEN_NEGATION:
    case TOKEN_BITWISE_NOT:
    case TOKEN_LOGICAL_NOT:
      parse_factor_unary(parser, *factor_node); break;
    case TOKEN_INCREMENT:
    case TOKEN_DECREMENT:
      parse_factor_prefix_expression(parser, *factor_node); break;
    case TOKEN_BITWISE_AND:
      parse_factor_address_of(parser, *factor_node); break;
    case TOKEN_ASTERISK:
      parse_factor_dereference(parser, *factor_node); break;
    case TOKEN_OPEN_PAREN: {
      if (is_type_identifier_token(peek_next_token(parser))) {
        parse_factor_cast_expression(parser, *factor_node); 
        break;
      }

      parse_factor_parenthetical_expression(parser, factor_node);
      break;
    }
    case TOKEN_IDENTIFIER: {    
      char *identifier = get_identifier(parser);

      switch(current_token(parser)->type) {
        case TOKEN_OPEN_PAREN: parse_factor_function_call(parser, *factor_node, identifier); break;
        default:               parse_factor_variable_expression(parser, *factor_node, identifier); break;
      }      
      break;
    }
    default:
      panic("Failed to parse factor for '%s' token", get_token_name(current_token(parser)->type));
  }
}

static void parse_factor_constant(Parser *parser, AstNode *factor_node, TokenType constant_type) {
  factor_node->line_number = current_token(parser)->line;

  expect(parser, constant_type);

  factor_node->type = AST_EXPRESSION_CONSTANT;

  char constant_slice[previous_token(parser)->end_index - (previous_token(parser)->start_index - 2)];
  strncpy(constant_slice, parser->file + previous_token(parser)->start_index, ((previous_token(parser)->end_index ) - previous_token(parser)->start_index) + 1);
  constant_slice[previous_token(parser)->end_index - (previous_token(parser)->start_index) + 1] = '\0';

  TypeNode *expression_type = arena_alloc(parser->type_arena);

  if (constant_type == TOKEN_CONSTANT_CHARACTER) {
    factor_node->data.expression_constant.constant_type = AST_CONSTANT_TYPE_CHAR;
    factor_node->data.expression_constant.char_value = (int)constant_slice[0];

    expression_type->type = TYPE_CHAR;
    factor_node->data.expression_constant.expression_type = expression_type;

    return;
  }
  
  //@Note: Floating points constants can't go out of range since a double supports positive and negative infinity
  if (constant_type == TOKEN_CONSTANT_FLOAT) {
    char *end_pointer;
    double double_value = strtod(constant_slice, &end_pointer);

    factor_node->data.expression_constant.constant_type = AST_CONSTANT_TYPE_DOUBLE;
    factor_node->data.expression_constant.double_value = double_value;

    expression_type->type = TYPE_DOUBLE;  
    
    factor_node->data.expression_constant.expression_type = expression_type;
    return;    
  }

  if (constant_type == TOKEN_CONSTANT_UNSIGNED_LONG) {
    char *end_pointer;
    unsigned long ul_value = strtoul(constant_slice, &end_pointer, BASE_TEN);

    factor_node->data.expression_constant.constant_type = AST_CONSTANT_TYPE_ULONG;
    factor_node->data.expression_constant.ulong_value = ul_value;

    expression_type->type = TYPE_ULONG;  
    
    factor_node->data.expression_constant.expression_type = expression_type;
    return;    
  }

  char *end_pointer;
  long constant_value = strtol(constant_slice, &end_pointer, BASE_TEN);
  
  if (constant_value < LONG_MIN || constant_value > LONG_MAX) {
    input_error_with_line("Out of bounds int/long constant '%ld'", current_token(parser)->line, constant_value);
  }
  
  if (constant_type == TOKEN_CONSTANT_INT && constant_value > INT_MIN && constant_value < INT_MAX) {
    factor_node->data.expression_constant.constant_type = AST_CONSTANT_TYPE_INT;
    factor_node->data.expression_constant.int_value = (int)constant_value;

    expression_type->type = TYPE_INT;  
    
    factor_node->data.expression_constant.expression_type = expression_type;

    return;
  }
  
  if (constant_type == TOKEN_CONSTANT_UNSIGNED_INT && constant_value >= 0  && constant_value < UINT_MAX) {
    factor_node->data.expression_constant.constant_type = AST_CONSTANT_TYPE_UINT;
    factor_node->data.expression_constant.uint_value = (unsigned int)constant_value;

    expression_type->type = TYPE_UINT;  
    
    factor_node->data.expression_constant.expression_type = expression_type;

    return;
  }

  //TODO: Added unsigned logic above. Re-evaluate this
  if (constant_type == TOKEN_CONSTANT_UNSIGNED_LONG && constant_value >= 0  && constant_value < ULONG_MAX) {
    factor_node->data.expression_constant.constant_type = AST_CONSTANT_TYPE_ULONG;
    factor_node->data.expression_constant.ulong_value = (unsigned long)constant_value;

    expression_type->type = TYPE_ULONG;  
    
    factor_node->data.expression_constant.expression_type = expression_type;

    return;
  }

  factor_node->data.expression_constant.constant_type = AST_CONSTANT_TYPE_LONG;
  factor_node->data.expression_constant.long_value = constant_value;

  expression_type->type = TYPE_LONG;  
    
  factor_node->data.expression_constant.expression_type = expression_type;
}

static void parse_factor_unary(Parser *parser, AstNode *factor_node) {
  factor_node->line_number = current_token(parser)->line;
  
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
      panic("Unary token type '%s' not found", get_token_name(current_token(parser)->type));
  }
  
  parser->current_token_index++;

  AstNode *unary_value_expression_node = arena_alloc(parser->node_arena);
  parse_unary_expression(parser, &unary_value_expression_node);

  factor_node->type = AST_EXPRESSION_UNARY;
  factor_node->data.expression_unary.op_type = op_type;  
  factor_node->data.expression_unary.expression = unary_value_expression_node;
  factor_node->data.expression_unary.expression_type = NULL;
}

static void parse_factor_prefix_expression(Parser *parser, AstNode *factor_node) {
  // AstNode *prefix_expression = arena_alloc(parser->node_arena);
  // prefix_expression->line_number = current_token(parser)->line;

  NodeType expression_type;

  if (current_token(parser)->type == TOKEN_INCREMENT) {
    expect(parser, TOKEN_INCREMENT);
    expression_type = AST_EXPRESSION_PREFIX_INCREMENT;
    // prefix_expression->type = AST_EXPRESSION_PREFIX_INCREMENT;
  } else {
    expect(parser, TOKEN_DECREMENT);
    expression_type = AST_EXPRESSION_PREFIX_DECREMENT;
    // prefix_expression->type = AST_EXPRESSION_PREFIX_DECREMENT;
  }

  AstNode *left = arena_alloc(parser->node_arena);

  parse_unary_expression(parser, &left);

  factor_node->type = AST_EXPRESSION_ASSIGNMENT;
  factor_node->line_number = current_token(parser)->line;
  factor_node->data.expression_assignment.left_expression = left;

  AstNode *prefix_constant = arena_alloc(parser->node_arena);
  prefix_constant->type = AST_EXPRESSION_CONSTANT;
  prefix_constant->line_number = current_token(parser)->line;
  prefix_constant->data.expression_constant.int_value = 1;
  prefix_constant->data.expression_constant.expression_type = NULL;

  AstNode *prefix_binary = arena_alloc(parser->node_arena);
  prefix_binary->type = AST_EXPRESSION_BINARY;
  prefix_binary->line_number = current_token(parser)->line;

  if (expression_type == AST_EXPRESSION_PREFIX_INCREMENT) {
    prefix_binary->data.expression_binary.op_type = AST_BINARY_ADD;
  } else {
    prefix_binary->data.expression_binary.op_type = AST_BINARY_SUBTRACT;
  }

  prefix_binary->data.expression_binary.left_expression = left;
  prefix_binary->data.expression_binary.right_expression = prefix_constant;
  prefix_binary->data.expression_binary.expression_type = NULL;

  factor_node->data.expression_assignment.right_expression = prefix_binary;

  // prefix_expression->data.expression_increment_decrement.expression = factor_node;
}

static void parse_factor_parenthetical_expression(Parser *parser, AstNode **factor_node) {
  expect(parser, TOKEN_OPEN_PAREN);
  parse_expression(parser, factor_node, 0);    
  expect(parser, TOKEN_CLOSE_PAREN);
}

static void parse_factor_cast_expression(Parser *parser, AstNode *factor_node) {
  factor_node->line_number = current_token(parser)->line;

  expect(parser, TOKEN_OPEN_PAREN);

  Specifier specifier = parse_specifier(parser, true);

  TypeNode *type_node = arena_alloc(parser->type_arena);
  type_node->type = specifier.specifier_type; 

  AbstractDeclarator *abstract_declarator = parse_abstract_declarator(parser);
  TypeNode *abstract_declarator_type_node = process_abstract_declarator(parser, abstract_declarator, type_node);
  
  expect(parser, TOKEN_CLOSE_PAREN);

  AstNode *expression_node = arena_alloc(parser->node_arena);
  parse_factor(parser, &expression_node);
  
  factor_node->type = AST_EXPRESSION_CAST;
  factor_node->data.expression_cast.target_type = abstract_declarator_type_node;
  factor_node->data.expression_cast.expression = expression_node;
  //factor_node->data.expression_cast.expression_type = NULL;
}

//@Note: '*' by itself is a valid abstract declarator but not a valid regular declarator.
static AbstractDeclarator* parse_abstract_declarator(Parser *parser) {
  if (current_token(parser)->type == TOKEN_ASTERISK) {
    expect(parser, TOKEN_ASTERISK);

    AbstractDeclarator *pointer_declarator = malloc(sizeof(AbstractDeclarator));
    pointer_declarator->type = ABSTRACT_DECLARATOR_POINTER;
    pointer_declarator->data.abstract_pointer.abstract_declarator = parse_abstract_declarator(parser); 

    return pointer_declarator;
  }

  return parse_direct_abstract_declarator(parser);
}

static AbstractDeclarator* parse_direct_abstract_declarator(Parser *parser) {
  if (current_token(parser)->type == TOKEN_OPEN_PAREN) {
    expect(parser, TOKEN_OPEN_PAREN);
    AbstractDeclarator *abstract_declarator = parse_abstract_declarator(parser);
    expect(parser, TOKEN_CLOSE_PAREN);

    if (current_token(parser)->type != TOKEN_OPEN_BRACKET) {
      return abstract_declarator;
    }

    AbstractDeclarator *head_array_abstract = malloc(sizeof(AbstractDeclarator));
    head_array_abstract->type = ABSTRACT_ARRAY;
    AbstractDeclarator *tail_array_abstract = head_array_abstract;
    AbstractDeclarator *cur_array_abstract = head_array_abstract;

    while (current_token(parser)->type == TOKEN_OPEN_BRACKET) {
      expect(parser, TOKEN_OPEN_BRACKET);

      //@Debt: The code below is copied from parse_factor_constant(). Consolidate.

      char constant_slice[current_token(parser)->end_index - (current_token(parser)->start_index - 2)];
      strncpy(constant_slice, parser->file + current_token(parser)->start_index, ((current_token(parser)->end_index ) - current_token(parser)->start_index) + 1);
      constant_slice[current_token(parser)->end_index - (current_token(parser)->start_index) + 1] = '\0';
  
      char *end_pointer;
      // AbstractDeclarator *array_abstract = malloc(sizeof(AbstractDeclarator));
      // array_abstract->type = ABSTRACT_ARRAY;

      switch(current_token(parser)->type) {
        case TOKEN_CONSTANT_UNSIGNED_LONG:        
          cur_array_abstract->data.abstract_array.size = strtoul(constant_slice, &end_pointer, BASE_TEN);
          break;
        case TOKEN_CONSTANT_INT:
        case TOKEN_CONSTANT_LONG:
        case TOKEN_CONSTANT_UNSIGNED_INT:        
          cur_array_abstract->data.abstract_array.size = strtol(constant_slice, &end_pointer, BASE_TEN);
          break;
        default:
          panic("Unsupported array size type '%s'", get_token_name(current_token(parser)->type));
      }

      parser->current_token_index++;
      expect(parser, TOKEN_CLOSE_BRACKET);

      cur_array_abstract->data.abstract_array.abstract_declarator = head_array_abstract;
      head_array_abstract = cur_array_abstract;

      if (current_token(parser)->type != TOKEN_OPEN_BRACKET) {

        tail_array_abstract->data.abstract_array.abstract_declarator = abstract_declarator;
        break;
      }

      cur_array_abstract = malloc(sizeof(AbstractDeclarator));
      cur_array_abstract->type = ABSTRACT_ARRAY;      
    }

    // array_abstract->data.abstract_array.abstract_declarator = abstract_declarator;
    // return array_abstract;

    return head_array_abstract;
  }

  if (current_token(parser)->type == TOKEN_OPEN_BRACKET) {
    expect(parser, TOKEN_OPEN_BRACKET);

    char constant_slice[current_token(parser)->end_index - (current_token(parser)->start_index - 2)];
    strncpy(constant_slice, parser->file + current_token(parser)->start_index, ((current_token(parser)->end_index ) - current_token(parser)->start_index) + 1);
    constant_slice[current_token(parser)->end_index - (current_token(parser)->start_index) + 1] = '\0';
  
    char *end_pointer;
    AbstractDeclarator *array_abstract = malloc(sizeof(AbstractDeclarator));
    array_abstract->type = ABSTRACT_ARRAY;

    switch(current_token(parser)->type) {
      case TOKEN_CONSTANT_UNSIGNED_LONG:        
        array_abstract->data.abstract_array.size = strtoul(constant_slice, &end_pointer, BASE_TEN);
        break;
      case TOKEN_CONSTANT_INT:
      case TOKEN_CONSTANT_LONG:
      case TOKEN_CONSTANT_UNSIGNED_INT:        
        array_abstract->data.abstract_array.size = strtol(constant_slice, &end_pointer, BASE_TEN);
        break;
      default:
        fprintf(stderr, "ERROR - Parser: Unsupported array size type. (Line %d)", current_token(parser)->line);
        break;      
    }
    parser->current_token_index++;
    expect(parser, TOKEN_CLOSE_BRACKET);

    return array_abstract;
  }
   
  AbstractDeclarator *base_declarator = malloc(sizeof(AbstractDeclarator));
  base_declarator->type = ABSTRACT_DECLARATOR_BASE;

  return base_declarator;
}

static TypeNode* process_abstract_declarator(Parser *parser, AbstractDeclarator *abstract_declarator, TypeNode *base_type) {
  if (abstract_declarator->type == ABSTRACT_DECLARATOR_POINTER) {
    TypeNode *pointer_type = arena_alloc(parser->type_arena);
    pointer_type->type = TYPE_POINTER;
    pointer_type->data.pointer_type.reference_type = base_type;

    return process_abstract_declarator(parser, abstract_declarator->data.abstract_pointer.abstract_declarator, pointer_type);
  }

  if (abstract_declarator->type == ABSTRACT_ARRAY) {
    TypeNode *pointer_type = arena_alloc(parser->type_arena);
    pointer_type->type = TYPE_ARRAY;
    pointer_type->data.array_type.element_type = base_type;
    pointer_type->data.array_type.size = abstract_declarator->data.abstract_array.size;

    //return pointer_type;
    return process_abstract_declarator(parser, abstract_declarator->data.abstract_array.abstract_declarator, pointer_type);
  }

  return base_type;
}

static void parse_factor_goto_label(Parser *parser, AstNode *factor_node) {
  char *label_identifier = get_identifier(parser);

  expect(parser, TOKEN_COLON);
  factor_node->type = AST_STATEMENT_GOTO_LABEL;
  factor_node->data.statement_goto_label.label = label_identifier;
}

static void parse_factor_variable_expression(Parser *parser, AstNode *factor_node, char *label_identifier) {
  factor_node->line_number = current_token(parser)->line;
  factor_node->type = AST_EXPRESSION_VARIABLE;
  factor_node->data.expression_variable.identifier = label_identifier;
}

static void parse_factor_function_call(Parser *parser, AstNode *factor_node, char *identifier) {
  expect(parser, TOKEN_OPEN_PAREN);

  factor_node->type = AST_EXPRESSION_FUNCTION_CALL;
  factor_node->data.expression_function_call.identifier = identifier;
  factor_node->data.expression_function_call.argument_count = 0;
  factor_node->data.expression_function_call.expression_type = NULL;
  
  NodePointer *argument_pointers = malloc(sizeof(NodePointer));
  init_node_pointer(argument_pointers);
  factor_node->data.expression_function_call.argument_ptrs = argument_pointers;

  if (current_token(parser)->type == TOKEN_CLOSE_PAREN) {
    expect(parser, TOKEN_CLOSE_PAREN);
    return;
  }  

  AstNode *expression_node = arena_alloc(parser->node_arena);
  parse_expression(parser, &expression_node, 0);
  add_to_node_pointer(expression_node, argument_pointers);
  factor_node->data.expression_function_call.argument_count++;

  while (current_token(parser)->type == TOKEN_COMMA) {
    expect(parser, TOKEN_COMMA);
    AstNode *next_expression_node = arena_alloc(parser->node_arena);
    parse_expression(parser, &next_expression_node, 0);
    add_to_node_pointer(next_expression_node, argument_pointers);
    factor_node->data.expression_function_call.argument_count++;
  }

  expect(parser, TOKEN_CLOSE_PAREN);
} 

static void parse_factor_address_of(Parser *parser, AstNode *factor_node) {
  factor_node->line_number = current_token(parser)->line;
  expect(parser, TOKEN_BITWISE_AND);

  AstNode *address_of_expression = arena_alloc(parser->node_arena);

  parse_unary_expression(parser, &address_of_expression);

  if (address_of_expression->type == AST_EXPRESSION_ASSIGNMENT) {
    input_error_with_line("Illegal to take an address of an assignment", current_token(parser)->line);
  }

  factor_node->data.expression_address_of.expression = address_of_expression;
  factor_node->type = AST_EXPRESSION_ADDRESS_OF;
}

static void parse_factor_dereference(Parser *parser, AstNode *factor_node) {
  factor_node->line_number = current_token(parser)->line;
  
  //TODO: Look into if we should be using 'parse_abstract_declarator' and 'process_abstract_declarator'
  expect(parser, TOKEN_ASTERISK);
  
  AstNode *dereference_expression = arena_alloc(parser->node_arena);
  parse_factor(parser, &dereference_expression);

  factor_node->data.expression_dereference.expression = dereference_expression;
  factor_node->type = AST_EXPRESSION_DEREFERENCE;
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
  bool has_unsigned_specifier = false;
  bool has_signed_specifier = false;

  specifier.storage_class_type = AST_STORAGE_CLASS_NONE;

  while(true) {
    switch (current_token(parser)->type) {
      case TOKEN_STATIC:
        if (error_if_storage_class_found) {
          input_error_with_line("Declared type '%s' cannot contain 'static' storage class", current_token(parser)->line, get_token_name(current_token(parser)->type));
        }

        if (specifier.storage_class_type != AST_STORAGE_CLASS_NONE) {
          input_error_with_line("Declared 'static' cannot be included with '%s'", current_token(parser)->line, get_token_name(current_token(parser)->type));
        }
        
        specifier.storage_class_type = AST_STORAGE_CLASS_STATIC;
        parser->current_token_index++;
        continue;
      case TOKEN_EXTERN:
        if (error_if_storage_class_found) {
          input_error_with_line("Declared '%s' type cannot contain 'extern' storage class", current_token(parser)->line, get_token_name(current_token(parser)->type));
        }

        if (specifier.storage_class_type != AST_STORAGE_CLASS_NONE) {
          input_error_with_line("Declared 'static' cannot be included with '%s'", current_token(parser)->line, get_token_name(current_token(parser)->type));
        }

        specifier.storage_class_type = AST_STORAGE_CLASS_EXTERN;
        parser->current_token_index++;
        continue;
      case TOKEN_UNSIGNED:
      case TOKEN_SIGNED:
      case TOKEN_INT:
      case TOKEN_LONG:
      case TOKEN_DOUBLE:
      case TOKEN_CHAR: {
        if (current_token(parser)->type == TOKEN_UNSIGNED) {
          if (has_unsigned_specifier) {
            input_error_with_line("Cannot declare unsigned specifier more than once.", current_token(parser)->line);
          }

          has_unsigned_specifier = true;
        } else if (current_token(parser)->type == TOKEN_SIGNED) {
          if (has_signed_specifier) {
            input_error_with_line("Cannot declare signed specifier more than once.", current_token(parser)->line);
          }

          has_signed_specifier = true;
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

  if (has_signed_specifier && has_unsigned_specifier) {
    input_error_with_line("Cannot have both signed and unsigned specifier.", current_token(parser)->line);
  }

  for (int i = 0; i < type_specifier_count; i++) {
    if (type_specifiers[i] == TOKEN_DOUBLE) {
      if (type_specifier_count > 1) {
        input_error_with_line("Double cannot contain another type specifier.", current_token(parser)->line);
      } else {
        specifier.specifier_type = TYPE_DOUBLE;
        return specifier;
      }
    }
    
    if (type_specifiers[i] == TOKEN_LONG) {
      if (has_unsigned_specifier) {
        specifier.specifier_type = TYPE_ULONG;
      } else {
        specifier.specifier_type = TYPE_LONG;
      }
      
      return specifier;
    }

    if (type_specifiers[i] == TOKEN_INT ) {
      if (has_unsigned_specifier) {
        specifier.specifier_type = TYPE_UINT;
      } else {
        specifier.specifier_type = TYPE_INT;
      }
    }

    //@Debt: This function is getting hard to read. Doing special stuff here for chars.
    if (type_specifiers[i] == TOKEN_CHAR) {
      if (has_unsigned_specifier) {
        specifier.specifier_type = TYPE_UNSIGNED_CHAR;
      } else if (has_signed_specifier) {
        specifier.specifier_type = TYPE_SIGNED_CHAR;
      } else {
        specifier.specifier_type = TYPE_CHAR;
      }

      return specifier;
    }
  }

  if (has_unsigned_specifier) {
    specifier.specifier_type = TYPE_UINT;
  } else {
    specifier.specifier_type = TYPE_INT;
  }

  return specifier;
}

static Declarator* parse_declarator(Parser *parser) {
  if (current_token(parser)->type == TOKEN_ASTERISK) {
    expect(parser, TOKEN_ASTERISK);

    Declarator *pointer_declarator = malloc(sizeof(Declarator));    
    pointer_declarator->type = DECLARATOR_TYPE_POINTER;
    pointer_declarator->data.pointer_declaration.declarator = parse_declarator(parser);  

    return pointer_declarator;
  }

  return parse_direct_declarator(parser);
}

static Declarator* parse_direct_declarator(Parser *parser){
  Declarator *simple_declarator = parse_simple_declarator(parser);
  Declarator *suffix = parse_declarator_suffix(parser);

  if (suffix != NULL && suffix->type == DECLARATOR_FUNCTION) {
    suffix->data.function_declarator.declarator = simple_declarator;
    return suffix;
  }

  if (suffix != NULL && suffix->type == DECLARATOR_TYPE_ARRAY) {
    Declarator **inner_declarator = &suffix->data.array_declarator.declarator;

    while (*inner_declarator != NULL) {
      inner_declarator = &(*inner_declarator)->data.array_declarator.declarator;
    }

    *inner_declarator = simple_declarator;
    return suffix;
  }

  return simple_declarator;
}

static Declarator* parse_simple_declarator(Parser *parser) {
  if (current_token(parser)->type == TOKEN_IDENTIFIER) {
    char *identifier = get_identifier(parser);

    Declarator *identifier_declarator = malloc(sizeof(Declarator));
    identifier_declarator->type = DECLARATOR_TYPE_IDENTIFIER;
    identifier_declarator->data.identifier.identifier = identifier;

    return identifier_declarator;
  }
  
  //Supports casted declarations like '*(var)'
  if (current_token(parser)->type == TOKEN_OPEN_PAREN) {
    expect(parser, TOKEN_OPEN_PAREN);
    Declarator *declarator = parse_declarator(parser);
    expect(parser, TOKEN_CLOSE_PAREN);
    return declarator;
  }

  return NULL;  
}

static Declarator* parse_declarator_suffix(Parser *parser) {
  if (current_token(parser)->type == TOKEN_OPEN_BRACKET) {
    Declarator *head = malloc(sizeof(Declarator));
    head->type = DECLARATOR_TYPE_ARRAY;
    Declarator *tail = head;
    Declarator *cur_node = head;

    while (true) {
      expect(parser, TOKEN_OPEN_BRACKET);
      
      //@Debt: The code below is copied from parse_factor_constant(). Consolidate.

      char constant_slice[current_token(parser)->end_index - (current_token(parser)->start_index - 2)];
      strncpy(constant_slice, parser->file + current_token(parser)->start_index, ((current_token(parser)->end_index ) - current_token(parser)->start_index) + 1);
      constant_slice[current_token(parser)->end_index - (current_token(parser)->start_index) + 1] = '\0';

      char *end_pointer;

      switch(current_token(parser)->type) {
       case TOKEN_CONSTANT_UNSIGNED_LONG:
         cur_node->data.array_declarator.size = strtoul(constant_slice, &end_pointer, BASE_TEN);
         break;
       case TOKEN_CONSTANT_INT:
       case TOKEN_CONSTANT_LONG:
       case TOKEN_CONSTANT_UNSIGNED_INT:
         cur_node->data.array_declarator.size = strtol(constant_slice, &end_pointer, BASE_TEN);
         break;
       default:
         panic("Unsupported array size type '%s' in declarator suffix", get_token_name(current_token(parser)->type));
      }

      parser->current_token_index++;
      expect(parser, TOKEN_CLOSE_BRACKET);

      cur_node->data.array_declarator.declarator = head;
      head = cur_node;

      if (current_token(parser)->type != TOKEN_OPEN_BRACKET) {
        tail->data.array_declarator.declarator = NULL;
        break;
      }

      cur_node = malloc(sizeof(Declarator));
      cur_node->type = DECLARATOR_TYPE_ARRAY;
    }

    return head;
  }

  if (current_token(parser)->type == TOKEN_OPEN_PAREN) {
    expect(parser, TOKEN_OPEN_PAREN);
    
    Declarator *function_declarator = malloc(sizeof(Declarator));
    function_declarator->type = DECLARATOR_FUNCTION;
    // function_declarator->data.function_declarator.declarator = identifier_declarator;
    function_declarator->data.function_declarator.param_capacity = 0;
    function_declarator->data.function_declarator.param_count = 0;
    function_declarator->data.function_declarator.declarator_parameters = NULL;

    Specifier parameter_specifier = parse_specifier(parser, true);
    Declarator *param_declarator = parse_declarator(parser);

    add_function_parameter_to_declarator(function_declarator, parameter_specifier.specifier_type, param_declarator);

    while(current_token(parser)->type == TOKEN_COMMA) {
      expect(parser, TOKEN_COMMA);

      Specifier next_parameter_specifier = parse_specifier(parser, true);

      if (!next_parameter_specifier.specifier_type_found) {
        fprintf(stderr, "ERROR - Parser: Parameter specifier not found. (Line %d)\n", current_token(parser)->line);
        exit(1);
      }

      param_declarator = parse_declarator(parser);

      add_function_parameter_to_declarator(function_declarator, next_parameter_specifier.specifier_type, param_declarator);
    }

    expect(parser, TOKEN_CLOSE_PAREN);

    return function_declarator;
  }

  return NULL;
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
  if (function_declaration_node->data.declaration_function.parameter_identifier_count == function_declaration_node->data.declaration_function.parameter_identifier_capacity) {
    int size = function_declaration_node->data.declaration_function.parameter_identifier_capacity == 0 ? FUNCTION_IDENTIFIER_INIT_CAPACITY : function_declaration_node->data.declaration_function.parameter_identifier_capacity * 2;
    function_declaration_node->data.declaration_function.parameter_identifier_capacity = size;
    function_declaration_node->data.declaration_function.parameter_identifiers = realloc(function_declaration_node->data.declaration_function.parameter_identifiers, size * sizeof(char*));
  }

  function_declaration_node->data.declaration_function.parameter_identifiers[function_declaration_node->data.declaration_function.parameter_identifier_count] = identifier;
  function_declaration_node->data.declaration_function.parameter_identifier_count++;
}

static void add_function_parameter_to_declarator(Declarator *function_declarator, Types param_type, Declarator *param_declarator) {
  if (function_declarator->data.function_declarator.param_count == function_declarator->data.function_declarator.param_capacity) {
    int size = function_declarator->data.function_declarator.param_capacity == 0 ? FUNCTION_DECLARATOR_INIT_CAPACITY : function_declarator->data.function_declarator.param_capacity * 2;
    function_declarator->data.function_declarator.param_capacity = size;
    function_declarator->data.function_declarator.declarator_parameters = realloc(function_declarator->data.function_declarator.declarator_parameters, size * sizeof(DeclaratorParameter));    
  }

  int count = function_declarator->data.function_declarator.param_count;

  function_declarator->data.function_declarator.declarator_parameters[count].param_type = param_type;
  function_declarator->data.function_declarator.declarator_parameters[count].declarator = param_declarator;
  function_declarator->data.function_declarator.param_count++;
}   

static void add_function_parameter_identifier_to_declarator_results(char *identifier, DeclaratorResults *declarator_results) {  
  if (declarator_results->param_identifiers_count == declarator_results->param_identifiers_capacity) {
    int size = declarator_results->param_identifiers_capacity == 0 ? FUNCTION_IDENTIFIER_INIT_CAPACITY : declarator_results->param_identifiers_capacity * 2;
    declarator_results->param_identifiers_capacity = size;
    declarator_results->param_identifiers = realloc(declarator_results->param_identifiers, size * sizeof(char*));
  }

  declarator_results->param_identifiers[declarator_results->param_identifiers_count] = identifier;
  declarator_results->param_identifiers_count++;
}

char* get_binary_op_type_string(BinaryOpType op_type) {
  switch (op_type) {
    case AST_BINARY_EQUAL:                return "Equal";
    case AST_BINARY_NOT_EQUAL:            return "Not Equal";
    case AST_BINARY_LESS_THAN:            return "Less Than";
    case AST_BINARY_LESS_OR_EQUAL:        return "Less Equal Than";
    case AST_BINARY_GREATER_THAN:         return "Greater Than";
    case AST_BINARY_GREATER_OR_EQUAL:     return "Greater Equal Than";
    case AST_BINARY_ADD:                  return "Add";
    case AST_BINARY_SUBTRACT:             return "Subtract";
    case AST_BINARY_MULTIPLY:             return "Multiply";
    case AST_BINARY_DIVIDE:               return "Divide";
    case AST_BINARY_REMAINDER:            return "Modulo";
    case AST_BINARY_AND:                  return "Logical And";
    case AST_BINARY_OR:                   return "Local Or";
    case AST_BINARY_BITWISE_OR:           return "Bitwise OR";
    case AST_BINARY_BITWISE_AND:          return "Bitwise AND";
    case AST_BINARY_BITWISE_XOR:          return "Bitwise XOR";
    case AST_BINARY_BITWISE_LEFT_SHIFT:   return "Bitwise Left Shift";
    case AST_BINARY_BITWISE_RIGHT_SHIFT:  return "Bitwise Right Shift";
  }
}

char* get_ast_node_string(AstNode *node) {
  switch (node->type) {
    case AST_PROGRAM:                       return "Program";
    case AST_VARIABLE_DECLARATION:          return "Variable Declaration";
    case AST_FUNCTION_DECLARATION:          return "Function Declaration";
    case AST_INITIALIZER:                   return "Initializer";
    case AST_BLOCK:                         return "Block";
    case AST_STATEMENT_RETURN:              return "Return Statement";
    case AST_STATEMENT_NULL:                return "Null Statement";
    case AST_STATEMENT_IF:                  return "If Statement";
    case AST_STATEMENT_GOTO:                return "Goto Statement";
    case AST_STATEMENT_GOTO_LABEL:          return "Goto Label Statement";
    case AST_STATEMENT_BREAK:               return "Break Statement";
    case AST_STATEMENT_CONTINUE:            return "Continue Statement";
    case AST_STATEMENT_WHILE:               return "While Statement";
    case AST_STATEMENT_DO_WHILE:            return "Do While Statement";
    case AST_STATEMENT_FOR:                 return "For Statement";
    case AST_STATEMENT_COMPOUND:            return "Compound Statement";
    case AST_EXPRESSION_CAST:               return "Cast Expression";
    case AST_EXPRESSION_BINARY:             return "Binary Expression";
    case AST_EXPRESSION_CONSTANT:           return "Constant Expression";
    case AST_EXPRESSION_UNARY:              return "Unary Expression";
    case AST_EXPRESSION_VARIABLE:           return "Variable Expression";
    case AST_EXPRESSION_ASSIGNMENT:         return "Assignment Expression";
    case AST_EXPRESSION_CONDITIONAL:        return "Conditional Expression";
    case AST_EXPRESSION_POSTFIX_INCREMENT:  return "Postfix Increment Expression";
    case AST_EXPRESSION_POSTFIX_DECREMENT:  return "Postfix Decrement Expression";
    case AST_EXPRESSION_PREFIX_INCREMENT:   return "Prefix Increment Expression";
    case AST_EXPRESSION_PREFIX_DECREMENT:   return "Prefix Decrement Expression";
    case AST_EXPRESSION_FUNCTION_CALL:      return "Function Call Expression";
    case AST_EXPRESSION_DEREFERENCE:        return "Dereference Expression";
    case AST_EXPRESSION_ADDRESS_OF:         return "Address Of Expression";
    case AST_EXPRESSION_SUBSCRIPT:          return "Subscript Expression";
    case AST_EXPRESSION_STRING:             return "String Expression";
    default:                                panic("AST Node '%d' not supported in get_ast_node_string()", node->type);
  }
}
