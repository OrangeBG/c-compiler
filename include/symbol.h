#ifndef SYMBOL
#define SYMBOL

#include "../include/hash_table.h"
#include "../include/arena.h"
#include "../include/types.h"
#include <stdbool.h>

#define STATIC_INITIAL_VALUE_CAPACITY 4

typedef enum {
  SYMBOL_VARIABLE,
  SYMBOL_FUNCTION
} SymbolType;

typedef enum {
  INITIALIZATION_TYPE_TENTATIVE,
  INITIALIZATION_TYPE_INITIALIZED,
  INITIALIZATION_TYPE_NO_INITIALIZER
} InitializationType;

typedef enum {
  INITIAL_VALUE_TYPE_INT,
  INITIAL_VALUE_TYPE_LONG,
  INITIAL_VALUE_TYPE_UINT,
  INITIAL_VALUE_TYPE_ULONG,
  INITIAL_VALUE_TYPE_DOUBLE,  
  INITIAL_VALUE_TYPE_ZERO_INIT  
} InitialValueType;

typedef struct {
  bool is_defined;
  bool is_global;
  TypeNode *value_type;
  int param_count;
  TypeNode *param_types;
} FunctionSymbol;

typedef struct {
  InitialValueType type;
  union {
    int int_value;
    long long_value;
    unsigned int uint_value;
    unsigned long ulong_value;
    double double_value;
    int zero_init_array_bytes;
  } data;
} InitialValue;

typedef struct {
  int count;
  int capacity;
  InitialValue *items;
} InitialValueArray;

typedef struct {
  TypeNode *value_type;
  bool is_automatic_storage_duration;
  InitializationType static_initialization_type;
  InitialValueArray *static_initial_value_array;
  bool static_is_global;
} VariableSymbol; 

typedef struct {
  SymbolType symbol_type;
  union {
    FunctionSymbol *function_symbol;
    VariableSymbol *variable_symbol;
  } data;
} Symbol;

typedef struct {
  HashTable *symbol_table;
  //TODO: May not need to keep 3 separate arenas if I change the union to be the structs themselves rather than pointers to the struct.
  Arena *symbol_arena;
  Arena *variable_symbol_arena;
  Arena *function_symbol_arena;
} SymbolTable;

void symbol_table_init(SymbolTable *symbol_table);
void symbol_table_free(SymbolTable *symbol_table);
Symbol* get_symbol(char *identifier, SymbolTable *symbol_table, bool error_if_null);
Symbol* add_function_symbol(SymbolTable *symbol_table, char *function_name, TypeNode *function_value_type, int parameter_count, TypeNode *param_types, bool is_global, bool is_defined); 
void add_automatic_variable_symbol(SymbolTable *symbol_table, TypeNode *value_type, char *symbol_key);  
void add_static_variable_symbol(SymbolTable *symbol_table, TypeNode *value_type, InitialValueArray *initial_value_array, char *symbol_key, bool is_global, InitializationType initial_value_type);   
void add_static_extern_variable_symbol(SymbolTable *symbol_table, TypeNode *value_type, char *symbol_key);   
void symbol_table_print(SymbolTable *symbol_table); 
void symbol_initialize_to_zero(TypeNode *type_node, InitialValue *initial_value); 
InitialValueArray* initial_value_array_init();

#endif
