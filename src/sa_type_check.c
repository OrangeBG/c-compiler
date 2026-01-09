#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "../include/sa_type_check.h"
#include "../include/arena.h"
#include "../include/parser.h"
#include "../include/declaration_symbol.h"

//TODO: Check to see how we can better optimize these types of buffers. Exact same use of this buffer is in sa_variable_resolution
#define IDENTIFIER_BUFFER 256

static void             function_and_variable_type_check(AstNode *node, DeclarationSymbolTable *declaration_table, AstNode *function_declaration_node, ParserResults *parser_results);
static void             type_check_file_scope_variable_declaration(AstNode *variable_declaration_node, DeclarationSymbolTable *declaration_table); 
static void             type_check_block_scope_variable_declaration(AstNode *variable_declaration_node, DeclarationSymbolTable *declaration_table, char *function_name); 
static void             add_function_parameter_to_symbol_table(TypeNode *parameter_type, char *parameter_identifier, char *function_name, DeclarationSymbolTable *declaration_table, ParserResults *parser_results); 
static TypeNode*        type_check_init(TypeNode *target_type, AstNode *ast_initializer, DeclarationSymbolTable *declaration_table, AstNode *function_declaration_node, ParserResults *parser_results); 
static TypeNode*        expression_type_check(AstNode *node, DeclarationSymbolTable *declaration_table, AstNode *function_declaration_node, ParserResults *parser_results); 
static TypeNode*        expression_type_check_binary(AstNode *binary_node, AstNode *function_declaration_node, TypeNode *left_expression_type, TypeNode *right_expression_type, DeclarationSymbolTable *declaration_table, ParserResults *parser_results);  
static TypeNode*        expression_type_check_binary_logical(AstNode *node, ParserResults *parser_results); 
static TypeNode*        expression_type_check_binary_add(AstNode *add_node, TypeNode *left_expression_type, TypeNode *right_expression_type,  DeclarationSymbolTable *declaration_table, ParserResults *parser_results); 
static TypeNode*        expression_type_check_binary_subtract(AstNode *subtract_node, TypeNode *left_expression_type, TypeNode *right_expression_type, DeclarationSymbolTable *declaration_table, ParserResults *parser_results);  
static TypeNode*        expression_type_check_and_convert(AstNode **node, DeclarationSymbolTable *declaration_table, AstNode *function_declaration_node, ParserResults *parser_results);
static TypeNode*        get_common_real_type(TypeNode *type_1, TypeNode *type_2);
static TypeNode*        get_common_pointer_type(AstNode *expression_1, AstNode *expression_2, DeclarationSymbolTable *declaration_table, AstNode *function_declaration_node, ParserResults *parser_results); 
static AstNode*         convert_to(AstNode *expression, TypeNode *expression_type, TypeNode *target_type, ParserResults *parser_results); 
static long             convert_variable_declaration_constant_to_long(AstNode *variable_declaration_node); 
static int              convert_variable_declaration_constant_to_int(AstNode *variable_declaration_node); 
static unsigned long    convert_variable_declaration_constant_to_ulong(AstNode *variable_declaration_node); 
static unsigned int     convert_variable_declaration_constant_to_uint(AstNode *variable_declaration_node); 
static double           convert_variable_declaration_constant_to_double(AstNode *variable_declaration_node); 
static AstNode*         convert_by_assignment(AstNode *right_assignment_expression, TypeNode *right_assignment_type, TypeNode *target_type, ParserResults *parser_results); 
static bool             is_null_pointer_constant(AstNode *ast_node);
static AstNode*         zero_initializer(const TypeNode *type_node, const ParserResults *parser_results);

void sa_type_check(ParserResults *parser_results, DeclarationSymbolTable *declaration_table) {
  AstNode *ast_nodes = arena_get_by_index(parser_results->ast_node_arena, 0);

  for (int i = 0; i < ast_nodes->data.program.declaration_count; i++) {
    AstNode *node = ast_nodes->data.program.declaration_ptrs->node_pointers[i];

    if (node->type == AST_FUNCTION_DECLARATION) {
      function_and_variable_type_check(node, declaration_table, node, parser_results);
      continue;
    } 

    if (node->type == AST_VARIABLE_DECLARATION) {
      function_and_variable_type_check(node, declaration_table, NULL, parser_results);
      continue;
    }

    fprintf(stderr, "ERROR - SA Type Check: Unexpected declaration type\n");
    exit(1);
  } 
}

