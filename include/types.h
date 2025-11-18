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
  TYPE_POINTER
} Types;

typedef struct TypeNode TypeNode;

typedef struct TypeNode {
  Types type;
  TypeNode *pointer_reference;
} TypeNode;

size_t get_type_size(Types type);
bool is_type_signed(Types type); 
char* get_type_string(Types type);

#endif
