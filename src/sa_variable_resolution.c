#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/sa_variable_resolution.h"
#include "../include/hash_table.h"
#include "../include/stack.h"

#define VARIABLE_RESOLUTION_STACK_SIZE 16
#define IDENTIFIER_BUFFER 256

enum DeclarationType {
  DECLARATION_TYPE_FUNCTION,
  DECLARATION_TYPE_VARIABLE
};

typedef struct {
  char *name;
  enum DeclarationType declaration_type;
  bool from_current_scope;
  bool has_linkage;
  int stack_declaration_offset;
} Declaration;

static void variable_resolve_node(AstNode *node, Stack *declaration_stack); 
static void resolve_file_scope_variable_declaration(char *identifier, enum DeclarationType declaration_type, HashTable *declaration_table);  
static void resolve_local_scope_variable_declaration(AstNode *ast_node, enum DeclarationType declaration_type, HashTable *declaration_table, int stack_count);   
static void add_declaration_to_table(Declaration *declaration, char* identifier_key, HashTable *declaration_table); 

void sa_variable_resolution(AstNode *ast_nodes) {
  Stack declaration_stack;
  stack_init(&declaration_stack, VARIABLE_RESOLUTION_STACK_SIZE);

  HashTable symbols_table;
  hash_table_init(&symbols_table);

  StackValue file_scope_stack;
  file_scope_stack.type = STACK_STRUCT;
  file_scope_stack.data.structure = &symbols_table;

  stack_push(&declaration_stack, file_scope_stack);

  for (int i = 0; i < ast_nodes->data.program.declaration_count; i++) {
    AstNode *declaration_node = ast_nodes->data.program.declaration_ptrs->node_pointers[i];
    variable_resolve_node(declaration_node, &declaration_stack);
  }
}

static void variable_resolve_node(AstNode *node, Stack *declaration_stack) {
  if (declaration_stack->count == 0) {
    fprintf(stderr, "ERROR - SA Variable Resolution: Declaration stack reached a 0 count");
    exit(1);
  }
  
  StackValue *declaration_top_stack = stack_top(declaration_stack);
  HashTable *declaration_table = declaration_top_stack->data.structure;

  switch (node->type) {
    case AST_VARIABLE_DECLARATION: {
      if (declaration_stack->count == 1) {
        resolve_file_scope_variable_declaration(node->data.variable_declaration.name, DECLARATION_TYPE_VARIABLE, declaration_table);
      } else {
        resolve_local_scope_variable_declaration(node, DECLARATION_TYPE_VARIABLE, declaration_table, declaration_stack->count);   
      }

      if (node->data.variable_declaration.has_expression == true) {
        variable_resolve_node(node->data.variable_declaration.init_expression, declaration_stack);
      }      
      break;
    }
    case AST_FUNCTION_DECLARATION: {
      char *function_identifier = node->data.function_declaration.name;
      HashTableEntry *table_entry = hash_table_get_entry(declaration_table, function_identifier);
      
      if (table_entry != NULL && table_entry->key != NULL) {
        Declaration *previous_declaration = table_entry->value->structure;

        if (previous_declaration->from_current_scope && previous_declaration->has_linkage) {
            fprintf(stderr, "ERROR - SA Variable Resolution: Duplicate function declaration '%s'\n", function_identifier);
            exit(1);
        }        
      } else {
        Declaration *new_declaration = malloc(sizeof(Declaration));
        new_declaration->name = function_identifier;
        new_declaration->from_current_scope = true;
        new_declaration->has_linkage = true;
        new_declaration->stack_declaration_offset = declaration_stack->count;

        add_declaration_to_table(new_declaration, function_identifier, declaration_table);
      }

      for (int i = 0; i < node->data.function_declaration.parameter_count; i++) {
        AstNode *parameter_node = node->data.function_declaration.parameter_ptrs->node_pointers[i];
        variable_resolve_node(parameter_node, declaration_stack);
      }
  
      if (node->data.function_declaration.body_block != NULL) {
        variable_resolve_node(node->data.function_declaration.body_block, declaration_stack);
      }
    }
    case AST_EXPRESSION_FUNCTION_CALL: {
      HashTableEntry *table_entry = hash_table_get_entry(declaration_table, node->data.function_call_expression.identfier);

      if (table_entry == NULL || table_entry->key == NULL) {
        fprintf(stderr, "ERROR - SA Variable Resolution: Undeclared function '%s'\n", node->data.function_call_expression.identfier);
        exit(1);
      }

      for (int i = 0; i < node->data.function_call_expression.argument_count; i++) {
        AstNode *argument_node = node->data.function_call_expression.argument_ptrs->node_pointers[i];
        variable_resolve_node(argument_node, declaration_stack); 
      }      
      break;      
    }
    case AST_FUNCTION_PARAMETER: {
      if (node->data.function_parameters.type == AST_PARAMETER_VOID) {
        return;
      }
      
      char* identifier = node->data.function_parameters.name;
      HashTableEntry *existing_variable = hash_table_get_entry(declaration_table, identifier);

      if (existing_variable != NULL && existing_variable->key != NULL) {
        if (declaration_stack->count == ((Declaration*)existing_variable->value->structure)->stack_declaration_offset) {
          fprintf(stderr, "ERROR - SA Variable Resolution: Duplicate '%s' function variable found\n", identifier);
          exit(1);
        }
        
        char *converted_identifier = malloc(IDENTIFIER_BUFFER);
        snprintf(converted_identifier, IDENTIFIER_BUFFER, "%s.%d", identifier, ((Declaration*)existing_variable->value->structure)->stack_declaration_offset + 1);

        node->data.function_parameters.name = converted_identifier;

        break;
      }

      Declaration *file_scope_declaration = malloc(sizeof(Declaration));
      file_scope_declaration->declaration_type = DECLARATION_TYPE_VARIABLE;
      file_scope_declaration->from_current_scope = true;
      file_scope_declaration->has_linkage = true;
      file_scope_declaration->name = identifier;
      file_scope_declaration->stack_declaration_offset = declaration_stack->count;

      add_declaration_to_table(file_scope_declaration, identifier, declaration_table);

      char *converted_identifier = malloc(IDENTIFIER_BUFFER);
      snprintf(converted_identifier, IDENTIFIER_BUFFER, "%s.%d", identifier, declaration_stack->count);
      node->data.function_parameters.name = converted_identifier;
      break;
    }
    case AST_BLOCK: {
      StackValue *new_block_stack_values = malloc(sizeof(StackValue) * declaration_stack->capacity);
      
      memcpy(new_block_stack_values, declaration_top_stack, sizeof(StackValue));
      stack_push(declaration_stack, *new_block_stack_values);
      
      for (int i = 0; i < node->data.block.block_count; i++) {   
        AstNode *block_item_node = node->data.block.block_ptrs->node_pointers[i];
        variable_resolve_node(block_item_node, declaration_stack); 
      }

      stack_pop(declaration_stack);

      break;
    }
    case AST_STATEMENT_COMPOUND:
      variable_resolve_node(node->data.compound_statement.block, declaration_stack);
      break;
    case AST_STATEMENT_RETURN:
      variable_resolve_node(node->data.return_statement.expression, declaration_stack);
      break;
    default:
      fprintf(stderr, "ERROR - SA Variable Resolution: AST type '%d' not supported\n", node->type);
      exit(1);
      break;
  }
} 

