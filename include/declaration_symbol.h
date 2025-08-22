#ifndef DECLARATION_SYMBOL
#define DECLARATION_SYMBOL

#include <stdbool.h>

typedef enum {
  DECLARATION_SYMBOL_TYPE_INT,
  DECLARATION_SYMBOL_TYPE_LONG
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
  bool defined;
  bool global;
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

#endif
