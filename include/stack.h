#ifndef STACK
#define STACK

#include <stddef.h>
#include <stdio.h>

typedef struct {
  size_t capacity;
  size_t base_size;
  int offset;
  void *stack;
} Stack;

void stack_init(Stack *stack, size_t base_size, size_t capacity);
void stack_pop(Stack *stack);
void* stack_push(Stack *stack);
void* stack_top(Stack *stack);

#endif
