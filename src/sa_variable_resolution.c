#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/sa_variable_resolution.h"
#include "../include/hash_table.h"
#include "../include/stack.h"
#include "../include/parser.h"
#include "../include/error.h"

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

typedef struct {
  Stack *declaration_stack;
  int function_count;
  int block_count;
} VariableResolution; 

static VariableResolution* init_variable_resolution();
static void variable_resolve_node(AstNode *node, VariableResolution *variable_resolution); 
static void resolve_file_scope_variable_declaration(char *identifier, enum DeclarationType declaration_type, HashTable *declaration_table);  
static void resolve_local_scope_variable_declaration(AstNode *ast_node, enum DeclarationType declaration_type, VariableResolution *variable_resolution);   
static void add_declaration_to_table(Declaration *declaration, char* identifier_key, HashTable *declaration_table); //TODO: This will be moved to the type checker
static char* get_identifier_with_stack_offset(char *identifier, int stack_offset, int function_count, int block_count); 
static void push_new_declaration_stack(Stack *declaration_stack); 
static void resolve_function_parameter(TypeNode *param_type_node, AstNode *function_declaration_node, int identifier_idx, VariableResolution *variable_resolution); 
static void print_declaration_stack(Stack *declaration_stack);

void sa_variable_resolution(AstNode *ast_nodes) {
  VariableResolution *variable_resolution = init_variable_resolution();

  for (int i = 0; i < ast_nodes->data.program.declaration_count; i++) {
    AstNode *declaration_node = ast_nodes->data.program.declaration_ptrs->node_pointers[i];

    if (declaration_node->type == AST_FUNCTION_DECLARATION) {
      variable_resolution->function_count++;
    }

    variable_resolve_node(declaration_node, variable_resolution);
  }
}

static VariableResolution* init_variable_resolution() {
  Stack *declaration_stack = malloc(sizeof(Stack));
  stack_init(declaration_stack, VARIABLE_RESOLUTION_STACK_SIZE);

  HashTable *symbols_table = malloc(sizeof(HashTable));
  hash_table_init(symbols_table);

  StackValue *file_scope_stack = malloc(sizeof(StackValue));
  file_scope_stack->type = STACK_HASH_TABLE;
  file_scope_stack->data.hash_table = symbols_table;

  stack_push(declaration_stack, file_scope_stack);

  VariableResolution *variable_resolution = malloc(sizeof(VariableResolution));
  variable_resolution->declaration_stack = declaration_stack;
  variable_resolution->block_count = 0;
  variable_resolution->function_count = 0;

  return variable_resolution;
}

