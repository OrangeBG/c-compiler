#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include "../include/sa_type_check.h"
#include "arena.h"
#include "parser.h"

//TODO: Check to see how we can better optimize these types of buffers. Exact same use of this buffer is in sa_variable_resolution
#define IDENTIFIER_BUFFER 256

static void  function_and_variable_type_check(AstNode *node, HashTable *symbols, char *function_name, Arena *ast_arena);
static void  type_check_file_scope_variable_declaration(AstNode *variable_declaration_node, HashTable *symbols, char *function_name); 
static void  type_check_block_scope_variable_declaration(AstNode *variable_declaration_node, HashTable *symbols, char *function_name); 
static Types expression_type_check(AstNode *node, HashTable *symbols, char *function_name, Arena *ast_arena); 

void sa_type_check(AstNode *ast_nodes, HashTable *declaration_symbols, Arena *ast_arena) {
  for (int i = 0; i < ast_nodes->data.program.declaration_count; i++) {
    AstNode *node = ast_nodes->data.program.declaration_ptrs->node_pointers[i];

    if (node->type == AST_FUNCTION_DECLARATION) {
      function_and_variable_type_check(node, declaration_symbols, node->data.function_declaration.name, ast_arena);
      continue;
    } 

    if (node->type == AST_VARIABLE_DECLARATION) {
      function_and_variable_type_check(node, declaration_symbols, NULL, ast_arena);
      continue;
    }

    fprintf(stderr, "ERROR - SA Type Check: Unexpected declaration type");
    exit(1);
  } 
}

