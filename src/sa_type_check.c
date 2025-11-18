#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sys/select.h>
#include "../include/sa_type_check.h"
#include "../include/arena.h"
#include "../include/parser.h"
#include "../include/declaration_symbol.h"

//TODO: Check to see how we can better optimize these types of buffers. Exact same use of this buffer is in sa_variable_resolution
#define IDENTIFIER_BUFFER 256

static void             function_and_variable_type_check(AstNode *node, DeclarationSymbolTable *declaration_table, AstNode *function_declaration_node, Arena *ast_arena);
static void             type_check_file_scope_variable_declaration(AstNode *variable_declaration_node, DeclarationSymbolTable *declaration_table); 
static void             type_check_block_scope_variable_declaration(AstNode *variable_declaration_node, DeclarationSymbolTable *declaration_table, char *function_name); 
static void             add_function_parameter_to_symbol_table(TypeNode *parameter_type, char *parameter_identifier, char *function_name, DeclarationSymbolTable *declaration_table); 
static Types            expression_type_check(AstNode *node, DeclarationSymbolTable *declaration_table, AstNode *function_declaration_node, Arena *ast_arena); 
static Types            get_common_real_type(Types type_1, Types type_2);
static Types            get_common_pointer_type(AstNode *expression_1, AstNode *expression_2, DeclarationSymbolTable *declaration_table, AstNode *function_declaration_node, Arena *ast_arena); 
static AstNode*         convert_to(AstNode *expression, Types expression_type, Types target_type, Arena *ast_arena); 
static long             convert_variable_declaration_constant_to_long(AstNode *variable_declaration_node); 
static int              convert_variable_declaration_constant_to_int(AstNode *variable_declaration_node); 
static unsigned long    convert_variable_declaration_constant_to_ulong(AstNode *variable_declaration_node); 
static unsigned int     convert_variable_declaration_constant_to_uint(AstNode *variable_declaration_node); 
static double           convert_variable_declaration_constant_to_double(AstNode *variable_declaration_node); 
static AstNode*         convert_by_assignment(AstNode *right_assignment_expression, Types right_assignment_type, Types target_type, Arena *ast_arena); 
static bool             is_null_pointer_constant(AstNode *ast_node);

void sa_type_check(AstNode *ast_nodes, DeclarationSymbolTable *declaration_table, Arena *ast_arena) {
  for (int i = 0; i < ast_nodes->data.program.declaration_count; i++) {
    AstNode *node = ast_nodes->data.program.declaration_ptrs->node_pointers[i];

    if (node->type == AST_FUNCTION_DECLARATION) {
      function_and_variable_type_check(node, declaration_table, node, ast_arena);
      continue;
    } 

    if (node->type == AST_VARIABLE_DECLARATION) {
      function_and_variable_type_check(node, declaration_table, NULL, ast_arena);
      continue;
    }

    fprintf(stderr, "ERROR - SA Type Check: Unexpected declaration type\n");
    exit(1);
  } 
}