static void function_and_variable_type_check(AstNode *node, DeclarationSymbolTable *declaration_table, AstNode *function_declaration_node, ParserResults *parser_results) {
  switch (node->type) {
    case AST_VARIABLE_DECLARATION: {
      if (function_declaration_node == NULL) {
        type_check_file_scope_variable_declaration(node, declaration_table);
      } else {
        type_check_block_scope_variable_declaration(node, declaration_table, function_declaration_node->data.declaration_function.name);
      }

      if (node->data.declaration_variable.has_expression) {
        type_check_init(node->data.declaration_variable.type, node->data.declaration_variable.init_expression, declaration_table, function_declaration_node, parser_results);
      }
      break;
    }
    case AST_FUNCTION_DECLARATION: {
      HashTableEntry *entry = hash_table_get_entry(declaration_table->symbol_table, node->data.declaration_function.name);

      if (entry != NULL && entry->key != NULL) {
        DeclarationSymbol *existing_function_symbol = entry->value->structure;

        if (existing_function_symbol->symbol_type == DECLARATION_SYMBOL_VARIABLE) {
          fprintf(stderr, "ERROR - SA Type Check: '%s' declared as variable\n", entry->key);
          exit(1);
        }

        if (existing_function_symbol->data.function_symbol->value_type->type != node->data.declaration_function.function_type->data.function_type.return_type->type) {
          fprintf(stderr, "ERROR - SA Type Check: Incompatible function declarations for '%s\n'", entry->key);
          exit(1);
        }

        if (existing_function_symbol->data.function_symbol->is_defined && node->data.declaration_function.body_block != NULL) {
          fprintf(stderr, "ERROR - SA Type Check: Function defined more than once '%s'\n", entry->key);
          exit(1);
        }

        if (existing_function_symbol->data.function_symbol->is_global == node->data.declaration_function.storage_class_type == AST_STORAGE_CLASS_STATIC) {
          fprintf(stderr, "ERROR - SA Type Check: Static function '%s' declaration follows non-static\n", node->data.declaration_function.name);
          exit(1);
        }

        if (existing_function_symbol->data.function_symbol->param_count != node->data.declaration_function.function_type->data.function_type.param_type_count) {
          fprintf(stderr, "ERROR - SA Type Check: '%s' function declaration has different set parameters\n", node->data.declaration_function.name);
          exit(1);
        }

        for (int i = 0; i < node->data.declaration_function.function_type->data.function_type.param_type_count; i++) {
          if (existing_function_symbol->data.function_symbol->param_types[i].type != node->data.declaration_function.function_type->data.function_type.param_types[i].type) {
            fprintf(stderr, "ERROR - SA Type Check: '%s' function declaration has different set parameters\n", node->data.declaration_function.name);
            exit(1);
          }

          //We only want to add function param names for function definitions
          if (node->data.declaration_function.body_block != NULL) {
            add_function_parameter_to_symbol_table(&node->data.declaration_function.function_type->data.function_type.param_types[i], node->data.declaration_function.parameter_identifiers[i], node->data.declaration_function.name, declaration_table, parser_results);
          }
        }

        if (!existing_function_symbol->data.function_symbol->is_defined && node->data.declaration_function.body_block != NULL) {
          existing_function_symbol->data.function_symbol->is_defined = true;
          function_and_variable_type_check(node->data.declaration_function.body_block, declaration_table, node, parser_results);
        }

        break;
      }

      if (node->data.declaration_function.function_type->data.function_type.return_type->type == TYPE_ARRAY) {
        fprintf(stderr, "ERROR: SA Type Check - Cannot have array as function return type\n");
        exit(1);
      }

      TypeNode *param_types = malloc(sizeof(TypeNode) * node->data.declaration_function.function_type->data.function_type.param_type_count);
      
      bool is_defined = node->data.declaration_function.body_block != NULL;
      bool is_global = (node->data.declaration_function.storage_class_type != AST_STORAGE_CLASS_STATIC || strcmp(node->data.declaration_function.name, "main") == 0);
        
      for (int i = 0; i < node->data.declaration_function.function_type->data.function_type.param_type_count; i++) {
        TypeNode *parameter_type = &node->data.declaration_function.function_type->data.function_type.param_types[i];

        param_types[i] = *parameter_type;

        if (parameter_type->type == TYPE_VOID) {
          continue;
        }

        //We only want to add function param names for function definitions
        if (node->data.declaration_function.body_block == NULL) {
          continue;
        }

        add_function_parameter_to_symbol_table(parameter_type, node->data.declaration_function.parameter_identifiers[i], node->data.declaration_function.name, declaration_table, parser_results);
      }

      DeclarationSymbol *function_declaration_symbol = add_function_declaration_symbol(declaration_table, node->data.declaration_function.name, node->data.declaration_function.function_type->data.function_type.return_type, node->data.declaration_function.function_type->data.function_type.param_type_count, param_types, is_global, is_defined);

      if (node->data.declaration_function.body_block != NULL) {
        function_and_variable_type_check(node->data.declaration_function.body_block, declaration_table, node, parser_results);
      }
      break;
    }
    case AST_EXPRESSION_FUNCTION_CALL: {
      HashTableEntry *entry = hash_table_get_entry(declaration_table->symbol_table, node->data.expression_function_call.identfier);

      if (entry != NULL && entry->key != NULL) {
        DeclarationSymbol *existing_symbol = entry->value->structure;

        if (existing_symbol->symbol_type == DECLARATION_SYMBOL_VARIABLE) {
          fprintf(stderr, "ERROR - SA Type Check: Variable '%s' is used as a function name\n", node->data.expression_function_call.identfier);
          exit(1);
        }               

        if (existing_symbol->data.function_symbol->param_count != node->data.expression_function_call.argument_count) {
          fprintf(stderr, "ERROR - SA Type Check: Function '%s' called with incorrect number of arguments\n", node->data.expression_function_call.identfier);
          exit(1);
        }
      }

      if (node->data.expression_function_call.expression_type == NULL) { 
        expression_type_check_and_convert(&node, declaration_table, function_declaration_node, parser_results);
      }

      for (int i = 0; i < node->data.expression_function_call.argument_count; i++) {
        AstNode *argument_node = node->data.expression_function_call.argument_ptrs->node_pointers[i];
        function_and_variable_type_check(argument_node, declaration_table, function_declaration_node, parser_results);
      }
      break;
    }
    case AST_INITIALIZER:      
      if (node->data.initializer.type == AST_INITIALIZER_SINGLE) {
        function_and_variable_type_check(node->data.initializer.initializer_node.single_init_expression, declaration_table, function_declaration_node, parser_results);
        break;
      } 

      for (int i = 0; i < node->data.initializer.initializer_node.compound_initializer->count; i++) {
        function_and_variable_type_check(&node->data.initializer.initializer_node.compound_initializer->items[i], declaration_table, function_declaration_node, parser_results);
      }
      break;
    case AST_EXPRESSION_VARIABLE:
    case AST_EXPRESSION_CONSTANT:
    case AST_EXPRESSION_CAST: 
    case AST_EXPRESSION_UNARY:
    case AST_EXPRESSION_BINARY:
    case AST_EXPRESSION_ASSIGNMENT:
    case AST_EXPRESSION_POSTFIX_INCREMENT:
    case AST_EXPRESSION_POSTFIX_DECREMENT:
    case AST_EXPRESSION_PREFIX_INCREMENT:
    case AST_EXPRESSION_PREFIX_DECREMENT: 
    case AST_EXPRESSION_CONDITIONAL:
    case AST_EXPRESSION_DEREFERENCE:
    case AST_EXPRESSION_ADDRESS_OF:
      expression_type_check(node, declaration_table, function_declaration_node, parser_results);
      break;
    case AST_BLOCK: {
      for (int i = 0; i < node->data.block.block_count; i++) {   
        AstNode *block_item_node = node->data.block.block_ptrs->node_pointers[i];
        function_and_variable_type_check(block_item_node, declaration_table, function_declaration_node, parser_results);
      }
      break;
    }
    case AST_STATEMENT_IF: {
      function_and_variable_type_check(node->data.statement_if.condition_expression, declaration_table, function_declaration_node, parser_results);
      function_and_variable_type_check(node->data.statement_if.then_statement, declaration_table, function_declaration_node, parser_results);

      if (node->data.statement_if.else_statement != NULL) {
        function_and_variable_type_check(node->data.statement_if.else_statement, declaration_table, function_declaration_node, parser_results);
      }
      break;
    }
    case AST_STATEMENT_RETURN: {
      TypeNode *return_expression_type = expression_type_check_and_convert(&node->data.statement_return.expression, declaration_table, function_declaration_node, parser_results);
      TypeNode *function_return_type = function_declaration_node->data.declaration_function.function_type->data.function_type.return_type;

      if (function_return_type->type == TYPE_POINTER && return_expression_type->type == TYPE_POINTER) {
        if (get_pointer_base_type(return_expression_type) != get_pointer_base_type(function_return_type)) {
          fprintf(stderr, "ERROR: Type Check - Cannot implicitly convert one pointer type to another\n");
          exit(1);
        }        
      } else if (function_return_type->type == return_expression_type->type) {
        break;
      }

      node->data.statement_return.expression = convert_to(node->data.statement_return.expression, return_expression_type, function_return_type, parser_results);
      break;
    }
    case AST_STATEMENT_FOR: {
      if (node->data.statement_for.for_loop_init != NULL) {        
        function_and_variable_type_check(node->data.statement_for.for_loop_init, declaration_table, function_declaration_node, parser_results);
      }

      if (node->data.statement_for.condition_expression != NULL) {
        function_and_variable_type_check(node->data.statement_for.condition_expression, declaration_table, function_declaration_node, parser_results);
      }

      if (node->data.statement_for.post_expression != NULL) {
        function_and_variable_type_check(node->data.statement_for.post_expression, declaration_table, function_declaration_node, parser_results);
      }

      function_and_variable_type_check(node->data.statement_for.statement_body, declaration_table, function_declaration_node, parser_results);
      break;
    }
    case AST_STATEMENT_WHILE: {
      function_and_variable_type_check(node->data.statement_while.condition, declaration_table, function_declaration_node, parser_results);
      function_and_variable_type_check(node->data.statement_while.statement_body, declaration_table, function_declaration_node, parser_results);
      break;
    }
    case AST_STATEMENT_DO_WHILE: {
      function_and_variable_type_check(node->data.statement_do_while.condition, declaration_table, function_declaration_node, parser_results);
      function_and_variable_type_check(node->data.statement_do_while.statement_body, declaration_table, function_declaration_node, parser_results);
      break;
    }
    case AST_STATEMENT_COMPOUND:      
      function_and_variable_type_check(node->data.statement_compound.block, declaration_table, function_declaration_node, parser_results);
      break;
    case AST_STATEMENT_GOTO_LABEL:
    case AST_STATEMENT_GOTO:
    case AST_STATEMENT_BREAK:
    case AST_STATEMENT_CONTINUE:
    case AST_STATEMENT_NULL:
      break;
    default:    
      fprintf(stderr, "ERROR - SA Type Check: Unsupported AST type '%d' found in function and variable type check\n", node->type);
      exit(1);
  }  
}

