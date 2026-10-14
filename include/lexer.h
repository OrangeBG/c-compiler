#ifndef LEXER
#define LEXER

#include <stdbool.h>
#include "../include/token.h"

typedef struct {
  TokenType type;
  int start_index;
  int end_index;
  int line;
} Token;

typedef struct {
  int capacity;
  int count;
  Token* items;  
} TokenArray;

typedef struct {
  int start_index;
  int current_index;
  int line;
  TokenArray *tokens;
} Lexer;

Lexer init_lexer();
void load_tokens(Lexer *lexer, const char *file);
void print_tokens(const Lexer *lexer, const char *file);

#endif
