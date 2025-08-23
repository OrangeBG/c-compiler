#include <stdio.h>
#include <stdlib.h>
#include "../include/arena.h"

void arena_init(Arena *arena, int base_size, int capacity, bool allow_expand) {
  arena->allocation = malloc(capacity);  
  arena->capacity = capacity;
  arena->base_size = base_size;
  arena->offset = 0;
  arena->max_index = 0;
  arena->allow_expand = allow_expand;
}

void* arena_alloc(Arena *arena) {
  if (arena->offset + arena->base_size > arena->capacity) {
    if (!arena->allow_expand) {
      fprintf(stderr, "Ran out of memory in arena");
      exit(1);
    }    

    printf("Expanding Arena capacity from %zd to %zd\n", arena->capacity, arena->capacity * 2);
    arena->capacity *= 2;
    void* new_allocation = realloc(arena->allocation, arena->capacity);
    arena->allocation = new_allocation;
  }

  //Notes on char* cast:
  //At first glance, it might look odd that we're casting to char *, but this is very intentional in C. In C, pointer arithmetic (i.e., adding an offset to a pointer) is done in units of the size of the pointed-to type. The char type in C is exactly 1 byte by definition. By casting void* to char*, you're saying ""Hey, I want to move this pointer in byte-sized steps."
  void *current_offset = (void*)((char *)arena->allocation + arena->offset);

  arena->offset += arena->base_size;
  arena->max_index = arena->offset / arena->base_size;

  return current_offset;
}

void arena_free(Arena *arena) {
  free(arena->allocation);
  arena->base_size = 0;
  arena->capacity = 0;
  arena->offset = 0;
}

void arena_reset(Arena *arena) {
  arena->offset = 0;
}

void* arena_get_by_index(Arena *arena, int index) {
  int offset = arena->base_size * index;
  void *current_offset = (void*)((char *)arena->allocation + offset);
  return current_offset;
}
