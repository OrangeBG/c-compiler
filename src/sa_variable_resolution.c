#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/sa_variable_resolution.h"
#include "../include/hash_table.h"

#define IDENTIFIER_BUFFER 256

typedef struct {
  HashTable stack_variable_table;
  HashTable parent_variable_table;
  HashTable local_variable_table;  
} VariableResolution;

typedef struct {
  char *name;
  bool from_current_scope;
  bool has_linkage;
} FunctionDeclaration;

void sa_variable_resolve_node(AstNode *node, VariableResolution *variables, HashTable *function_identifier_table); 

void sa_variable_resolution(AstNode *ast_nodes) {
  HashTable stack_variable_table;
  hash_table_init(&stack_variable_table); 

  HashTable parent_variable_table;
  hash_table_init(&parent_variable_table); 

  HashTable local_variable_table;
  hash_table_init(&local_variable_table); 

  VariableResolution variables = {
    .local_variable_table = local_variable_table,
    .parent_variable_table = parent_variable_table,
    .stack_variable_table = stack_variable_table
  };
  
  HashTable function_identifier_table;
  hash_table_init(&function_identifier_table); 

  for (int i = 0; i < ast_nodes->data.program.function_count; i++) {
    sa_variable_resolve_node(&ast_nodes->data.program.function_declarations[i], &variables, &function_identifier_table);
  }
}

