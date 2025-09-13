#include <stdlib.h>
#include <stdio.h>
#include "../include/declaration_symbol.h"

void declaration_symbol_table_init(DeclarationSymbolTable *declaration_symbol_table) {
  HashTable *symbol_table = malloc(sizeof(HashTable));
  hash_table_init(symbol_table);
  
  Arena *declaration_symbol_arena = malloc(sizeof(Arena));
  arena_init(declaration_symbol_arena, sizeof(DeclarationSymbol), sizeof(DeclarationSymbol) * 1000, true);
  Arena *variable_symbol_arena = malloc(sizeof(Arena));
  arena_init(variable_symbol_arena, sizeof(VariableSymbol), sizeof(VariableSymbol) * 1000, true);
  Arena *function_symbol_arena = malloc(sizeof(Arena));
  arena_init(function_symbol_arena, sizeof(FunctionSymbol), sizeof(FunctionSymbol) * 1000, true);

  declaration_symbol_table->symbol_table = symbol_table;
  declaration_symbol_table->declaration_symbol_arena = declaration_symbol_arena;
  declaration_symbol_table->variable_symbol_arena = variable_symbol_arena;
  declaration_symbol_table->function_symbol_arena = function_symbol_arena;
}

void declaration_symbol_table_free(DeclarationSymbolTable *declaration_symbol_table) {
  arena_free(declaration_symbol_table->variable_symbol_arena);
  arena_free(declaration_symbol_table->declaration_symbol_arena);
  arena_free(declaration_symbol_table->function_symbol_arena);
  free(declaration_symbol_table->symbol_table);
}

DeclarationSymbol* add_function_declaration_symbol(DeclarationSymbolTable *declaration_symbol_table, char *function_name, DeclarationSymbolValueType function_value_type, int parameter_count, bool is_global, bool is_defined) {
  FunctionSymbol *function_symbol = arena_alloc(declaration_symbol_table->function_symbol_arena);

  function_symbol->value_type = function_value_type;
  function_symbol->is_defined = is_defined;
  function_symbol->is_global = is_global;
  function_symbol->param_count = parameter_count;

  DeclarationSymbol *function_declaration_symbol = arena_alloc(declaration_symbol_table->declaration_symbol_arena);
  function_declaration_symbol->symbol_type = DECLARATION_SYMBOL_FUNCTION;
  function_declaration_symbol->data.function_symbol = function_symbol;

  HashValue *new_value = malloc(sizeof(HashValue));
  new_value->type = HASH_STRUCT;
  new_value->structure = function_declaration_symbol;

  HashTableEntry *new_entry = malloc(sizeof(HashTableEntry));
  new_entry->key = function_name;
  new_entry->value = new_value;

  hash_table_add_entry(declaration_symbol_table->symbol_table, new_entry);

  return function_declaration_symbol;
}

void add_automatic_variable_declaration_symbol(DeclarationSymbolTable *declaration_symbol_table, DeclarationSymbolValueType value_type, char *symbol_key) {  
  VariableSymbol *variable_symbol = arena_alloc(declaration_symbol_table->variable_symbol_arena);
  variable_symbol->value_type = value_type;
  variable_symbol->is_automatic_storage_duration = true;

  DeclarationSymbol *declaration_symbol = arena_alloc(declaration_symbol_table->declaration_symbol_arena);
  declaration_symbol->symbol_type = DECLARATION_SYMBOL_VARIABLE;
  declaration_symbol->data.variable_symbol = variable_symbol;

  HashTableEntry *entry = malloc(sizeof(HashTableEntry));
  entry->key = symbol_key;

  HashValue *value = malloc(sizeof(HashValue));
  value->type = HASH_STRUCT;
  value->structure = declaration_symbol;

  entry->value = value;

  hash_table_add_entry(declaration_symbol_table->symbol_table, entry); 
}

void add_static_variable_declaration_symbol(DeclarationSymbolTable *declaration_symbol_table, DeclarationSymbolValueType value_type, InitialValue initial_value, char *symbol_key, bool is_global, InitialValueType initial_value_type) {  
  VariableSymbol *variable_symbol = arena_alloc(declaration_symbol_table->variable_symbol_arena);
  variable_symbol->is_automatic_storage_duration = false;
  variable_symbol->value_type = value_type;
  variable_symbol->static_initial_value = initial_value;
  variable_symbol->static_is_global = is_global;
  variable_symbol->static_initial_type = initial_value_type;
  
  DeclarationSymbol *declaration_symbol = arena_alloc(declaration_symbol_table->declaration_symbol_arena);
  declaration_symbol->symbol_type = DECLARATION_SYMBOL_VARIABLE;
  declaration_symbol->data.variable_symbol = variable_symbol;

  HashValue *new_value = malloc(sizeof(HashValue));
  new_value->type = HASH_STRUCT;
  new_value->structure = declaration_symbol;

  HashTableEntry *new_entry = malloc(sizeof(HashTableEntry));
  new_entry->key = symbol_key;
  new_entry->value = new_value;

  hash_table_add_entry(declaration_symbol_table->symbol_table, new_entry);
}

