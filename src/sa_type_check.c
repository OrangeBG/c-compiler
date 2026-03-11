#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "../include/sa_type_check.h"
#include "../include/dynamic_array.h"
#include "../include/arena.h"
#include "../include/parser.h"
#include "../include/symbol.h"
#include "../include/error.h"

//TODO: Check to see how we can better optimize these types of buffers. Exact same use of this buffer is in sa_variable_resolution
#define IDENTIFIER_BUFFER 256

static void             function_and_variable_type_check(AstNode *node, SymbolTable *symbol_table, AstNode *function_declaration_node, ParserResults *parser_results);
static void             type_check_file_scope_variable_declaration(AstNode *variable_declaration_node, SymbolTable *symbol_table); 
static void             type_check_block_scope_variable_declaration(AstNode *variable_declaration_node, SymbolTable *symbol_table, char *function_name);
static void             add_function_parameter_to_symbol_table(TypeNode *parameter_type, char *parameter_identifier, char *function_name, SymbolTable *symbol_table, ParserResults *parser_results); 
static TypeNode*        type_check_init(TypeNode *target_type, AstNode *ast_initializer, SymbolTable *symbol_table, AstNode *function_declaration_node, ParserResults *parser_results); 
static TypeNode*        expression_type_check(AstNode *node, SymbolTable *symbol_table, AstNode *function_declaration_node, ParserResults *parser_results); 
static TypeNode*        expression_type_check_binary(AstNode *binary_node, AstNode *function_declaration_node, TypeNode *left_expression_type, TypeNode *right_expression_type, SymbolTable *symbol_table, ParserResults *parser_results);  
static TypeNode*        expression_type_check_binary_logical(AstNode *node, ParserResults *parser_results); 
static TypeNode*        expression_type_check_binary_add(AstNode *add_node, TypeNode *left_expression_type, TypeNode *right_expression_type,  SymbolTable *symbol_table, ParserResults *parser_results); 
static TypeNode*        expression_type_check_binary_subtract(AstNode *subtract_node, TypeNode *left_expression_type, TypeNode *right_expression_type, SymbolTable *symbol_table, ParserResults *parser_results);  
static TypeNode*        expression_type_check_and_convert(AstNode *source_node, AstNode **node, SymbolTable *symbol_table, AstNode *function_declaration_node, ParserResults *parser_results);
static TypeNode*        get_common_real_type(TypeNode *type_1, TypeNode *type_2);
static TypeNode*        get_common_pointer_type(AstNode *source, AstNode *expression_1, AstNode *expression_2, SymbolTable *symbol_table, AstNode *function_declaration_node, ParserResults *parser_results); 
static AstNode*         convert_to(AstNode *expression, TypeNode *expression_type, TypeNode *target_type, ParserResults *parser_results); 
static long             convert_variable_declaration_constant_to_long(AstNode *constant_node); 
static int              convert_variable_declaration_constant_to_int(AstNode *constant_node); 
static unsigned long    convert_variable_declaration_constant_to_ulong(AstNode *constant_node); 
static unsigned int     convert_variable_declaration_constant_to_uint(AstNode *constant_node); 
static double           convert_variable_declaration_constant_to_double(AstNode *constant_node); 
static AstNode*         convert_by_assignment(AstNode *right_assignment_expression, TypeNode *right_assignment_type, TypeNode *target_type, ParserResults *parser_results); 
static bool             is_null_pointer_constant(AstNode *ast_node);
static AstNode*         zero_initializer(const TypeNode *type_node, const ParserResults *parser_results);
static void             add_variable_declaration_single_init_to_array(InitialValueArray *initial_value_array, TypeNode *declaration_type, AstNode *single_init); 
static void             add_variable_declaration_compound_init_to_array(InitialValueArray *initial_value_array, TypeNode *declaration_type, AstNode *compound_init);

void sa_type_check(ParserResults *parser_results, SymbolTable *symbol_table) {
  AstNode *ast_nodes = arena_get_by_index(parser_results->ast_node_arena, 0);

  for (int i = 0; i < ast_nodes->data.program.declaration_count; i++) {
    AstNode *node = ast_nodes->data.program.declaration_ptrs->node_pointers[i];

    if (node->type == AST_FUNCTION_DECLARATION) {
      function_and_variable_type_check(node, symbol_table, node, parser_results);
      continue;
    } 

    if (node->type == AST_VARIABLE_DECLARATION) {
      function_and_variable_type_check(node, symbol_table, NULL, parser_results);
      continue;
    }

    panic("Unexpected declaration type");
  }
}