static TypeNode* type_check_init(TypeNode *target_type, AstNode *ast_initializer, DeclarationSymbolTable *declaration_table, AstNode *function_declaration_node, ParserResults *parser_results) {
  if (ast_initializer->data.initializer.type == AST_INITIALIZER_SINGLE) {
    TypeNode *expression_type = expression_type_check_and_convert(&ast_initializer->data.initializer.initializer_node.single_init_expression, declaration_table, function_declaration_node, parser_results);
    ast_initializer = convert_by_assignment(ast_initializer->data.initializer.initializer_node.single_init_expression, expression_type, target_type, parser_results);

    return expression_type;
  }

  if (ast_initializer->data.initializer.type == AST_INITIALIZER_COMPOUND && target_type->type == TYPE_ARRAY) {
    if (ast_initializer->data.initializer.initializer_node.compound_initializer->count > target_type->data.array_type.size) {
      fprintf(stderr, "ERROR - SA Type Check: %d values initialized for an array of %lu size\n", ast_initializer->data.initializer.initializer_node.compound_initializer->count, target_type->data.array_type.size);
      exit(1);
    }

    for (int i = 0; i < ast_initializer->data.initializer.initializer_node.compound_initializer->count; i++) {
      type_check_init(target_type->data.array_type.element_type, &ast_initializer->data.initializer.initializer_node.compound_initializer->items[i], declaration_table, function_declaration_node, parser_results);
    }


    for (int i = ast_initializer->data.initializer.initializer_node.compound_initializer->count; i < target_type->data.array_type.size; i++) {
      AstNode *zero_init = zero_initializer(target_type, parser_results);
      //TODO: Add to initializer
    }

    return target_type;
  }

  fprintf(stderr, "ERROR - SA Type Check: Can't initialize a scalar object with a compound initializer\n");
  exit(1);
}

static AstNode* zero_initializer(const TypeNode *type_node, const ParserResults *parser_results) {
  if (type_node->type != TYPE_ARRAY) {
    AstNode *constant = arena_alloc(parser_results->ast_node_arena);
    constant->type = AST_EXPRESSION_CONSTANT;
    //TODO: Support other constant types
    constant->data.expression_constant.constant_type = AST_CONSTANT_TYPE_INT;
    constant->data.expression_constant.int_value = 0;

    AstNode *single_init = arena_alloc(parser_results->ast_node_arena);
    single_init->type = AST_INITIALIZER;
    single_init->data.initializer.type = AST_INITIALIZER_SINGLE;
    single_init->data.initializer.initializer_node.single_init_expression = constant;

    return single_init;
  }

  AstNode *compound_init = arena_alloc(parser_results->ast_node_arena);
  compound_init->type = AST_INITIALIZER;
  compound_init->data.initializer.type = AST_INITIALIZER_COMPOUND;

  //TODO: Need to finish

  return compound_init;
}

static void type_check_file_scope_variable_declaration(AstNode *variable_declaration_node, DeclarationSymbolTable *declaration_table) {
  InitialValueType initial_value_type; 
  InitialValue initial_value;

  if (variable_declaration_node->data.declaration_variable.has_expression && variable_declaration_node->data.declaration_variable.init_expression->data.expression_assignment.right_expression->type == AST_EXPRESSION_CONSTANT) {
    initial_value_type = INITIAL_VALUE_INITIALIZED;

    switch (variable_declaration_node->data.declaration_variable.type->type) {
      case TYPE_INT:     initial_value.int_value = convert_variable_declaration_constant_to_int(variable_declaration_node); break;
      case TYPE_LONG:    initial_value.long_value = convert_variable_declaration_constant_to_long(variable_declaration_node); break;
      case TYPE_UINT:    initial_value.uint_value = convert_variable_declaration_constant_to_uint(variable_declaration_node); break;
      case TYPE_ULONG:   initial_value.ulong_value = convert_variable_declaration_constant_to_ulong(variable_declaration_node); break;
      case TYPE_DOUBLE:  initial_value.double_value = convert_variable_declaration_constant_to_double(variable_declaration_node); break;
      case TYPE_POINTER: initial_value.ulong_value = convert_variable_declaration_constant_to_ulong(variable_declaration_node); break;
      default:
        fprintf(stderr, "ERROR: SA Type Check: Unsupported constant expression type when checking file scope variable\n");
        exit(1);
    }
  } else if (!variable_declaration_node->data.declaration_variable.has_expression) {
    if (variable_declaration_node->data.declaration_variable.storage_class_type == AST_STORAGE_CLASS_EXTERN) {
      initial_value_type = INITIAL_VALUE_NO_INITIALIZER;
    } else {
      initial_value_type = INITIAL_VALUE_TENTATIVE;
    }

    declaration_symbol_initialize_to_zero(variable_declaration_node->data.declaration_variable.type, &initial_value);
  } else {
    fprintf(stderr, "ERROR: SA Type Check: Non-constant initializer for variable declaration '%s'\n", variable_declaration_node->data.declaration_variable.name);
    exit(1);
  }

  bool is_global = variable_declaration_node->data.declaration_variable.storage_class_type != AST_STORAGE_CLASS_STATIC;

  HashTableEntry *entry = hash_table_get_entry(declaration_table->symbol_table, variable_declaration_node->data.declaration_variable.name);

  if (entry != NULL && entry->key != NULL) {
    DeclarationSymbol *existing_variable_symbol = entry->value->structure;

    if (existing_variable_symbol->symbol_type == DECLARATION_SYMBOL_FUNCTION) {
      fprintf(stderr, "ERROR: SA Type Check: Function '%s' redeclared as variable\n", variable_declaration_node->data.declaration_variable.name);
      exit(1);
    }

    if (variable_declaration_node->data.declaration_variable.type->type != existing_variable_symbol->data.variable_symbol->value_type->type) {
      fprintf(stderr, "ERROR: SA Type Check: Previously declared '%s' variable has type of '%s'\n", variable_declaration_node->data.declaration_variable.name, get_type_string(existing_variable_symbol->data.variable_symbol->value_type->type));
      exit(1);
    }

    if (variable_declaration_node->data.declaration_variable.storage_class_type == AST_STORAGE_CLASS_EXTERN) {
      existing_variable_symbol->data.variable_symbol->static_is_global = true;
    }
    else if (existing_variable_symbol->data.variable_symbol->static_is_global != is_global) {
      fprintf(stderr, "ERROR: SA Type Check: Function '%s' conflicting variable linkage\n", variable_declaration_node->data.declaration_variable.name);
      exit(1);
    }

    if (existing_variable_symbol->data.variable_symbol->static_initial_type == INITIAL_VALUE_INITIALIZED) {
      if (initial_value_type == INITIAL_VALUE_INITIALIZED) {
        fprintf(stderr, "ERROR: SA Type Check: Function '%s' conflicting file scope variable definitions\n", variable_declaration_node->data.declaration_variable.name);
        exit(1);
      }
    } else {
      existing_variable_symbol->data.variable_symbol->static_initial_type = initial_value_type;
      existing_variable_symbol->data.variable_symbol->static_initial_value = initial_value;
    }

    return;
  }

  add_static_variable_declaration_symbol(declaration_table, variable_declaration_node->data.declaration_variable.type, initial_value, variable_declaration_node->data.declaration_variable.name, is_global, initial_value_type);  
}