static void function_and_variable_type_check(AstNode *node, HashTable *symbols, char *function_name, Arena *ast_arena) {
  switch (node->type) {
    case AST_VARIABLE_DECLARATION: {
      if (function_name == NULL) {
        type_check_file_scope_variable_declaration(node, symbols, function_name);
      } else {
        type_check_block_scope_variable_declaration(node, symbols, function_name);
      }

      if (node->data.variable_declaration.has_expression) {
        function_and_variable_type_check(node->data.variable_declaration.init_expression, symbols, function_name, ast_arena);
      }
      break;
    }
    case AST_FUNCTION_DECLARATION: {
      HashTableEntry *entry = hash_table_get_entry(symbols, node->data.function_declaration.name);

      if (entry != NULL && entry->key != NULL) {
        TypeCheckSymbol *existing_function_symbol = entry->value->structure;

        if (existing_function_symbol->data.function_symbol->value_type != TYPE_INT) {
          fprintf(stderr, "ERROR - SA Type Check: Incompatible function declarations for '%s\n'", entry->key);
          exit(1);
        }

        // is_defined = existing_function_symbol->data.function_symbol->defined;

        if (existing_function_symbol->data.function_symbol->defined && node->data.function_declaration.body_block != NULL) {
          fprintf(stderr, "ERROR - SA Type Check: Function defined more than once '%s'\n", entry->key);
          exit(1);
        }

        if (existing_function_symbol->data.function_symbol->global == node->data.function_declaration.storage_class_type == AST_STORAGE_CLASS_STATIC) {
          fprintf(stderr, "ERROR - SA Type Check: Static function '%s' declaration follows non-static\n", node->data.function_declaration.name);
          exit(1);
        }

        if (!existing_function_symbol->data.function_symbol->defined) {
          existing_function_symbol->data.function_symbol->defined = node->data.function_declaration.body_block != NULL;
        }

        break;
      }

      FunctionSymbol *function_symbol = malloc(sizeof(FunctionSymbol));
      function_symbol->value_type = TYPE_INT;
      function_symbol->param_count = 0;
      function_symbol->defined = node->data.function_declaration.body_block != NULL;
      function_symbol->global = (node->data.function_declaration.storage_class_type != AST_STORAGE_CLASS_STATIC || strcmp(node->data.function_declaration.name, "main") == 0);

      TypeCheckSymbol *new_symbol = malloc(sizeof(TypeCheckSymbol));
      new_symbol->symbol_type = SYMBOL_FUNCTION;
      new_symbol->data.function_symbol = function_symbol;

      HashValue *new_value = malloc(sizeof(HashValue));
      new_value->type = HASH_STRUCT;
      new_value->structure = new_symbol;

      HashTableEntry *new_entry = malloc(sizeof(HashTableEntry));
      new_entry->key = node->data.function_declaration.name;
      new_entry->value = new_value;

      hash_table_add_entry(symbols, new_entry);
        
      for (int i = 0; i < node->data.function_declaration.parameter_count; i++) {
        AstNode *parameter_node = node->data.function_declaration.parameter_ptrs->node_pointers[i];

        if (parameter_node->data.function_parameters.type == AST_PARAMETER_VOID) {
          continue;
        }

        function_symbol->param_count++;
        function_and_variable_type_check(parameter_node, symbols, function_name, ast_arena);
      }

      if (node->data.function_declaration.body_block != NULL) {
        function_and_variable_type_check(node->data.function_declaration.body_block, symbols, function_name, ast_arena);
      }
      break;
    }
    case AST_FUNCTION_PARAMETER: {
      if (node->data.function_parameters.type == AST_PARAMETER_VOID) {
        break;
      }
      
      char *identifier = node->data.function_parameters.name;

      //This pass happens after variable resolution, so no need to check to see if the variable is duplicated in the hash table
      TypeCheckSymbol *symbol = malloc(sizeof(TypeCheckSymbol));
      symbol->symbol_type = SYMBOL_VARIABLE;

      VariableSymbol *variable_symbol = malloc(sizeof(VariableSymbol));
      variable_symbol->value_type = TYPE_INT;
      variable_symbol->is_automatic_storage_duration = true;

      symbol->data.variable_symbol = variable_symbol;

      HashTableEntry *entry = malloc(IDENTIFIER_BUFFER);

      char *symbol_key = malloc(IDENTIFIER_BUFFER); 
      snprintf(symbol_key, IDENTIFIER_BUFFER, "%s.%s", function_name,  identifier);
      entry->key = symbol_key;

      HashValue *value = malloc(sizeof(HashValue));
      value->type = HASH_STRUCT;
      value->structure = symbol;

      entry->value = value;

      hash_table_add_entry(symbols, entry); 
      break;
    }
    case AST_EXPRESSION_FUNCTION_CALL: {
      HashTableEntry *entry = hash_table_get_entry(symbols, node->data.function_call_expression.identfier);

      if (entry != NULL && entry->key != NULL) {
        TypeCheckSymbol *existing_symbol = entry->value->structure;

        if (existing_symbol->symbol_type == SYMBOL_VARIABLE) {
          fprintf(stderr, "ERROR - SA Type Check: Variable '%s' is used as a function name\n", node->data.function_call_expression.identfier);
          exit(1);
        }               

        if (existing_symbol->data.function_symbol->param_count != node->data.function_call_expression.argument_count) {
          fprintf(stderr, "ERROR - SA Type Check: Function '%s' called with incorrect number of arguments\n", node->data.function_call_expression.identfier);
          exit(1);
        }
      }

      for (int i = 0; i < node->data.function_call_expression.argument_count; i++) {
        AstNode *argument_node = node->data.function_call_expression.argument_ptrs->node_pointers[i];
        function_and_variable_type_check(argument_node, symbols, function_name, ast_arena);
      }
      break;
    }
    case AST_EXPRESSION_VARIABLE:
    case AST_EXPRESSION_CONSTANT:
    case AST_EXPRESSION_CAST: 
    case AST_EXPRESSION_UNARY:
      expression_type_check(node, symbols, function_name, ast_arena);
      break;
    case AST_BLOCK: {
      for (int i = 0; i < node->data.block.block_count; i++) {   
        AstNode *block_item_node = node->data.block.block_ptrs->node_pointers[i];
        function_and_variable_type_check(block_item_node, symbols, function_name, ast_arena);
      }
      break;
    }
    case AST_STATEMENT_IF: {
      function_and_variable_type_check(node->data.if_statement.condition_expression, symbols, function_name, ast_arena);
      function_and_variable_type_check(node->data.if_statement.then_statement, symbols, function_name, ast_arena);

      if (node->data.if_statement.else_statement != NULL) {
        function_and_variable_type_check(node->data.if_statement.else_statement, symbols, function_name, ast_arena);
      }
      break;
    }
    case AST_STATEMENT_RETURN: {
      function_and_variable_type_check(node->data.return_statement.expression, symbols, function_name, ast_arena);
      break;
    }
    case AST_STATEMENT_FOR: {
      if (node->data.for_statement.for_loop_init != NULL) {        
        function_and_variable_type_check(node->data.for_statement.for_loop_init, symbols, function_name, ast_arena);
      }

      if (node->data.for_statement.condition_expression != NULL) {
        function_and_variable_type_check(node->data.for_statement.condition_expression, symbols, function_name, ast_arena);
      }

      if (node->data.for_statement.post_expression != NULL) {
        function_and_variable_type_check(node->data.for_statement.post_expression, symbols, function_name, ast_arena);
      }

      function_and_variable_type_check(node->data.for_statement.statement_body, symbols, function_name, ast_arena);
      break;
    }
    case AST_STATEMENT_WHILE: {
      function_and_variable_type_check(node->data.while_statement.condition, symbols, function_name, ast_arena);
      function_and_variable_type_check(node->data.while_statement.statement_body, symbols, function_name, ast_arena);
      break;
    }
    case AST_STATEMENT_DO_WHILE: {
      function_and_variable_type_check(node->data.do_while_statement.condition, symbols, function_name, ast_arena);
      function_and_variable_type_check(node->data.do_while_statement.statement_body, symbols, function_name, ast_arena);
      break;
    }
    case AST_EXPRESSION_ASSIGNMENT: {
      function_and_variable_type_check(node->data.assignement_expression.left_expression, symbols, function_name, ast_arena);
      function_and_variable_type_check(node->data.assignement_expression.right_expression, symbols, function_name, ast_arena);
      break;
    }
    case AST_EXPRESSION_BINARY: {
      function_and_variable_type_check(node->data.binary_expression.left_expression, symbols, function_name, ast_arena);
      function_and_variable_type_check(node->data.binary_expression.right_expression, symbols, function_name, ast_arena);
      break;
    }
    case AST_EXPRESSION_POSTFIX_INCREMENT:
    case AST_EXPRESSION_POSTFIX_DECREMENT:
    case AST_EXPRESSION_PREFIX_INCREMENT:
    case AST_EXPRESSION_PREFIX_DECREMENT: 
      function_and_variable_type_check(node->data.increment_decrement_expression.expression, symbols, function_name, ast_arena);
      break;
  }  
}

