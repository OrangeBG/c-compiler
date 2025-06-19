#ifndef STACK
#define STACK

#include <stddef.h>
#include <stdio.h>

typedef enum {
  STACK_INT,
  STACK_STRING
} StackType;

typedef struct {
  StackType type;
  union {
    char* string;
    int integer;
  } data;  
} StackValue;

typedef struct {
  int capacity;
  int count;
  StackValue *stack;
} Stack;

void stack_init(Stack *stack, int capacity);
void stack_pop(Stack *stack);
void stack_push(Stack *stack, StackValue value);
void stack_print(Stack *stack);
StackValue* stack_top(Stack *stack);

#endif