static void function_and_variable_type_check(AstNode *node, DeclarationSymbolTable *declaration_table, AstNode *function_declaration_node, Arena *ast_arena) {
  switch (node->type) {
    case AST_VARIABLE_DECLARATION: {
      if (function_declaration_node == NULL) {
        type_check_file_scope_variable_declaration(node, declaration_table);
      } else {
        type_check_block_scope_variable_declaration(node, declaration_table, function_declaration_node->data.declaration_function.name);
      }

      if (node->data.declaration_variable.has_expression) {
        function_and_variable_type_check(node->data.declaration_variable.init_expression, declaration_table, function_declaration_node, ast_arena);
      }
      break;
    }
    case AST_FUNCTION_DECLARATION: {
      HashTableEntry *entry = hash_table_get_entry(declaration_table->symbol_table, node->data.declaration_function.name);

      if (entry != NULL && entry->key != NULL) {
        DeclarationSymbol *existing_function_symbol = entry->value->structure;

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
          if (node->data.declaration_function.body_block != NULL)
          {
            add_function_parameter_to_symbol_table(&node->data.declaration_function.function_type->data.function_type.param_types[i], node->data.declaration_function.parameter_identifiers[i], node->data.declaration_function.name, declaration_table);
          }
        }

        if (!existing_function_symbol->data.function_symbol->is_defined && node->data.declaration_function.body_block != NULL) {
          existing_function_symbol->data.function_symbol->is_defined = true;
          function_and_variable_type_check(node->data.declaration_function.body_block, declaration_table, node, ast_arena);
        }

        break;
      }

      Types *param_types = malloc(sizeof(Types) * node->data.declaration_function.function_type->data.function_type.param_type_count);
      
      bool is_defined = node->data.declaration_function.body_block != NULL;
      bool is_global = (node->data.declaration_function.storage_class_type != AST_STORAGE_CLASS_STATIC || strcmp(node->data.declaration_function.name, "main") == 0);
        
      for (int i = 0; i < node->data.declaration_function.function_type->data.function_type.param_type_count; i++) {
        TypeNode *parameter_type = &node->data.declaration_function.function_type->data.function_type.param_types[i];

        param_types[i] = parameter_type->type;

        if (parameter_type->type == TYPE_VOID) {
          continue;
        }

        //We only want to add function param names for function definitions
        if (node->data.declaration_function.body_block != NULL)
        {
          add_function_parameter_to_symbol_table(parameter_type, node->data.declaration_function.parameter_identifiers[i], node->data.declaration_function.name, declaration_table);
        }
      }
      
      DeclarationSymbol *function_declaration_symbol = add_function_declaration_symbol(declaration_table, node->data.declaration_function.name, node->data.declaration_function.function_type->data.function_type.return_type->type, node->data.declaration_function.function_type->data.function_type.param_type_count, param_types, is_global, is_defined);

      if (node->data.declaration_function.body_block != NULL) {
        function_and_variable_type_check(node->data.declaration_function.body_block, declaration_table, node, ast_arena);
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
        expression_type_check(node, declaration_table, function_declaration_node, ast_arena);
      }

      for (int i = 0; i < node->data.expression_function_call.argument_count; i++) {
        AstNode *argument_node = node->data.expression_function_call.argument_ptrs->node_pointers[i];
        function_and_variable_type_check(argument_node, declaration_table, function_declaration_node, ast_arena);
      }
      break;
    }
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
      expression_type_check(node, declaration_table, function_declaration_node, ast_arena);
      break;
    case AST_BLOCK: {
      for (int i = 0; i < node->data.block.block_count; i++) {   
        AstNode *block_item_node = node->data.block.block_ptrs->node_pointers[i];
        function_and_variable_type_check(block_item_node, declaration_table, function_declaration_node, ast_arena);
      }
      break;
    }
    case AST_STATEMENT_IF: {
      function_and_variable_type_check(node->data.statement_if.condition_expression, declaration_table, function_declaration_node, ast_arena);
      function_and_variable_type_check(node->data.statement_if.then_statement, declaration_table, function_declaration_node, ast_arena);

      if (node->data.statement_if.else_statement != NULL) {
        function_and_variable_type_check(node->data.statement_if.else_statement, declaration_table, function_declaration_node, ast_arena);
      }
      break;
    }
    case AST_STATEMENT_RETURN: {
      Types return_expression_type = expression_type_check(node->data.statement_return.expression, declaration_table, function_declaration_node, ast_arena);
      Types function_return_type = function_declaration_node->data.declaration_function.function_type->data.type.function_return_type->data.type.type;

      if (function_return_type == return_expression_type) {
        break;
      }

      node->data.statement_return.expression = convert_to(node->data.statement_return.expression, return_expression_type, function_return_type, ast_arena);
      break;
    }
    case AST_STATEMENT_FOR: {
      if (node->data.statement_for.for_loop_init != NULL) {        
        function_and_variable_type_check(node->data.statement_for.for_loop_init, declaration_table, function_declaration_node, ast_arena);
      }

      if (node->data.statement_for.condition_expression != NULL) {
        function_and_variable_type_check(node->data.statement_for.condition_expression, declaration_table, function_declaration_node, ast_arena);
      }

      if (node->data.statement_for.post_expression != NULL) {
        function_and_variable_type_check(node->data.statement_for.post_expression, declaration_table, function_declaration_node, ast_arena);
      }

      function_and_variable_type_check(node->data.statement_for.statement_body, declaration_table, function_declaration_node, ast_arena);
      break;
    }
    case AST_STATEMENT_WHILE: {
      function_and_variable_type_check(node->data.statement_while.condition, declaration_table, function_declaration_node, ast_arena);
      function_and_variable_type_check(node->data.statement_while.statement_body, declaration_table, function_declaration_node, ast_arena);
      break;
    }
    case AST_STATEMENT_DO_WHILE: {
      function_and_variable_type_check(node->data.statement_do_while.condition, declaration_table, function_declaration_node, ast_arena);
      function_and_variable_type_check(node->data.statement_do_while.statement_body, declaration_table, function_declaration_node, ast_arena);
      break;
    }
    case AST_STATEMENT_COMPOUND:      
      function_and_variable_type_check(node->data.statement_compound.block, declaration_table, function_declaration_node, ast_arena);
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

static void type_check_file_scope_variable_declaration(AstNode *variable_declaration_node, DeclarationSymbolTable *declaration_table) {
  InitialValueType initial_value_type; 
  InitialValue initial_value;

  if (variable_declaration_node->data.declaration_variable.has_expression && variable_declaration_node->data.declaration_variable.init_expression->data.expression_assignment.right_expression->type == AST_EXPRESSION_CONSTANT) {
    initial_value_type = INITIAL_VALUE_INITIALIZED;

    switch (variable_declaration_node->data.declaration_variable.type->data.type.type) {
      case TYPE_INT:     initial_value.int_value = convert_variable_declaration_constant_to_int(variable_declaration_node); break;
      case TYPE_LONG:    initial_value.long_value = convert_variable_declaration_constant_to_long(variable_declaration_node); break;
      case TYPE_UINT:    initial_value.uint_value = convert_variable_declaration_constant_to_uint(variable_declaration_node); break;
      case TYPE_ULONG:   initial_value.ulong_value = convert_variable_declaration_constant_to_ulong(variable_declaration_node); break;
      case TYPE_DOUBLE:  initial_value.double_value = convert_variable_declaration_constant_to_double(variable_declaration_node); break;
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
  } else {
    fprintf(stderr, "ERROR: SA Type Check: Non-constant initializer\n");
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

    if (variable_declaration_node->data.declaration_variable.type->data.type.type != existing_variable_symbol->data.variable_symbol->value_type) {
      fprintf(stderr, "ERROR: SA Type Check: Previously declared '%s' variable has type of '%s'\n", variable_declaration_node->data.declaration_variable.name, get_type_string(existing_variable_symbol->data.variable_symbol->value_type));
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

  add_static_variable_declaration_symbol(declaration_table, variable_declaration_node->data.declaration_variable.type->data.type.type, initial_value, variable_declaration_node->data.declaration_variable.name, is_global, initial_value_type);  
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
      add_static_extern_variable_declaration_symbol(declaration_table, variable_declaration_node->data.declaration_variable.type->data.type.type, variable_declaration_node->data.declaration_variable.name); 
    }
    
    return;
  }

  if (variable_declaration_node->data.declaration_variable.storage_class_type == AST_STORAGE_CLASS_STATIC) {
    InitialValue initial_value;
    
    if (!variable_declaration_node->data.declaration_variable.has_expression) {
      switch (variable_declaration_node->data.declaration_variable.type->data.type.type) {
        case TYPE_INT:     initial_value.int_value = 0; break;
        case TYPE_UINT:    initial_value.uint_value = 0; break;
        case TYPE_LONG:    initial_value.long_value = 0; break;
        case TYPE_ULONG:   initial_value.ulong_value = 0; break;
        case TYPE_DOUBLE:  initial_value.double_value = 0; break;
        default:
          fprintf(stderr, "ERROR - SA Type Check: Unsupported initial value AST Type '%d'\n", variable_declaration_node->data.declaration_variable.type->data.type.type);
          exit(1);
      }
    } else if (variable_declaration_node->data.declaration_variable.init_expression->data.expression_assignment.right_expression->type == AST_EXPRESSION_CONSTANT) {

      Types constant_expression_type = variable_declaration_node->data.declaration_variable.init_expression->data.expression_assignment.right_expression->data.expression_constant.expression_type->data.type.type;

      switch(variable_declaration_node->data.declaration_variable.type->data.type.type) {
        case TYPE_INT:     initial_value.int_value = convert_variable_declaration_constant_to_int(variable_declaration_node); break;
        case TYPE_LONG:    initial_value.long_value = convert_variable_declaration_constant_to_long(variable_declaration_node); break;
        case TYPE_UINT:    initial_value.uint_value = convert_variable_declaration_constant_to_uint(variable_declaration_node); break;
        case TYPE_ULONG:   initial_value.ulong_value = convert_variable_declaration_constant_to_ulong(variable_declaration_node); break;
        case TYPE_DOUBLE:  initial_value.double_value = convert_variable_declaration_constant_to_double(variable_declaration_node); break;
        default:
          fprintf(stderr, "ERROR - SA Type Check: Unsupported initial value AST Type '%d'\n",variable_declaration_node->data.declaration_variable.type->data.type.type);
          exit(1);
      }
    } else {
      fprintf(stderr, "ERROR - SA Type Check: Non-constant initializer on local static variable '%s'\n", variable_declaration_node->data.declaration_variable.name);
      exit(1);
    }

    add_static_variable_declaration_symbol(declaration_table, variable_declaration_node->data.declaration_variable.type->data.type.type, initial_value, variable_declaration_node->data.declaration_variable.name, false, INITIAL_VALUE_INITIALIZED);
    
    return;
  }   

  add_automatic_variable_declaration_symbol(declaration_table, variable_declaration_node->data.declaration_variable.type->data.type.type, variable_declaration_node->data.declaration_variable.name);
} 

static Types expression_type_check(AstNode *node, DeclarationSymbolTable *declaration_table, AstNode *function_declaration_node, Arena *ast_arena) {
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

      AstNode *ast_expression_type = arena_alloc(ast_arena);
      ast_expression_type->type = AST_TYPE;
      ast_expression_type->data.type.type = symbol->data.variable_symbol->value_type;       

      node->data.expression_variable.expression_type = ast_expression_type;

      return ast_expression_type->data.type.type;
    }
    case AST_EXPRESSION_CONSTANT: {
      AstNode *ast_expression_type = arena_alloc(ast_arena);
      ast_expression_type->type = AST_TYPE;
      
      switch (node->data.expression_constant.constant_type) {
        case AST_CONSTANT_TYPE_INT:    ast_expression_type->data.type.type = TYPE_INT; break;
        case AST_CONSTANT_TYPE_LONG:   ast_expression_type->data.type.type = TYPE_LONG; break;
        case AST_CONSTANT_TYPE_UINT:   ast_expression_type->data.type.type = TYPE_UINT; break;
        case AST_CONSTANT_TYPE_ULONG:  ast_expression_type->data.type.type = TYPE_ULONG; break;
        case AST_CONSTANT_TYPE_DOUBLE: ast_expression_type->data.type.type = TYPE_DOUBLE; break;
        default:
          fprintf(stderr, "ERROR - Type Check: Could not resolve value type in variable symbol\n");
          exit(1);
      }

      node->data.expression_constant.expression_type = ast_expression_type;

      return ast_expression_type->data.type.type;
    }
    case AST_EXPRESSION_CAST: {
      //@Bug: I think this is not right. Use the following as an example: long gg = (long)5;. Expression type returned is int
      Types expression_type = expression_type_check(node->data.expression_cast.expression, declaration_table, function_declaration_node, ast_arena);

      AstNode *ast_expression_type_node = arena_alloc(ast_arena);
      ast_expression_type_node->type = AST_TYPE;
      ast_expression_type_node->data.type.type = expression_type;

      node->data.expression_cast.expression_type = ast_expression_type_node;

      return expression_type;
    }
    case AST_EXPRESSION_UNARY: {
      Types expression_type = expression_type_check(node->data.expression_unary.expression, declaration_table, function_declaration_node, ast_arena);

      if (node->data.expression_unary.op_type == AST_UNARY_COMPLEMENT && expression_type == TYPE_DOUBLE) {
        fprintf(stderr, "ERROR - SA Type Check: Cannot apply unary complement operator to a double\n");
        exit(1);
      }

      AstNode *ast_expression_type_node = arena_alloc(ast_arena);
      ast_expression_type_node->type = AST_TYPE;
      ast_expression_type_node->data.type.type = expression_type;

      node->data.expression_unary.expression_type = ast_expression_type_node;

      if (node->data.expression_unary.op_type == AST_UNARY_NOT) {
        return TYPE_INT;
      }

      return expression_type;
    }
    case AST_EXPRESSION_BINARY: {
      Types left_expression_type = expression_type_check(node->data.expression_binary.left_expression, declaration_table, function_declaration_node, ast_arena);
      Types right_expression_type = expression_type_check(node->data.expression_binary.right_expression, declaration_table, function_declaration_node, ast_arena);

      if (right_expression_type == TYPE_DOUBLE || left_expression_type == TYPE_DOUBLE) {
        if (node->data.expression_binary.op_type == AST_BINARY_BITWISE_OR) {
          fprintf(stderr, "ERROR - SA Type Check: Cannot apply binary bitwise OR operator with a double value\n");
          exit(1);
        } 

        if (node->data.expression_binary.op_type == AST_BINARY_BITWISE_LEFT_SHIFT) {
          fprintf(stderr, "ERROR - SA Type Check: Cannot apply binary bitwise left shift operator with a double value\n");
          exit(1);
        }
        
        if (node->data.expression_binary.op_type == AST_BINARY_BITWISE_RIGHT_SHIFT) {
          fprintf(stderr, "ERROR - SA Type Check: Cannot apply binary bitwise right shift operator with a double value\n");
          exit(1);
        }

        if (node->data.expression_binary.op_type == AST_BINARY_BITWISE_XOR) {
          fprintf(stderr, "ERROR - SA Type Check: Cannot apply binary bitwise XOR operator with a double value\n");
          exit(1);
        } 

        if (node->data.expression_binary.op_type == AST_BINARY_BITWISE_AND) {
          fprintf(stderr, "ERROR - SA Type Check: Cannot apply binary bitwise AND operator with a double value\n");
          exit(1);
        } 
        
        if (node->data.expression_binary.op_type == AST_BINARY_REMAINDER) {
          fprintf(stderr, "ERROR - SA Type Check: Cannot apply Modulo operator with a double value\n");
          exit(1);
        } 
      }

      if (node->data.expression_binary.op_type == AST_BINARY_AND || node->data.expression_binary.op_type == AST_BINARY_OR) {
        AstNode *ast_expression_type_node = arena_alloc(ast_arena);
        ast_expression_type_node->type = AST_TYPE;
        ast_expression_type_node->data.type.type = TYPE_INT;

        node->data.expression_binary.expression_type = ast_expression_type_node;
        return TYPE_INT;
      }

      Types common_type;
      
      if (left_expression_type == TYPE_POINTER || right_expression_type == TYPE_POINTER) {
        common_type = get_common_pointer_type(node->data.expression_binary.left_expression, node->data.expression_binary.right_expression, declaration_table, function_declaration_node, ast_arena);
      } else {
        common_type = get_common_real_type(left_expression_type, right_expression_type);
      }

      node->data.expression_binary.left_expression = convert_to(node->data.expression_binary.left_expression, left_expression_type, common_type, ast_arena);
      node->data.expression_binary.right_expression = convert_to(node->data.expression_binary.right_expression, right_expression_type, common_type, ast_arena);
      
      AstNode *ast_expression_type_node = arena_alloc(ast_arena);
      ast_expression_type_node->type = AST_TYPE;
      ast_expression_type_node->data.type.type = common_type;

      node->data.expression_binary.expression_type = ast_expression_type_node;
      
      switch (node->data.expression_binary.op_type) {
        case AST_BINARY_ADD:
        case AST_BINARY_SUBTRACT:
        case AST_BINARY_MULTIPLY:
        case AST_BINARY_DIVIDE:
        case AST_BINARY_REMAINDER:
          return common_type;
        default:
          return TYPE_INT;
      }
    }
    case AST_EXPRESSION_ASSIGNMENT: {
      Types left_expression_type = expression_type_check(node->data.expression_assignment.left_expression, declaration_table, function_declaration_node, ast_arena);
      Types right_expression_type = expression_type_check(node->data.expression_assignment.right_expression, declaration_table, function_declaration_node, ast_arena);

      // //TODO: Need to look into this. I don't think it's working correctly
      // node->data.expression_assignment.right_expression = convert_to(node->data.expression_assignment.right_expression, right_expression_type, left_expression_type, ast_arena);      
      node->data.expression_assignment.right_expression = convert_by_assignment(node->data.expression_assignment.right_expression, right_expression_type, left_expression_type, ast_arena);

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
        function_and_variable_type_check(argument_node, declaration_table, function_declaration_node, ast_arena);
      }
    
      AstNode *ast_expression_type_node = arena_alloc(ast_arena);
      ast_expression_type_node->type = AST_TYPE;
      ast_expression_type_node->data.type.type = existing_symbol->data.function_symbol->value_type;

      node->data.expression_function_call.expression_type = ast_expression_type_node;

      return ast_expression_type_node->data.type.type;
    }
    case AST_EXPRESSION_CONDITIONAL: {
      expression_type_check(node->data.expression_conditional.condition, declaration_table, function_declaration_node, ast_arena);

      Types true_expression_type = expression_type_check(node->data.expression_conditional.true_expression, declaration_table, function_declaration_node, ast_arena);
      Types false_expression_type = expression_type_check(node->data.expression_conditional.false_expression, declaration_table, function_declaration_node, ast_arena);
      
      Types common_type;

      if (true_expression_type == TYPE_POINTER || false_expression_type == TYPE_POINTER) {
        common_type = get_common_pointer_type(node->data.expression_conditional.true_expression, node->data.expression_conditional.false_expression, declaration_table, function_declaration_node, ast_arena);
      } else {
        common_type = get_common_real_type(true_expression_type, false_expression_type);
      }

      node->data.expression_conditional.true_expression = convert_to(node->data.expression_conditional.true_expression, true_expression_type, common_type, ast_arena);
      node->data.expression_conditional.false_expression = convert_to(node->data.expression_conditional.false_expression, false_expression_type, common_type, ast_arena);
      return common_type;
    }
    case AST_EXPRESSION_PREFIX_INCREMENT:
    case AST_EXPRESSION_POSTFIX_INCREMENT:
    case AST_EXPRESSION_PREFIX_DECREMENT:
    case AST_EXPRESSION_POSTFIX_DECREMENT: {
      return expression_type_check(node->data.expression_increment_decrement.expression, declaration_table, function_declaration_node, ast_arena);
      break;
    }
    case AST_EXPRESSION_ADDRESS_OF: {
      /*
        LValue = Expressions that can appear on the left side of an assignment
        Section 6.3.2.1, paragraph 1, of the C standard - "An lvalue is an expression...that potentially designates an object:
          - Variables
          - Dereference 
      */

      if (node->data.expression_address_of.expression->type != AST_EXPRESSION_VARIABLE && node->data.expression_address_of.expression->type != AST_EXPRESSION_DEREFERENCE) {
        fprintf(stderr, "ERROR - SA Type Check: Cannot take the address of a non-lvalue\n");
        exit(1);
      }

      expression_type_check(node->data.expression_address_of.expression, declaration_table, function_declaration_node, ast_arena);
      return TYPE_POINTER;
    }
    case AST_EXPRESSION_DEREFERENCE: {
      Types expression_type = expression_type_check(node->data.expression_dereference.expression, declaration_table, function_declaration_node, ast_arena);

      if (expression_type != TYPE_POINTER) {
        fprintf(stderr, "ERROR - SA Type Check: Cannot dereference a non-pointer\n");
        exit(1);
      }

      //TODO: Returning the Type in this function may be incorrect. There is no Dereference Type, so maybe I need to return the AST Type node..
      return expression_type;
    }
    default:
      fprintf(stderr, "ERROR - SA Type Check: Invalid AST type '%d' found in expression type check\n", node->type);
      exit(1);
  }
}