static void type_check_file_scope_variable_declaration(AstNode *variable_declaration_node, HashTable *symbols, char *function_name) {
  InitialValueType initial_value_type; 
  int initial_value = 0;

  if (variable_declaration_node->data.variable_declaration.has_expression && variable_declaration_node->data.variable_declaration.init_expression->data.assignement_expression.right_expression->type == AST_EXPRESSION_CONSTANT) {
    initial_value_type = INITIAL_VALUE_INITIALIZED;
    //TODO: Will need to support long constants
    initial_value = variable_declaration_node->data.variable_declaration.init_expression->data.assignement_expression.right_expression->data.constant_expression.int_value;
  } else if (!variable_declaration_node->data.variable_declaration.has_expression) {
    if (variable_declaration_node->data.variable_declaration.storage_class_type == AST_STORAGE_CLASS_EXTERN) {
      initial_value_type = INITIAL_VALUE_NO_INITIALIZER;
    } else {
      initial_value_type = INITIAL_VALUE_TENTATIVE;
    }
  } else {
    fprintf(stderr, "ERROR: SA Type Check: Non-constant initializer\n");
    exit(1);
  }

  bool is_global = variable_declaration_node->data.variable_declaration.storage_class_type != AST_STORAGE_CLASS_STATIC;

  HashTableEntry *entry = hash_table_get_entry(symbols, variable_declaration_node->data.variable_declaration.name);

  if (entry != NULL && entry->key != NULL) {
    TypeCheckSymbol *existing_variable_symbol = entry->value->structure;

    if (existing_variable_symbol->data.variable_symbol->value_type != TYPE_INT) {
      fprintf(stderr, "ERROR: SA Type Check: Function '%s' redeclared as variable\n", variable_declaration_node->data.variable_declaration.name);
      exit(1);
    }

    if (variable_declaration_node->data.variable_declaration.storage_class_type == AST_STORAGE_CLASS_EXTERN) {
      existing_variable_symbol->data.variable_symbol->static_storage_duration->is_global = true;
    }
    else if (existing_variable_symbol->data.variable_symbol->static_storage_duration->is_global != is_global) {
      fprintf(stderr, "ERROR: SA Type Check: Function '%s' conflicting variable linkage\n", variable_declaration_node->data.variable_declaration.name);
      exit(1);
    }

    if (existing_variable_symbol->data.variable_symbol->static_storage_duration->initial_type == INITIAL_VALUE_INITIALIZED) {
      if (initial_value_type == INITIAL_VALUE_INITIALIZED) {
        fprintf(stderr, "ERROR: SA Type Check: Function '%s' conflicting file scope variable definitions\n", variable_declaration_node->data.variable_declaration.name);
        exit(1);
      }
    } else {
      existing_variable_symbol->data.variable_symbol->static_storage_duration->initial_type = initial_value_type;
      existing_variable_symbol->data.variable_symbol->static_storage_duration->initial_value = initial_value;
    }

    return;
  }

  TypeCheckSymbol *variable_symbol = malloc(sizeof(TypeCheckSymbol));
  variable_symbol->symbol_type = SYMBOL_VARIABLE;

  VariableSymbol *symbol = malloc(sizeof(VariableSymbol));
  symbol->is_automatic_storage_duration = false;
  symbol->value_type = TYPE_INT;

  variable_symbol->data.variable_symbol = symbol;

  StaticStorageDuration *attribute = malloc(sizeof(StaticStorageDuration));
  attribute->is_global = variable_declaration_node->data.variable_declaration.storage_class_type != AST_STORAGE_CLASS_STATIC;
  attribute->initial_type = initial_value_type;
  attribute->initial_value = initial_value;

  symbol->static_storage_duration = attribute;

  HashValue *new_value = malloc(sizeof(HashValue));
  new_value->type = HASH_STRUCT;
  new_value->structure = variable_symbol;

  HashTableEntry *new_entry = malloc(sizeof(HashTableEntry));
  new_entry->key = variable_declaration_node->data.variable_declaration.name;
  new_entry->value = new_value;

  hash_table_add_entry(symbols, new_entry);
}