static void variable_resolve_node(AstNode *node, VariableResolution *variable_resolution) {
  switch (node->type) {
    case AST_VARIABLE_DECLARATION: {
      StackValue *declaration_top_stack = stack_top(variable_resolution->declaration_stack);
      HashTable *declaration_table = declaration_top_stack->data.hash_table;
      char *identifier = NULL;

      if (variable_resolution->declaration_stack->count == 1) {
        identifier = node->data.declaration_variable.name;
      } else {
        identifier = get_identifier_with_stack_offset(node->data.declaration_variable.name, variable_resolution->declaration_stack->count, variable_resolution->function_count, variable_resolution->block_count);
      }

      HashTableEntry *existing_variable = hash_table_get_entry(declaration_table, identifier);
      
      //Duplicate declarations at the file scope level are allowed. Only error when declarations in the same scope within functions are found
      if (existing_variable != NULL && existing_variable->key != NULL && variable_resolution->declaration_stack->count != 1) {      
        if (((Declaration*)existing_variable->value->structure)->from_current_scope) {
          input_error("Duplicate '%s' variable found in block", node->data.declaration_variable.name);
        }
      }

      if (variable_resolution->declaration_stack->count == 1) {
        //Don't process variable resolution for existing table entries for file scoped variables
        if (existing_variable == NULL || existing_variable->key == NULL) {
          resolve_file_scope_variable_declaration(node->data.declaration_variable.name, DECLARATION_TYPE_VARIABLE, declaration_table);
        }
      } else {
        resolve_local_scope_variable_declaration(node, DECLARATION_TYPE_VARIABLE, variable_resolution);   
      }

      if (node->data.declaration_variable.has_expression == true) {
        variable_resolve_node(node->data.declaration_variable.init_expression, variable_resolution);
      }      
      break;
    }
    case AST_FUNCTION_DECLARATION: {
      StackValue *declaration_top_stack = stack_top(variable_resolution->declaration_stack);
      HashTable *declaration_table = declaration_top_stack->data.hash_table;
      
      char *function_identifier = node->data.declaration_function.name;
      HashTableEntry *table_entry = hash_table_get_entry(declaration_table, function_identifier);
      
      if (table_entry != NULL && table_entry->key != NULL) {
        Declaration *previous_declaration = table_entry->value->structure;

        if (previous_declaration->from_current_scope && previous_declaration->has_linkage) {
          input_error("Duplicate function declaration '%s'", function_identifier);
        }        
      } else {
        Declaration *new_declaration = malloc(sizeof(Declaration));
        new_declaration->declaration_type = DECLARATION_TYPE_FUNCTION;
        new_declaration->from_current_scope = true;
        new_declaration->has_linkage = true;
        new_declaration->stack_declaration_offset = variable_resolution->declaration_stack->count - 1;

        add_declaration_to_table(new_declaration, function_identifier, declaration_table);
      }

      //Add a new stack for the function variable declarations
      push_new_declaration_stack(variable_resolution->declaration_stack);

      for (int i = 0; i < node->data.declaration_function.function_type->data.function_type.param_type_count; i++) {
        TypeNode *param_type = &node->data.declaration_function.function_type->data.function_type.param_types[i];
        resolve_function_parameter(param_type, node, i, variable_resolution);
      }
  
      if (node->data.declaration_function.body_block != NULL) {
        variable_resolve_node(node->data.declaration_function.body_block, variable_resolution);
      }

      stack_pop(variable_resolution->declaration_stack);
      break;
    }
    case AST_INITIALIZER:
      if (node->data.initializer.type == AST_INITIALIZER_SINGLE) {
        variable_resolve_node(node->data.initializer.initializer_node.single_init_expression, variable_resolution);
        break;
      } 

      for (int i = 0; i < node->data.initializer.initializer_node.compound_initializer->count; i++) {
        variable_resolve_node(&node->data.initializer.initializer_node.compound_initializer->items[i], variable_resolution);
      }
      break;
    case AST_EXPRESSION_FUNCTION_CALL: {
      StackValue *declaration_top_stack = stack_top(variable_resolution->declaration_stack);
      HashTable *declaration_table = declaration_top_stack->data.hash_table;
      HashTableEntry *table_entry = hash_table_get_entry(declaration_table, node->data.expression_function_call.identifier);

      if (table_entry == NULL || table_entry->key == NULL) {
        input_error("Undeclared function '%s'", node->data.expression_function_call.identifier);
      }

      for (int i = 0; i < node->data.expression_function_call.argument_count; i++) {
        AstNode *argument_node = node->data.expression_function_call.argument_ptrs->node_pointers[i];
        variable_resolve_node(argument_node, variable_resolution); 
      }      
      break;      
    }
    case AST_BLOCK: {
      variable_resolution->block_count++;
      push_new_declaration_stack(variable_resolution->declaration_stack);
      
      for (int i = 0; i < node->data.block.block_count; i++) {   
        AstNode *block_item_node = node->data.block.block_ptrs->node_pointers[i];
        variable_resolve_node(block_item_node, variable_resolution);
       }

      stack_pop(variable_resolution->declaration_stack);
      break;
    }
    case AST_STATEMENT_COMPOUND:
      variable_resolve_node(node->data.statement_compound.block, variable_resolution);
      break;
    case AST_STATEMENT_RETURN:
      variable_resolve_node(node->data.statement_return.expression, variable_resolution);
      break;
    case AST_STATEMENT_IF: {
      variable_resolve_node(node->data.statement_if.condition_expression, variable_resolution);
      variable_resolve_node(node->data.statement_if.then_statement, variable_resolution);

      if (node->data.statement_if.else_statement != NULL) {
        variable_resolve_node(node->data.statement_if.else_statement, variable_resolution);
      }
      break;
    }
    case AST_STATEMENT_FOR: {
      push_new_declaration_stack(variable_resolution->declaration_stack);

      if (node->data.statement_for.for_loop_init != NULL) {
        variable_resolve_node(node->data.statement_for.for_loop_init, variable_resolution);
      }

      if (node->data.statement_for.condition_expression != NULL) {
        variable_resolve_node(node->data.statement_for.condition_expression, variable_resolution);
      }

      if (node->data.statement_for.post_expression != NULL) {
        variable_resolve_node(node->data.statement_for.post_expression, variable_resolution);
      }

      variable_resolve_node(node->data.statement_for.statement_body, variable_resolution);

      stack_pop(variable_resolution->declaration_stack);
      break;
    }
    case AST_STATEMENT_WHILE: {
      variable_resolve_node(node->data.statement_while.condition, variable_resolution);
      variable_resolve_node(node->data.statement_while.statement_body, variable_resolution);
      break;
    }
    case AST_STATEMENT_DO_WHILE: {
      variable_resolve_node(node->data.statement_do_while.condition, variable_resolution);
      variable_resolve_node(node->data.statement_do_while.statement_body, variable_resolution);
      break;
    }
    case AST_EXPRESSION_ASSIGNMENT: {
      //TODO: This will be moved to the type checker
      // if (node->data.expression_assignment.left_expression->type != AST_EXPRESSION_VARIABLE && node->data.expression_assignment.left_expression->type != AST_EXPRESSION_UNARY) {
      //   fprintf(stderr, "ERROR - SA Variable Resolution: Invalid LValue for assignment expression\n");
      //   exit(1);
      // }

      variable_resolve_node(node->data.expression_assignment.left_expression, variable_resolution);
      variable_resolve_node(node->data.expression_assignment.right_expression, variable_resolution);
      break;
    }
    case AST_EXPRESSION_BINARY: {
      variable_resolve_node(node->data.expression_binary.left_expression, variable_resolution);
      variable_resolve_node(node->data.expression_binary.right_expression, variable_resolution);
      break;
    }
    case AST_EXPRESSION_CONDITIONAL:
      variable_resolve_node(node->data.expression_conditional.condition, variable_resolution);
      variable_resolve_node(node->data.expression_conditional.true_expression, variable_resolution);
      variable_resolve_node(node->data.expression_conditional.false_expression, variable_resolution);
      break;
    case AST_EXPRESSION_POSTFIX_INCREMENT:
    case AST_EXPRESSION_POSTFIX_DECREMENT:
    case AST_EXPRESSION_PREFIX_INCREMENT:
    case AST_EXPRESSION_PREFIX_DECREMENT: 
      variable_resolve_node(node->data.expression_increment_decrement.expression, variable_resolution);
      break;
    case AST_EXPRESSION_UNARY:
      variable_resolve_node(node->data.expression_unary.expression, variable_resolution);
      break;
    case AST_EXPRESSION_CAST:
      variable_resolve_node(node->data.expression_cast.expression, variable_resolution);
      break;
    case AST_EXPRESSION_ADDRESS_OF:
      variable_resolve_node(node->data.expression_address_of.expression, variable_resolution);
      break;
    case AST_EXPRESSION_DEREFERENCE:
      variable_resolve_node(node->data.expression_dereference.expression, variable_resolution);
      break;
    case AST_EXPRESSION_SUBSCRIPT:
      variable_resolve_node(node->data.expression_subscript.expression_1, variable_resolution);
      variable_resolve_node(node->data.expression_subscript.expression_2, variable_resolution);
      break;
    case AST_EXPRESSION_VARIABLE: {
      StackValue *declaration_top_stack = stack_top(variable_resolution->declaration_stack);
      HashTable *declaration_table = declaration_top_stack->data.hash_table;

      //Check to see if we already converted the identifier. Since we're adding '.' to identifiers as part of the semantic analysis variable resolution, check to see if the period exists.
      char *found_period = (char*)memchr(node->data.expression_variable.identifier, '.', strlen(node->data.expression_variable.identifier));
      HashTableEntry *entry;
      char *identifier = get_identifier_with_stack_offset(node->data.expression_variable.identifier, variable_resolution->declaration_stack->count, variable_resolution->function_count, variable_resolution->block_count);
      
      entry = hash_table_get_entry(declaration_table, identifier);

      if (entry == NULL || entry->key == NULL)
      {
        identifier = node->data.expression_variable.identifier;
        entry = hash_table_get_entry(declaration_table, identifier);
      }

      if (entry != NULL && entry->key != NULL) {
        Declaration *declaration = entry->value->structure;

        //If the found entry record is a shadowed function that shares the same name as the variable, reset the entry record
        if (declaration->declaration_type == DECLARATION_TYPE_FUNCTION) {
          entry = NULL;
        }
      }
      
      if (entry == NULL || entry->key == NULL) {
        //Check if there is a parent declared variable by traversing backwards from the current stack offset.
        int stack_offset = variable_resolution->declaration_stack->count;
        int current_block_count = variable_resolution->block_count;

        bool entry_found = false;

        while (stack_offset > 0) {
          for (int i = variable_resolution->block_count; i >= 0; i--) {
            char *previous_stack_identifier = get_identifier_with_stack_offset(node->data.expression_variable.identifier, stack_offset, variable_resolution->function_count, i);

            entry = hash_table_get_entry(declaration_table, previous_stack_identifier);
          
            if (entry != NULL && entry->key != NULL) {
              identifier = previous_stack_identifier;
              entry_found = true;
              break;
            }
          }

          if (entry_found) {
            break;
          }

          stack_offset--;
        }        
      } 

      if (entry == NULL || entry->key == NULL) {
        panic("Undeclared variable hash table entry for '%s'", node->data.expression_variable.identifier);
      }
      node->data.expression_variable.identifier = identifier;
      break;
    }
    case AST_EXPRESSION_CONSTANT:
    case AST_STATEMENT_NULL:
    case AST_STATEMENT_CONTINUE:
    case AST_STATEMENT_BREAK:
    case AST_STATEMENT_GOTO:
    case AST_STATEMENT_GOTO_LABEL:
      break;
    default:
      panic("Unsupported AST Type '%d' when resolving node", node->type);
      break;
  }
} 

