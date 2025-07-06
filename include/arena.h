#ifndef ARENA
#define ARENA

#include <stddef.h>
#include <stdlib.h>

typedef struct {
  size_t capacity;
  size_t offset;
  size_t base_size;
  void *allocation;
} Arena;

void arena_init(Arena *arena, int base_size, int capacity);  
void arena_free(Arena *arena);
void arena_reset(Arena *arena);
void* arena_alloc(Arena *arena);

#endif
