#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/sa_variable_resolution.h"
#include "../include/hash_table.h"

void sa_variable_resolve_node(AstNode *node, HashTable *variable_table, HashTable *parent_variable_table, HashTable *local_declared_variables); 

void sa_variable_resolution(AstNode *ast_nodes) {
  HashTable variable_table;
  hash_table_init(&variable_table); 

  HashTable parent_variable_table;
  hash_table_init(&parent_variable_table); 

  HashTable local_variable_table;
  hash_table_init(&local_variable_table); 

  sa_variable_resolve_node(ast_nodes->data.program.function->data.function.block, &variable_table, &parent_variable_table, &local_variable_table);
}

void sa_variable_resolve_node(AstNode *node, HashTable *variable_table, HashTable *parent_variable_table, HashTable *local_declared_variables) {
  switch (node->type) {
    case AST_DECLARATION: {
      char* identifier = node->data.declaration.identifier;
      HashTableEntry *existing_variable = hash_table_get_entry(local_declared_variables, identifier);

      if (existing_variable != NULL && existing_variable->key != NULL) {
        fprintf(stderr, "Duplicate '%s' variable found in block\n", identifier);
        exit(1);
      }

      char *converted_identifier = malloc(256);
      HashTableEntry *parent_entry = hash_table_get_entry(parent_variable_table, identifier);

      if (parent_entry != NULL && parent_entry->key != NULL) {
        existing_variable = hash_table_get_entry(variable_table, identifier);
        existing_variable->value.integer = parent_entry->value.integer + 1;
        snprintf(converted_identifier, sizeof(converted_identifier), "%s.%d", identifier, parent_entry->value.integer + 1);
        hash_table_add_entry(local_declared_variables, existing_variable);
      } else {  
        HashTableEntry *new_variable_entry = malloc(sizeof(HashTableEntry));
        new_variable_entry->key = identifier;
        new_variable_entry->value.type = HASH_INT;
        new_variable_entry->value.integer = 0;
        snprintf(converted_identifier, sizeof(converted_identifier), "%s.%d", identifier, 0);
        hash_table_add_entry(variable_table, new_variable_entry);
        hash_table_add_entry(local_declared_variables, new_variable_entry);
      }    

      if (node->data.declaration.has_expression == true) {
        sa_variable_resolve_node(node->data.declaration.expression, variable_table, parent_variable_table, local_declared_variables);
      }

      node->data.declaration.identifier = converted_identifier;
      break;
    }
    case AST_BLOCK: {
      HashTable *block_variable_table = hash_table_clone(variable_table);
      HashTable local_declared_variables;
      hash_table_init(&local_declared_variables);

      for (int i = 0; i < node->data.block.block_count; i++) {   
        sa_variable_resolve_node(&node->data.block.block_items[i], block_variable_table, variable_table, &local_declared_variables); 
      }

      free(local_declared_variables.entries);
      break;
    }
    case AST_STATEMENT_IF: {
      sa_variable_resolve_node(node->data.if_statement.condition_expression, variable_table, parent_variable_table, local_declared_variables);
      sa_variable_resolve_node(node->data.if_statement.then_statement, variable_table, parent_variable_table, local_declared_variables);

      if (node->data.if_statement.else_statement != NULL) {
        sa_variable_resolve_node(node->data.if_statement.else_statement, variable_table, parent_variable_table, local_declared_variables);
      }
      break;
    }
    case AST_STATEMENT_RETURN: {
      sa_variable_resolve_node(node->data.return_statement.expression, variable_table, parent_variable_table, local_declared_variables);
      break;
    }
    case AST_STATEMENT_EXPRESSION: {
      sa_variable_resolve_node(node->data.expression_statement.expression, variable_table, parent_variable_table, local_declared_variables);
      break;
    }
    case AST_STATEMENT_FOR: {
      if (node->data.for_statement.for_loop_init != NULL) {
        sa_variable_resolve_node(node->data.for_statement.for_loop_init, variable_table, parent_variable_table, local_declared_variables);
      }

      if (node->data.for_statement.condition_expression != NULL) {
        sa_variable_resolve_node(node->data.for_statement.condition_expression, variable_table, parent_variable_table, local_declared_variables);
      }

      if (node->data.for_statement.post_expression != NULL) {
        sa_variable_resolve_node(node->data.for_statement.post_expression, variable_table, parent_variable_table, local_declared_variables);
      }

      sa_variable_resolve_node(node->data.for_statement.statement_body, variable_table, parent_variable_table, local_declared_variables);
      break;
    }
    case AST_STATEMENT_WHILE: {
      sa_variable_resolve_node(node->data.while_statement.condition, variable_table, parent_variable_table, local_declared_variables);
      sa_variable_resolve_node(node->data.while_statement.statement_body, variable_table, parent_variable_table, local_declared_variables);
      break;
    }
    case AST_STATEMENT_DO_WHILE: {
      sa_variable_resolve_node(node->data.do_while_statement.condition, variable_table, parent_variable_table, local_declared_variables);
      sa_variable_resolve_node(node->data.do_while_statement.statement_body, variable_table, parent_variable_table, local_declared_variables);
      break;
    }
    case AST_EXPRESSION_ASSIGNMENT: {
      if (node->data.assignement_expression.left_expression->type != AST_EXPRESSION_VARIABLE && node->data.assignement_expression.left_expression->type != AST_EXPRESSION_UNARY) {
        fprintf(stderr, "ERROR - SA Variable Resolution: Invalid LValue for assignment expression\n");
        exit(1);
      }

      sa_variable_resolve_node(node->data.assignement_expression.left_expression, variable_table, parent_variable_table, local_declared_variables);
      sa_variable_resolve_node(node->data.assignement_expression.right_expression, variable_table, parent_variable_table, local_declared_variables);
      break;
    }
    case AST_EXPRESSION_BINARY: {
      sa_variable_resolve_node(node->data.binary_expression.left_expression, variable_table, parent_variable_table, local_declared_variables);
      sa_variable_resolve_node(node->data.binary_expression.right_expression, variable_table, parent_variable_table, local_declared_variables);
      break;
    }
    case AST_EXPRESSION_POSTFIX_INCREMENT:
    case AST_EXPRESSION_POSTFIX_DECREMENT:
    case AST_EXPRESSION_PREFIX_INCREMENT:
    case AST_EXPRESSION_PREFIX_DECREMENT: 
      sa_variable_resolve_node(node->data.increment_decrement_expression.expression, variable_table, parent_variable_table, local_declared_variables);
      break;
    case AST_EXPRESSION_UNARY:
      sa_variable_resolve_node(node->data.unary_expression.expression, variable_table, parent_variable_table, local_declared_variables);
      break;
    case AST_EXPRESSION_VARIABLE: {
      HashTableEntry *entry = hash_table_get_entry(variable_table, node->data.variable_expression.identifier);  

      if (entry == NULL || entry->key == NULL) {
        //check to see if we already converted the identifier. Since we're adding '.' to identifiers as part of the semantic analysis variable resolution, check to see if the period exists.
        char *found_period = (char*)memchr(node->data.variable_expression.identifier, '.', strlen(node->data.variable_expression.identifier));

        if (found_period == NULL) {      
          fprintf(stderr, "ERROR - SA Variable Resolution: Undeclared variable hash table entry for '%s'\n", node->data.variable_expression.identifier);
          exit(1);
        }

        return;
      } 
    
      char *converted_identifier = malloc(256);
      snprintf(converted_identifier, 256, "%s.%d", entry->key, entry->value.integer);
      node->data.variable_expression.identifier = converted_identifier;
      break;
    }
  }
}
