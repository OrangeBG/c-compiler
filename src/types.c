#include <stdio.h>
#include "../include/types.h"

size_t get_type_size(Types type) {
  switch(type) {
    case TYPE_INT:    return sizeof(int);
    case TYPE_UINT:   return sizeof(unsigned int);
    case TYPE_LONG:   return sizeof(long);
    case TYPE_ULONG:  return sizeof(unsigned long);
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

char* get_type_string(Types type) {
  switch (type) {
    case TYPE_VOID:     return "Void";
    case TYPE_INT:      return "Int";
    case TYPE_UINT:     return "Unsigned Int";
    case TYPE_LONG:     return "Long";
    case TYPE_ULONG:    return "Unsigned Long";
    case TYPE_DOUBLE:   return "Double";
    case TYPE_FUNCTION: return "Function";
    default:
      fprintf(stderr, "ERROR - Types: get_type_string() type %d not supported", type);
      exit(1);
  }
}
