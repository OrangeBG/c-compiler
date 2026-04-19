#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "../include/symbol.h"
#include "../include/error.h"

void symbol_table_init(SymbolTable *symbol_table) {
  HashTable *symbol_hash_table = malloc(sizeof(HashTable));
  hash_table_init(symbol_hash_table);
  
  //TODO: Hard coded allocation count
  Arena *symbol_arena = malloc(sizeof(Arena));
  arena_init(symbol_arena, sizeof(Symbol), sizeof(Symbol) * 1000, true);
  symbol_table->symbol_table = symbol_hash_table;
  symbol_table->symbol_arena = symbol_arena;
}

void symbol_table_free(SymbolTable *symbol_table) {
  arena_free(symbol_table->symbol_arena);
  free(symbol_table->symbol_table);
}

Symbol* get_symbol(char *identifier, SymbolTable *symbol_table, bool error_if_null) {
  HashTableEntry *symbol_entry = hash_table_get_entry(symbol_table->symbol_table, identifier);

  if (symbol_entry == NULL || symbol_entry->key == NULL) {
    if (error_if_null) {
      panic("Symbol '%s' not found in symbol table", identifier);
    }

    return NULL;
  }

  return symbol_entry->value->structure;  
}

Symbol* add_function_symbol(SymbolTable *symbol_table, char *function_name, TypeNode *function_value_type, int parameter_count, TypeNode *param_types, bool is_global, bool is_defined) {
  Symbol *symbol = arena_alloc(symbol_table->symbol_arena);

  symbol->type = SYMBOL_FUNCTION;
  symbol->value_type = function_value_type;
  symbol->data.function_symbol.is_defined = is_defined;
  symbol->data.function_symbol.is_global = is_global;
  symbol->data.function_symbol.param_count = parameter_count;
  symbol->data.function_symbol.param_types = param_types;

  HashValue *new_value = malloc(sizeof(HashValue));
  new_value->type = HASH_STRUCT;
  new_value->structure = symbol;

  HashTableEntry *new_entry = malloc(sizeof(HashTableEntry));
  new_entry->key = function_name;
  new_entry->value = new_value;

  hash_table_add_entry(symbol_table->symbol_table, new_entry);

  return symbol;
}

Symbol* add_local_symbol(SymbolTable *symbol_table, TypeNode *value_type, char *symbol_key) {  
  Symbol *symbol = arena_alloc(symbol_table->symbol_arena);
  symbol->type = SYMBOL_LOCAL;
  symbol->value_type = value_type;

  HashTableEntry *entry = malloc(sizeof(HashTableEntry));
  entry->key = symbol_key;

  HashValue *value = malloc(sizeof(HashValue));
  value->type = HASH_STRUCT;
  value->structure = symbol;

  entry->value = value;

  hash_table_add_entry(symbol_table->symbol_table, entry); 

  return symbol;
}

Symbol* add_static_symbol(SymbolTable *symbol_table, TypeNode *value_type, InitialValueArray *initial_value_array, char *symbol_key, bool is_global, InitializationType initial_value_type) {  
  Symbol *symbol = arena_alloc(symbol_table->symbol_arena);

  symbol->type = SYMBOL_STATIC;
  symbol->value_type = value_type;
  symbol->data.static_symbol.initial_value_array = initial_value_array;
  symbol->data.static_symbol.is_global = is_global;
  symbol->data.static_symbol.initialization_type = initial_value_type;

  HashValue *new_value = malloc(sizeof(HashValue));
  new_value->type = HASH_STRUCT;
  new_value->structure = symbol;

  HashTableEntry *new_entry = malloc(sizeof(HashTableEntry));
  new_entry->key = symbol_key;
  new_entry->value = new_value;

  hash_table_add_entry(symbol_table->symbol_table, new_entry);

  return symbol;
}

Symbol* add_constant_symbol(SymbolTable *symbol_table, TypeNode *value_type, InitialValue *initial_value, char *symbol_key) {
  Symbol *symbol = arena_alloc(symbol_table->symbol_arena);

  symbol->type = SYMBOL_CONSTANT;
  symbol->value_type = value_type;
  symbol->data.constant_symbol.static_initial_value = initial_value;

  HashValue *new_value = malloc(sizeof(HashValue));
  new_value->type = HASH_STRUCT;
  new_value->structure = symbol;

  HashTableEntry *new_entry = malloc(sizeof(HashTableEntry));
  new_entry->key = symbol_key;
  new_entry->value = new_value;

  hash_table_add_entry(symbol_table->symbol_table, new_entry);

  return symbol;
}

