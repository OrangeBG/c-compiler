#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include "../include/lexer.h"

#define TOKEN_ARRAY_START_SIZE 8

void add_token(TokenType type, Lexer* lexer);
bool is_alpha_char(char character);
bool is_numeric_char(char character);
void add_number_token(Lexer* lexer, char* file); 
 
Lexer init_lexer() {
  Lexer lexer = {
    .start_index = 0,
    .current_index = 0,
    .token_capacity = 0,
    .token_count = 0,
    .tokens = NULL
  };

  return lexer;
}

void load_tokens(Lexer* lexer, char* file) {
  while (true) {
    char cur_char = file[lexer->start_index];
    if (cur_char == '\0') {
      break;
    }

    if (is_alpha_char(cur_char)) {
    } 

    if (is_numeric_char(cur_char)) {
      add_number_token(lexer, file);
      lexer->start_index++;
      lexer->current_index = lexer->start_index;
      continue;
    }

    switch (cur_char) {
      case ' ': break;
      case '\t': break;
      case '\r': break;
      case '\n': lexer->line++; break;
      case '(': add_token(TOKEN_OPEN_PAREN, lexer); break;
      case ')': add_token(TOKEN_CLOSE_PAREN, lexer); break;
      case '{': add_token(TOKEN_OPEN_BRACE, lexer); break;
      case '}': add_token(TOKEN_CLOSE_BRACE, lexer); break;
      case ';': add_token(TOKEN_SEMICOLON, lexer); break;
      default:
        break;
    }

    lexer->start_index++;
    lexer->current_index = lexer->start_index;
  }  
}

void print_tokens(Lexer* lexer, char* file) {
  printf("\n******************** LEXER PRINT ********************\n");
  for (int i = 0; i < lexer->token_count; i++) {
    switch (lexer->tokens[i].type) {       
      case TOKEN_KEYWORD: printf("Keyword\t"); break;
      case TOKEN_IDENTIFIER: printf("Identifier\t");  break;
      case TOKEN_CONSTANT_INT: printf("Constant\t"); break;
      case TOKEN_OPEN_PAREN: printf("Open Paren\t"); break;
      case TOKEN_CLOSE_PAREN: printf("Close Paren\t"); break;
      case TOKEN_OPEN_BRACE: printf("Open Brace\t"); break;
      case TOKEN_CLOSE_BRACE: printf("Close Brace\t"); break;
      case TOKEN_SEMICOLON: printf("Semicolon\t"); break;
      default: fprintf(stderr, "ERROR - Lexer: No print for type %d\n", lexer->tokens[i].type);
    }

    printf(" -> ");

    for (int j = lexer->tokens[i].start_index; j <= lexer->tokens[i].end_index; j++) {
      printf("%c", file[j]);
    } 

    printf("\t line: %d\n", lexer->tokens[i].line);
  }
}

void add_token(TokenType type, Lexer* lexer) {  
  if (lexer->token_count == lexer->token_capacity) {
    int size = lexer->token_capacity == 0 ? TOKEN_ARRAY_START_SIZE : lexer->token_capacity * 2;

    printf("Lexer: Growing token array. Size: %d -> %d\n", lexer->token_capacity, size);

    lexer->token_capacity = size;
    //TODO: Add error when realloc fails
    lexer->tokens = realloc(lexer->tokens, size);
  }

  Token new_token = {
    .type = type,
    .start_index = lexer->start_index,
    .end_index = lexer->current_index,
    .line = lexer->line
  };

  lexer->tokens[lexer->token_count] = new_token;
  lexer->token_count++;
}

bool is_alpha_char(char character) {
  if ((character >= 'a' && character <= 'z') || (character >= 'A' && character <= 'Z')) {
    return true;
  }

  return false;
}

bool is_numeric_char(char character) {
  if (character >= '0' && character <= '9') {
    return true;
  } 

  return false;
}

void add_number_token(Lexer* lexer, char* file) {
  //TODO: Floats and decimals not supported yet
  while (file[lexer->current_index + 1] != '\0' && is_numeric_char(file[lexer->current_index + 1])) {
    lexer->current_index++;
  }

  add_token(TOKEN_CONSTANT_INT, lexer); 
}
