#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/sa_variable_resolution.h"
#include "../include/hash_table.h"
#include "../include/stack.h"
#include "../include/parser.h"

#define VARIABLE_RESOLUTION_STACK_SIZE 16
#define IDENTIFIER_BUFFER 256

enum DeclarationType {
  DECLARATION_TYPE_FUNCTION,
  DECLARATION_TYPE_VARIABLE
};

typedef struct {
  enum DeclarationType declaration_type;
  bool from_current_scope;
  bool has_linkage;
  int stack_declaration_offset;
} Declaration;

static void variable_resolve_node(AstNode *node, Stack *declaration_stack); 
static void resolve_file_scope_variable_declaration(char *identifier, enum DeclarationType declaration_type, HashTable *declaration_table);  
static void resolve_local_scope_variable_declaration(AstNode *ast_node, enum DeclarationType declaration_type, HashTable *declaration_table, int stack_count);   
static void add_declaration_to_table(Declaration *declaration, char* identifier_key, HashTable *declaration_table); 
static char* get_identifier_with_stack_offset(char *identifier, int stack_offset); 
static void push_new_declaration_stack(Stack *declaration_stack); 
static void resolve_function_parameter(AstNode *param_type_node, char **parameter_identifier, Stack *declaration_stack); 

void sa_variable_resolution(AstNode *ast_nodes) {
  Stack *declaration_stack = malloc(sizeof(Stack));
  stack_init(declaration_stack, VARIABLE_RESOLUTION_STACK_SIZE);

  HashTable *symbols_table = malloc(sizeof(HashTable));
  hash_table_init(symbols_table);

  StackValue *file_scope_stack = malloc(sizeof(StackValue));
  file_scope_stack->type = STACK_HASH_TABLE;
  file_scope_stack->data.hash_table = symbols_table;

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
    case AST_VARIABLE_DECLARATION: {
      StackValue *declaration_top_stack = stack_top(declaration_stack);
      HashTable *declaration_table = declaration_top_stack->data.hash_table;
      char *identifier = NULL;

      if (declaration_stack->count == 1) {
        identifier = node->data.variable_declaration.name;
      } else {
        identifier = get_identifier_with_stack_offset(node->data.variable_declaration.name, declaration_stack->count);
      }

      HashTableEntry *existing_variable = hash_table_get_entry(declaration_table, identifier);
      
      //Duplicate declarations at the file scope level are allowed. Only error when declarations in the same scope within functions are found
      if (existing_variable != NULL && existing_variable->key != NULL && declaration_stack->count != 1) {      
        if (((Declaration*)existing_variable->value->structure)->from_current_scope) {
          fprintf(stderr, "ERROR - SA Variable Resolution: Duplicate '%s' variable found in block\n", node->data.variable_declaration.name);
          exit(1);
        }
      }

      if (declaration_stack->count == 1) {
        //Don't process variable resolution for existing table entries for file scoped variables
        if (existing_variable == NULL || existing_variable->key == NULL) {
          resolve_file_scope_variable_declaration(node->data.variable_declaration.name, DECLARATION_TYPE_VARIABLE, declaration_table);
        }
      } else {
        resolve_local_scope_variable_declaration(node, DECLARATION_TYPE_VARIABLE, declaration_table, declaration_stack->count);   
      }

      if (node->data.variable_declaration.has_expression == true) {
        variable_resolve_node(node->data.variable_declaration.init_expression, declaration_stack);
      }      
      break;
    }
    case AST_FUNCTION_DECLARATION: {
      push_new_declaration_stack(declaration_stack);

      StackValue *declaration_top_stack = stack_top(declaration_stack);
      HashTable *declaration_table = declaration_top_stack->data.hash_table;
      
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
        new_declaration->declaration_type = DECLARATION_TYPE_FUNCTION;
        new_declaration->from_current_scope = true;
        new_declaration->has_linkage = true;
        new_declaration->stack_declaration_offset = declaration_stack->count;

        add_declaration_to_table(new_declaration, function_identifier, declaration_table);
      }

      //Add a new stack for the function variable declarations
      push_new_declaration_stack(declaration_stack);

      for (int i = 0; i < node->data.function_declaration.parameter_count; i++) {
        AstNode *param_type = &node->data.function_declaration.function_type->data.type.function_param_types[i];
        char *identifier = node->data.function_declaration.parameter_identifiers[i];
        resolve_function_parameter(param_type, &identifier, declaration_stack);
      }
  
      if (node->data.function_declaration.body_block != NULL) {
        variable_resolve_node(node->data.function_declaration.body_block, declaration_stack);
      }

      stack_pop(declaration_stack);
      break;
    }
    case AST_EXPRESSION_FUNCTION_CALL: {
      StackValue *declaration_top_stack = stack_top(declaration_stack);
      HashTable *declaration_table = declaration_top_stack->data.hash_table;
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
    case AST_BLOCK: {
      push_new_declaration_stack(declaration_stack);
      
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
    case AST_STATEMENT_IF: {
      variable_resolve_node(node->data.if_statement.condition_expression, declaration_stack);
      variable_resolve_node(node->data.if_statement.then_statement, declaration_stack);

      if (node->data.if_statement.else_statement != NULL) {
        variable_resolve_node(node->data.if_statement.else_statement, declaration_stack);
      }
      break;
    }
    case AST_STATEMENT_FOR: {
      push_new_declaration_stack(declaration_stack);

      if (node->data.for_statement.for_loop_init != NULL) {
        variable_resolve_node(node->data.for_statement.for_loop_init, declaration_stack);
      }

      if (node->data.for_statement.condition_expression != NULL) {
        variable_resolve_node(node->data.for_statement.condition_expression, declaration_stack);
      }

      if (node->data.for_statement.post_expression != NULL) {
        variable_resolve_node(node->data.for_statement.post_expression, declaration_stack);
      }

      variable_resolve_node(node->data.for_statement.statement_body, declaration_stack);

      stack_pop(declaration_stack);
      break;
    }
    case AST_STATEMENT_WHILE: {
      variable_resolve_node(node->data.while_statement.condition, declaration_stack);
      variable_resolve_node(node->data.while_statement.statement_body, declaration_stack);
      break;
    }
    case AST_STATEMENT_DO_WHILE: {
      variable_resolve_node(node->data.do_while_statement.condition, declaration_stack);
      variable_resolve_node(node->data.do_while_statement.statement_body, declaration_stack);
      break;
    }
    case AST_EXPRESSION_ASSIGNMENT: {
      if (node->data.assignement_expression.left_expression->type != AST_EXPRESSION_VARIABLE && node->data.assignement_expression.left_expression->type != AST_EXPRESSION_UNARY) {
        fprintf(stderr, "ERROR - SA Variable Resolution: Invalid LValue for assignment expression\n");
        exit(1);
      }

      variable_resolve_node(node->data.assignement_expression.left_expression, declaration_stack);
      variable_resolve_node(node->data.assignement_expression.right_expression, declaration_stack);
      break;
    }
    case AST_EXPRESSION_BINARY: {
      variable_resolve_node(node->data.binary_expression.left_expression, declaration_stack);
      variable_resolve_node(node->data.binary_expression.right_expression, declaration_stack);
      break;
    }
    case AST_EXPRESSION_POSTFIX_INCREMENT:
    case AST_EXPRESSION_POSTFIX_DECREMENT:
    case AST_EXPRESSION_PREFIX_INCREMENT:
    case AST_EXPRESSION_PREFIX_DECREMENT: 
      variable_resolve_node(node->data.increment_decrement_expression.expression, declaration_stack);
      break;
    case AST_EXPRESSION_UNARY:
      variable_resolve_node(node->data.unary_expression.expression, declaration_stack);
      break;
    case AST_EXPRESSION_CAST:
      variable_resolve_node(node->data.cast_expression.target_type, declaration_stack);
      break;
    case AST_EXPRESSION_VARIABLE: {
      StackValue *declaration_top_stack = stack_top(declaration_stack);
      HashTable *declaration_table = declaration_top_stack->data.hash_table;

      //Check to see if we already converted the identifier. Since we're adding '.' to identifiers as part of the semantic analysis variable resolution, check to see if the period exists.
      char *found_period = (char*)memchr(node->data.variable_expression.identifier, '.', strlen(node->data.variable_expression.identifier));
      HashTableEntry *entry;
      char *identifier = get_identifier_with_stack_offset(node->data.variable_expression.identifier, declaration_stack->count);
      
      entry = hash_table_get_entry(declaration_table, identifier);

      if (entry == NULL || entry->key == NULL)
      {
        identifier = node->data.variable_expression.identifier;
        entry = hash_table_get_entry(declaration_table, identifier);
      }
      
      if (entry == NULL || entry->key == NULL) {
        //Check if there is a parent declared variable by traversing backwards from the current stack offset.
        int stack_offset = declaration_stack->count - 1;

        while (stack_offset > 0) {
          char *previous_stack_identifier = get_identifier_with_stack_offset(node->data.variable_expression.identifier, stack_offset);
          entry = hash_table_get_entry(declaration_table, previous_stack_identifier);
          
          if (entry != NULL && entry->key != NULL) {
            identifier = previous_stack_identifier;
            break;
          }

          stack_offset--;
        }        
      } 

      if (entry == NULL || entry->key == NULL) {
        fprintf(stderr, "ERROR - SA Variable Resolution: Undeclared variable hash table entry for '%s'\n", node->data.variable_expression.identifier);
        exit(1);
      }
      node->data.variable_expression.identifier = identifier;
      break;
    }
  }
} 

