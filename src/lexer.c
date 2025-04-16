#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "../include/lexer.h"

//TODO: May be better to have a larger start size
#define TOKEN_ARRAY_START_SIZE 8

bool is_alpha_char(char character);
bool is_numeric_char(char character);
void add_token(TokenType type, Lexer *lexer);
void add_number_token(Lexer *lexer, char *file); 
void add_identifier_token(Lexer *lexer, char *file); 
TokenType check_keyword(int start, int length, char *rest, TokenType type, Lexer *lexer, char *file); 
TokenType get_identifier_type(Lexer *lexer, char *file); 
 
Lexer init_lexer() {
  Lexer lexer = {
    .start_index = 0,
    .current_index = 0,
    .line = 1,
    .token_capacity = 0,
    .token_count = 0,
    .tokens = NULL
  };

  return lexer;
}

void load_tokens(Lexer *lexer, char *file) {
  while (true) {
    char cur_char = file[lexer->start_index];
    if (cur_char == '\0') {
      break;
    }

    if (is_alpha_char(cur_char)) {
      add_identifier_token(lexer, file);
      lexer->start_index = lexer->current_index + 1;
      lexer->current_index = lexer->start_index;
      continue;
    } 

    if (is_numeric_char(cur_char)) {
      add_number_token(lexer, file);
      lexer->start_index = lexer->current_index + 1;
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
        fprintf(stderr, "ERROR - Lexer: Invalid token '%c' (line %d)", cur_char, lexer->line);
        exit(1);
    }

    lexer->start_index++;
    lexer->current_index = lexer->start_index;
  }  
}

void print_tokens(Lexer *lexer, char *file) {
  printf("\n******************** LEXER PRINT ********************\n");

  for (int i = 0; i < lexer->token_count; i++) {
    printf("line %d     ", lexer->tokens[i].line);
    switch (lexer->tokens[i].type) {       
      case TOKEN_CLOSE_BRACE: printf("Close Brace"); break;
      case TOKEN_CLOSE_PAREN: printf("Close Paren"); break;
      case TOKEN_CONSTANT_INT: printf("Constant   "); break;
      case TOKEN_IDENTIFIER: printf("Identifier ");  break;
      case TOKEN_INT: printf("Int        ");  break;
      case TOKEN_OPEN_PAREN: printf("Open Paren "); break;
      case TOKEN_OPEN_BRACE: printf("Open Brace "); break;
      case TOKEN_RETURN: printf("Return     "); break;
      case TOKEN_SEMICOLON: printf("Semicolon  "); break;
      case TOKEN_VOID: printf("Void       "); break;
      default: fprintf(stderr, "ERROR - Lexer: No print for type %d\n", lexer->tokens[i].type);
    }

    printf(" -> ");

    for (int j = lexer->tokens[i].start_index; j <= lexer->tokens[i].end_index; j++) {
      printf("%c", file[j]);
    } 
    printf("\n");
  }
}

void add_token(TokenType type, Lexer *lexer) {  
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

void add_number_token(Lexer *lexer, char *file) {
  //TODO: Floats and decimals not supported yet
  while (file[lexer->current_index + 1] != '\0' && is_numeric_char(file[lexer->current_index + 1])) {
    lexer->current_index++;
  }

  if (is_alpha_char(file[lexer->current_index + 1])) {
    fprintf(stderr, "ERROR - Lexer: Invalid character '%c' in number (line %d)", file[lexer->current_index + 1], lexer->line);
    exit(1);
  } 

  add_token(TOKEN_CONSTANT_INT, lexer); 
}

void add_identifier_token(Lexer *lexer, char *file) {
  while (file[lexer->current_index + 1] != '\0' && (is_alpha_char(file[lexer->current_index + 1]) || is_numeric_char(file[lexer->current_index + 1]))) {
    lexer->current_index++;
  }
  
  TokenType type = get_identifier_type(lexer, file);

  add_token(type, lexer);
}

TokenType check_keyword(int start, int length, char *rest, TokenType type, Lexer *lexer, char *file) { 
  if (lexer->current_index - lexer->start_index == start + length && memcmp(&file[lexer->start_index + 1], rest, length) == 0) {
    return type;
  }

  return TOKEN_IDENTIFIER;
}

TokenType get_identifier_type(Lexer *lexer, char  *file) {
  //TODO: Need to support the rest of the keywords
  //TODO: Having start point be at the current index seems wrong
  switch (file[lexer->start_index]) {
    case 'i': return check_keyword(0, 2, "nt", TOKEN_INT, lexer, file); 
    case 'r': return check_keyword(0, 5, "eturn", TOKEN_RETURN, lexer, file);
    case 'v': return check_keyword(0, 3, "oid", TOKEN_VOID, lexer, file);
  }

  return TOKEN_IDENTIFIER;
}
