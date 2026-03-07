#ifndef TYPES
#define TYPES

#include <stdlib.h>
#include <stdbool.h>

typedef enum {
  TYPE_VOID,
  TYPE_INT,
  TYPE_UINT,
  TYPE_LONG,
  TYPE_ULONG,
  TYPE_DOUBLE,
  TYPE_FUNCTION,
  TYPE_POINTER,
  TYPE_ARRAY
} Types;

typedef struct TypeNode TypeNode;

typedef struct TypeNode {
  Types type;
  union {
  struct FunctionType { TypeNode *param_types; int param_type_count; int param_type_capacity; TypeNode *return_type; } function_type;
  struct PointerType { TypeNode *reference_type; } pointer_type;
  struct ArrayType { TypeNode *element_type; unsigned long size; } array_type;
  } data;
} TypeNode;

size_t get_type_size(TypeNode *type_node);
size_t get_array_base_size(TypeNode *array_node); 
size_t get_pointer_base_size(TypeNode *pointer_node); 
bool   is_type_signed(Types type); 
bool   is_arithmetic_type(TypeNode *type_node);
bool   is_integer_type(TypeNode *type_node); 
char*  get_type_string(Types type);
void   print_type_node(TypeNode *type_node); 
void   add_function_parameter_type(TypeNode *parameter_type, TypeNode *function_type); 
Types  get_pointer_base_type(TypeNode *pointer_node);
Types  get_array_base_type(TypeNode *array_node);

#endif
