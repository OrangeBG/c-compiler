#include <stdio.h>
#include <stdlib.h>
#include "../include/arena.h"

void arena_init(Arena *arena, int base_size, int capacity) {
  arena->allocation = malloc(capacity);  
  arena->capacity = capacity;
  arena->base_size = base_size;
  arena->offset = 0;
}

void* arena_alloc(Arena *arena) {
  //TODO: Check to see if we ever want to expand the arena
  if (arena->offset + arena->base_size > arena->capacity) {
    fprintf(stderr, "Ran out memory in arena");
    exit(1);
  }

  //Notes on char* cast:
  //At first glance, it might look odd that we're casting to char *, but this is very intentional in C. In C, pointer arithmetic (i.e., adding an offset to a pointer) is done in units of the size of the pointed-to type. The char type in C is exactly 1 byte by definition. By casting void* to char*, you're saying ""Hey, I want to move this pointer in byte-sized steps."
  void *current_offset = (void*)((char *)arena->allocation + arena->offset);

  arena->offset += arena->base_size;

  return current_offset;
}

void arena_free(Arena *arena) {
  free(arena->allocation);
  arena->base_size = 0;
  arena->capacity = 0;
  arena->offset = 0;
}
