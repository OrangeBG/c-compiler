#ifndef DYNAMIC_ARRAY
#define DYNAMIC_ARRAY

#define dynamic_array_add(arr, item, initial_capacity)  \
  do {\
    if (arr->count == arr->capacity) { \
      int size = arr->capacity == 0 ? (initial_capacity) : (arr->capacity) * 2; \
      arr->capacity = size; \
      arr->items = realloc(arr->items, size * sizeof(*arr->items)); \
    } \
    arr->items[arr->count] = item;\
    arr->count++; \
  } while(0);\

#endif
