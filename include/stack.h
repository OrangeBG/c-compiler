#ifndef STACK
#define STACK

#include <stddef.h>
#include <stdio.h>
#include "../include/hash_table.h"

typedef enum {
  STACK_INT,
  STACK_STRING,
  STACK_STRUCT,
  STACK_HASH_TABLE
} StackType;

typedef struct {
  StackType type;
  union {
    char* string;
    void* structure;
    HashTable* hash_table;
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
void stack_push(Stack *stack, StackValue *value);
void stack_print(Stack *stack);
StackValue* stack_top(Stack *stack);

#endif