static void type_check_block_scope_variable_declaration(AstNode *variable_declaration_node, DeclarationSymbolTable *declaration_table, char *function_name) {
  if (variable_declaration_node->data.declaration_variable.storage_class_type == AST_STORAGE_CLASS_EXTERN) {
    if (variable_declaration_node->data.declaration_variable.has_expression) {
      fprintf(stderr, "ERROR - SA Type Check: Initializer on local extern variable declaration '%s'\n", variable_declaration_node->data.declaration_variable.name);
      exit(1);
    }
    
    HashTableEntry *entry = hash_table_get_entry(declaration_table->symbol_table, variable_declaration_node->data.declaration_variable.name);

    if (entry != NULL && entry->key != NULL) {
      DeclarationSymbol *existing_variable_symbol = entry->value->structure;

      if (existing_variable_symbol->symbol_type == DECLARATION_SYMBOL_FUNCTION) {        
        fprintf(stderr, "ERROR - SA Type Check: Function redeclared as variable\n");
        exit(1);
      }
    } else {
      add_static_extern_variable_declaration_symbol(declaration_table, variable_declaration_node->data.declaration_variable.type, variable_declaration_node->data.declaration_variable.name); 
    }
    
    return;
  }

  if (variable_declaration_node->data.declaration_variable.storage_class_type == AST_STORAGE_CLASS_STATIC) {
    InitialValue initial_value;
    
    if (!variable_declaration_node->data.declaration_variable.has_expression) {
      declaration_symbol_initialize_to_zero(variable_declaration_node->data.declaration_variable.type, &initial_value);
    } else if (variable_declaration_node->data.declaration_variable.init_expression->data.expression_assignment.right_expression->type == AST_EXPRESSION_CONSTANT) {

      Types constant_expression_type = variable_declaration_node->data.declaration_variable.init_expression->data.expression_assignment.right_expression->data.expression_constant.expression_type->type;

      switch(variable_declaration_node->data.declaration_variable.type->type) {
        case TYPE_INT:     initial_value.int_value = convert_variable_declaration_constant_to_int(variable_declaration_node); break;
        case TYPE_LONG:    initial_value.long_value = convert_variable_declaration_constant_to_long(variable_declaration_node); break;
        case TYPE_UINT:    initial_value.uint_value = convert_variable_declaration_constant_to_uint(variable_declaration_node); break;
        case TYPE_ULONG:   initial_value.ulong_value = convert_variable_declaration_constant_to_ulong(variable_declaration_node); break;
        case TYPE_DOUBLE:  initial_value.double_value = convert_variable_declaration_constant_to_double(variable_declaration_node); break;
        case TYPE_POINTER: {
          unsigned long value = convert_variable_declaration_constant_to_ulong(variable_declaration_node);

          if (value != 0) {
            fprintf(stderr, "ERROR - SA Type Check: Cannot assign value '%ld' to a static pointer\n", value);
            exit(1);
          }

          initial_value.ulong_value = value;
          break;
        }
        default:
          fprintf(stderr, "ERROR - SA Type Check: Unsupported initial value AST Type '%d'\n",variable_declaration_node->data.declaration_variable.type->type);
          exit(1);
      }
    } else {
      fprintf(stderr, "ERROR - SA Type Check: Non-constant initializer on local static variable '%s'\n", variable_declaration_node->data.declaration_variable.name);
      exit(1);
    }

    add_static_variable_declaration_symbol(declaration_table, variable_declaration_node->data.declaration_variable.type, initial_value, variable_declaration_node->data.declaration_variable.name, false, INITIAL_VALUE_INITIALIZED);
    
    return;
  }   

  add_automatic_variable_declaration_symbol(declaration_table, variable_declaration_node->data.declaration_variable.type, variable_declaration_node->data.declaration_variable.name);
} 