static void resolve_file_scope_variable_declaration(char *identifier, enum DeclarationType declaration_type, HashTable *declaration_table) {  
  HashTableEntry *table_entry = hash_table_get_entry(declaration_table, identifier);

  Declaration *file_scope_declaration = malloc(sizeof(Declaration));
  file_scope_declaration->declaration_type = declaration_type;
  file_scope_declaration->from_current_scope = true;
  file_scope_declaration->has_linkage = true;
  file_scope_declaration->name = identifier;
  file_scope_declaration->stack_declaration_offset = 1;

  add_declaration_to_table(file_scope_declaration, identifier, declaration_table);
}

static void resolve_local_scope_variable_declaration(AstNode *ast_node, enum DeclarationType declaration_type, HashTable *declaration_table, int stack_count) {  
  char *identifier = ast_node->data.variable_declaration.name;
  HashTableEntry *table_entry = hash_table_get_entry(declaration_table, identifier);

  if (table_entry != NULL && table_entry->key != NULL) {
    Declaration *previous_declaration = table_entry->value->structure;

    //TODO: Need to test
    if (previous_declaration->has_linkage == false || ast_node->data.variable_declaration.storage_class_type != AST_STORAGE_CLASS_EXTERN) {
      fprintf(stderr, "ERROR: SA Variable Resolution: Conflicting local '%s' declarations", identifier);
      exit(1);
    }
  }  

  if (ast_node->data.variable_declaration.storage_class_type == AST_STORAGE_CLASS_EXTERN) {
    Declaration *file_scope_declaration = malloc(sizeof(Declaration));
    file_scope_declaration->declaration_type = declaration_type;
    file_scope_declaration->from_current_scope = true;
    file_scope_declaration->has_linkage = true;
    file_scope_declaration->name = identifier;

    add_declaration_to_table(file_scope_declaration, identifier, declaration_table);

    return;
  }
  
  char *converted_identifier = malloc(IDENTIFIER_BUFFER);
  snprintf(converted_identifier, IDENTIFIER_BUFFER, "%s.%d", identifier, stack_count);

  Declaration *file_scope_declaration = malloc(sizeof(Declaration));
  file_scope_declaration->declaration_type = declaration_type;
  file_scope_declaration->from_current_scope = true;
  file_scope_declaration->has_linkage = false;
  file_scope_declaration->name = converted_identifier;

  add_declaration_to_table(file_scope_declaration, identifier, declaration_table);

  ast_node->data.variable_declaration.name = converted_identifier;
}

static void add_declaration_to_table(Declaration *declaration, char* identifier_key, HashTable *declaration_table) {
  HashValue *value = malloc(sizeof(HashValue));
  value->type = HASH_STRUCT;
  value->structure = declaration;

  HashTableEntry *entry = malloc(sizeof(HashTableEntry));
  entry->key = identifier_key;
  entry->value = value;

  hash_table_add_entry(declaration_table, entry);
}