static void type_check_block_scope_variable_declaration(AstNode *variable_declaration_node, HashTable *symbols, char *function_name) {
  if (variable_declaration_node->data.variable_declaration.storage_class_type == AST_STORAGE_CLASS_EXTERN) {
    if (variable_declaration_node->data.variable_declaration.has_expression) {
      fprintf(stderr, "ERROR - SA Type Check: Initializer on local extern variable declaration '%s'\n", variable_declaration_node->data.variable_declaration.name);
      exit(1);
    }
    
    HashTableEntry *entry = hash_table_get_entry(symbols, variable_declaration_node->data.variable_declaration.name);

    if (entry != NULL && entry->key != NULL) {
      TypeCheckSymbol *existing_variable_symbol = entry->value->structure;

      if (existing_variable_symbol->symbol_type == SYMBOL_FUNCTION) {        
        fprintf(stderr, "ERROR - SA Type Check: Function redeclared as variable");
        exit(1);
      }
    } else {
      TypeCheckSymbol *variable_symbol = malloc(sizeof(TypeCheckSymbol));
      variable_symbol->symbol_type = SYMBOL_VARIABLE;

      VariableSymbol *symbol = malloc(sizeof(VariableSymbol));
      symbol->is_automatic_storage_duration = false;
      symbol->value_type = TYPE_INT;

      variable_symbol->data.variable_symbol = symbol;

      StaticStorageDuration *attribute = malloc(sizeof(StaticStorageDuration));
      attribute->is_global = true;
      attribute->initial_type = INITIAL_VALUE_NO_INITIALIZER;

      symbol->static_storage_duration = attribute;

      HashValue *new_value = malloc(sizeof(HashValue));
      new_value->type = HASH_STRUCT;
      new_value->structure = variable_symbol;

      HashTableEntry *new_entry = malloc(sizeof(HashTableEntry));
      new_entry->key = variable_declaration_node->data.variable_declaration.name;
      new_entry->value = new_value;

      hash_table_add_entry(symbols, new_entry);
    }
    
    return;
  }

  if (variable_declaration_node->data.variable_declaration.storage_class_type == AST_STORAGE_CLASS_STATIC) {
    int initial_value; 
    if (!variable_declaration_node->data.variable_declaration.has_expression) {
      initial_value = 0;
    } else if (variable_declaration_node->data.variable_declaration.init_expression->data.assignement_expression.right_expression->type == AST_EXPRESSION_CONSTANT) {
      //TODO: Will need to support long constants
      initial_value = variable_declaration_node->data.variable_declaration.init_expression->data.assignement_expression.right_expression->data.constant_expression.int_value;
    } else {
      fprintf(stderr, "ERROR - SA Type Check: Non-constance initializer on local staic variable '%s'\n", variable_declaration_node->data.variable_declaration.name);
      exit(1);
    }

    TypeCheckSymbol *variable_symbol = malloc(sizeof(TypeCheckSymbol));
    variable_symbol->symbol_type = SYMBOL_VARIABLE;

    VariableSymbol *symbol = malloc(sizeof(VariableSymbol));
    symbol->is_automatic_storage_duration = false;
    symbol->value_type = TYPE_INT;

    variable_symbol->data.variable_symbol = symbol;

    StaticStorageDuration *attribute = malloc(sizeof(StaticStorageDuration));
    attribute->is_global = false;
    attribute->initial_type = INITIAL_VALUE_INITIALIZED;
    attribute->initial_value = initial_value;

    symbol->static_storage_duration = attribute;

    HashValue *new_value = malloc(sizeof(HashValue));
    new_value->type = HASH_STRUCT;
    new_value->structure = variable_symbol;

    HashTableEntry *new_entry = malloc(sizeof(HashTableEntry));
    new_entry->key = variable_declaration_node->data.variable_declaration.name;
    new_entry->value = new_value;

    hash_table_add_entry(symbols, new_entry);
    
    return;
  }   

  
  TypeCheckSymbol *variable_symbol = malloc(sizeof(TypeCheckSymbol));
  variable_symbol->symbol_type = SYMBOL_VARIABLE;

  VariableSymbol *symbol = malloc(sizeof(VariableSymbol));
  symbol->is_automatic_storage_duration = true;
  symbol->value_type = TYPE_INT;

  variable_symbol->data.variable_symbol = symbol;

  HashValue *new_value = malloc(sizeof(HashValue));
  new_value->type = HASH_STRUCT;
  new_value->structure = variable_symbol;

  HashTableEntry *new_entry = malloc(sizeof(HashTableEntry));
  new_entry->key = variable_declaration_node->data.variable_declaration.name;
  new_entry->value = new_value;

  hash_table_add_entry(symbols, new_entry);

  return;
} 

