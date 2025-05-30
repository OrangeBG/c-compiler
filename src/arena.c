#include "../include/arena.h"

void arena_init(Arena *arena, int base_size, int capacity) {
  arena->arena = calloc(capacity, base_size);  
  arena->capacity = capacity;
  arena->next = arena;
}

void arena_add(Arena *arena) {
  arena->count++;
  arena->next = arena->arena++;
}
