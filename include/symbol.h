#ifndef SYMBOL
#define SYMBOL

#include "../include/hash_table.h"
#include "../include/arena.h"
#include "../include/types.h"
#include <stdbool.h>

#define STATIC_INITIAL_VALUE_CAPACITY 4

typedef enum {
  SYMBOL_STATIC,
  SYMBOL_LOCAL,
  SYMBOL_FUNCTION,
  SYMBOL_CONSTANT
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
  INITIAL_VALUE_TYPE_CHAR,
  INITIAL_VALUE_TYPE_UCHAR,
  INITIAL_VALUE_TYPE_ZERO_INIT,
  INITIAL_VALUE_TYPE_STRING,
  INITIAL_VALUE_TYPE_POINTER
} InitialValueType;

typedef struct {
  InitialValueType type;
  union {
    int int_value;
    long long_value;
    unsigned int uint_value;
    unsigned long ulong_value;
    double double_value;
    int char_value;
    unsigned int uchar_value;
    int zero_init_array_bytes;
    char *pointer_name;
    struct { char *string_value; bool is_null_terminated; } string_value;
  } data;
} InitialValue;

typedef struct {
  int count;
  int capacity;
  InitialValue *items;
} InitialValueArray;

typedef struct {
  SymbolType type;
  TypeNode *value_type;
  union {
    struct FunctionSymbol { bool is_defined; bool is_global; int param_count; TypeNode *param_types; } function_symbol;
    struct StaticSymbol { InitializationType initialization_type; InitialValueArray *initial_value_array; bool is_global; } static_symbol;
    struct ConstantSymbol { InitialValue *static_initial_value; } constant_symbol;
  } data;
} Symbol;

typedef struct {
  HashTable *symbol_table;
  Arena *symbol_arena;
} SymbolTable;

void symbol_table_init(SymbolTable *symbol_table);
void symbol_table_free(SymbolTable *symbol_table);
Symbol* get_symbol(char *identifier, SymbolTable *symbol_table, bool error_if_null);
Symbol* add_function_symbol(SymbolTable *symbol_table, char *function_name, TypeNode *function_value_type, int parameter_count, TypeNode *param_types, bool is_global, bool is_defined); 
Symbol* add_local_symbol(SymbolTable *symbol_table, TypeNode *value_type, char *symbol_key);  
Symbol* add_static_symbol(SymbolTable *symbol_table, TypeNode *value_type, InitialValueArray *initial_value_array, char *symbol_key, bool is_global, InitializationType initial_value_type);   
void add_static_extern_variable_symbol(SymbolTable *symbol_table, TypeNode *value_type, char *symbol_key);   
void symbol_table_print(SymbolTable *symbol_table); 
InitialValue* symbol_initialize_to_zero(TypeNode *type_node); 
InitialValueArray* initial_value_array_init();

#endif
