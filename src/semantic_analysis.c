#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/semantic_analysis.h"
#include "../include/hash_table.h"

void semantic_variable_resolution(AstNode *ast_nodes);
void semantic_resolve_declaration(AstNode *ast_nodes);

void run_semantic_analysis(AstNode *ast_nodes) {
  semantic_variable_resolution(ast_nodes);
}

//Create a mapping for each defined variable into a unique name which will help keep track of variables in multi-scoped functions/blocks
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

      snprintf(new_identifier + var_length, 100 - var_length, "%d", var_suffix);
      new_variable_entry->value.string = new_identifier;

      hash_table_add_entry(&variable_table, new_variable_entry);
    }
  }

  hash_table_print(&variable_table);
}