void add_static_extern_variable_declaration_symbol(DeclarationSymbolTable *declaration_symbol_table, DeclarationSymbolValueType value_type, char *symbol_key) {
  VariableSymbol *variable_symbol = arena_alloc(declaration_symbol_table->variable_symbol_arena);
  variable_symbol->is_automatic_storage_duration = false;
  variable_symbol->value_type = value_type;

  DeclarationSymbol *declaration_symbol = arena_alloc(declaration_symbol_table->declaration_symbol_arena);;
  declaration_symbol->symbol_type = DECLARATION_SYMBOL_VARIABLE;
  declaration_symbol->data.variable_symbol = variable_symbol;

  variable_symbol->static_is_global = true;
  variable_symbol->static_initial_type = INITIAL_VALUE_NO_INITIALIZER;

  HashValue *new_value = malloc(sizeof(HashValue));
  new_value->type = HASH_STRUCT;
  new_value->structure = declaration_symbol;

  HashTableEntry *new_entry = malloc(sizeof(HashTableEntry));
  new_entry->key = symbol_key;
  new_entry->value = new_value;

  hash_table_add_entry(declaration_symbol_table->symbol_table, new_entry);
}   

void declaration_symbol_table_print(DeclarationSymbolTable *declaration_symbol_table) {
  printf("Declaration Table: \n");

  for (int i = 0; i < declaration_symbol_table->symbol_table->capacity; i++) {
    if (declaration_symbol_table->symbol_table->entries[i].key == NULL) {
      continue;
    }
    
    printf("index: %d\tkey: %s \t", i, declaration_symbol_table->symbol_table->entries[i].key);    
    
    HashValue *hash_value = declaration_symbol_table->symbol_table->entries[i].value;
    DeclarationSymbol *symbol = hash_value->structure;

    if (symbol->symbol_type == DECLARATION_SYMBOL_VARIABLE) {
      printf("type: Variable\n");
      printf("\tvalue_type: ");

      switch (symbol->data.variable_symbol->value_type) {
        case DECLARATION_SYMBOL_TYPE_INT:     printf("int\n"); break;
        case DECLARATION_SYMBOL_TYPE_LONG:    printf("long\n"); break;
        case DECLARATION_SYMBOL_TYPE_UINT:    printf("uint\n"); break;
        case DECLARATION_SYMBOL_TYPE_ULONG:   printf("ulong\n"); break;
        case DECLARATION_SYMBOL_TYPE_VOID:    printf("void\n"); break;
        default:
          fprintf(stderr, "ERROR - Declaration Symbol: Unsupported value type '%d' when attempting to print\n", symbol->data.variable_symbol->value_type);
          exit(1);
      }      

      printf("\tis_automatic_storage_duration: %d\n", symbol->data.variable_symbol->is_automatic_storage_duration);

      if (symbol->data.variable_symbol->is_automatic_storage_duration) {
        continue;
      }

      printf("\tstatic_initial_type: ");

      switch(symbol->data.variable_symbol->static_initial_type) {
        case INITIAL_VALUE_INITIALIZED:     printf("Initialized\n"); break;
        case INITIAL_VALUE_NO_INITIALIZER:  printf("Not Initialized\n"); break;
        case INITIAL_VALUE_TENTATIVE:       printf("Tentative\n"); break;
      }

      printf("\tstatic_initial_value: ");

      switch (symbol->data.variable_symbol->value_type) {
        case DECLARATION_SYMBOL_TYPE_INT:    printf("%d\n", symbol->data.variable_symbol->static_initial_value.int_value); break;
        case DECLARATION_SYMBOL_TYPE_UINT:   printf("%d\n", symbol->data.variable_symbol->static_initial_value.uint_value); break;
        case DECLARATION_SYMBOL_TYPE_LONG:   printf("%ld\n", symbol->data.variable_symbol->static_initial_value.long_value); break;
        case DECLARATION_SYMBOL_TYPE_ULONG:  printf("%ld\n", symbol->data.variable_symbol->static_initial_value.ulong_value); break;
        default:
          fprintf(stderr, "ERROR - Declaration Symbol: Unsupported value type '%d' when attempting to print\n", symbol->data.variable_symbol->value_type);
          exit(1);
      }

      printf("\tstatic_is_global: %d\n", symbol->data.variable_symbol->static_is_global);

    } else {
      printf("type: Function\n");
      printf("\tvalue_type: ");

      switch (symbol->data.function_symbol->value_type) {
        case DECLARATION_SYMBOL_TYPE_INT:    printf("int\n"); break;
        case DECLARATION_SYMBOL_TYPE_UINT:    printf("uint\n"); break;
        case DECLARATION_SYMBOL_TYPE_LONG:   printf("long\n"); break;
        case DECLARATION_SYMBOL_TYPE_ULONG:   printf("ulong\n"); break;
        case DECLARATION_SYMBOL_TYPE_VOID:   printf("void\n"); break;
        default:
          fprintf(stderr, "ERROR - Declaration Symbol: Unsupported function value type '%d' when attempting to print\n", symbol->data.function_symbol->value_type);
          exit(1);
      }      

      printf("\tparam_count: %d\n", symbol->data.function_symbol->param_count);
      printf("\tis_defined: %d\n", symbol->data.function_symbol->is_defined);
      printf("\tis_global: %d\n", symbol->data.function_symbol->is_global);
    }
  }
}