static void function_and_variable_type_check(AstNode *node, SymbolTable *symbol_table, AstNode *function_declaration_node, ParserResults *parser_results) {
  switch (node->type) {
    case AST_VARIABLE_DECLARATION: {
      if (function_declaration_node == NULL) {
        type_check_file_scope_variable_declaration(node, symbol_table);
      } else {
        type_check_block_scope_variable_declaration(node, symbol_table, function_declaration_node->data.declaration_function.name);
      }

      if (node->data.declaration_variable.has_expression) {
        type_check_init(node->data.declaration_variable.type, node->data.declaration_variable.init_expression, symbol_table, function_declaration_node, parser_results);
      }
      break;
    }
    case AST_FUNCTION_DECLARATION: {
      Symbol *existing_function_symbol = get_symbol(node->data.declaration_function.name, symbol_table, false);

      if (existing_function_symbol != NULL ) {
        if (existing_function_symbol->symbol_type == SYMBOL_VARIABLE) {
          input_error_with_line("'%s' declared as variable", node->line_number, node->data.declaration_function.name);
        }

        if (existing_function_symbol->data.function_symbol->value_type->type != node->data.declaration_function.function_type->data.function_type.return_type->type) {
          input_error_with_line("Incompatible function declarations for '%s'", node->line_number, node->data.declaration_function.name);
        }

        if (existing_function_symbol->data.function_symbol->is_defined && node->data.declaration_function.body_block != NULL) {
          input_error_with_line("Function defined more than once '%s'", node->line_number, node->data.declaration_function.name);
        }

        if (existing_function_symbol->data.function_symbol->is_global == node->data.declaration_function.storage_class_type == AST_STORAGE_CLASS_STATIC) {
          input_error_with_line("Static function '%s' declaration follows non-static", node->line_number, node->data.declaration_function.name);
        }

        if (existing_function_symbol->data.function_symbol->param_count != node->data.declaration_function.function_type->data.function_type.param_type_count) {
          input_error_with_line("'%s' function declaration has different set parameters", node->line_number, node->data.declaration_function.name);
        }

        for (int i = 0; i < node->data.declaration_function.function_type->data.function_type.param_type_count; i++) {
          if (existing_function_symbol->data.function_symbol->param_types[i].type != node->data.declaration_function.function_type->data.function_type.param_types[i].type) {
            input_error_with_line("'%s' function declaration has different set parameters", node->line_number, node->data.declaration_function.name);
          }

          //We only want to add function param names for function definitions
          if (node->data.declaration_function.body_block != NULL) {
            add_function_parameter_to_symbol_table(&node->data.declaration_function.function_type->data.function_type.param_types[i], node->data.declaration_function.parameter_identifiers[i], node->data.declaration_function.name, symbol_table, parser_results);
          }
        }

        if (!existing_function_symbol->data.function_symbol->is_defined && node->data.declaration_function.body_block != NULL) {
          existing_function_symbol->data.function_symbol->is_defined = true;
          function_and_variable_type_check(node->data.declaration_function.body_block, symbol_table, node, parser_results);
        }

        break;
      }

      if (node->data.declaration_function.function_type->data.function_type.return_type->type == TYPE_ARRAY) {
        input_error_with_line("Cannot have array as function return type", node->line_number);
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

        add_function_parameter_to_symbol_table(parameter_type, node->data.declaration_function.parameter_identifiers[i], node->data.declaration_function.name, symbol_table, parser_results);
      }

      Symbol *function_symbol = add_function_symbol(symbol_table, node->data.declaration_function.name, node->data.declaration_function.function_type->data.function_type.return_type, node->data.declaration_function.function_type->data.function_type.param_type_count, param_types, is_global, is_defined);

      if (node->data.declaration_function.body_block != NULL) {
        function_and_variable_type_check(node->data.declaration_function.body_block, symbol_table, node, parser_results);
      }
      break;
    }
    case AST_EXPRESSION_FUNCTION_CALL: {
      Symbol *existing_symbol = get_symbol(node->data.expression_function_call.identifier, symbol_table, false);

      if (existing_symbol != NULL) {
        if (existing_symbol->symbol_type == SYMBOL_VARIABLE) {
          input_error_with_line("Variable '%s' is used as a function name", node->line_number, node->data.expression_function_call.identifier);
        }               

        if (existing_symbol->data.function_symbol->param_count != node->data.expression_function_call.argument_count) {
          input_error_with_line("Function '%s' called with incorrect number of arguments", node->line_number, node->data.expression_function_call.identifier);
        }
      }

      if (node->data.expression_function_call.expression_type == NULL) { 
        expression_type_check_and_convert(node, &node, symbol_table, function_declaration_node, parser_results);
      }

      for (int i = 0; i < node->data.expression_function_call.argument_count; i++) {
        AstNode *argument_node = node->data.expression_function_call.argument_ptrs->node_pointers[i];
        function_and_variable_type_check(argument_node, symbol_table, function_declaration_node, parser_results);
      }
      break;
    }
    case AST_INITIALIZER:      
      if (node->data.initializer.type == AST_INITIALIZER_SINGLE) {
        function_and_variable_type_check(node->data.initializer.initializer_node.single_init_expression, symbol_table, function_declaration_node, parser_results);
        break;
      } 

      for (int i = 0; i < node->data.initializer.initializer_node.compound_initializer->count; i++) {
        function_and_variable_type_check(&node->data.initializer.initializer_node.compound_initializer->items[i], symbol_table, function_declaration_node, parser_results);
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
    case AST_EXPRESSION_SUBSCRIPT:
      expression_type_check(node, symbol_table, function_declaration_node, parser_results);
      break;
    case AST_BLOCK: {
      for (int i = 0; i < node->data.block.block_count; i++) {   
        AstNode *block_item_node = node->data.block.block_ptrs->node_pointers[i];
        function_and_variable_type_check(block_item_node, symbol_table, function_declaration_node, parser_results);
      }
      break;
    }
    case AST_STATEMENT_IF: {
      function_and_variable_type_check(node->data.statement_if.condition_expression, symbol_table, function_declaration_node, parser_results);
      function_and_variable_type_check(node->data.statement_if.then_statement, symbol_table, function_declaration_node, parser_results);

      if (node->data.statement_if.else_statement != NULL) {
        function_and_variable_type_check(node->data.statement_if.else_statement, symbol_table, function_declaration_node, parser_results);
      }
      break;
    }
    case AST_STATEMENT_RETURN: {
      TypeNode *return_expression_type = expression_type_check_and_convert(node, &node->data.statement_return.expression, symbol_table, function_declaration_node, parser_results);
      TypeNode *function_return_type = function_declaration_node->data.declaration_function.function_type->data.function_type.return_type;

      if (function_return_type->type == TYPE_POINTER && return_expression_type->type == TYPE_POINTER) {
        if (get_pointer_base_type(return_expression_type) != get_pointer_base_type(function_return_type)) {
          input_error_with_line("Cannot implicitly convert one pointer type to another", node->line_number);
        }        
      } else if (function_return_type->type == return_expression_type->type) {
        break;
      }

      node->data.statement_return.expression = convert_to(node->data.statement_return.expression, return_expression_type, function_return_type, parser_results);
      break;
    }
    case AST_STATEMENT_FOR: {
      if (node->data.statement_for.for_loop_init != NULL) {        
        function_and_variable_type_check(node->data.statement_for.for_loop_init, symbol_table, function_declaration_node, parser_results);
      }

      if (node->data.statement_for.condition_expression != NULL) {
        function_and_variable_type_check(node->data.statement_for.condition_expression, symbol_table, function_declaration_node, parser_results);
      }

      if (node->data.statement_for.post_expression != NULL) {
        function_and_variable_type_check(node->data.statement_for.post_expression, symbol_table, function_declaration_node, parser_results);
      }

      function_and_variable_type_check(node->data.statement_for.statement_body, symbol_table, function_declaration_node, parser_results);
      break;
    }
    case AST_STATEMENT_WHILE: {
      function_and_variable_type_check(node->data.statement_while.condition, symbol_table, function_declaration_node, parser_results);
      function_and_variable_type_check(node->data.statement_while.statement_body, symbol_table, function_declaration_node, parser_results);
      break;
    }
    case AST_STATEMENT_DO_WHILE: {
      function_and_variable_type_check(node->data.statement_do_while.condition, symbol_table, function_declaration_node, parser_results);
      function_and_variable_type_check(node->data.statement_do_while.statement_body, symbol_table, function_declaration_node, parser_results);
      break;
    }
    case AST_STATEMENT_COMPOUND:      
      function_and_variable_type_check(node->data.statement_compound.block, symbol_table, function_declaration_node, parser_results);
      break;
    case AST_STATEMENT_GOTO_LABEL:
    case AST_STATEMENT_GOTO:
    case AST_STATEMENT_BREAK:
    case AST_STATEMENT_CONTINUE:
    case AST_STATEMENT_NULL:
      break;
    default:    
      panic("Unsupported AST type '%s' found in function and variable type check", get_ast_node_string(node));
  }  
}

static TypeNode* type_check_init(TypeNode *target_type, AstNode *ast_initializer, SymbolTable *symbol_table, AstNode *function_declaration_node, ParserResults *parser_results) {
  if (ast_initializer->data.initializer.type == AST_INITIALIZER_SINGLE) {
    TypeNode *expression_type = expression_type_check_and_convert(ast_initializer, &ast_initializer->data.initializer.initializer_node.single_init_expression, symbol_table, function_declaration_node, parser_results);

    if (target_type->type == TYPE_ARRAY) {
      ast_initializer = convert_by_assignment(ast_initializer->data.initializer.initializer_node.single_init_expression, expression_type, target_type->data.array_type.element_type, parser_results);
    } else {
      ast_initializer = convert_by_assignment(ast_initializer->data.initializer.initializer_node.single_init_expression, expression_type, target_type, parser_results);
    }

    return expression_type;
  }

  if (ast_initializer->data.initializer.type == AST_INITIALIZER_COMPOUND && target_type->type == TYPE_ARRAY) {
    if (ast_initializer->data.initializer.initializer_node.compound_initializer->count > target_type->data.array_type.size) {
      input_error_with_line("%d values initialized for an array of %lu size", ast_initializer->line_number, ast_initializer->data.initializer.initializer_node.compound_initializer->count, target_type->data.array_type.size);
    }

    for (int i = 0; i < ast_initializer->data.initializer.initializer_node.compound_initializer->count; i++) {
      type_check_init(target_type->data.array_type.element_type, &ast_initializer->data.initializer.initializer_node.compound_initializer->items[i], symbol_table, function_declaration_node, parser_results);
    }

    for (int i = ast_initializer->data.initializer.initializer_node.compound_initializer->count; i < target_type->data.array_type.size; i++) {
      AstNode *zero_init = zero_initializer(target_type->data.array_type.element_type, parser_results);
      dynamic_array_add(ast_initializer->data.initializer.initializer_node.compound_initializer, *zero_init, COMPOUND_INITIALIZER_CAPACITY);
    }

    return target_type;
  }

  input_error_with_line("Can't initialize a scalar object with a compound initializer", ast_initializer->line_number);
}

static AstNode* zero_initializer(const TypeNode *type_node, const ParserResults *parser_results) {
  if (type_node->type == TYPE_ARRAY) {
    CompoundInitArray *compound_array = malloc(sizeof(CompoundInitArray));
    compound_array->capacity = 0;
    compound_array->count = 0;
    compound_array->items = NULL;

    AstNode *compound_init = arena_alloc(parser_results->ast_node_arena);
    compound_init->type = AST_INITIALIZER;
    compound_init->data.initializer.type = AST_INITIALIZER_COMPOUND;
    compound_init->data.initializer.initializer_node.compound_initializer = compound_array;

    AstNode *array_init = zero_initializer(type_node->data.array_type.element_type, parser_results);

    dynamic_array_add(compound_array, *array_init, COMPOUND_INITIALIZER_CAPACITY);

    return compound_init;
  }

  AstNode *constant = arena_alloc(parser_results->ast_node_arena);
  constant->type = AST_EXPRESSION_CONSTANT;

  switch (type_node->type) {
    case TYPE_INT:      
      constant->data.expression_constant.constant_type = AST_CONSTANT_TYPE_INT;
      constant->data.expression_constant.int_value = 0;
      break;
    case TYPE_LONG:      
      constant->data.expression_constant.constant_type = AST_CONSTANT_TYPE_LONG;
      constant->data.expression_constant.long_value = 0;
      break;
    case TYPE_UINT:      
      constant->data.expression_constant.constant_type = AST_CONSTANT_TYPE_UINT;
      constant->data.expression_constant.uint_value = 0;
      break;
    case TYPE_ULONG:      
      constant->data.expression_constant.constant_type = AST_CONSTANT_TYPE_ULONG;
      constant->data.expression_constant.ulong_value = 0;
      break;
    case TYPE_DOUBLE:
      constant->data.expression_constant.constant_type = AST_CONSTANT_TYPE_DOUBLE;
      constant->data.expression_constant.double_value = 0;
      break;
    default:
      panic("Type not found for array zero initializer");
  }

  AstNode *single_init = arena_alloc(parser_results->ast_node_arena);
  single_init->type = AST_INITIALIZER;
  single_init->data.initializer.type = AST_INITIALIZER_SINGLE;
  single_init->data.initializer.initializer_node.single_init_expression = constant;

  return single_init;
}

static void type_check_file_scope_variable_declaration(AstNode *variable_declaration_node, SymbolTable *symbol_table) {
  InitializationType initialization_type; 
  InitialValueArray *initial_value_array = initial_value_array_init();
  InitialValue initial_value;

  if (variable_declaration_node->data.declaration_variable.has_expression && variable_declaration_node->data.declaration_variable.init_expression->data.expression_assignment.right_expression->type == AST_EXPRESSION_CONSTANT) {
    initialization_type = INITIALIZATION_TYPE_INITIALIZED;

    AstNode *constant_expression = variable_declaration_node->data.declaration_variable.init_expression->data.expression_assignment.right_expression;

    switch (variable_declaration_node->data.declaration_variable.type->type) {
      case TYPE_INT:
        initial_value.type = INITIAL_VALUE_TYPE_INT;
        initial_value.data.int_value = convert_variable_declaration_constant_to_int(constant_expression);
        break;
      case TYPE_LONG:
        initial_value.type = INITIAL_VALUE_TYPE_LONG;
        initial_value.data.long_value = convert_variable_declaration_constant_to_long(constant_expression);
        break;
      case TYPE_UINT:
        initial_value.type = INITIAL_VALUE_TYPE_UINT;
        initial_value.data.uint_value = convert_variable_declaration_constant_to_uint(constant_expression);
        break;
      case TYPE_ULONG:
        initial_value.type = INITIAL_VALUE_TYPE_ULONG;
        initial_value.data.ulong_value = convert_variable_declaration_constant_to_ulong(constant_expression);
        break;
      case TYPE_DOUBLE:
        initial_value.type = INITIAL_VALUE_TYPE_DOUBLE;
        initial_value.data.double_value = convert_variable_declaration_constant_to_double(constant_expression);
        break;
      case TYPE_POINTER:
        initial_value.type = INITIAL_VALUE_TYPE_ULONG;
        initial_value.data.ulong_value = convert_variable_declaration_constant_to_ulong(constant_expression);
        break;
      default:
        panic("Unsupported constant expression type when checking file scope variable");
    }
  } else if (!variable_declaration_node->data.declaration_variable.has_expression) {
    if (variable_declaration_node->data.declaration_variable.storage_class_type == AST_STORAGE_CLASS_EXTERN) {
      initialization_type = INITIALIZATION_TYPE_NO_INITIALIZER;
    } else {
      initialization_type = INITIALIZATION_TYPE_TENTATIVE;
    }

    symbol_initialize_to_zero(variable_declaration_node->data.declaration_variable.type, &initial_value);
  } else {
    input_error_with_line("Non-constant initializer for variable declaration '%s'", variable_declaration_node->line_number, variable_declaration_node->data.declaration_variable.name);
  }

  bool is_global = variable_declaration_node->data.declaration_variable.storage_class_type != AST_STORAGE_CLASS_STATIC;

  Symbol *existing_variable_symbol = get_symbol(variable_declaration_node->data.declaration_variable.name, symbol_table, false);

  if (existing_variable_symbol != NULL) {
    if (existing_variable_symbol->symbol_type == SYMBOL_FUNCTION) {
      input_error_with_line("Function '%s' redeclared as variable", variable_declaration_node->line_number, variable_declaration_node->data.declaration_variable.name);
    }

    if (variable_declaration_node->data.declaration_variable.type->type != existing_variable_symbol->data.variable_symbol->value_type->type) {
      input_error_with_line("Previously declared '%s' variable has type of '%s'", variable_declaration_node->line_number, variable_declaration_node->data.declaration_variable.name, get_type_string(existing_variable_symbol->data.variable_symbol->value_type->type));
    }

    if (variable_declaration_node->data.declaration_variable.storage_class_type == AST_STORAGE_CLASS_EXTERN) {
      existing_variable_symbol->data.variable_symbol->static_is_global = true;
    }
    else if (existing_variable_symbol->data.variable_symbol->static_is_global != is_global) {
      input_error_with_line("Function '%s' conflicting variable linkage", variable_declaration_node->line_number, variable_declaration_node->data.declaration_variable.name);
    }

    if (existing_variable_symbol->data.variable_symbol->static_initialization_type == INITIALIZATION_TYPE_INITIALIZED) {
      if (initialization_type == INITIALIZATION_TYPE_INITIALIZED) {
        input_error_with_line("Function '%s' conflicting file scope variable definitions", variable_declaration_node->line_number, variable_declaration_node->data.declaration_variable.name);
      }
    } else {
      existing_variable_symbol->data.variable_symbol->static_initialization_type = initialization_type;
      dynamic_array_add(existing_variable_symbol->data.variable_symbol->static_initial_value_array, initial_value, STATIC_INITIAL_VALUE_CAPACITY);
    }

    return;
  }

  dynamic_array_add(initial_value_array, initial_value, STATIC_INITIAL_VALUE_CAPACITY);
  add_static_variable_symbol(symbol_table, variable_declaration_node->data.declaration_variable.type, initial_value_array, variable_declaration_node->data.declaration_variable.name, is_global, initialization_type);  
}

static void type_check_block_scope_variable_declaration(AstNode *variable_declaration_node, SymbolTable *symbol_table, char *function_name) {
  if (variable_declaration_node->data.declaration_variable.storage_class_type == AST_STORAGE_CLASS_EXTERN) {
    if (variable_declaration_node->data.declaration_variable.has_expression) {
      input_error_with_line("Initializer on local extern variable declaration '%s'", variable_declaration_node->line_number, variable_declaration_node->data.declaration_variable.name);
    }
    
    Symbol *existing_variable_symbol = get_symbol(variable_declaration_node->data.declaration_variable.name, symbol_table, false);

    if (existing_variable_symbol != NULL) {
      if (existing_variable_symbol->symbol_type == SYMBOL_FUNCTION) {        
        input_error_with_line("Function redeclared as variable", variable_declaration_node->line_number);
      }
    } else {
      add_static_extern_variable_symbol(symbol_table, variable_declaration_node->data.declaration_variable.type, variable_declaration_node->data.declaration_variable.name); 
    }
    
    return;
  }

  if (variable_declaration_node->data.declaration_variable.storage_class_type == AST_STORAGE_CLASS_STATIC) {
    InitialValue initial_value;
    InitialValueArray *initial_value_array = initial_value_array_init();
    
    if (!variable_declaration_node->data.declaration_variable.has_expression) {
      symbol_initialize_to_zero(variable_declaration_node->data.declaration_variable.type, &initial_value);
      add_static_variable_symbol(symbol_table, variable_declaration_node->data.declaration_variable.type, initial_value_array, variable_declaration_node->data.declaration_variable.name, false, INITIALIZATION_TYPE_INITIALIZED);
      return;
    }

    if (variable_declaration_node->data.declaration_variable.init_expression->data.initializer.type == AST_INITIALIZER_SINGLE && variable_declaration_node->data.declaration_variable.init_expression->data.initializer.initializer_node.single_init_expression->type == AST_EXPRESSION_CONSTANT) {
        add_variable_declaration_single_init_to_array(initial_value_array, variable_declaration_node->data.declaration_variable.type, variable_declaration_node->data.declaration_variable.init_expression);        
        add_static_variable_symbol(symbol_table, variable_declaration_node->data.declaration_variable.type, initial_value_array, variable_declaration_node->data.declaration_variable.name, false, INITIALIZATION_TYPE_INITIALIZED);
        return;
    }

    if (variable_declaration_node->data.declaration_variable.init_expression->data.initializer.type == AST_INITIALIZER_COMPOUND) {
      add_variable_declaration_compound_init_to_array(initial_value_array, variable_declaration_node->data.declaration_variable.type, variable_declaration_node->data.declaration_variable.init_expression);
      add_static_variable_symbol(symbol_table, variable_declaration_node->data.declaration_variable.type, initial_value_array, variable_declaration_node->data.declaration_variable.name, false, INITIALIZATION_TYPE_INITIALIZED);
      return;
    }

    input_error_with_line("Non-constant initializer on local static variable '%s'\n", variable_declaration_node->line_number, variable_declaration_node->data.declaration_variable.name);
  }   

  add_automatic_variable_symbol(symbol_table, variable_declaration_node->data.declaration_variable.type, variable_declaration_node->data.declaration_variable.name);
} 

static void add_variable_declaration_single_init_to_array(InitialValueArray *initial_value_array, TypeNode *declaration_type, AstNode *single_init) {
  InitialValue *initial_value = malloc(sizeof(InitialValue));

  AstNode *constant_node = single_init->data.initializer.initializer_node.single_init_expression; 

  if (declaration_type->type == TYPE_ARRAY) {
    declaration_type = declaration_type->data.array_type.element_type;
  }

  switch(declaration_type->type) {
   case TYPE_INT:
     initial_value->type = INITIAL_VALUE_TYPE_INT;
     initial_value->data.int_value = convert_variable_declaration_constant_to_int(constant_node);
     break;
   case TYPE_LONG:
     initial_value->type = INITIAL_VALUE_TYPE_LONG;
     initial_value->data.long_value = convert_variable_declaration_constant_to_long(constant_node);
     break;
   case TYPE_UINT:
     initial_value->type = INITIAL_VALUE_TYPE_UINT;
     initial_value->data.uint_value = convert_variable_declaration_constant_to_uint(constant_node);
     break;
   case TYPE_ULONG:
     initial_value->type = INITIAL_VALUE_TYPE_ULONG;
     initial_value->data.ulong_value = convert_variable_declaration_constant_to_ulong(constant_node);
     break;
   case TYPE_DOUBLE:
     initial_value->type = INITIAL_VALUE_TYPE_DOUBLE;
     initial_value->data.double_value = convert_variable_declaration_constant_to_double(constant_node);
     break;
   case TYPE_POINTER: {
     unsigned long value = convert_variable_declaration_constant_to_ulong(constant_node);

     if (value != 0) {
       input_error_with_line("Cannot assign value '%ld' to a static pointer\n", single_init->line_number, value);
     }

     initial_value->type = INITIAL_VALUE_TYPE_ULONG;
     initial_value->data.ulong_value = value;
     break;
   }
   default:
     panic("Unsupported initial value AST Type '%s'", get_type_string(declaration_type->type));
  }

  dynamic_array_add(initial_value_array, *initial_value, STATIC_INITIAL_VALUE_CAPACITY);
}

static void add_variable_declaration_compound_init_to_array(InitialValueArray *initial_value_array, TypeNode *declaration_type, AstNode *compound_init) {
    if (declaration_type->data.array_type.element_type->type == TYPE_ARRAY) {
      for (int i = 0; i < compound_init->data.initializer.initializer_node.compound_initializer->count; i++) {
        add_variable_declaration_compound_init_to_array(initial_value_array, declaration_type->data.array_type.element_type, &compound_init->data.initializer.initializer_node.compound_initializer->items[i]);
      }

    int compound_diff = declaration_type->data.array_type.size - compound_init->data.initializer.initializer_node.compound_initializer->count;

    if (compound_diff != 0) {
      size_t size = get_array_base_size(declaration_type);

      InitialValue *initial_value = malloc(sizeof(InitialValue));
      initial_value->type = INITIAL_VALUE_TYPE_ZERO_INIT;
      //TODO: Warning. Casting to int.
      initial_value->data.zero_init_array_bytes = (int)size * (compound_diff * (int)declaration_type->data.array_type.size);
      dynamic_array_add(initial_value_array, *initial_value, STATIC_INITIAL_VALUE_CAPACITY);
    }
  } else {
    for (int i = 0; i < compound_init->data.initializer.initializer_node.compound_initializer->count; i++) {
      add_variable_declaration_single_init_to_array(initial_value_array, declaration_type, &compound_init->data.initializer.initializer_node.compound_initializer->items[i]);
    }

    int compound_diff = declaration_type->data.array_type.size - compound_init->data.initializer.initializer_node.compound_initializer->count;

    if (compound_diff != 0) {
      size_t size = get_array_base_size(declaration_type);

      InitialValue *initial_value = malloc(sizeof(InitialValue));
      initial_value->type = INITIAL_VALUE_TYPE_ZERO_INIT;
      //@Warning: Casting to int.
      initial_value->data.zero_init_array_bytes = (int)size * compound_diff;
      dynamic_array_add(initial_value_array, *initial_value, STATIC_INITIAL_VALUE_CAPACITY);
    }
  }
}

static TypeNode* expression_type_check(AstNode *node, SymbolTable *symbol_table, AstNode *function_declaration_node, ParserResults *parser_results) {
  switch (node->type) {
    case AST_EXPRESSION_VARIABLE: {
      Symbol* symbol = get_symbol(node->data.expression_variable.identifier, symbol_table, true);

      if (symbol->symbol_type == SYMBOL_FUNCTION) {
        input_error_with_line("Function name '%s' is being used as a variable", node->line_number, node->data.expression_variable.identifier);
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
          panic("Could not resolve value type in variable symbol");
      }

      node->data.expression_constant.expression_type = expression_type;

      return expression_type;
    }
    case AST_EXPRESSION_CAST: {
      TypeNode *expression_type = expression_type_check_and_convert(node, &node->data.expression_cast.expression, symbol_table, function_declaration_node, parser_results);

      if (node->data.expression_cast.target_type->type == TYPE_ARRAY) {
        input_error_with_line("Cannot cast to an array type", node->line_number);
      }
      
      if (node->data.expression_cast.target_type->type == TYPE_DOUBLE && node->data.expression_cast.target_type->type == TYPE_POINTER && get_pointer_base_type(node->data.expression_cast.target_type) == TYPE_DOUBLE) {
        input_error_with_line("Cannot cast double pointer to double", node->line_number);
      }

      if (expression_type->type == TYPE_DOUBLE && node->data.expression_cast.target_type->type == TYPE_POINTER && get_pointer_base_type(node->data.expression_cast.target_type) != TYPE_DOUBLE) {
        input_error_with_line("Double cannot be cast to pointer type", node->line_number);
      }

      return node->data.expression_cast.target_type;
    }
    case AST_EXPRESSION_UNARY: {
      TypeNode *expression_type = expression_type_check_and_convert(node, &node->data.expression_unary.expression, symbol_table, function_declaration_node, parser_results);

      if (node->data.expression_unary.op_type == AST_UNARY_COMPLEMENT && expression_type->type == TYPE_DOUBLE) {
        input_error_with_line("Cannot apply unary complement operator to a double", node->line_number);
      }

      if (expression_type->type == TYPE_POINTER && (node->data.expression_unary.op_type == AST_UNARY_COMPLEMENT || node->data.expression_unary.op_type == AST_UNARY_NEGATE)) {
        input_error_with_line("Cannot apply unary complement or negate operator to a pointer", node->line_number);
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
      TypeNode *left_expression_type = expression_type_check_and_convert(node, &node->data.expression_binary.left_expression, symbol_table, function_declaration_node, parser_results);
      TypeNode *right_expression_type = expression_type_check_and_convert(node, &node->data.expression_binary.right_expression, symbol_table, function_declaration_node, parser_results);

      if (right_expression_type->type == TYPE_DOUBLE || left_expression_type->type == TYPE_DOUBLE) {
        switch (node->data.expression_binary.op_type) {
          case AST_BINARY_BITWISE_OR:
          case AST_BINARY_BITWISE_XOR:
          case AST_BINARY_BITWISE_RIGHT_SHIFT:
          case AST_BINARY_BITWISE_LEFT_SHIFT:
          case AST_BINARY_BITWISE_AND:
          case AST_BINARY_REMAINDER:
            input_error_with_line("Cannot apply binary %s operator with a double value", node->line_number, get_binary_op_type_string(node->data.expression_binary.op_type));
          default: break;
        }
      }

      if (right_expression_type->type == TYPE_POINTER || left_expression_type->type == TYPE_POINTER) {
        switch (node->data.expression_binary.op_type) {
          case AST_BINARY_MULTIPLY:
          case AST_BINARY_DIVIDE:
          case AST_BINARY_REMAINDER:
          case AST_BINARY_BITWISE_AND:
          case AST_BINARY_BITWISE_XOR:
          case AST_BINARY_BITWISE_RIGHT_SHIFT:
          case AST_BINARY_BITWISE_LEFT_SHIFT:
            input_error_with_line("Cannot apply a binary %s operator with a pointer", node->line_number, get_binary_op_type_string(node->data.expression_binary.op_type));
          case AST_BINARY_EQUAL:
          case AST_BINARY_LESS_THAN:
          case AST_BINARY_LESS_OR_EQUAL:
          case AST_BINARY_GREATER_THAN:
          case AST_BINARY_GREATER_OR_EQUAL:             
            if (right_expression_type->type == TYPE_POINTER && left_expression_type->type == TYPE_POINTER && get_pointer_base_type(left_expression_type) != get_pointer_base_type(right_expression_type)) {
              input_error_with_line("Cannot compare pointers of different types", node->line_number);
            }
            break;        
          default: break;
          }

        switch (node->data.expression_binary.op_type) {
          case AST_BINARY_ADD:
          case AST_BINARY_SUBTRACT:
          case AST_BINARY_LESS_THAN:
          case AST_BINARY_LESS_OR_EQUAL:
          case AST_BINARY_GREATER_THAN:
          case AST_BINARY_GREATER_OR_EQUAL:             
            if (is_null_pointer_constant(node->data.expression_binary.left_expression) || is_null_pointer_constant(node->data.expression_binary.right_expression)) {
              input_error_with_line("Cannot perform %s operation with a null constant", node->line_number, get_binary_op_type_string(node->data.expression_binary.op_type));
            }
            break;
          default: break;
        }
      }

      switch (node->data.expression_binary.op_type) {
        case AST_BINARY_AND:
        case AST_BINARY_OR:
          return expression_type_check_binary_logical(node, parser_results);
        case AST_BINARY_ADD:
          return expression_type_check_binary_add(node, left_expression_type, right_expression_type, symbol_table, parser_results);
        case AST_BINARY_SUBTRACT:
          return expression_type_check_binary_subtract(node, left_expression_type, right_expression_type, symbol_table, parser_results);
        default:
          return expression_type_check_binary(node, function_declaration_node, left_expression_type, right_expression_type, symbol_table, parser_results);
      }
    }
    case AST_EXPRESSION_ASSIGNMENT: {
      TypeNode *left_expression_type = expression_type_check_and_convert(node, &node->data.expression_assignment.left_expression, symbol_table, function_declaration_node, parser_results);

      //@Note: Added AST_EXPRESSION_ASSIGNMENT here to support expressions like 'a = b = d = += h';
      if (node->data.expression_assignment.left_expression->type != AST_EXPRESSION_ASSIGNMENT && node->data.expression_assignment.left_expression->type != AST_EXPRESSION_VARIABLE && node->data.expression_assignment.left_expression->type != AST_EXPRESSION_DEREFERENCE && node->data.expression_assignment.left_expression->type != AST_EXPRESSION_SUBSCRIPT) {
        input_error_with_line("Tried to assign to a non-lvalue", node->line_number);
      }

      TypeNode *right_expression_type = expression_type_check_and_convert(node, &node->data.expression_assignment.right_expression, symbol_table, function_declaration_node, parser_results);

      if (left_expression_type->type == TYPE_POINTER && right_expression_type->type == TYPE_POINTER && get_pointer_base_type(left_expression_type) != get_pointer_base_type(right_expression_type)) {        
        input_error_with_line("Expression assignment of pointers aren't for the same type", node->line_number);
      }

      node->data.expression_assignment.right_expression = convert_by_assignment(node->data.expression_assignment.right_expression, right_expression_type, left_expression_type, parser_results);

      return left_expression_type;
    }
    case AST_EXPRESSION_FUNCTION_CALL: {
      Symbol *existing_symbol = get_symbol(node->data.expression_function_call.identifier, symbol_table, true);

      if (existing_symbol->symbol_type == SYMBOL_VARIABLE) {
        input_error_with_line("Variable '%s' is used as a function name", node->line_number, node->data.expression_function_call.identifier);
      }               

      if (existing_symbol->data.function_symbol->param_count != node->data.expression_function_call.argument_count) {
        input_error_with_line("Function '%s' called with incorrect number of arguments", node->line_number, node->data.expression_function_call.identifier);
      }
      
      for (int i = 0; i < node->data.expression_function_call.argument_count; i++) {
        AstNode *argument_node = node->data.expression_function_call.argument_ptrs->node_pointers[i];
        function_and_variable_type_check(argument_node, symbol_table, function_declaration_node, parser_results);
      }
    
      //@NOTE: Attempting to reuse existing types here rather than creating a new one
      node->data.expression_function_call.expression_type = existing_symbol->data.function_symbol->value_type;

      return existing_symbol->data.function_symbol->value_type;
    }
    case AST_EXPRESSION_CONDITIONAL: {
      //TODO: Confirm that the conditional expression type does not need to do anything with the set common type
      TypeNode* condition_type = expression_type_check_and_convert(node, &node->data.expression_conditional.condition, symbol_table, function_declaration_node, parser_results);
        node->data.expression_conditional.expression_type = condition_type;

      TypeNode *true_expression_type = expression_type_check_and_convert(node, &node->data.expression_conditional.true_expression, symbol_table, function_declaration_node, parser_results);
      TypeNode *false_expression_type = expression_type_check_and_convert(node, &node->data.expression_conditional.false_expression, symbol_table, function_declaration_node, parser_results);
      
      TypeNode *common_type;

      if (true_expression_type->type == TYPE_POINTER || false_expression_type->type == TYPE_POINTER) {
        common_type = get_common_pointer_type(node, node->data.expression_conditional.true_expression, node->data.expression_conditional.false_expression, symbol_table, function_declaration_node, parser_results);
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
      return expression_type_check_and_convert(node, &node->data.expression_increment_decrement.expression, symbol_table, function_declaration_node, parser_results);
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
        input_error_with_line("Cannot take the address of a non-lvalue", node->line_number);
      }

      //@Note: Do not call 'expression_type_check_convert()' for address_of operand
      TypeNode *address_of_expression_type = expression_type_check(node->data.expression_address_of.expression, symbol_table, function_declaration_node, parser_results);

      TypeNode *pointer_type_node = arena_alloc(parser_results->type_node_arena);
      pointer_type_node->type = TYPE_POINTER;
      pointer_type_node->data.pointer_type.reference_type = address_of_expression_type;

      return pointer_type_node;
    }
    case AST_EXPRESSION_DEREFERENCE: {
      TypeNode *expression_type = expression_type_check_and_convert(node, &node->data.expression_dereference.expression, symbol_table, function_declaration_node, parser_results);

      if (expression_type->type != TYPE_POINTER) {
        input_error_with_line("Cannot dereference a non-pointer", node->line_number);
      }

      //TODO: Will this work if it's greater than one level? Example: int** 
      return expression_type->data.pointer_type.reference_type;
    }
    case AST_EXPRESSION_SUBSCRIPT: {
      TypeNode *expression_1_type = expression_type_check_and_convert(node, &node->data.expression_subscript.expression_1, symbol_table, function_declaration_node, parser_results);
      TypeNode *expression_2_type = expression_type_check_and_convert(node, &node->data.expression_subscript.expression_2, symbol_table, function_declaration_node, parser_results);

      TypeNode *long_type_node = arena_alloc(parser_results->type_node_arena);
      long_type_node->type = TYPE_LONG;

      //@Test: Not sure if these returned expression types are correct
      if (expression_1_type->type == TYPE_POINTER && is_integer_type(expression_2_type)) {
        node->data.expression_subscript.expression_2 = convert_to(node->data.expression_subscript.expression_2, expression_2_type, long_type_node, parser_results);
        node->data.expression_subscript.expression_type = expression_1_type->data.pointer_type.reference_type;
        return node->data.expression_subscript.expression_type;
      }

      if (expression_2_type->type == TYPE_POINTER && is_integer_type(expression_1_type)) {
        node->data.expression_subscript.expression_1 = convert_to(node->data.expression_subscript.expression_1, expression_1_type, long_type_node, parser_results);
        node->data.expression_subscript.expression_type = expression_2_type->data.pointer_type.reference_type;
        return node->data.expression_subscript.expression_type;
      }

      input_error_with_line("Subscript must have an integer and pointer operand", node->line_number);
    }
    default:
      panic("Invalid AST type '%d' found in expression type check", node->type);
  }
}

static TypeNode* expression_type_check_binary_logical(AstNode *node, ParserResults *parser_results) {
  TypeNode *expression_type_node = arena_alloc(parser_results->type_node_arena);
  expression_type_node->type = TYPE_INT;

  node->data.expression_binary.expression_type = expression_type_node;
  return expression_type_node;
}

static TypeNode* expression_type_check_binary(AstNode *binary_node, AstNode *function_declaration_node, TypeNode *left_expression_type, TypeNode *right_expression_type, SymbolTable *symbol_table, ParserResults *parser_results) { 
  TypeNode *common_type;
  
  if ((binary_node->data.expression_binary.op_type == AST_BINARY_EQUAL || binary_node->data.expression_binary.op_type == AST_BINARY_NOT_EQUAL) && (left_expression_type->type == TYPE_POINTER || right_expression_type->type == TYPE_POINTER)) {
    common_type = get_common_pointer_type(binary_node, binary_node->data.expression_binary.left_expression, binary_node->data.expression_binary.right_expression, symbol_table, function_declaration_node, parser_results);
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

static TypeNode* expression_type_check_binary_add(AstNode *add_node, TypeNode *left_expression_type, TypeNode *right_expression_type, SymbolTable *symbol_table, ParserResults *parser_results) { 
  TypeNode *common_type = get_common_real_type(left_expression_type, right_expression_type);

  if (is_arithmetic_type(left_expression_type) && is_arithmetic_type(right_expression_type)) {
    add_node->data.expression_binary.left_expression = convert_to(add_node->data.expression_binary.left_expression, left_expression_type, common_type, parser_results);
    add_node->data.expression_binary.right_expression = convert_to(add_node->data.expression_binary.right_expression, right_expression_type, common_type, parser_results);

    TypeNode *expression_type_node = arena_alloc(parser_results->type_node_arena);
    expression_type_node = common_type;

    add_node->data.expression_binary.expression_type = expression_type_node;

    return expression_type_node;
  }

  //TODO: Look into areas where I'm doing something similar for types that aren't Pointer. No need in allocating the same long, int, etc type nodes. Instead, do it once and point things to it.
  TypeNode *long_type_node = arena_alloc(parser_results->type_node_arena);
  long_type_node->type = TYPE_LONG;

  if (left_expression_type->type == TYPE_POINTER && is_integer_type(right_expression_type)) {
    add_node->data.expression_binary.right_expression = convert_to(add_node->data.expression_binary.right_expression, right_expression_type, long_type_node, parser_results);
    add_node->data.expression_binary.expression_type = left_expression_type;
    return left_expression_type;
  }

  if (right_expression_type->type == TYPE_POINTER && is_integer_type(left_expression_type)) {
    add_node->data.expression_binary.left_expression = convert_to(add_node->data.expression_binary.left_expression, left_expression_type, long_type_node, parser_results);
    add_node->data.expression_binary.expression_type = right_expression_type;
    return right_expression_type;
  }

  input_error_with_line("Invalid operands for addition", add_node->line_number);
}

static TypeNode* expression_type_check_binary_subtract(AstNode *subtract_node, TypeNode *left_expression_type, TypeNode *right_expression_type, SymbolTable *symbol_table, ParserResults *parser_results) { 
  TypeNode *common_type = get_common_real_type(left_expression_type, right_expression_type);

  if (is_arithmetic_type(left_expression_type) && is_arithmetic_type(right_expression_type)) {
    subtract_node->data.expression_binary.left_expression = convert_to(subtract_node->data.expression_binary.left_expression, left_expression_type, common_type, parser_results);
    subtract_node->data.expression_binary.right_expression = convert_to(subtract_node->data.expression_binary.right_expression, right_expression_type, common_type, parser_results);

    TypeNode *expression_type_node = arena_alloc(parser_results->type_node_arena);
    expression_type_node = common_type;

    subtract_node->data.expression_binary.expression_type = expression_type_node;

    return expression_type_node;
  }

  //TODO: Look into areas where I'm doing something similar for types that aren't Pointer. No need in allocating the same long, int, etc type nodes. Instead, do it once and point things to it.
  TypeNode *long_type_node = arena_alloc(parser_results->type_node_arena);
  long_type_node->type = TYPE_LONG;

  if (left_expression_type->type == TYPE_POINTER && is_integer_type(right_expression_type)) {
    subtract_node->data.expression_binary.right_expression = convert_to(subtract_node->data.expression_binary.right_expression, right_expression_type, long_type_node, parser_results);

    subtract_node->data.expression_binary.expression_type = left_expression_type;
    return left_expression_type;
  }

  if (left_expression_type->type == TYPE_POINTER && get_pointer_base_type(left_expression_type) == get_pointer_base_type(right_expression_type)) {
    subtract_node->data.expression_binary.expression_type = long_type_node;
    return long_type_node;
  }

  input_error_with_line("Invalid operands for subtraction", subtract_node->line_number);
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

  if (get_type_size(type_1) == get_type_size(type_2)) {
    if (type_1->type == TYPE_INT || type_1->type == TYPE_LONG) {
      return type_2;
    }

    return type_1;
  }  

  if (get_type_size(type_1) > get_type_size(type_2)) {
    return type_1;
  } 

  return type_2;
}

static TypeNode* get_common_pointer_type(AstNode *source, AstNode *expression_1, AstNode *expression_2, SymbolTable *symbol_table, AstNode *function_declaration_node, ParserResults *parser_results) {
  TypeNode *expression_1_type = expression_type_check_and_convert(source, &expression_1, symbol_table, function_declaration_node, parser_results); 
  TypeNode *expression_2_type = expression_type_check_and_convert(source, &expression_2, symbol_table, function_declaration_node, parser_results); 

  if (expression_1_type->type == expression_2_type->type) {
    return expression_1_type;
  }

  if (is_null_pointer_constant(expression_1)) {
    return expression_2_type;
  }

  if (is_null_pointer_constant(expression_2)) {
    return expression_1_type;
  }

  input_error_with_line("Common pointer expressions have incompatible types", expression_1->line_number);
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

static void add_function_parameter_to_symbol_table(TypeNode *parameter_type, char *parameter_identifier, char *function_name, SymbolTable *symbol_table, ParserResults *parser_results) {
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
    
    add_automatic_variable_symbol(symbol_table, pointer_type_node, symbol_key);
  } else {
    add_automatic_variable_symbol(symbol_table, parameter_type, symbol_key);
  }
}

static long convert_variable_declaration_constant_to_long(AstNode *constant_node) {
  switch (constant_node->data.expression_constant.constant_type) {
    case AST_CONSTANT_TYPE_INT:    return (long)constant_node->data.expression_constant.int_value;
    case AST_CONSTANT_TYPE_UINT:   return (long)constant_node->data.expression_constant.uint_value;
    case AST_CONSTANT_TYPE_ULONG:  return (long)constant_node->data.expression_constant.ulong_value;
    case AST_CONSTANT_TYPE_DOUBLE: return (long)constant_node->data.expression_constant.double_value;
    case AST_CONSTANT_TYPE_LONG:   return constant_node->data.expression_constant.long_value;
    default:                       panic("Unsupported constant type when converting to long");
  }
}

static int convert_variable_declaration_constant_to_int(AstNode *constant_node) {
  switch (constant_node->data.expression_constant.constant_type) {
    case AST_CONSTANT_TYPE_INT:    return constant_node->data.expression_constant.int_value;
    case AST_CONSTANT_TYPE_UINT:   return (int)constant_node->data.expression_constant.uint_value;
    case AST_CONSTANT_TYPE_ULONG:  return (int)constant_node->data.expression_constant.ulong_value;
    case AST_CONSTANT_TYPE_DOUBLE: return (int)constant_node->data.expression_constant.double_value;
    case AST_CONSTANT_TYPE_LONG:   return (int)constant_node->data.expression_constant.long_value;
    default:                       panic("Unsupported constant type when converting to int");
  }
}

static unsigned int convert_variable_declaration_constant_to_uint(AstNode *constant_node) {
  switch (constant_node->data.expression_constant.constant_type) {
    case AST_CONSTANT_TYPE_INT:    return (unsigned int)constant_node->data.expression_constant.int_value;
    case AST_CONSTANT_TYPE_UINT:   return constant_node->data.expression_constant.uint_value;
    case AST_CONSTANT_TYPE_ULONG:  return (unsigned int)constant_node->data.expression_constant.ulong_value;
    case AST_CONSTANT_TYPE_DOUBLE: return (unsigned int)constant_node->data.expression_constant.double_value;
    case AST_CONSTANT_TYPE_LONG:   return (unsigned int)constant_node->data.expression_constant.long_value;
    default:                       panic("Unsupported constant type when converting to uint");
  }
}

static unsigned long convert_variable_declaration_constant_to_ulong(AstNode *constant_node) {
  switch (constant_node->data.expression_constant.constant_type) {
    case AST_CONSTANT_TYPE_INT:    return (unsigned long)constant_node->data.expression_constant.int_value;
    case AST_CONSTANT_TYPE_UINT:   return (unsigned long)constant_node->data.expression_constant.uint_value;
    case AST_CONSTANT_TYPE_ULONG:  return constant_node->data.expression_constant.ulong_value;
    case AST_CONSTANT_TYPE_DOUBLE: return (unsigned long)constant_node->data.expression_constant.double_value;
    case AST_CONSTANT_TYPE_LONG:   return (unsigned long)constant_node->data.expression_constant.long_value;
    default:                       panic("Unsupported constant type when converting to uint");
  }
}

static double convert_variable_declaration_constant_to_double(AstNode *constant_node) {
  switch (constant_node->data.expression_constant.constant_type) {
    case AST_CONSTANT_TYPE_INT:    return (double)constant_node->data.expression_constant.int_value;
    case AST_CONSTANT_TYPE_UINT:   return (double)constant_node->data.expression_constant.uint_value;
    case AST_CONSTANT_TYPE_ULONG:  return (double)constant_node->data.expression_constant.ulong_value;
    case AST_CONSTANT_TYPE_DOUBLE: return constant_node->data.expression_constant.double_value;
    case AST_CONSTANT_TYPE_LONG:   return (double)constant_node->data.expression_constant.long_value;
    default:                       panic("Unsupported constant type when converting to double");
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

  input_error_with_line("Cannot convert type for assignment expression", right_assignment_expression->line_number);
}

static TypeNode* expression_type_check_and_convert(AstNode *source_node, AstNode **node, SymbolTable *symbol_table, AstNode *function_declaration_node, ParserResults *parser_results) {
  TypeNode *expression_type = expression_type_check(*node, symbol_table, function_declaration_node, parser_results);

  if (expression_type->type != TYPE_ARRAY) {
    return expression_type;
  }

  //@Debt: This was added because of the way subscript expressions are returning their referenced type. This led to the main subscript node also being wrapped in an 'address of' node that we did not want. To prevent this, the 'source_node' param was added to track if the source node is not a subscript. A 'source node' was also added get_common_pointer_type due to dependencies. This isn't the ideal solution, but one to get passed subscripting issues.  
  //@Debt: Another thing we are doing here is this while loop to return the base type of the TypeNode when it is a multi-dimensional array. Example: arr[i][j] where arr is an int returns an array type for the head Subscript node when it should be returning an int.
  if ((*node)->type == AST_EXPRESSION_SUBSCRIPT && source_node->type != AST_EXPRESSION_SUBSCRIPT) {
    if ((*node)->data.expression_subscript.expression_type->type == TYPE_ARRAY) {
      TypeNode *curNode = (*node)->data.expression_subscript.expression_type;
      while (curNode->type == TYPE_ARRAY) {
        curNode = curNode->data.array_type.element_type;
      }

      (*node)->data.expression_subscript.expression_type = curNode;
      return curNode;
    }
    return expression_type;
  }

  AstNode *address_of_array = arena_alloc(parser_results->ast_node_arena);
  address_of_array->line_number = (*node)->line_number;
  address_of_array->type = AST_EXPRESSION_ADDRESS_OF;
  address_of_array->data.expression_address_of.expression = *node;

  *node = address_of_array;

  //@Test: Test that this is the correct reference type that we want to add. I think this is wrong and it should be the indexed element
   TypeNode *address_of_array_pointer = arena_alloc(parser_results->type_node_arena);
   address_of_array_pointer->type = TYPE_POINTER;
   address_of_array_pointer->data.pointer_type.reference_type = expression_type;

  return address_of_array_pointer;
  //return expression_type;
}