static Types get_common_real_type(Types type_1, Types type_2) {
  if (type_1 == type_2) {
    return type_1;
  }  

  if (type_1 == TYPE_DOUBLE || type_2 == TYPE_DOUBLE) {
    return TYPE_DOUBLE;
  }

  if (get_type_size(type_1) == get_type_size(type_2)) {
    if (type_1 == TYPE_INT || type_1 == TYPE_LONG) {
      return type_2;
    }

    return type_1;
  }  

  if (get_type_size(type_1) > get_type_size(type_2)) {
    return type_1;
  } 

  return type_2;
}

static Types get_common_pointer_type(AstNode *expression_1, AstNode *expression_2, DeclarationSymbolTable *declaration_table, AstNode *function_declaration_node, Arena *ast_arena) {
  Types expression_1_type = expression_type_check(expression_1, declaration_table, function_declaration_node, ast_arena); 
  Types expression_2_type = expression_type_check(expression_2, declaration_table, function_declaration_node, ast_arena); 

  if (expression_1_type == expression_2_type) {
    return expression_1_type;
  }

  if (is_null_pointer_constant(expression_1)) {
    return expression_2_type;
  }

  if (is_null_pointer_constant(expression_2)) {
    return expression_1_type;
  }

  fprintf(stderr, "ERROR - SA Type Check: Common pointer expressions have incompatible types");
  exit(1);
}

