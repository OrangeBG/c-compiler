#include <stdio.h>
#include "../include/types.h"
#include "../include/error.h"

#define FUNCTION_PARAMETER_TYPE_INIT_CAPACITY 4

size_t get_type_size(TypeNode *type_node) {
  switch(type_node->type) {
    case TYPE_INT:     return sizeof(int);
    case TYPE_UINT:    return sizeof(unsigned int);
    case TYPE_LONG:    return sizeof(long);
    case TYPE_ULONG:   return sizeof(unsigned long);
    case TYPE_DOUBLE:  return sizeof(double);
    case TYPE_POINTER: return sizeof(int*);
    case TYPE_ARRAY:   return type_node->data.array_type.size;
    default:
      panic("Unsupported type when attempting to get Type size");
  }
}

bool is_type_signed(Types type) {
  switch (type) {
    case TYPE_UINT:
    case TYPE_ULONG:
      return false;
    default:
      return true;
  }
}

bool is_arithmetic_type(TypeNode *type_node) {
  /*
    @Note: Pointers and arrays are not considered arithmetic types.
    ISO 9899:2018 - p.32 - "Integer and floating types are collectively called 'arithmetic types'"  
  */
  switch(type_node->type) {
    case TYPE_DOUBLE:
    case TYPE_INT:
    case TYPE_UINT:
    case TYPE_LONG:
    case TYPE_ULONG:
      return true;
    default:
      return false;
  }
}

bool is_integer_type(TypeNode *type_node) {
  switch(type_node->type) {
    case TYPE_INT:
    case TYPE_UINT:
    case TYPE_LONG:
    case TYPE_ULONG:
      return true;
    default:
      return false;
  }
}

char* get_type_string(Types type) {
  switch (type) {
    case TYPE_VOID:     return "void";
    case TYPE_INT:      return "int";
    case TYPE_UINT:     return "uint";
    case TYPE_LONG:     return "long";
    case TYPE_ULONG:    return "ulong";
    case TYPE_DOUBLE:   return "double";
    case TYPE_FUNCTION: return "function";
    case TYPE_POINTER:  return "pointer";
    case TYPE_ARRAY:    return "array";
    default:
      panic("get_type_string() type %d not supported", type);
  }
}

void print_type_node(TypeNode *type_node) {
  switch (type_node->type) {
    case TYPE_VOID:     printf("void"); break;
    case TYPE_INT:      printf("int"); break;
    case TYPE_UINT:     printf("uint"); break;
    case TYPE_LONG:     printf("long"); break;
    case TYPE_ULONG:    printf("ulong"); break;
    case TYPE_DOUBLE:   printf("double"); break;
    case TYPE_FUNCTION: printf("function"); break;
    case TYPE_POINTER:
      printf("Pointer(");
      print_type_node(type_node->data.pointer_type.reference_type);
      printf(")");
      break;
    case TYPE_ARRAY:
      printf("Array(");
      print_type_node(type_node->data.array_type.element_type);
      printf(", %lu", type_node->data.array_type.size);
      printf(")");
      break;
    default:
      panic("Could not find Type '%d' when printing", type_node->type);
  }
}

void add_function_parameter_type(TypeNode *parameter_type, TypeNode *function_type) {
  if (function_type->data.function_type.param_type_count == function_type->data.function_type.param_type_capacity) {
    int size = function_type->data.function_type.param_type_capacity == 0 ? FUNCTION_PARAMETER_TYPE_INIT_CAPACITY : function_type->data.function_type.param_type_capacity * 2;
    function_type->data.function_type.param_type_capacity = size;
    function_type->data.function_type.param_types = realloc(function_type->data.function_type.param_types, size * sizeof(TypeNode));
  }

  function_type->data.function_type.param_types[function_type->data.function_type.param_type_count] = *parameter_type;
  function_type->data.function_type.param_type_count++;
}   

size_t get_pointer_base_size(TypeNode *pointer_node) {
  if (pointer_node->type != TYPE_POINTER) {
    panic("Passed non-pointer to get_pointer_base_size()");
  }

  if (pointer_node->data.pointer_type.reference_type->type == TYPE_POINTER) {
    return get_pointer_base_size(pointer_node->data.pointer_type.reference_type);    
  }

  if (pointer_node->data.pointer_type.reference_type->type == TYPE_ARRAY) {
    return get_array_base_size(pointer_node->data.pointer_type.reference_type);
  }

  return get_type_size(pointer_node->data.pointer_type.reference_type);
}

Types get_pointer_base_type(TypeNode *pointer_node) {
  if (pointer_node->type != TYPE_POINTER) {
    panic("Passed non-pointer to get_pointer_base_type()");
  }

  if (pointer_node->data.pointer_type.reference_type->type == TYPE_POINTER) {
    return get_pointer_base_type(pointer_node->data.pointer_type.reference_type);    
  }

  if (pointer_node->data.pointer_type.reference_type->type == TYPE_ARRAY) {
    return get_array_base_type(pointer_node->data.pointer_type.reference_type);
  }

  return pointer_node->data.pointer_type.reference_type->type;
}

size_t get_array_base_size(TypeNode *array_node) {
  if (array_node->type != TYPE_ARRAY) {
    panic("Passed non-array to get_array_base_size()");
  }

  if (array_node->data.array_type.element_type->type == TYPE_ARRAY) {
    return get_array_base_size(array_node->data.array_type.element_type);
  }

  return get_type_size(array_node->data.array_type.element_type);
} 

Types get_array_base_type(TypeNode *array_node)  {
  if (array_node->type != TYPE_ARRAY) {
    panic("Passed non-array to get_array_base_type()");
  }

  if (array_node->data.array_type.element_type->type == TYPE_ARRAY) {
    return get_array_base_type(array_node->data.array_type.element_type);
  }

  return array_node->data.array_type.element_type->type;
}