void sa_variable_resolve_node(AstNode *node, VariableResolution *variables, HashTable *function_identifier_table) {
  switch (node->type) {
    case AST_VARIABLE_DECLARATION: {
      char* identifier = node->data.variable_declaration.name;
      HashTableEntry *existing_variable = hash_table_get_entry(&variables->local_variable_table, identifier);

      if (existing_variable != NULL && existing_variable->key != NULL) {
        fprintf(stderr, "ERROR - SA Variable Resolution: Duplicate '%s' variable found in block\n", identifier);
        exit(1);
      }

      char *converted_identifier = malloc(IDENTIFIER_BUFFER);
      HashTableEntry *parent_entry = hash_table_get_entry(&variables->parent_variable_table, identifier);

      if (parent_entry != NULL && parent_entry->key != NULL) {
        existing_variable = hash_table_get_entry(&variables->stack_variable_table, identifier);
        existing_variable->value->integer = parent_entry->value->integer + 1;
        snprintf(converted_identifier, IDENTIFIER_BUFFER, "%s.%d", identifier, parent_entry->value->integer + 1);
        hash_table_add_entry(&variables->local_variable_table, existing_variable);
      } else {  
        HashTableEntry *new_variable_entry = malloc(sizeof(HashTableEntry));
        new_variable_entry->key = identifier;
        HashValue *new_value = malloc(sizeof(HashValue));
        new_value->type = HASH_INT;
        new_value->integer = 0;

        new_variable_entry->value = new_value;

        snprintf(converted_identifier, IDENTIFIER_BUFFER, "%s.%d", identifier, 0);
        hash_table_add_entry(&variables->stack_variable_table, new_variable_entry);
        hash_table_add_entry(&variables->local_variable_table, new_variable_entry);
      }    

      if (node->data.variable_declaration.has_expression == true) {
        sa_variable_resolve_node(node->data.variable_declaration.init_expression, variables, function_identifier_table);
      }

      node->data.variable_declaration.name = converted_identifier;
      break;
    }
    case AST_FUNCTION_DECLARATION: {
        char *function_identifier = node->data.function_declaration.name;
        HashTableEntry *existing_entry = hash_table_get_entry(function_identifier_table, function_identifier);
        
        if (existing_entry != NULL && existing_entry->key != NULL) {         
          FunctionDeclaration *declaration = existing_entry->value->structure;
          if (declaration->from_current_scope && !declaration->has_linkage) { 
            fprintf(stderr, "ERROR - SA Variable Resolution: Duplicate function declaration '%s'\n", function_identifier);
            exit(1);
          }

          //TODO: Will need to look at a better way of doing this. Duplicate code used when there is and isn't an existing entry
          for (int i = 0; i < node->data.function_declaration.parameter_count; i++) {
            sa_variable_resolve_node(&node->data.function_declaration.parameters[i], variables, function_identifier_table);
          }
        
          if (node->data.function_declaration.body_block != NULL) {
            for (int i = 0; i < node->data.function_declaration.body_block->data.block.block_count; i++) {
              sa_variable_resolve_node(&node->data.function_declaration.body_block->data.block.block_items[i], variables, function_identifier_table); 
            }
          }
          break;
        }

        FunctionDeclaration *declaration = malloc(sizeof(FunctionDeclaration));
        declaration->name = function_identifier;
        declaration->from_current_scope = true;
        declaration->has_linkage = true;

        HashValue *value = malloc(sizeof(HashValue));
        value->type = HASH_STRUCT;
        value->structure = declaration;

        HashTableEntry *entry = malloc(sizeof(HashTableEntry));
        entry->key = function_identifier;
        entry->value = value;
        
        hash_table_add_entry(function_identifier_table, entry);

        HashTable *block_variable_table = hash_table_clone(&variables->stack_variable_table);
        HashTable local_declared_variables;
        hash_table_init(&local_declared_variables);
        VariableResolution new_variables = {
          .stack_variable_table = *block_variable_table,
          .local_variable_table = local_declared_variables,
          .parent_variable_table = variables->stack_variable_table
        };

        for (int i = 0; i < node->data.function_declaration.parameter_count; i++) {
          sa_variable_resolve_node(&node->data.function_declaration.parameters[i], &new_variables, function_identifier_table);
        }
        
        if (node->data.function_declaration.body_block != NULL) {
          for (int i = 0; i < node->data.function_declaration.body_block->data.block.block_count; i++) {
            sa_variable_resolve_node(&node->data.function_declaration.body_block->data.block.block_items[i], &new_variables, function_identifier_table); 
          }
        }
        break;
    }
    case AST_EXPRESSION_FUNCTION_CALL: {
      char *function_identifier = node->data.function_call_expression.identfier;
      HashTableEntry *existing_entry = hash_table_get_entry(function_identifier_table, function_identifier);

      if (existing_entry == NULL || existing_entry->key == NULL) {
        fprintf(stderr, "ERROR - SA Variable Resolution: Undeclared function '%s'\n", function_identifier);
        exit(1);
      }

      for (int i = 0; i < node->data.function_call_expression.argument_count; i++) {
        sa_variable_resolve_node(&node->data.function_call_expression.arguments[i], variables, function_identifier_table); 
      }      
      break;
    }
    case AST_FUNCTION_PARAMETER: {
      if (node->data.function_parameters.type == AST_PARAMETER_VOID) {
        return;
      }
      
      char* identifier = node->data.function_parameters.name;
      HashTableEntry *existing_variable = hash_table_get_entry(&variables->local_variable_table, identifier);

      if (existing_variable != NULL && existing_variable->key != NULL) {
        fprintf(stderr, "ERROR - SA Variable Resolution: Duplicate '%s' function variable found\n", identifier);
        exit(1);
      }

      char *converted_identifier = malloc(IDENTIFIER_BUFFER);
      HashTableEntry *parent_entry = hash_table_get_entry(&variables->parent_variable_table, identifier);

      if (parent_entry != NULL && parent_entry->key != NULL) {
        existing_variable = hash_table_get_entry(&variables->stack_variable_table, identifier);
        existing_variable->value->integer = parent_entry->value->integer + 1;
        snprintf(converted_identifier, IDENTIFIER_BUFFER, "%s.%d", identifier, parent_entry->value->integer + 1);
        hash_table_add_entry(&variables->local_variable_table, existing_variable);
      } else {  
        HashTableEntry *new_variable_entry = malloc(sizeof(HashTableEntry));
        new_variable_entry->key = identifier;
        HashValue *new_value = malloc(sizeof(HashValue));
        new_value->type = HASH_INT;
        new_value->integer = 0;
        new_variable_entry->value = new_value;
        snprintf(converted_identifier, IDENTIFIER_BUFFER, "%s.%d", identifier, 0);
        hash_table_add_entry(&variables->stack_variable_table, new_variable_entry);
        hash_table_add_entry(&variables->local_variable_table, new_variable_entry);
      }    

      node->data.function_parameters.name = converted_identifier;
      break;
    }
    case AST_BLOCK: {
      HashTable *block_variable_table = hash_table_clone(&variables->stack_variable_table);
      HashTable local_declared_variables;
      hash_table_init(&local_declared_variables);

      VariableResolution new_variables = {
        .stack_variable_table = *block_variable_table,
        .local_variable_table = local_declared_variables,
        .parent_variable_table = variables->stack_variable_table
      };

      for (int i = 0; i < node->data.block.block_count; i++) {   
        sa_variable_resolve_node(&node->data.block.block_items[i], &new_variables, function_identifier_table); 
      }

      free(local_declared_variables.entries);
      break;
    }
    case AST_STATEMENT_IF: {
      sa_variable_resolve_node(node->data.if_statement.condition_expression, variables, function_identifier_table);
      sa_variable_resolve_node(node->data.if_statement.then_statement, variables, function_identifier_table);

      if (node->data.if_statement.else_statement != NULL) {
        sa_variable_resolve_node(node->data.if_statement.else_statement, variables, function_identifier_table);
      }
      break;
    }
    case AST_STATEMENT_RETURN: {
      sa_variable_resolve_node(node->data.return_statement.expression, variables, function_identifier_table);
      break;
    }
    case AST_STATEMENT_EXPRESSION: {
      sa_variable_resolve_node(node->data.expression_statement.expression, variables, function_identifier_table);
      break;
    }
    case AST_STATEMENT_FOR: {
      if (node->data.for_statement.for_loop_init != NULL) {
        sa_variable_resolve_node(node->data.for_statement.for_loop_init, variables, function_identifier_table);
      }

      if (node->data.for_statement.condition_expression != NULL) {
        sa_variable_resolve_node(node->data.for_statement.condition_expression, variables, function_identifier_table);
      }

      if (node->data.for_statement.post_expression != NULL) {
        sa_variable_resolve_node(node->data.for_statement.post_expression, variables, function_identifier_table);
      }

      sa_variable_resolve_node(node->data.for_statement.statement_body, variables, function_identifier_table);
      break;
    }
    case AST_STATEMENT_WHILE: {
      sa_variable_resolve_node(node->data.while_statement.condition, variables, function_identifier_table);
      sa_variable_resolve_node(node->data.while_statement.statement_body, variables, function_identifier_table);
      break;
    }
    case AST_STATEMENT_DO_WHILE: {
      sa_variable_resolve_node(node->data.do_while_statement.condition, variables, function_identifier_table);
      sa_variable_resolve_node(node->data.do_while_statement.statement_body, variables, function_identifier_table);
      break;
    }
    case AST_EXPRESSION_ASSIGNMENT: {
      if (node->data.assignement_expression.left_expression->type != AST_EXPRESSION_VARIABLE && node->data.assignement_expression.left_expression->type != AST_EXPRESSION_UNARY) {
        fprintf(stderr, "ERROR - SA Variable Resolution: Invalid LValue for assignment expression\n");
        exit(1);
      }

      sa_variable_resolve_node(node->data.assignement_expression.left_expression, variables, function_identifier_table);
      sa_variable_resolve_node(node->data.assignement_expression.right_expression, variables, function_identifier_table);
      break;
    }
    case AST_EXPRESSION_BINARY: {
      sa_variable_resolve_node(node->data.binary_expression.left_expression, variables, function_identifier_table);
      sa_variable_resolve_node(node->data.binary_expression.right_expression, variables, function_identifier_table);
      break;
    }
    case AST_EXPRESSION_POSTFIX_INCREMENT:
    case AST_EXPRESSION_POSTFIX_DECREMENT:
    case AST_EXPRESSION_PREFIX_INCREMENT:
    case AST_EXPRESSION_PREFIX_DECREMENT: 
      sa_variable_resolve_node(node->data.increment_decrement_expression.expression, variables, function_identifier_table);
      break;
    case AST_EXPRESSION_UNARY:
      sa_variable_resolve_node(node->data.unary_expression.expression, variables, function_identifier_table);
      break;
    case AST_EXPRESSION_VARIABLE: {
      HashTableEntry *entry = hash_table_get_entry(&variables->stack_variable_table, node->data.variable_expression.identifier);  

      if (entry == NULL || entry->key == NULL) {
        //check to see if we already converted the identifier. Since we're adding '.' to identifiers as part of the semantic analysis variable resolution, check to see if the period exists.
        char *found_period = (char*)memchr(node->data.variable_expression.identifier, '.', strlen(node->data.variable_expression.identifier));

        if (found_period == NULL) {      
          fprintf(stderr, "ERROR - SA Variable Resolution: Undeclared variable hash table entry for '%s'\n", node->data.variable_expression.identifier);
          exit(1);
        }

        return;
      } 
    
      char *converted_identifier = malloc(IDENTIFIER_BUFFER);
      snprintf(converted_identifier, IDENTIFIER_BUFFER, "%s.%d", entry->key, entry->value->integer);
      node->data.variable_expression.identifier = converted_identifier;
      break;
    }
  }
}