//TODO: Maybe need to return the whole TypeNode rather than the type enum
static TypeNode* expression_type_check(AstNode *node, DeclarationSymbolTable *declaration_table, AstNode *function_declaration_node, ParserResults *parser_results) {
  switch (node->type) {
    case AST_EXPRESSION_VARIABLE: {
      HashTableEntry *entry = hash_table_get_entry(declaration_table->symbol_table, node->data.expression_variable.identifier);

      if (entry == NULL || entry->key == NULL) {
        fprintf(stderr, "ERROR - SA Type Check: Expression variable '%s' not found in declaration symbol table\n", node->data.expression_variable.identifier);
        exit(1);
      }

      DeclarationSymbol* symbol = entry->value->structure; 

      if (symbol->symbol_type == DECLARATION_SYMBOL_FUNCTION) {
        fprintf(stderr, "ERROR - SA Type Check: Function name '%s' is being used as a variable\n", node->data.expression_variable.identifier);
        exit(1);
      }

      //@NOTE: Experimenting with something here. Rather than creating a new type node. Pass the pointer to the existing one. 
      node->data.expression_variable.expression_type = symbol->data.variable_symbol->value_type;

      return node->data.expression_variable.expression_type;
    }
    case AST_EXPRESSION_CONSTANT: {
      TypeNode *expression_type = arena_alloc(parser_results->type_node_arena);
      
      switch (node->data.expression_constant.constant_type) {
        case AST_CONSTANT_TYPE_INT:    expression_type->type = TYPE_INT; break;
        case AST_CONSTANT_TYPE_LONG:   expression_type->type = TYPE_LONG; break;
        case AST_CONSTANT_TYPE_UINT:   expression_type->type = TYPE_UINT; break;
        case AST_CONSTANT_TYPE_ULONG:  expression_type->type = TYPE_ULONG; break;
        case AST_CONSTANT_TYPE_DOUBLE: expression_type->type = TYPE_DOUBLE; break;
        default:
          fprintf(stderr, "ERROR - Type Check: Could not resolve value type in variable symbol\n");
          exit(1);
      }

      node->data.expression_constant.expression_type = expression_type;

      return expression_type;
    }
    case AST_EXPRESSION_CAST: {
      TypeNode *expression_type = expression_type_check_and_convert(&node->data.expression_cast.expression, declaration_table, function_declaration_node, parser_results);

      if (node->data.expression_cast.target_type->type == TYPE_ARRAY) {
        fprintf(stderr, "ERROR - SA Type Check: Cannot cast to an array type\n");
        exit(1);
      }
      
      if (node->data.expression_cast.target_type->type == TYPE_DOUBLE && node->data.expression_cast.target_type->type == TYPE_POINTER && get_pointer_base_type(node->data.expression_cast.target_type) == TYPE_DOUBLE) {
        fprintf(stderr, "ERROR - SA Type Check: Cannot cast double pointer to double\n");
        exit(1);
      }

      if (expression_type->type == TYPE_DOUBLE && node->data.expression_cast.target_type->type == TYPE_POINTER && get_pointer_base_type(node->data.expression_cast.target_type) != TYPE_DOUBLE) {
        fprintf(stderr, "ERROR - SA Type Check: Double cannot be cast to pointer type\n");
        exit(1);
      }

      return node->data.expression_cast.target_type;
    }
    case AST_EXPRESSION_UNARY: {
      TypeNode *expression_type = expression_type_check_and_convert(&node->data.expression_unary.expression, declaration_table, function_declaration_node, parser_results);

      if (node->data.expression_unary.op_type == AST_UNARY_COMPLEMENT && expression_type->type == TYPE_DOUBLE) {
        fprintf(stderr, "ERROR - SA Type Check: Cannot apply unary complement operator to a double\n");
        exit(1);
      }

      if (expression_type->type == TYPE_POINTER && (node->data.expression_unary.op_type == AST_UNARY_COMPLEMENT || node->data.expression_unary.op_type == AST_UNARY_NEGATE)) {
        fprintf(stderr, "ERROR - SA Type Check: Cannot apply unary complement or negate operator to a pointer\n");
        exit(1);
      }

      node->data.expression_unary.expression_type = expression_type;

      if (node->data.expression_unary.op_type == AST_UNARY_NOT) {
        TypeNode *int_type = arena_alloc(parser_results->type_node_arena);
        int_type->type = TYPE_INT;

        return int_type;
      }

      return expression_type;
    }
    case AST_EXPRESSION_BINARY: {
      TypeNode *left_expression_type = expression_type_check_and_convert(&node->data.expression_binary.left_expression, declaration_table, function_declaration_node, parser_results);
      TypeNode *right_expression_type = expression_type_check_and_convert(&node->data.expression_binary.right_expression, declaration_table, function_declaration_node, parser_results);

      if (right_expression_type->type == TYPE_DOUBLE || left_expression_type->type == TYPE_DOUBLE) {
        switch (node->data.expression_binary.op_type) {
          case AST_BINARY_BITWISE_OR:
          case AST_BINARY_BITWISE_XOR:
          case AST_BINARY_BITWISE_RIGHT_SHIFT:
          case AST_BINARY_BITWISE_LEFT_SHIFT:
          case AST_BINARY_BITWISE_AND:
          case AST_BINARY_REMAINDER:
            fprintf(stderr, "ERROR - SA Type Check: Cannot apply binary %s operator with a double value\n", get_binary_op_type_string(node->data.expression_binary.op_type));
            exit(1);          
        }
      }

      if (right_expression_type->type == TYPE_POINTER || left_expression_type->type == TYPE_POINTER) {
        switch (node->data.expression_binary.op_type) {
          case AST_BINARY_MULTIPLY:
          case AST_BINARY_DIVIDE:
          case AST_BINARY_REMAINDER:
          case AST_BINARY_BITWISE_AND:
          case AST_BINARY_BITWISE_XOR:
          case AST_BINARY_OR: 
          case AST_BINARY_BITWISE_RIGHT_SHIFT:
          case AST_BINARY_BITWISE_LEFT_SHIFT:
            fprintf(stderr, "ERROR - SA Type Check: Cannot apply a binary %s operator with a pointer\n", get_binary_op_type_string(node->data.expression_binary.op_type));
            exit(1);
          case AST_BINARY_EQUAL:
          case AST_BINARY_LESS_THAN:
          case AST_BINARY_LESS_OR_EQUAL:
          case AST_BINARY_GREATER_THAN:
          case AST_BINARY_GREATER_OR_EQUAL:             
            if (right_expression_type->type == TYPE_POINTER && left_expression_type->type == TYPE_POINTER && get_pointer_base_type(left_expression_type) != get_pointer_base_type(right_expression_type)) {
              fprintf(stderr, "ERROR - SA Type Check: Cannot compare pointers of different types\n");
              exit(1);
            }
            break;        
          }

        switch (node->data.expression_binary.op_type) {
          case AST_BINARY_ADD:
          case AST_BINARY_SUBTRACT:
          case AST_BINARY_LESS_THAN:
          case AST_BINARY_LESS_OR_EQUAL:
          case AST_BINARY_GREATER_THAN:
          case AST_BINARY_GREATER_OR_EQUAL:             
            if (is_null_pointer_constant(node->data.expression_binary.left_expression) || is_null_pointer_constant(node->data.expression_binary.right_expression)) {
              fprintf(stderr, "ERROR - SA Type Check: Cannot perform %s operation with a null constant\n", get_binary_op_type_string(node->data.expression_binary.op_type));
              exit(1);
            }
            break;
        }
      }

      switch (node->data.expression_binary.op_type) {
        case AST_BINARY_AND:
        case AST_BINARY_OR:
          return expression_type_check_binary_logical(node, parser_results);
        case AST_BINARY_ADD:
          return expression_type_check_binary_add(node, left_expression_type, right_expression_type, declaration_table, parser_results);
        case AST_BINARY_SUBTRACT:
          return expression_type_check_binary_subtract(node, left_expression_type, right_expression_type, declaration_table, parser_results);
        default:
          return expression_type_check_binary(node, function_declaration_node, left_expression_type, right_expression_type, declaration_table, parser_results);
      }
    }
    case AST_EXPRESSION_ASSIGNMENT: {
      TypeNode *left_expression_type = expression_type_check_and_convert(&node->data.expression_assignment.left_expression, declaration_table, function_declaration_node, parser_results);

      if (node->data.expression_assignment.left_expression->type != AST_EXPRESSION_VARIABLE && node->data.expression_assignment.left_expression->type != AST_EXPRESSION_DEREFERENCE && node->data.expression_assignment.left_expression->type != AST_EXPRESSION_SUBSCRIPT) {
        fprintf(stderr, "ERROR - SA Type Check: Tried to assign to a non-lvalue\n");
        exit(1);
      }

      TypeNode *right_expression_type = expression_type_check_and_convert(&node->data.expression_assignment.right_expression, declaration_table, function_declaration_node, parser_results);

      if (left_expression_type->type == TYPE_POINTER && right_expression_type->type == TYPE_POINTER && get_pointer_base_type(left_expression_type) != get_pointer_base_type(right_expression_type)) {        
        fprintf(stderr, "ERROR - SA Type Check: Expression assignment of pointers aren't for the same type\n");
        exit(1);
      }

      node->data.expression_assignment.right_expression = convert_by_assignment(node->data.expression_assignment.right_expression, right_expression_type, left_expression_type, parser_results);

      return left_expression_type;
    }
    case AST_EXPRESSION_FUNCTION_CALL: {
      HashTableEntry *entry = hash_table_get_entry(declaration_table->symbol_table, node->data.expression_function_call.identfier);
      if (entry == NULL && entry->key == NULL) {
        fprintf(stderr, "ERROR - SA Type Check: Called function '%s' not found in symbol table\n", node->data.expression_function_call.identfier);
        exit(1);
      }

      DeclarationSymbol *existing_symbol = entry->value->structure;

      if (existing_symbol->symbol_type == DECLARATION_SYMBOL_VARIABLE) {
        fprintf(stderr, "ERROR - SA Type Check: Variable '%s' is used as a function name\n", node->data.expression_function_call.identfier);
        exit(1);
      }               

      if (existing_symbol->data.function_symbol->param_count != node->data.expression_function_call.argument_count) {
        fprintf(stderr, "ERROR - SA Type Check: Function '%s' called with incorrect number of arguments\n", node->data.expression_function_call.identfier);
        exit(1);
      }
      
      for (int i = 0; i < node->data.expression_function_call.argument_count; i++) {
        AstNode *argument_node = node->data.expression_function_call.argument_ptrs->node_pointers[i];
        function_and_variable_type_check(argument_node, declaration_table, function_declaration_node, parser_results);
      }
    
      //@NOTE: Attempting to reuse existing types here rather than creating a new one
      node->data.expression_function_call.expression_type = existing_symbol->data.function_symbol->value_type;

      return existing_symbol->data.function_symbol->value_type;
    }
    case AST_EXPRESSION_CONDITIONAL: {
      //TODO: Confirm that the conditional expression type does not need to do anything with the set common type
      TypeNode* condition_type = expression_type_check_and_convert(&node->data.expression_conditional.condition, declaration_table, function_declaration_node, parser_results);
        node->data.expression_conditional.expression_type = condition_type;

      TypeNode *true_expression_type = expression_type_check_and_convert(&node->data.expression_conditional.true_expression, declaration_table, function_declaration_node, parser_results);
      TypeNode *false_expression_type = expression_type_check_and_convert(&node->data.expression_conditional.false_expression, declaration_table, function_declaration_node, parser_results);
      
      TypeNode *common_type;

      if (true_expression_type->type == TYPE_POINTER || false_expression_type->type == TYPE_POINTER) {
        common_type = get_common_pointer_type(node->data.expression_conditional.true_expression, node->data.expression_conditional.false_expression, declaration_table, function_declaration_node, parser_results);
      } else {
        common_type = get_common_real_type(true_expression_type, false_expression_type);
      }

      node->data.expression_conditional.true_expression = convert_to(node->data.expression_conditional.true_expression, true_expression_type, common_type, parser_results);
      node->data.expression_conditional.false_expression = convert_to(node->data.expression_conditional.false_expression, false_expression_type, common_type, parser_results);

      return common_type;
    }
    case AST_EXPRESSION_PREFIX_INCREMENT:
    case AST_EXPRESSION_POSTFIX_INCREMENT:
    case AST_EXPRESSION_PREFIX_DECREMENT:
    case AST_EXPRESSION_POSTFIX_DECREMENT: {
      return expression_type_check_and_convert(&node->data.expression_increment_decrement.expression, declaration_table, function_declaration_node, parser_results);
      break;
    }
    case AST_EXPRESSION_ADDRESS_OF: {
      /*
        LValue = Expressions that can appear on the left side of an assignment
        Section 6.3.2.1, paragraph 1, of the C standard - "An lvalue is an expression...that potentially designates an object:
          - Variables
          - Dereference 
          - Subscript
      */

      if (node->data.expression_address_of.expression->type != AST_EXPRESSION_VARIABLE && node->data.expression_address_of.expression->type != AST_EXPRESSION_DEREFERENCE && node->data.expression_address_of.expression->type != AST_EXPRESSION_SUBSCRIPT) {
        fprintf(stderr, "ERROR - SA Type Check: Cannot take the address of a non-lvalue\n");
        exit(1);
      }

      //@Note: Do not call 'expression_type_check_convert()' for address_of operand
      TypeNode *address_of_expression_type = expression_type_check(node->data.expression_address_of.expression, declaration_table, function_declaration_node, parser_results);

      TypeNode *pointer_type_node = arena_alloc(parser_results->type_node_arena);
      pointer_type_node->type = TYPE_POINTER;
      pointer_type_node->data.pointer_type.reference_type = address_of_expression_type;

      return pointer_type_node;
    }
    case AST_EXPRESSION_DEREFERENCE: {
      TypeNode *expression_type = expression_type_check_and_convert(&node->data.expression_dereference.expression, declaration_table, function_declaration_node, parser_results);

      if (expression_type->type != TYPE_POINTER) {
        fprintf(stderr, "ERROR - SA Type Check: Cannot dereference a non-pointer\n");
        exit(1);
      }

      //TODO: Will this work if it's greater than one level? Example: int** 
      return expression_type->data.pointer_type.reference_type;
    }
    case AST_EXPRESSION_SUBSCRIPT: {
      TypeNode *expression_1_type = expression_type_check_and_convert(&node->data.expression_subscript.expression_1, declaration_table, function_declaration_node, parser_results);
      TypeNode *expression_2_type = expression_type_check_and_convert(&node->data.expression_subscript.expression_2, declaration_table, function_declaration_node, parser_results);

      TypeNode *long_type_node = arena_alloc(parser_results->type_node_arena);
      long_type_node->type = TYPE_LONG;

      //@Test: Not sure if these returned expression types are correct
      if (expression_1_type->type == TYPE_POINTER && is_integer_type(expression_2_type)) {
        node->data.expression_subscript.expression_2 = convert_to(node->data.expression_subscript.expression_2, expression_2_type, long_type_node, parser_results);
        return expression_1_type;
      }

      if (expression_2_type->type == TYPE_POINTER && is_integer_type(expression_1_type)) {
        node->data.expression_subscript.expression_1 = convert_to(node->data.expression_subscript.expression_1, expression_1_type, long_type_node, parser_results);
        return expression_2_type;
      }

      fprintf(stderr, "ERROR - SA Type Check: Subscript must have an integer and pointer operand\n");
      exit(1);
    }
    default:
      fprintf(stderr, "ERROR - SA Type Check: Invalid AST type '%d' found in expression type check\n", node->type);
      exit(1);
  }
}

