#ifndef DECLARATION_SYMBOL
#define DECLARATION_SYMBOL

#include "../include/hash_table.h"
#include "../include/arena.h"
#include <stdbool.h>

typedef enum {
  DECLARATION_SYMBOL_TYPE_INT,
  DECLARATION_SYMBOL_TYPE_LONG,
  DECLARATION_SYMBOL_TYPE_VOID
} DeclarationSymbolValueType;

typedef enum {
  DECLARATION_SYMBOL_VARIABLE,
  DECLARATION_SYMBOL_FUNCTION
} DeclarationSymbolType;

typedef enum {
  INITIAL_VALUE_TENTATIVE,
  INITIAL_VALUE_INITIALIZED,
  INITIAL_VALUE_NO_INITIALIZER
} InitialValueType;

typedef struct {
  bool is_defined;
  bool is_global;
  DeclarationSymbolValueType value_type;
  int param_count;
} FunctionSymbol;

typedef union {
    int int_value;
    long long_value;
} InitialValue;

typedef struct {
  DeclarationSymbolValueType value_type;
  bool is_automatic_storage_duration;
  InitialValueType static_initial_type;
  InitialValue static_initial_value;
  bool static_is_global;
} VariableSymbol; 

typedef struct {
  DeclarationSymbolType symbol_type;
  union {
    FunctionSymbol *function_symbol;
    VariableSymbol *variable_symbol;
  } data;
} DeclarationSymbol;

typedef struct {
  HashTable *symbol_table;
  //TODO: May not need to keep 3 separate arenas if I change the union to be the structs themselves rather than pointers to the struct.
  Arena *declaration_symbol_arena;
  Arena *variable_symbol_arena;
  Arena *function_symbol_arena;
} DeclarationSymbolTable;

void declaration_symbol_table_init(DeclarationSymbolTable *declaration_symbol_table);
void declaration_symbol_table_free(DeclarationSymbolTable *declaration_symbol_table);
DeclarationSymbol* add_function_declaration_symbol(DeclarationSymbolTable *declaration_symbol_table, char *function_name, DeclarationSymbolValueType function_value_type, int parameter_count, bool is_global, bool is_defined); 
void add_automatic_variable_declaration_symbol(DeclarationSymbolTable *declaration_symbol_table, DeclarationSymbolValueType value_type, char *symbol_key);  
void add_static_variable_declaration_symbol(DeclarationSymbolTable *declaration_symbol_table, DeclarationSymbolValueType value_type, InitialValue initial_value, char *symbol_key, bool is_global, InitialValueType initial_value_type);   
void add_static_extern_variable_declaration_symbol(DeclarationSymbolTable *declaration_symbol_table, DeclarationSymbolValueType value_type, char *symbol_key);   
void print_declaration_symbol_table(DeclarationSymbolTable *declaration_symbol_table); 
#endif