void add_static_extern_symbol(SymbolTable *symbol_table, TypeNode *value_type, char *symbol_key) {
  Symbol *variable_symbol = arena_alloc(symbol_table->symbol_arena);

  Symbol *symbol = arena_alloc(symbol_table->symbol_arena);
  symbol->type = SYMBOL_STATIC;
  symbol->value_type = value_type;
  symbol->data.static_symbol.is_global = true;
  symbol->data.static_symbol.initialization_type = INITIALIZATION_TYPE_NO_INITIALIZER;

  HashValue *new_value = malloc(sizeof(HashValue));
  new_value->type = HASH_STRUCT;
  new_value->structure = symbol;

  HashTableEntry *new_entry = malloc(sizeof(HashTableEntry));
  new_entry->key = symbol_key;
  new_entry->value = new_value;

  hash_table_add_entry(symbol_table->symbol_table, new_entry);
}   

void symbol_table_print(SymbolTable *symbol_table) {
  printf("Symbol Table: \n");

  for (int i = 0; i < symbol_table->symbol_table->capacity; i++) {
    if (symbol_table->symbol_table->entries[i].key == NULL) {
      continue;
    }
    
    printf("index: %d\tkey: %s \t", i, symbol_table->symbol_table->entries[i].key);    
    
    HashValue *hash_value = symbol_table->symbol_table->entries[i].value;
    Symbol *symbol = hash_value->structure;

    //@Temporary: Work on reimplementing this after symbol update
    // if (symbol->symbol_type == SYMBOL_VARIABLE) {
    //   printf("type: Variable\n");
    //   printf("\tvalue_type: ");
    //   print_type_node(symbol->data.variable_symbol->value_type);
    //   printf("\n");

    //   printf("\tis_automatic_storage_duration: %d\n", symbol->data.variable_symbol->is_automatic_storage_duration);

    //   if (symbol->data.variable_symbol->is_automatic_storage_duration) {
    //     continue;
    //   }

    //   printf("\tstatic_initialization_type: ");

    //   switch(symbol->data.variable_symbol->static_initialization_type) {
    //     case INITIALIZATION_TYPE_INITIALIZED:     printf("Initialized\n"); break;
    //     case INITIALIZATION_TYPE_NO_INITIALIZER:  printf("Not Initialized\n"); break;
    //     case INITIALIZATION_TYPE_TENTATIVE:       printf("Tentative\n"); break;
    //   }

    //   printf("\tstatic_initial_value(s): \n");

    //   TypeNode *variable_value_type = symbol->data.variable_symbol->value_type;

    //   if (variable_value_type->type == TYPE_ARRAY) {
    //     variable_value_type = variable_value_type->data.array_type.element_type;
    //   } 
      
    //   for (int i = 0; i < symbol->data.variable_symbol->static_initial_value_array->count; i++) {
    //     switch (symbol->data.variable_symbol->static_initial_value_array->items[i].type) {
    //       case INITIAL_VALUE_TYPE_INT:            
    //         printf("\t\tint %d\n", symbol->data.variable_symbol->static_initial_value_array->items[i].data.int_value);
    //         break;
    //       case INITIAL_VALUE_TYPE_UINT:
    //         printf("\t\tuint %d\n", symbol->data.variable_symbol->static_initial_value_array->items[i].data.uint_value);
    //         break;
    //       case INITIAL_VALUE_TYPE_LONG:
    //         printf("\t\tlong %ld\n", symbol->data.variable_symbol->static_initial_value_array->items[i].data.long_value);
    //         break;
    //       case INITIAL_VALUE_TYPE_ULONG:
    //         printf("\t\tulong %ld\n", symbol->data.variable_symbol->static_initial_value_array->items[i].data.ulong_value);
    //         break;
    //       case INITIAL_VALUE_TYPE_DOUBLE:
    //         printf("\t\tdouble %f\n", symbol->data.variable_symbol->static_initial_value_array->items[i].data.double_value);
    //         break;
    //       case INITIAL_VALUE_TYPE_ZERO_INIT:            
    //         printf("\t\tzero init bytes %d\n", symbol->data.variable_symbol->static_initial_value_array->items[i].data.zero_init_array_bytes);
    //         break;
    //       case INITIAL_VALUE_TYPE_STRING:
    //         printf("\t\tstring %s (is null terminated: %d)\n", symbol->data.variable_symbol->static_initial_value_array->items[i].data.string_value.string_value, symbol->data.variable_symbol->static_initial_value_array->items[i].data.string_value.is_null_terminated);
    //         break;
    //       case INITIAL_VALUE_TYPE_CHAR:
    //         printf("\t\tchar %c\n", symbol->data.variable_symbol->static_initial_value_array->items[i].data.char_value);
    //         break;            
    //       case INITIAL_VALUE_TYPE_UCHAR:
    //         printf("\t\tunsigned char %c\n", symbol->data.variable_symbol->static_initial_value_array->items[i].data.uchar_value);
    //         break;            
    //       default:
    //         panic("Unsupported value type '%d' when attempting to print", symbol->data.variable_symbol->value_type->type);
    //       }
    //     }

    //   printf("\tstatic_is_global: %d\n", symbol->data.variable_symbol->static_is_global);

    // } else {
    //   printf("type: Function\n");
    //   printf("\tvalue_type: ");

    //   switch (symbol->data.function_symbol->value_type->type) {
    //     case TYPE_INT:      printf("int\n"); break;
    //     case TYPE_UINT:     printf("uint\n"); break;
    //     case TYPE_LONG:     printf("long\n"); break;
    //     case TYPE_ULONG:    printf("ulong\n"); break;
    //     case TYPE_VOID:     printf("void\n"); break;
    //     case TYPE_DOUBLE:   printf("double\n"); break;
    //     case TYPE_POINTER:  printf("pointer\n"); break;
    //     default:
    //       panic("Unsupported function value type '%d' when attempting to print", symbol->data.function_symbol->value_type->type);
    //   }      

    //   printf("\tparam_count: %d\n", symbol->data.function_symbol->param_count);
    //   printf("\tis_defined: %d\n", symbol->data.function_symbol->is_defined);
    //   printf("\tis_global: %d\n", symbol->data.function_symbol->is_global);
    // }
  }
}