static AstNode* convert_to(AstNode *expression, Types expression_type, Types target_type, Arena *ast_arena) {
  if (expression_type == target_type) {
    return expression;
  }

  AstNode *type_node = arena_alloc(ast_arena);
  type_node->type = AST_TYPE;
  type_node->data.type.type = target_type;

  AstNode *casted_expression = arena_alloc(ast_arena);
  casted_expression->type = AST_EXPRESSION_CAST;
  casted_expression->data.expression_cast.target_type = type_node;
  casted_expression->data.expression_cast.expression = expression;
  
  AstNode *cast_expression_type = NULL;

  switch (expression->type) {
    case AST_EXPRESSION_CONSTANT:      cast_expression_type = expression->data.expression_constant.expression_type; break;
    case AST_EXPRESSION_VARIABLE:      cast_expression_type = expression->data.expression_variable.expression_type; break;
    case AST_EXPRESSION_CAST:          cast_expression_type = expression->data.expression_cast.expression_type; break;
    case AST_EXPRESSION_UNARY:         cast_expression_type = expression->data.expression_unary.expression_type; break;
    case AST_EXPRESSION_BINARY:        cast_expression_type = expression->data.expression_binary.expression_type; break;
    case AST_EXPRESSION_ASSIGNMENT:    cast_expression_type = expression->data.expression_assignment.expression_type; break;
    case AST_EXPRESSION_CONDITIONAL:   cast_expression_type = expression->data.expression_conditional.expression_type; break;
    case AST_EXPRESSION_FUNCTION_CALL: cast_expression_type = expression->data.expression_variable.expression_type; break;
    default:
      fprintf(stderr, "ERROR - Type Check: Unsupported cast expression type '%d'\n", expression->type);
      exit(1);
  }

  casted_expression->data.expression_cast.expression_type = cast_expression_type;
  
  return casted_expression;
}

