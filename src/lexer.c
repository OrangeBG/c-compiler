#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include "../include/lexer.h"

#define TOKEN_ARRAY_START_SIZE 8

Tokens* init_tokens();
void load_tokens(Tokens* tokens, char* file);
void add_token(TokenType type, Tokens* tokens);
 
Tokens* init_tokens() {
  Tokens* tokens = malloc(sizeof(Tokens));
  tokens->capacity = 0;
  tokens->count = 0;
  tokens->tokens = NULL;

  return tokens;
}

void load_tokens(Tokens* tokens, char* file) {
  int index = 0;

  while (true) {
    char cur_char = file[index];
    if (cur_char == '\0') {
      break;
    }

    switch (cur_char) {
      case '(': add_token(TOKEN_OPEN_PAREN, tokens); break;
      case ')': add_token(TOKEN_CLOSE_PAREN, tokens); break;
      case '{': add_token(TOKEN_OPEN_BRACE, tokens); break;
      case '}': add_token(TOKEN_CLOSE_BRACE, tokens); break;
      case ';': add_token(TOKEN_SEMICOLON, tokens); break;
      default:
        break;
    }

    index++;
  }  
}

void add_token(TokenType type, Tokens* tokens) {  
  if (tokens->count == tokens->capacity) {
    int size = tokens->capacity == 0 ? TOKEN_ARRAY_START_SIZE : tokens->capacity * 2;

    printf("Lexer: Growing token array. Size: %d -> %d\n", tokens->capacity, size);

    tokens->capacity = size;
    //TODO: Add error when realloc fails
    tokens->tokens = realloc(tokens->tokens, size);
  }

  Token new_token = {
    .type = type
  };

  tokens->tokens[tokens->count] = new_token;
  tokens->count++;
}