static TypeNode* expression_type_check_binary_logical(AstNode *node, ParserResults *parser_results) {
  TypeNode *expression_type_node = arena_alloc(parser_results->type_node_arena);
  expression_type_node->type = TYPE_INT;

  node->data.expression_binary.expression_type = expression_type_node;
  return expression_type_node;
}

static TypeNode* expression_type_check_binary(AstNode *binary_node, AstNode *function_declaration_node, TypeNode *left_expression_type, TypeNode *right_expression_type, DeclarationSymbolTable *declaration_table, ParserResults *parser_results) { 
  TypeNode *common_type;
  
  if ((binary_node->data.expression_binary.op_type == AST_BINARY_EQUAL || binary_node->data.expression_binary.op_type == AST_BINARY_NOT_EQUAL) && (left_expression_type->type == TYPE_POINTER || right_expression_type->type == TYPE_POINTER)) {
    common_type = get_common_pointer_type(binary_node->data.expression_binary.left_expression, binary_node->data.expression_binary.right_expression, declaration_table, function_declaration_node, parser_results);
  } else {
    common_type = get_common_real_type(left_expression_type, right_expression_type);
  }

  binary_node->data.expression_binary.left_expression = convert_to(binary_node->data.expression_binary.left_expression, left_expression_type, common_type, parser_results);
  binary_node->data.expression_binary.right_expression = convert_to(binary_node->data.expression_binary.right_expression, right_expression_type, common_type, parser_results);

  TypeNode *expression_type_node = arena_alloc(parser_results->type_node_arena);
  expression_type_node = common_type;

  binary_node->data.expression_binary.expression_type = expression_type_node;
  
  switch (binary_node->data.expression_binary.op_type) {
    case AST_BINARY_MULTIPLY:
    case AST_BINARY_DIVIDE:
    case AST_BINARY_REMAINDER:
      return expression_type_node;
    default: {
      TypeNode *int_type_node = arena_alloc(parser_results->type_node_arena);
      int_type_node->type = TYPE_INT;
      return int_type_node;
    }
  }
}

