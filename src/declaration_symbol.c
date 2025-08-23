#include <stdlib.h>
#include "../include/declaration_symbol.h"

void declaration_symbol_table_init(DeclarationSymbolTable *declaration_symbol_table) {
  HashTable *symbol_table = malloc(sizeof(HashTable));
  hash_table_init(symbol_table);
  
  Arena *declaration_symbol_arena = malloc(sizeof(Arena));
  arena_init(declaration_symbol_arena, sizeof(DeclarationSymbol), sizeof(DeclarationSymbol) * 1000, true);
  Arena *variable_symbol_arena = malloc(sizeof(Arena));
  arena_init(variable_symbol_arena, sizeof(VariableSymbol), sizeof(VariableSymbol) * 1000, true);

  declaration_symbol_table->symbol_table = symbol_table;
  declaration_symbol_table->declaration_symbol_arena = declaration_symbol_arena;
  declaration_symbol_table->variable_symbol_arena = variable_symbol_arena;
}

void declaration_symbol_table_free(DeclarationSymbolTable *declaration_symbol_table) {
  arena_free(declaration_symbol_table->variable_symbol_arena);
  arena_free(declaration_symbol_table->declaration_symbol_arena);
  free(declaration_symbol_table->symbol_table);
}

DeclarationSymbol* add_function_declaration_symbol(DeclarationSymbolTable *declaration_symbol_table, char *function_name, DeclarationSymbolValueType function_value_type, int parameter_count, bool is_global, bool is_defined) {
  FunctionSymbol *function_symbol = arena_alloc(declaration_symbol_table->declaration_symbol_arena);

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
  DeclarationSymbol *symbol = malloc(sizeof(DeclarationSymbol));
  symbol->symbol_type = DECLARATION_SYMBOL_VARIABLE;

  VariableSymbol *variable_symbol = malloc(sizeof(VariableSymbol));
  variable_symbol->value_type = value_type;
  variable_symbol->is_automatic_storage_duration = true;

  symbol->data.variable_symbol = variable_symbol;

  HashTableEntry *entry = malloc(sizeof(HashTableEntry));
  entry->key = symbol_key;

  HashValue *value = malloc(sizeof(HashValue));
  value->type = HASH_STRUCT;
  value->structure = symbol;

  entry->value = value;

  hash_table_add_entry(declaration_symbol_table->symbol_table, entry); 
}

void add_static_variable_declaration_symbol(DeclarationSymbolTable *declaration_symbol_table, DeclarationSymbolValueType value_type, char *symbol_key, bool is_global, InitialValueType initial_value_type) {  
  DeclarationSymbol *variable_symbol = malloc(sizeof(DeclarationSymbol));
  variable_symbol->symbol_type = DECLARATION_SYMBOL_VARIABLE;

  VariableSymbol *symbol = malloc(sizeof(VariableSymbol));
  symbol->is_automatic_storage_duration = false;

  assign_variable_symbol_value_type(symbol, variable_declaration_node);

  variable_symbol->data.variable_symbol = symbol;

  symbol->static_is_global = true;
  symbol->static_initial_type = INITIAL_VALUE_NO_INITIALIZER;

  HashValue *new_value = malloc(sizeof(HashValue));
  new_value->type = HASH_STRUCT;
  new_value->structure = variable_symbol;

  HashTableEntry *new_entry = malloc(sizeof(HashTableEntry));
  new_entry->key = variable_declaration_node->data.variable_declaration.name;
  new_entry->value = new_value;

  hash_table_add_entry(symbols, new_entry);
}