static void add_function_parameter_to_symbol_table(AstNode *parameter_type, char *parameter_identifier, char *function_name, DeclarationSymbolTable *declaration_table) {
  if (parameter_type->data.type.type == TYPE_VOID) {
    return;
  }

  char *symbol_key = malloc(IDENTIFIER_BUFFER); 
  snprintf(symbol_key, IDENTIFIER_BUFFER, "%s", parameter_identifier);

  add_automatic_variable_declaration_symbol(declaration_table, parameter_type->data.type.type, symbol_key);
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

static AstNode* convert_by_assignment(AstNode *right_assignment_expression, Types right_assignment_type, Types target_type, Arena *ast_arena) {
  if (right_assignment_type == target_type) {
    return right_assignment_expression;
  }

  //arithmetic types
  if (right_assignment_type == TYPE_DOUBLE || right_assignment_type == TYPE_INT || right_assignment_type == TYPE_UINT || right_assignment_type == TYPE_LONG || right_assignment_type == TYPE_ULONG) {
    return convert_to(right_assignment_expression, right_assignment_type, target_type, ast_arena);
  }

  if (is_null_pointer_constant(right_assignment_expression) && target_type == TYPE_POINTER) {
    return convert_to(right_assignment_expression, right_assignment_type, target_type, ast_arena);
  }

  fprintf(stderr, "ERROR - Type Check: Cannot convert type for assignment expression\n");
  exit(1);
}