static Types expression_type_check(AstNode *node, HashTable *symbols, char *function_name, Arena *ast_arena) {
  switch (node->type) {
    case AST_EXPRESSION_VARIABLE: {
      HashTableEntry *entry = hash_table_get_entry(symbols, node->data.variable_expression.identifier);

      if (entry == NULL || entry->key == NULL) {
        fprintf(stderr, "ERROR - SA Type Check: Expression variable '%s' not found in declaration symbol table\n", node->data.variable_expression.identifier);
        exit(1);
      }

      TypeCheckSymbol* symbol = entry->value->structure; 

      if (symbol->symbol_type == SYMBOL_FUNCTION) {
        fprintf(stderr, "ERROR - SA Type Check: Function name '%s' is being used as a variable\n", node->data.variable_expression.identifier);
        exit(1);
      }

      AstNode *ast_expression_type = arena_alloc(ast_arena);
      ast_expression_type->type = AST_TYPE;
      
      switch (symbol->data.variable_symbol->value_type) {
        case TYPE_INT:   ast_expression_type->data.type.type = AST_TYPE_INT; break;
        case TYPE_LONG:  ast_expression_type->data.type.type = AST_TYPE_LONG; break;
      }

      node->data.variable_expression.expression_type = ast_expression_type;

      return ast_expression_type->data.type.type;
    }
    case AST_EXPRESSION_CONSTANT: {
      AstNode *ast_expression_type = arena_alloc(ast_arena);
      ast_expression_type->type = AST_TYPE;
      
      switch (node->data.constant_expression.constant_type) {
        case AST_CONSTANT_TYPE_INT:   ast_expression_type->data.type.type = AST_TYPE_INT; break;
        case AST_CONSTANT_TYPE_LONG:  ast_expression_type->data.type.type = AST_TYPE_LONG; break;
      }

      node->data.constant_expression.expression_type = ast_expression_type;

      return ast_expression_type->data.type.type;
    }
    case AST_EXPRESSION_CAST: {
      Types expression_type = expression_type_check(node, symbols, function_name, ast_arena);

      AstNode *ast_expression_type_node = arena_alloc(ast_arena);
      ast_expression_type_node->type = AST_TYPE;
      ast_expression_type_node->data.type.type = expression_type;

      node->data.cast_expression.expression_type = ast_expression_type_node;

      return expression_type;
    }
    case AST_EXPRESSION_UNARY: {
      Types expression_type = expression_type_check(node->data.unary_expression.expression, symbols, function_name, ast_arena);

      AstNode *ast_expression_type_node = arena_alloc(ast_arena);
      ast_expression_type_node->type = AST_TYPE;
      ast_expression_type_node->data.type.type = expression_type;

      node->data.unary_expression.expression_type = ast_expression_type_node;

      if (node->data.unary_expression.op_type == AST_UNARY_NOT) {
        return AST_TYPE_INT;
      }

      return expression_type;
    }
  }
}
