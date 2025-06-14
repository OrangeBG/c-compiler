#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/semantic_analysis.h"
#include "../include/hash_table.h"

void semantic_variable_resolution(AstNode *ast_nodes);
void semantic_variable_resolve_expression(AstNode *expression, HashTable *variable_table); 
void semantic_variable_resolve_block(AstNode *block, HashTable *parent_variable_table); 

void run_semantic_analysis(AstNode *ast_nodes) {
  semantic_variable_resolution(ast_nodes);
}

//Create a mapping for each defined variable into a unique name which will help keep track of variables in multi-scoped functions/blocks
//TODO: Cleanup function
void semantic_variable_resolution(AstNode *ast_nodes) {
  HashTable variable_table;
  hash_table_init(&variable_table); 

  semantic_variable_resolve_block(ast_nodes->data.program.function->data.function.block, &variable_table);
}

void semantic_variable_resolve_block(AstNode *block, HashTable *parent_variable_table) {
  HashTable *variable_table = hash_table_clone(parent_variable_table);

  for (int i = 0; i < block->data.block.block_count; i++) {   
    AstNode *stmt_or_decl = &block->data.block.block_items[i];
    
    if (stmt_or_decl->type == AST_DECLARATION) {        
      char* identifier = stmt_or_decl->data.declaration.identifier;
      
      HashTableEntry *existing_variable_entry = hash_table_get_entry(variable_table, identifier);

      if (existing_variable_entry != NULL && existing_variable_entry->key != NULL) {
        fprintf(stderr, "Duplicate '%s' variable found in block", identifier);
        exit(1);
      }

      HashTableEntry *new_variable_entry = malloc(sizeof(HashTableEntry));
      new_variable_entry->key = identifier;
      new_variable_entry->value.type = HASH_INT;

      char converted_identifier[256];

      HashTableEntry *parent_entry = hash_table_get_entry(parent_variable_table, identifier);

      if (parent_entry != NULL && parent_entry->key != NULL) {
        new_variable_entry->value.integer = parent_entry->value.integer + 1;
        snprintf(converted_identifier, sizeof(converted_identifier), "%s%d", identifier, parent_entry->value.integer + 1);
      } else {  
        new_variable_entry->value.integer = 0;
        snprintf(converted_identifier, sizeof(converted_identifier), "%s%d", identifier, 0);
      }    

      hash_table_add_entry(variable_table, new_variable_entry);

      if (stmt_or_decl->data.declaration.has_expression == true) {
        semantic_variable_resolve_expression(stmt_or_decl->data.declaration.expression, variable_table);
      }

      stmt_or_decl->data.declaration.identifier = converted_identifier;
    }
    else if (stmt_or_decl->type == AST_STATEMENT_IF) {
      semantic_variable_resolve_expression(stmt_or_decl->data.if_statement.condition_expression, variable_table);
      semantic_variable_resolve_expression(stmt_or_decl->data.if_statement.then_statement, variable_table);

      if (stmt_or_decl->data.if_statement.else_statement != NULL) {
        semantic_variable_resolve_expression(stmt_or_decl->data.if_statement.else_statement, variable_table);
      }
    }
    else if (stmt_or_decl->type == AST_STATEMENT_RETURN) {
      semantic_variable_resolve_expression(stmt_or_decl->data.return_statement.expression, variable_table);
    }
    else if (stmt_or_decl->type == AST_STATEMENT_EXPRESSION) {
      semantic_variable_resolve_expression(stmt_or_decl->data.expression_statement.expression, variable_table);
    }
    else if (stmt_or_decl->type == AST_EXPRESSION_ASSIGNMENT) {
      semantic_variable_resolve_expression(stmt_or_decl->data.assignement_expression.left_expression, variable_table);
      semantic_variable_resolve_expression(stmt_or_decl->data.assignement_expression.right_expression, variable_table);
    } else if (stmt_or_decl->type == AST_BLOCK) {
      semantic_variable_resolve_block(stmt_or_decl, variable_table); 
    }
  }
}

void semantic_variable_resolve_expression(AstNode *expression, HashTable *variable_table) {
  if (expression->type == AST_EXPRESSION_ASSIGNMENT) {
    if (expression->data.assignement_expression.left_expression->type != AST_EXPRESSION_VARIABLE && expression->data.assignement_expression.left_expression->type != AST_EXPRESSION_UNARY) {
      fprintf(stderr, "ERROR - Semantic Analysis: Invalid LValue for assignment expression\n");
      exit(1);
    }

    semantic_variable_resolve_expression(expression->data.assignement_expression.left_expression, variable_table);
    semantic_variable_resolve_expression(expression->data.assignement_expression.right_expression, variable_table);
  } else if (expression->type == AST_EXPRESSION_BINARY) {
    semantic_variable_resolve_expression(expression->data.binary_expression.left_expression, variable_table);
    semantic_variable_resolve_expression(expression->data.binary_expression.right_expression, variable_table);
  } else if (expression->type == AST_EXPRESSION_POSTFIX_INCREMENT || expression->type == AST_EXPRESSION_POSTFIX_DECREMENT || expression->type == AST_EXPRESSION_PREFIX_INCREMENT || expression->type == AST_EXPRESSION_PREFIX_DECREMENT ) {
    semantic_variable_resolve_expression(expression->data.increment_decrement_expression.expression, variable_table);
  } else if (expression->type == AST_EXPRESSION_VARIABLE) {
    HashTableEntry *entry = hash_table_get_entry(variable_table, expression->data.variable_expression.identifier);  

    if (entry == NULL || entry->key == NULL) {
      //check to see if we already converted the identifier. Since we're adding '.' to identifiers as part of the semantic analysis variable resolution, check to see if the period exists.
      char *found_period = (char*)memchr(expression->data.variable_expression.identifier, '.', strlen(expression->data.variable_expression.identifier));

      if (found_period == NULL) {      
        fprintf(stderr, "Undeclared variable hash table entry for '%s'\n", expression->data.variable_expression.identifier);
        exit(1);
      }

      return;
    } 

    expression->data.variable_expression.identifier = entry->value.string;
  }
  else if (expression->type == AST_EXPRESSION_UNARY) {
    semantic_variable_resolve_expression(expression->data.unary_expression.expression, variable_table);
  }
}
