#include <stdlib.h>
#include "../include/stack.h"

void stack_init(Stack *stack, size_t base_size, size_t capacity) {
  stack->stack = malloc(capacity);
  stack->base_size = base_size;
  stack->capacity = capacity;
  stack->offset = 0;
}

void* stack_push(Stack *stack) {
  if (stack->offset + stack->base_size > stack->capacity) {
    fprintf(stderr, "Ran out memory in stack");
    exit(1);
  }

  void *current_offset = (void*)((char *)stack->stack + stack->offset);

  stack->offset += stack->base_size;

  return current_offset;
}

void stack_pop(Stack *stack) {
  if (stack->offset == 0) {
    return;
  }
  
  stack->offset = stack->offset - stack->base_size;
}

void* stack_top(Stack *stack) {
  if (stack->offset == 0) {
    return NULL;
  }
  
  return (void*)((char *)stack->stack + stack->offset);
}
