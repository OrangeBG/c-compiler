#include <stdlib.h>
#include "../include/stack.h"

void stack_init(Stack *stack, int capacity) {
  stack->stack = malloc(sizeof(StackValue) * capacity);
  stack->capacity = capacity;
  stack->count = 0;
}

void stack_push(Stack *stack, StackValue *value) {
  if (stack->count == stack->capacity) {
    fprintf(stderr, "Ran out of memory in stack");
    exit(1);
  }

  stack->stack[stack->count] = *value;
  stack->count++;  
}

void stack_pop(Stack *stack) {
  if (stack->count == 0) {
    return;
  }
  
  stack->count--;
}

StackValue* stack_top(Stack *stack) {
  if (stack->count == 0) {
    return NULL;
  }
  
  return &stack->stack[stack->count - 1];
}

void stack_print(Stack *stack) {
  if (stack->count == 0) {
    printf("Stack Count is 0");
    return;
  }
  
  printf("Stack:\n");
  for (int i = 0; i < stack->count; i++) {
    switch (stack->stack[i].type) {
      case STACK_INT:         printf("%d\n", stack->stack[i].data.integer); break;
      case STACK_STRING:      printf("%s\n", stack->stack[i].data.string); break;
      case STACK_HASH_TABLE:  hash_table_print(stack->stack[i].data.hash_table); break;
        break;
      default:
        //TODO: Print support for struct types needed
        break;
    }
  }
}
