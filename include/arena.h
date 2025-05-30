#ifndef ARENA
#define ARENA

#include <stddef.h>
#include <stdlib.h>

typedef struct {
  int capacity;
  int count;
  void *arena;
  void *next;
} Arena;

void arena_init(Arena *arena, int base_size, int capacity);  
void arena_add(Arena *arena);

#endif