InitialValue* symbol_initialize_to_zero(TypeNode *type_node) {
  InitialValue *initial_value = malloc(sizeof(InitialValue));

  switch (type_node->type) {
    case TYPE_INT:
      initial_value->type = INITIAL_VALUE_TYPE_INT;
      initial_value->data.int_value = 0;
      break;
    case TYPE_UINT:
      initial_value->type = INITIAL_VALUE_TYPE_UINT;
      initial_value->data.uint_value = 0;
      break;
    case TYPE_LONG:
      initial_value->type = INITIAL_VALUE_TYPE_LONG;
      initial_value->data.long_value = 0;
      break;
    case TYPE_ULONG:
      initial_value->type = INITIAL_VALUE_TYPE_ULONG;
      initial_value->data.ulong_value = 0;
      break;
    case TYPE_DOUBLE:
      initial_value->type = INITIAL_VALUE_TYPE_DOUBLE;
      initial_value->data.double_value = 0;
      break;
    case TYPE_POINTER:
      initial_value->type = INITIAL_VALUE_TYPE_ULONG;
      initial_value->data.ulong_value = 0;
      break;
    case TYPE_ARRAY:
      initial_value->type = INITIAL_VALUE_TYPE_ZERO_INIT;
      TypeNode *cur_type = type_node;
      unsigned long total_size = 0;

      while (true) {
        total_size += cur_type->data.array_type.size;

        if (cur_type->data.array_type.element_type->type != TYPE_ARRAY) {
          //@Debt: Total size is being narrowed from long to an int
          initial_value->data.zero_init_array_bytes = total_size * get_type_size(cur_type->data.array_type.element_type);
          break;
        }

        cur_type = cur_type->data.array_type.element_type;
      }

      break;
    default:
      panic("Unsupported initial value Type '%d'", type_node->type);
  }

  return initial_value;
}

InitialValueArray* initial_value_array_init() {
  InitialValueArray *initial_value_array = malloc(sizeof(InitialValueArray));
  initial_value_array->capacity = 0;
  initial_value_array->count = 0;
  initial_value_array->items = NULL;

  return initial_value_array;
}
