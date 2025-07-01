#include <stdio.h>
#include <stdlib.h>
#include "../include/sa_type_check.h"
#include "../include/hash_table.h"

typedef struct {
  HashTable variable_symbols;
  HashTable function_symbols;
} TypeCheckSymbol;

typedef enum {
  TYPE_INT
} Type;

typedef struct {
  bool from_current_scope;
  bool has_linkage;
} FunctionSymbol;

void sa_type_check_variable_declaration(AstNode *node, HashTable *variable_symbols);
void sa_type_check_function_declaration(AstNode *node, HashTable *function_symbols);

void sa_type_check(AstNode *ast_nodes) {
  TypeCheckSymbol symbols;
  hash_table_init(&symbols.variable_symbols);
  hash_table_init(&symbols.function_symbols);

  for (int i = 0; i < ast_nodes->data.program.function_count; i++) {
    sa_type_check_variable_declaration(&ast_nodes->data.program.function_declarations[i], &symbols.variable_symbols);
  }
  
  for (int i = 0; i < ast_nodes->data.program.function_count; i++) {
    sa_type_check_function_declaration(&ast_nodes->data.program.function_declarations[i], &symbols.function_symbols);
  }
}

void sa_type_check_variable_declaration(AstNode *node, HashTable *variable_symbols) {
  switch (node->type) {
    case AST_VARIABLE_DECLARATION: {
      char *identifier = node->data.variable_declaration.name;

      //This pass happens after variable resolution, so no need to check to see if the variable is duplicated in the hash table

      HashTableEntry *entry = malloc(sizeof(HashTableEntry));
      entry->key = identifier;
      entry->value.type = HASH_INT;
      entry->value.integer = TYPE_INT;

      hash_table_add_entry(variable_symbols, entry); 

      if (node->data.variable_declaration.has_expression) {
        sa_type_check_variable_declaration(node->data.variable_declaration.init_expression, variable_symbols);
      }
      break;
    }
    case AST_FUNCTION_DECLARATION: {
      for (int i = 0; i < node->data.function_declaration.parameter_count; i++) {
        sa_type_check_variable_declaration(&node->data.function_declaration.parameters[i], variable_symbols);
      }

      if (node->data.function_declaration.body_block != NULL) {
        sa_type_check_variable_declaration(node->data.function_declaration.body_block, variable_symbols);
      }
      break;
    }
    case AST_FUNCTION_PARAMETER: {
      char *identifier = node->data.function_parameters.name;

      //This pass happens after variable resolution, so no need to check to see if the variable is duplicated in the hash table
      HashTableEntry *entry = malloc(sizeof(HashTableEntry));
      entry->key = identifier;
      entry->value.type = HASH_INT;
      entry->value.integer = TYPE_INT;

      hash_table_add_entry(variable_symbols, entry); 
      break;
    }
    case AST_BLOCK: {
      for (int i = 0; i < node->data.block.block_count; i++) {   
        sa_type_check_variable_declaration(&node->data.block.block_items[i], variable_symbols);
      }
      break;
    }
    case AST_STATEMENT_IF: {
      sa_type_check_variable_declaration(node->data.if_statement.condition_expression, variable_symbols);
      sa_type_check_variable_declaration(node->data.if_statement.then_statement, variable_symbols);

      if (node->data.if_statement.else_statement != NULL) {
        sa_type_check_variable_declaration(node->data.if_statement.else_statement, variable_symbols);
      }
      break;
    }
    case AST_STATEMENT_RETURN: {
      sa_type_check_variable_declaration(node->data.return_statement.expression, variable_symbols);
      break;
    }
    case AST_STATEMENT_EXPRESSION: {
      sa_type_check_variable_declaration(node->data.expression_statement.expression, variable_symbols);
      break;
    }
    case AST_STATEMENT_FOR: {
      if (node->data.for_statement.for_loop_init != NULL) {
        sa_type_check_variable_declaration(node->data.for_statement.for_loop_init, variable_symbols);
      }

      if (node->data.for_statement.condition_expression != NULL) {
        sa_type_check_variable_declaration(node->data.for_statement.condition_expression, variable_symbols);
      }

      if (node->data.for_statement.post_expression != NULL) {
        sa_type_check_variable_declaration(node->data.for_statement.post_expression, variable_symbols);
      }

      sa_type_check_variable_declaration(node->data.for_statement.statement_body, variable_symbols);
      break;
    }
    case AST_STATEMENT_WHILE: {
      sa_type_check_variable_declaration(node->data.while_statement.condition, variable_symbols);
      sa_type_check_variable_declaration(node->data.while_statement.statement_body, variable_symbols);
      break;
    }
    case AST_STATEMENT_DO_WHILE: {
      sa_type_check_variable_declaration(node->data.do_while_statement.condition, variable_symbols);
      sa_type_check_variable_declaration(node->data.do_while_statement.statement_body, variable_symbols);
      break;
    }
    case AST_EXPRESSION_ASSIGNMENT: {
      sa_type_check_variable_declaration(node->data.assignement_expression.left_expression, variable_symbols);
      sa_type_check_variable_declaration(node->data.assignement_expression.right_expression, variable_symbols);
      break;
    }
    case AST_EXPRESSION_BINARY: {
      sa_type_check_variable_declaration(node->data.binary_expression.left_expression, variable_symbols);
      sa_type_check_variable_declaration(node->data.binary_expression.right_expression, variable_symbols);
      break;
    }
    case AST_EXPRESSION_POSTFIX_INCREMENT:
    case AST_EXPRESSION_POSTFIX_DECREMENT:
    case AST_EXPRESSION_PREFIX_INCREMENT:
    case AST_EXPRESSION_PREFIX_DECREMENT: 
      sa_type_check_variable_declaration(node->data.increment_decrement_expression.expression, variable_symbols);
      break;
    case AST_EXPRESSION_UNARY:
      sa_type_check_variable_declaration(node->data.unary_expression.expression, variable_symbols);
      break;
  }  
}

void sa_type_check_function_declaration(AstNode *node, HashTable *function_symbols) {
  switch (node->type) {
    case AST_FUNCTION_DECLARATION: {
      HashTableEntry *entry = hash_table_get_entry(function_symbols, node->data.function_declaration.name);

      if (entry != NULL && entry->key != NULL) {
        FunctionSymbol *symbol = (FunctionSymbol*)entry->value.structure;
        if (symbol->from_current_scope && !symbol->has_linkage) {
          fprintf(stderr, "ERROR - SA Type Check: Duplicate declaration '%s'", entry->key);
          exit(1);
        }
      }  

      FunctionSymbol *symbol = malloc(sizeof(FunctionSymbol));
      symbol->from_current_scope = true;
      symbol->has_linkage = true;

      HashTableEntry *new_entry = malloc(sizeof(FunctionSymbol));
      new_entry->key = node->data.function_declaration.name;
      new_entry->value.type = HASH_STRUCT;
      new_entry->value.structure = symbol;

      //TODO: I don't think this will work if we find and existing entry that wasn't a duplicate entry
      hash_table_add_entry(function_symbols, new_entry);
    }
    case AST_BLOCK: {
      for (int i = 0; i < node->data.block.block_count; i++) {
        sa_type_check_function_declaration(&node->data.block.block_items[i], function_symbols);
      }
    }
  }
}
