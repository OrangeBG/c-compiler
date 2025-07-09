#ifndef ARENA
#define ARENA

#include <stddef.h>
#include <stdlib.h>
#include <stdbool.h>

typedef struct {
  size_t capacity;
  size_t offset;
  size_t base_size;
  void *allocation;
  int max_index;
  bool allow_expand;
} Arena;

void arena_init(Arena *arena, int base_size, int capacity, bool allow_expand);  
void arena_free(Arena *arena);
void arena_reset(Arena *arena);
void* arena_alloc(Arena *arena);
void* arena_get_by_index(Arena *arena, int offset);

#endif
