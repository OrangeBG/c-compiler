#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/sa_variable_resolution.h"
#include "../include/hash_table.h"
#include "../include/stack.h"

#define VARIABLE_RESOLUTION_STACK_SIZE 16

enum DeclarationType {
  DECLARATION_TYPE_FUNCTION,
  DECLARATION_TYPE_VARIABLE
};

static void variable_resolve_node(AstNode *node, Stack *declaration_stack); 
static void resolve_file_scope_variable_declaration(char *identifier, enum DeclarationType declaration_type, Stack *declaration_stack);  
static void resolve_local_scope_variable_declaration(char *identifier, enum DeclarationType declaration_type, Stack *declaration_stack);   

typedef struct {
  char *name;
  enum DeclarationType declaration_type;
  bool from_current_scope;
  bool has_linkage;
} Declaration;

void sa_variable_resolution(AstNode *ast_nodes) {
  Stack *declaration_stack;
  stack_init(declaration_stack, VARIABLE_RESOLUTION_STACK_SIZE);

  HashTable symbols_table;
  hash_table_init(&symbols_table);

  StackValue file_scope_stack;
  file_scope_stack.type = STACK_STRUCT;
  file_scope_stack.data.structure = &symbols_table;

  stack_push(declaration_stack, file_scope_stack);

  for (int i = 0; i < ast_nodes->data.program.declaration_count; i++) {
    AstNode *declaration_node = ast_nodes->data.program.declaration_ptrs->node_pointers[i];
    variable_resolve_node(declaration_node, declaration_stack);
  }
}

static void variable_resolve_node(AstNode *node, Stack *declaration_stack) {
  if (declaration_stack->count == 0) {
    fprintf(stderr, "ERROR - SA Variable Resolution: Declaration stack reached a 0 count");
    exit(1);
  }
  
  switch (node->type) {
    case AST_VARIABLE_DECLARATION:
      if (declaration_stack->count == 1) {
        resolve_file_scope_variable_declaration(node->data.variable_declaration.name, DECLARATION_TYPE_VARIABLE, declaration_stack);
      }
      break;
    default:
      fprintf(stderr, "ERROR - SA Variable Resolution: AST type '%d' not supported", node->type);
      exit(1);
      break;

  }
} 

static void resolve_file_scope_variable_declaration(char *identifier, enum DeclarationType declaration_type, Stack *declaration_stack) {  
  HashTableEntry *table_entry = hash_table_get_entry(declaration_stack->stack->data.structure, identifier);

  Declaration *file_scope_declaration = malloc(sizeof(Declaration));
  file_scope_declaration->declaration_type = declaration_type;
  file_scope_declaration->from_current_scope = true;
  file_scope_declaration->has_linkage = true;
  file_scope_declaration->name = identifier;

  table_entry->key = identifier;
  table_entry->value->structure = file_scope_declaration;
}

static void resolve_local_scope_variable_declaration(char *identifier, enum DeclarationType declaration_type, Stack *declaration_stack) {  

}
