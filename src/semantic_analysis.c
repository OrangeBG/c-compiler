#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/semantic_analysis.h"
#include "../include/hash_table.h"

void semantic_variable_resolution(AstNode *ast_nodes);
void semantic_resolve_declaration(AstNode *ast_nodes);
void semantic_resolve_expressison(AstNode *expression, HashTable *variable_table); 

void run_semantic_analysis(AstNode *ast_nodes) {
  semantic_variable_resolution(ast_nodes);
}

//Create a mapping for each defined variable into a unique name which will help keep track of variables in multi-scoped functions/blocks
//TODO: Cleanup function
void semantic_variable_resolution(AstNode *ast_nodes) {
  HashTable variable_table;
  hash_table_init(&variable_table); 

  AstNode *function = ast_nodes->data.program.function;

  //TODO: Will need to increment this when blocks are added
  int var_suffix = 0;

  for (int i = 0; i < function->data.function.block_count; i++) {   
    AstNode *stmt_or_decl = &function->data.function.blocks[i];
    
    if (stmt_or_decl->type == AST_DECLARATION) {        
      char* identifier = stmt_or_decl->data.declaration.identifier;
      
      HashTableEntry *existing_variable_entry = hash_table_get_entry(&variable_table, identifier);

      if (existing_variable_entry != NULL && existing_variable_entry->key != NULL) {
        fprintf(stderr, "Duplicate '%s' variable found in function '%s'", identifier, function->data.function.name);
        exit(1);
      }

      HashTableEntry *new_variable_entry = malloc(sizeof(HashTableEntry));
      new_variable_entry->key = identifier;
      new_variable_entry->value.type = HASH_STRING;

      size_t var_length = strlen(identifier);

      //TODO: Find a better way to do this rather than '+5'
      char *new_identifier = malloc(var_length + 5);
      strcpy(new_identifier, identifier);

      snprintf(new_identifier + var_length, 100 - var_length, ".%d", var_suffix);
      new_variable_entry->value.string = new_identifier;

      hash_table_add_entry(&variable_table, new_variable_entry);

      if (stmt_or_decl->data.declaration.has_expression == true) {
        semantic_resolve_expressison(stmt_or_decl->data.declaration.expression, &variable_table);
      }

      stmt_or_decl->data.declaration.identifier = new_identifier;
    }
    else if (stmt_or_decl->type == AST_STATEMENT_IF) {
      semantic_resolve_expressison(stmt_or_decl->data.if_statement.condition_expression, &variable_table);
      semantic_resolve_expressison(stmt_or_decl->data.if_statement.then_statement, &variable_table);

      if (stmt_or_decl->data.if_statement.else_statement != NULL) {
        semantic_resolve_expressison(stmt_or_decl->data.if_statement.else_statement, &variable_table);
      }
    }
    else if (stmt_or_decl->type == AST_STATEMENT_RETURN) {
      semantic_resolve_expressison(stmt_or_decl->data.return_statement.expression, &variable_table);
    }
    else if (stmt_or_decl->type == AST_STATEMENT_EXPRESSION) {
      semantic_resolve_expressison(stmt_or_decl->data.expression_statement.expression, &variable_table);
    }
    else if (stmt_or_decl->type == AST_EXPRESSION_ASSIGNMENT) {
      semantic_resolve_expressison(stmt_or_decl->data.assignement_expression.left_expression, &variable_table);
      semantic_resolve_expressison(stmt_or_decl->data.assignement_expression.right_expression, &variable_table);
    }
  }
}

void semantic_resolve_expressison(AstNode *expression, HashTable *variable_table) {
  if (expression->type == AST_EXPRESSION_ASSIGNMENT) {
    if (expression->data.assignement_expression.left_expression->type != AST_EXPRESSION_VARIABLE) {
      fprintf(stderr, "Invalid LValue for assignment expression");
      exit(1);
    }

    semantic_resolve_expressison(expression->data.assignement_expression.left_expression, variable_table);
    semantic_resolve_expressison(expression->data.assignement_expression.right_expression, variable_table);
  } else if (expression->type == AST_EXPRESSION_BINARY) {
    semantic_resolve_expressison(expression->data.binary_expression.left_expression, variable_table);
    semantic_resolve_expressison(expression->data.binary_expression.right_expression, variable_table);
  } else if (expression->type == AST_EXPRESSION_POSTFIX_INCREMENT || expression->type == AST_EXPRESSION_POSTFIX_DECREMENT || expression->type == AST_EXPRESSION_PREFIX_INCREMENT || expression->type == AST_EXPRESSION_PREFIX_DECREMENT ) {
    semantic_resolve_expressison(expression->data.increment_decrement_expression.expression, variable_table);
  } else if (expression->type == AST_EXPRESSION_VARIABLE) {
    HashTableEntry *entry = hash_table_get_entry(variable_table, expression->data.variable_expression.identifier);  

    if (entry == NULL || entry->key == NULL) {
      //check to see if we already converted the identifier. Since we're adding '.' to identifiers as part of the semantic analysis variable resolution, check to see if the period exists.
      char *found_period = (char*)memchr(expression->data.variable_expression.identifier, '.', strlen(expression->data.variable_expression.identifier));

      if (found_period == NULL) {      
        fprintf(stderr, "Undeclared variable hash table entry for '%s'", expression->data.variable_expression.identifier);
        exit(1);
      }

      return;
    } 

    expression->data.variable_expression.identifier = entry->value.string;
  }
}