static TypeNode* expression_type_check_binary_add(AstNode *add_node, TypeNode *left_expression_type, TypeNode *right_expression_type, DeclarationSymbolTable *declaration_table, ParserResults *parser_results) { 
  TypeNode *common_type = get_common_real_type(left_expression_type, right_expression_type);

  if (is_arithmetic_type(left_expression_type) && is_arithmetic_type(right_expression_type)) {
    add_node->data.expression_binary.left_expression = convert_to(add_node->data.expression_binary.left_expression, left_expression_type, common_type, parser_results);
    add_node->data.expression_binary.right_expression = convert_to(add_node->data.expression_binary.right_expression, right_expression_type, common_type, parser_results);

    TypeNode *expression_type_node = arena_alloc(parser_results->type_node_arena);
    expression_type_node = common_type;

    return expression_type_node;
  }

  //TODO: Look into areas where I'm doing something similar for types that aren't Pointer. No need in allocating the same long, int, etc type nodes. Instead, do it once and point things to it.
  TypeNode *long_type_node = arena_alloc(parser_results->type_node_arena);
  long_type_node->type = TYPE_LONG;

  if (left_expression_type->type == TYPE_POINTER && is_integer_type(right_expression_type)) {
    add_node->data.expression_binary.right_expression = convert_to(add_node->data.expression_binary.right_expression, right_expression_type, long_type_node, parser_results);     
    return left_expression_type;
  }

  if (right_expression_type->type == TYPE_POINTER && is_integer_type(left_expression_type)) {
    add_node->data.expression_binary.left_expression = convert_to(add_node->data.expression_binary.left_expression, left_expression_type, long_type_node, parser_results);     
    return right_expression_type;
  }

  fprintf(stderr, "ERROR - SA Type Check: Invalid operands for addition\n");
  exit(1);
}

static TypeNode* expression_type_check_binary_subtract(AstNode *subtract_node, TypeNode *left_expression_type, TypeNode *right_expression_type, DeclarationSymbolTable *declaration_table, ParserResults *parser_results) { 
  TypeNode *common_type = get_common_real_type(left_expression_type, right_expression_type);

  if (is_arithmetic_type(left_expression_type) && is_arithmetic_type(right_expression_type)) {
    subtract_node->data.expression_binary.left_expression = convert_to(subtract_node->data.expression_binary.left_expression, left_expression_type, common_type, parser_results);
    subtract_node->data.expression_binary.right_expression = convert_to(subtract_node->data.expression_binary.right_expression, right_expression_type, common_type, parser_results);

    TypeNode *expression_type_node = arena_alloc(parser_results->type_node_arena);
    expression_type_node = common_type;

    return expression_type_node;
  }

  //TODO: Look into areas where I'm doing something similar for types that aren't Pointer. No need in allocating the same long, int, etc type nodes. Instead, do it once and point things to it.
  TypeNode *long_type_node = arena_alloc(parser_results->type_node_arena);
  long_type_node->type = TYPE_LONG;

  if (left_expression_type->type == TYPE_POINTER && is_integer_type(right_expression_type)) {
    subtract_node->data.expression_binary.right_expression = convert_to(subtract_node->data.expression_binary.right_expression, right_expression_type, long_type_node, parser_results);     
    return left_expression_type;
  }

  if (left_expression_type->type == TYPE_POINTER && get_pointer_base_type(left_expression_type) == get_pointer_base_type(right_expression_type)) {
    subtract_node->data.expression_binary.expression_type = long_type_node;
    return long_type_node;
  }

  fprintf(stderr, "ERROR - SA Type Check: Invalid operands for subtraction\n");
  exit(1);
}

static TypeNode* get_common_real_type(TypeNode *type_1, TypeNode *type_2) {
  if (type_1->type == type_2->type) {
    return type_1;
  }  

  if (type_1->type == TYPE_DOUBLE) {
    return type_1;
  }

  if (type_2->type == TYPE_DOUBLE) {
    return type_2;
  }

  if (get_type_size(type_1->type) == get_type_size(type_2->type)) {
    if (type_1->type == TYPE_INT || type_1->type == TYPE_LONG) {
      return type_2;
    }

    return type_1;
  }  

  if (get_type_size(type_1->type) > get_type_size(type_2->type)) {
    return type_1;
  } 

  return type_2;
}

static TypeNode* get_common_pointer_type(AstNode *expression_1, AstNode *expression_2, DeclarationSymbolTable *declaration_table, AstNode *function_declaration_node, ParserResults *parser_results) {
  TypeNode *expression_1_type = expression_type_check_and_convert(&expression_1, declaration_table, function_declaration_node, parser_results); 
  TypeNode *expression_2_type = expression_type_check_and_convert(&expression_2, declaration_table, function_declaration_node, parser_results); 

  if (expression_1_type->type == expression_2_type->type) {
    return expression_1_type;
  }

  if (is_null_pointer_constant(expression_1)) {
    return expression_2_type;
  }

  if (is_null_pointer_constant(expression_2)) {
    return expression_1_type;
  }

  fprintf(stderr, "ERROR - SA Type Check: Common pointer expressions have incompatible types (line %d)\n", expression_1->line_number);
  exit(1);
}

static AstNode* convert_to(AstNode *expression, TypeNode *expression_type, TypeNode *target_type, ParserResults *parser_results) {
  if (expression_type->type == target_type->type) {
    return expression;
  }

  TypeNode *type_node = arena_alloc(parser_results->type_node_arena);
  type_node = target_type;

  AstNode *casted_expression = arena_alloc(parser_results->ast_node_arena);
  casted_expression->type = AST_EXPRESSION_CAST;
  casted_expression->data.expression_cast.target_type = type_node;
  casted_expression->data.expression_cast.expression = expression;

  //@Note: Commented the following as I don't think it's needed. Cast target type should be the same as the expression type. Expression type was removed from the AST Cast node. Leaving this commented until confident we don't need it.
  //casted_expression->data.expression_cast.expression_type = target_type;
  
  // TypeNode *cast_expression_type = NULL;
  //
  // switch (expression->type) {
  //   case AST_EXPRESSION_CONSTANT:      cast_expression_type = expression->data.expression_constant.expression_type; break;
  //   case AST_EXPRESSION_VARIABLE:      cast_expression_type = expression->data.expression_variable.expression_type; break;
  //   case AST_EXPRESSION_CAST:          cast_expression_type = expression->data.expression_cast.expression_type; break;
  //   case AST_EXPRESSION_UNARY:         cast_expression_type = expression->data.expression_unary.expression_type; break;
  //   case AST_EXPRESSION_BINARY:        cast_expression_type = expression->data.expression_binary.expression_type; break;
  //   case AST_EXPRESSION_ASSIGNMENT:    cast_expression_type = expression->data.expression_assignment.expression_type; break;
  //   case AST_EXPRESSION_CONDITIONAL:   cast_expression_type = expression->data.expression_conditional.expression_type; break;
  //   case AST_EXPRESSION_FUNCTION_CALL: cast_expression_type = expression->data.expression_variable.expression_type; break;
  //   default:
  //     fprintf(stderr, "ERROR - Type Check: Unsupported cast expression type '%d'\n", expression->type);
  //     exit(1);
  // }
  //
  // casted_expression->data.expression_cast.expression_type = cast_expression_type;
  
  return casted_expression;
}

static void add_function_parameter_to_symbol_table(TypeNode *parameter_type, char *parameter_identifier, char *function_name, DeclarationSymbolTable *declaration_table, ParserResults *parser_results) {
  if (parameter_type->type == TYPE_VOID) {
    return;
  }

  char *symbol_key = malloc(IDENTIFIER_BUFFER); 
  snprintf(symbol_key, IDENTIFIER_BUFFER, "%s", parameter_identifier);

  //Array decay to pointer
  if (parameter_type->type == TYPE_ARRAY) {
    TypeNode *pointer_type_node = arena_alloc(parser_results->type_node_arena);
    pointer_type_node->type = TYPE_POINTER;
    pointer_type_node->data.pointer_type.reference_type = parameter_type;
    
    add_automatic_variable_declaration_symbol(declaration_table, pointer_type_node, symbol_key);
  } else {
    add_automatic_variable_declaration_symbol(declaration_table, parameter_type, symbol_key);
  }
}