static void resolve_file_scope_variable_declaration(char *identifier, enum DeclarationType declaration_type, HashTable *declaration_table) {  
  HashTableEntry *table_entry = hash_table_get_entry(declaration_table, identifier);

  Declaration *file_scope_declaration = malloc(sizeof(Declaration));
  file_scope_declaration->declaration_type = declaration_type;
  file_scope_declaration->from_current_scope = true;
  file_scope_declaration->has_linkage = true;
  file_scope_declaration->stack_declaration_offset = 1;

  add_declaration_to_table(file_scope_declaration, identifier, declaration_table);
}

static void resolve_local_scope_variable_declaration(AstNode *ast_node, enum DeclarationType declaration_type, HashTable *declaration_table, int stack_count) {  
  char *identifier = ast_node->data.variable_declaration.name;
  char *converted_identifier = malloc(IDENTIFIER_BUFFER);
  snprintf(converted_identifier, IDENTIFIER_BUFFER, "%s.%d", identifier, stack_count);
  HashTableEntry *table_entry = hash_table_get_entry(declaration_table, converted_identifier);

  if (table_entry != NULL && table_entry->key != NULL) {
    Declaration *previous_declaration = table_entry->value->structure;

    if (previous_declaration->from_current_scope) {
      if (!(previous_declaration->has_linkage && ast_node->data.variable_declaration.storage_class_type == AST_STORAGE_CLASS_EXTERN)) {
        fprintf(stderr, "ERROR: SA Variable Resolution: Conflicting local '%s' declarations\n", identifier);
        exit(1);
      }
    }
  }  

  if (ast_node->data.variable_declaration.storage_class_type == AST_STORAGE_CLASS_EXTERN) {
    Declaration *file_scope_declaration = malloc(sizeof(Declaration));
    file_scope_declaration->declaration_type = declaration_type;
    file_scope_declaration->from_current_scope = true;
    file_scope_declaration->has_linkage = true;

    add_declaration_to_table(file_scope_declaration, identifier, declaration_table);

    return;
  }

  if (table_entry == NULL || table_entry->key == NULL) {
    Declaration *file_scope_declaration = malloc(sizeof(Declaration));
    file_scope_declaration->declaration_type = declaration_type;
    file_scope_declaration->from_current_scope = true;
    file_scope_declaration->has_linkage = false;

    add_declaration_to_table(file_scope_declaration, converted_identifier, declaration_table);
  }

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

static char* get_identifier_with_stack_offset(char *identifier, int stack_offset) {
  char *converted_identifier = malloc(IDENTIFIER_BUFFER);
  snprintf(converted_identifier, IDENTIFIER_BUFFER, "%s.%d", identifier, stack_offset);

  return converted_identifier;
}

static void push_new_declaration_stack(Stack *declaration_stack) {
  StackValue *declaration_top_stack = stack_top(declaration_stack);
  HashTable *new_declaration_table = hash_table_clone(declaration_top_stack->data.hash_table);

  //Iterate through the new block table and reset the 'current_scope' flags since they will all be parent declarations
  for (int i = 0; i < new_declaration_table->capacity; i++) {
    HashValue *new_value = new_declaration_table->entries[i].value;

    if (new_value == NULL) continue;
  
    ((Declaration*)new_value->structure)->from_current_scope = false;
  }

  StackValue *new_stack_value = malloc(sizeof(StackValue));
  new_stack_value->data.hash_table = new_declaration_table;
  new_stack_value->type = STACK_HASH_TABLE;

  stack_push(declaration_stack, new_stack_value);
}

static void resolve_function_parameter(AstNode *param_type_node, char **parameter_identifier, Stack *declaration_stack) {
  if (param_type_node->data.type.type == AST_TYPE_VOID) {
    return;
  }

  StackValue *declaration_top_stack = stack_top(declaration_stack);
  HashTable *declaration_table = declaration_top_stack->data.hash_table;
  char* converted_identifier = get_identifier_with_stack_offset(*parameter_identifier, declaration_stack->count);
  HashTableEntry *existing_variable = hash_table_get_entry(declaration_table, converted_identifier);

  if (existing_variable != NULL && existing_variable->key != NULL) {
    if (declaration_stack->count == ((Declaration*)existing_variable->value->structure)->stack_declaration_offset) {
      fprintf(stderr, "ERROR - SA Variable Resolution: Duplicate '%s' function variable found\n", converted_identifier);
      exit(1);
    }      

    *parameter_identifier = converted_identifier;

    return;
  }

  Declaration *file_scope_declaration = malloc(sizeof(Declaration));
  file_scope_declaration->declaration_type = DECLARATION_TYPE_VARIABLE;
  file_scope_declaration->from_current_scope = true;
  file_scope_declaration->has_linkage = true;
  file_scope_declaration->stack_declaration_offset = declaration_stack->count;

  add_declaration_to_table(file_scope_declaration, converted_identifier, declaration_table);

  *parameter_identifier = converted_identifier;
}