static void resolve_file_scope_variable_declaration(char *identifier, enum DeclarationType declaration_type, HashTable *declaration_table) {  
  HashTableEntry *table_entry = hash_table_get_entry(declaration_table, identifier);

  Declaration *file_scope_declaration = malloc(sizeof(Declaration));
  file_scope_declaration->declaration_type = declaration_type;
  file_scope_declaration->from_current_scope = true;
  file_scope_declaration->has_linkage = true;
  file_scope_declaration->stack_declaration_offset = 0;

  add_declaration_to_table(file_scope_declaration, identifier, declaration_table);
}

static void resolve_local_scope_variable_declaration(AstNode *ast_node, enum DeclarationType declaration_type, VariableResolution *variable_resolution) {  
  char *identifier = ast_node->data.declaration_variable.name;
  char *converted_identifier = malloc(IDENTIFIER_BUFFER);
  snprintf(converted_identifier, IDENTIFIER_BUFFER, "%s.%d.%d.%d", identifier, variable_resolution->declaration_stack->count, variable_resolution->function_count, variable_resolution->block_count);
  
  HashTable *declaration_table = stack_top(variable_resolution->declaration_stack)->data.hash_table;  
  HashTableEntry *table_entry = hash_table_get_entry(declaration_table, converted_identifier);

  if (table_entry != NULL && table_entry->key != NULL) {
    Declaration *previous_declaration = table_entry->value->structure;

    if (previous_declaration->from_current_scope) {
      if (!(previous_declaration->has_linkage && ast_node->data.declaration_variable.storage_class_type == AST_STORAGE_CLASS_EXTERN)) {
        input_error("Conflicting local '%s' declarations", identifier);
      }
    }
  }  

  if (ast_node->data.declaration_variable.storage_class_type == AST_STORAGE_CLASS_EXTERN) {
    Declaration *file_scope_declaration = malloc(sizeof(Declaration));
    file_scope_declaration->declaration_type = declaration_type;
    file_scope_declaration->from_current_scope = true;
    file_scope_declaration->has_linkage = true;

    add_declaration_to_table(file_scope_declaration, converted_identifier, declaration_table);

    return;
  }

  if (table_entry == NULL || table_entry->key == NULL) {
    Declaration *file_scope_declaration = malloc(sizeof(Declaration));
    file_scope_declaration->declaration_type = declaration_type;
    file_scope_declaration->from_current_scope = true;
    file_scope_declaration->has_linkage = false;

    add_declaration_to_table(file_scope_declaration, converted_identifier, declaration_table);
  }

  ast_node->data.declaration_variable.name = converted_identifier;
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

static char* get_identifier_with_stack_offset(char *identifier, int stack_offset, int function_count, int block_count) {
  char *converted_identifier = malloc(IDENTIFIER_BUFFER);
  snprintf(converted_identifier, IDENTIFIER_BUFFER, "%s.%d.%d.%d", identifier, stack_offset, function_count, block_count);

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

static void resolve_function_parameter(TypeNode *param_type_node, AstNode *function_declaration_node, int identifier_idx, VariableResolution *variable_resolution) {
  if (param_type_node->type == TYPE_VOID) {
    return;
  }

  StackValue *declaration_top_stack = stack_top(variable_resolution->declaration_stack);
  HashTable *declaration_table = declaration_top_stack->data.hash_table;

  char* converted_identifier = get_identifier_with_stack_offset(function_declaration_node->data.declaration_function.parameter_identifiers[identifier_idx], variable_resolution->declaration_stack->count, variable_resolution->function_count, 0);

  HashTableEntry *existing_variable = hash_table_get_entry(declaration_table, converted_identifier);

  if (existing_variable != NULL && existing_variable->key != NULL) {
    if (variable_resolution->declaration_stack->count == ((Declaration*)existing_variable->value->structure)->stack_declaration_offset) {
      input_error("Duplicate '%s' function variable found", converted_identifier);
    }      

    function_declaration_node->data.declaration_function.parameter_identifiers[identifier_idx] = converted_identifier;

    return;
  }

  Declaration *file_scope_declaration = malloc(sizeof(Declaration));
  file_scope_declaration->declaration_type = DECLARATION_TYPE_VARIABLE;
  file_scope_declaration->from_current_scope = true;
  file_scope_declaration->has_linkage = true;
  file_scope_declaration->stack_declaration_offset = variable_resolution->declaration_stack->count - 1;

  add_declaration_to_table(file_scope_declaration, converted_identifier, declaration_table);

  function_declaration_node->data.declaration_function.parameter_identifiers[identifier_idx] = converted_identifier;
}

static void print_declaration_stack(Stack *declaration_stack) {
  if (declaration_stack->count == 0) {
    printf("Stack Count is 0");
    return;
  }
  
  for (int i = 0; i < declaration_stack->count; i++) {
    printf("Stack Offset: %d\n", i);

    HashTable *declaration_hash_table = declaration_stack->stack[i].data.hash_table;

    for (int j = 0; j < declaration_hash_table->capacity; j++) {
      if (&declaration_hash_table->entries[j] == NULL || declaration_hash_table->entries[j].key == NULL) {
        continue;
      }  

      Declaration *declaration = declaration_hash_table->entries[j].value->structure;

      printf("\t- key: %s\n", declaration_hash_table->entries[j].key);
      printf("\t\ttype: %d, from_current_scope: %d, has_linkage: %d, offset:%d \n", declaration->declaration_type, declaration->from_current_scope,  declaration->has_linkage, declaration->stack_declaration_offset);
    }
  }
}