static long convert_variable_declaration_constant_to_long(AstNode *variable_declaration_node) {
  AstNode *constant_expression = variable_declaration_node->data.declaration_variable.init_expression->data.expression_assignment.right_expression;

  switch (constant_expression->data.expression_constant.constant_type) {
    case AST_CONSTANT_TYPE_INT:    return (long)constant_expression->data.expression_constant.int_value;
    case AST_CONSTANT_TYPE_UINT:   return (long)constant_expression->data.expression_constant.uint_value;
    case AST_CONSTANT_TYPE_ULONG:  return (long)constant_expression->data.expression_constant.ulong_value;
    case AST_CONSTANT_TYPE_DOUBLE: return (long)constant_expression->data.expression_constant.double_value;
    case AST_CONSTANT_TYPE_LONG:   return constant_expression->data.expression_constant.long_value;
    default:
      fprintf(stderr, "ERROR - SA Type Check: Unsupported constant type when converting to long\n");
      exit(1);
  }
}

static int convert_variable_declaration_constant_to_int(AstNode *variable_declaration_node) {
  AstNode *constant_expression = variable_declaration_node->data.declaration_variable.init_expression->data.expression_assignment.right_expression;

  switch (constant_expression->data.expression_constant.constant_type) {
    case AST_CONSTANT_TYPE_INT:    return constant_expression->data.expression_constant.int_value;
    case AST_CONSTANT_TYPE_UINT:   return (int)constant_expression->data.expression_constant.uint_value;
    case AST_CONSTANT_TYPE_ULONG:  return (int)constant_expression->data.expression_constant.ulong_value;
    case AST_CONSTANT_TYPE_DOUBLE: return (int)constant_expression->data.expression_constant.double_value;
    case AST_CONSTANT_TYPE_LONG:   return (int)constant_expression->data.expression_constant.long_value;
    default:
      fprintf(stderr, "ERROR - SA Type Check: Unsupported constant type when converting to int\n");
      exit(1);
  }
}

static unsigned int convert_variable_declaration_constant_to_uint(AstNode *variable_declaration_node) {
  AstNode *constant_expression = variable_declaration_node->data.declaration_variable.init_expression->data.expression_assignment.right_expression;

  switch (constant_expression->data.expression_constant.constant_type) {
    case AST_CONSTANT_TYPE_INT:    return (unsigned int)constant_expression->data.expression_constant.int_value;
    case AST_CONSTANT_TYPE_UINT:   return constant_expression->data.expression_constant.uint_value;
    case AST_CONSTANT_TYPE_ULONG:  return (unsigned int)constant_expression->data.expression_constant.ulong_value;
    case AST_CONSTANT_TYPE_DOUBLE: return (unsigned int)constant_expression->data.expression_constant.double_value;
    case AST_CONSTANT_TYPE_LONG:   return (unsigned int)constant_expression->data.expression_constant.long_value;
    default:
      fprintf(stderr, "ERROR - SA Type Check: Unsupported constant type when converting to uint\n");
      exit(1);
  }
}

static unsigned long convert_variable_declaration_constant_to_ulong(AstNode *variable_declaration_node) {
  AstNode *constant_expression = variable_declaration_node->data.declaration_variable.init_expression->data.expression_assignment.right_expression;

  switch (constant_expression->data.expression_constant.constant_type) {
    case AST_CONSTANT_TYPE_INT:    return (unsigned long)constant_expression->data.expression_constant.int_value;
    case AST_CONSTANT_TYPE_UINT:   return (unsigned long)constant_expression->data.expression_constant.uint_value;
    case AST_CONSTANT_TYPE_ULONG:  return constant_expression->data.expression_constant.ulong_value;
    case AST_CONSTANT_TYPE_DOUBLE: return (unsigned long)constant_expression->data.expression_constant.double_value;
    case AST_CONSTANT_TYPE_LONG:   return (unsigned long)constant_expression->data.expression_constant.long_value;
    default:
      fprintf(stderr, "ERROR - SA Type Check: Unsupported constant type when converting to uint\n");
      exit(1);
  }
}

static double convert_variable_declaration_constant_to_double(AstNode *variable_declaration_node) {
  AstNode *constant_expression = variable_declaration_node->data.declaration_variable.init_expression->data.expression_assignment.right_expression;

  switch (constant_expression->data.expression_constant.constant_type) {
    case AST_CONSTANT_TYPE_INT:    return (double)constant_expression->data.expression_constant.int_value;
    case AST_CONSTANT_TYPE_UINT:   return (double)constant_expression->data.expression_constant.uint_value;
    case AST_CONSTANT_TYPE_ULONG:  return (double)constant_expression->data.expression_constant.ulong_value;
    case AST_CONSTANT_TYPE_DOUBLE: return constant_expression->data.expression_constant.double_value;
    case AST_CONSTANT_TYPE_LONG:   return (double)constant_expression->data.expression_constant.long_value;
    default:
      fprintf(stderr, "ERROR - SA Type Check: Unsupported constant type when converting to double\n");
      exit(1);
  }
}

static bool is_null_pointer_constant(AstNode *ast_node) {
  // Defining null pointer constants more narrowly than the C standard
  if (ast_node->type != AST_EXPRESSION_CONSTANT) {
    return false;
  }

  switch (ast_node->data.expression_constant.constant_type) {
    case AST_CONSTANT_TYPE_INT:     return ast_node->data.expression_constant.int_value == 0;
    case AST_CONSTANT_TYPE_UINT:    return ast_node->data.expression_constant.uint_value == 0;
    case AST_CONSTANT_TYPE_LONG:    return ast_node->data.expression_constant.long_value == 0;
    case AST_CONSTANT_TYPE_ULONG:   return ast_node->data.expression_constant.ulong_value == 0;
    default:
      return false;
  }
}

static AstNode* convert_by_assignment(AstNode *right_assignment_expression, TypeNode *right_assignment_type, TypeNode *target_type, ParserResults *parser_results) {
  if (right_assignment_type->type == target_type->type) {
    return right_assignment_expression;
  }

  //arithmetic types
  if (is_arithmetic_type(right_assignment_type) && is_arithmetic_type(target_type)) {
    return convert_to(right_assignment_expression, right_assignment_type, target_type, parser_results);
  }

  if (is_null_pointer_constant(right_assignment_expression) && target_type->type == TYPE_POINTER) {
    return convert_to(right_assignment_expression, right_assignment_type, target_type, parser_results);
  }

  fprintf(stderr, "ERROR - Type Check: Cannot convert type for assignment expression (line %d)\n", right_assignment_expression->line_number);
  exit(1);
}

static TypeNode* expression_type_check_and_convert(AstNode **node, DeclarationSymbolTable *declaration_table, AstNode *function_declaration_node, ParserResults *parser_results) {
  TypeNode *expression_type = expression_type_check(*node, declaration_table, function_declaration_node, parser_results);

  if (expression_type->type != TYPE_ARRAY) {
    return expression_type;
  }

  AstNode *address_of_array = arena_alloc(parser_results->ast_node_arena); 
  address_of_array->type = AST_EXPRESSION_ADDRESS_OF;
  address_of_array->data.expression_address_of.expression = *node;

  node = &address_of_array;

  //@Test: Test that this is the correct reference type that we want to add. I think this is wrong and it should be the indexed element
  TypeNode *address_of_array_pointer = arena_alloc(parser_results->type_node_arena);
  address_of_array_pointer->type = TYPE_POINTER;
  address_of_array_pointer->data.pointer_type.reference_type = expression_type;

  return address_of_array_pointer;
}
