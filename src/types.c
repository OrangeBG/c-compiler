#include <stdio.h>
#include "../include/types.h"

#define FUNCTION_PARAMETER_TYPE_INIT_CAPACITY 4

size_t get_type_size(Types type) {
  switch(type) {
    case TYPE_INT:     return sizeof(int);
    case TYPE_UINT:    return sizeof(unsigned int);
    case TYPE_LONG:    return sizeof(long);
    case TYPE_ULONG:   return sizeof(unsigned long);
    case TYPE_DOUBLE:  return sizeof(double);
    case TYPE_POINTER: return sizeof(int*);
    default:
      fprintf(stderr, "ERROR - Types: Unsupported type when attempting to get Type size\n");
      exit(1);
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
  //TODO: Add pointers?
  switch(type_node->type) {
    case TYPE_DOUBLE:
    case TYPE_INT:
    case TYPE_UINT:
    case TYPE_LONG:
    case TYPE_ULONG:
      return true; 
    case TYPE_POINTER: {
      Types base_pointer_type = get_pointer_base_type(type_node);

      switch(base_pointer_type) {
        case TYPE_INT:
        case TYPE_LONG:
        case TYPE_UINT:
        case TYPE_ULONG:
          return true;
        default:
          return false;
      }
    }
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
    default:
      fprintf(stderr, "ERROR - Types: get_type_string() type %d not supported\n", type);
      exit(1);
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
      fprintf(stderr, "ERROR - Parser: Could not find Type '%d' when printing\n", type_node->type);
      exit(1);
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

Types get_pointer_base_type(TypeNode *pointer_node) {
  if (pointer_node->type != TYPE_POINTER) {
    fprintf(stderr, "ERROR - Type: Passed non-pointer to get_pointer_base_type()\n");
    exit(1);
  }

  if (pointer_node->data.pointer_type.reference_type->type == TYPE_POINTER) {
    return get_pointer_base_type(pointer_node->data.pointer_type.reference_type);    
  }

  if (pointer_node->data.pointer_type.reference_type->type == TYPE_ARRAY) {
    return get_array_base_type(pointer_node->data.pointer_type.reference_type);
  }

  return pointer_node->data.pointer_type.reference_type->type;
}

Types get_array_base_type(TypeNode *array_node)  {
  if (array_node->type != TYPE_ARRAY) {
    fprintf(stderr, "ERROR - Type: Passed non-array to get_array_base_type()\n");
    exit(1);
  }

  if (array_node->data.array_type.element_type->type == TYPE_ARRAY) {
    return get_array_base_type(array_node->data.array_type.element_type);
  }

  return array_node->data.array_type.element_type->type;
}
