#ifndef SA_TYPE_CHECK
#define SA_TYPE_CHECK

#include "../include/parser.h"
#include "../include/hash_table.h"

typedef enum { TYPE_INT, TYPE_LONG } ValueType;
typedef enum { SYMBOL_VARIABLE, SYMBOL_FUNCTION } SymbolType;
typedef enum { INITIAL_VALUE_TENTATIVE, INITIAL_VALUE_INITIALIZED, INITIAL_VALUE_NO_INITIALIZER } InitialValueType;

typedef struct {
  bool defined;
  bool global;
  ValueType value_type;
  int param_count;
} FunctionSymbol;

typedef struct {
  InitialValueType initial_type;
  int initial_value;
  bool is_global;
} StaticStorageDuration;

typedef struct {
  ValueType value_type;
  bool is_automatic_storage_duration;
  StaticStorageDuration *static_storage_duration;
} VariableSymbol; 

typedef struct {
  SymbolType symbol_type;
  union {
    FunctionSymbol *function_symbol;
    VariableSymbol *variable_symbol;
  } data;
} TypeCheckSymbol;

void sa_type_check(AstNode *ast_nodes, HashTable *declaration_symbols, Arena *ast_arena);

#endif
