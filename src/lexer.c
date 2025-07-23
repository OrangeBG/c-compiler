#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "../include/lexer.h"

#define TOKEN_ARRAY_START_SIZE 64

const char* TokenTypeStr[] = {
  "TOKEN_ASTERISK",
  "TOKEN_ASTERISK_EQUAL",
  "TOKEN_BITWISE_AND",
  "TOKEN_BITWISE_AND_EQUAL",
  "TOKEN_BITWISE_NOT",
  "TOKEN_BITWISE_OR",
  "TOKEN_BITWISE_OR_EQUAL",
  "TOKEN_BITWISE_XOR", 
  "TOKEN_BITWISE_XOR_EQUAL", 
  "TOKEN_BITWISE_LEFT_SHIFT",
  "TOKEN_BITWISE_LEFT_SHIFT_EQUAL",
  "TOKEN_BITWISE_RIGHT_SHIFT",
  "TOKEN_BITWISE_RIGHT_SHIFT_EQUAL",
  "TOKEN_BREAK",
  "TOKEN_CLOSE_BRACE",
  "TOKEN_CLOSE_PAREN",
  "TOKEN_COLON",
  "TOKEN_COMMA",
  "TOKEN_CONSTANT_INT",
  "TOKEN_CONTINUE",
  "TOKEN_DECREMENT",
  "TOKEN_DO",
  "TOKEN_ELSE",
  "TOKEN_EQUAL",
  "TOKEN_EXTERN",
  "TOKEN_FOR",
  "TOKEN_FORWARD_SLASH",
  "TOKEN_FORWARD_SLASH_EQUAL",
  "TOKEN_GOTO",
  "TOKEN_IDENTIFIER",
  "TOKEN_IF",
  "TOKEN_INCREMENT",
  "TOKEN_INT",
  "TOKEN_LOGICAL_AND",
  "TOKEN_LOGICAL_OR",
  "TOKEN_LOGICAL_NOT",  
  "TOKEN_NEGATION",
  "TOKEN_NEGATION_EQUAL",
  "TOKEN_OPEN_PAREN",
  "TOKEN_OPEN_BRACE",
  "TOKEN_PERCENT",
  "TOKEN_PERCENT_EQUAL",
  "TOKEN_PLUS",
  "TOKEN_PLUS_EQUAL",
  "TOKEN_QUESTION_MARK",
  "TOKEN_RELATIONAL_EQUAL",
  "TOKEN_RELATIONAL_NOT_EQUAL",
  "TOKEN_RELATIONAL_LESS_THAN",
  "TOKEN_RELATIONAL_LESS_OR_EQUAL",
  "TOKEN_RELATIONAL_GREATER_THAN",
  "TOKEN_RELATIONAL_GREATER_OR_EQUAL",
  "TOKEN_RETURN",
  "TOKEN_SEMICOLON",
  "TOKEN_STATIC",
  "TOKEN_VOID", 
  "TOKEN_WHILE",
  "TOKEN_EOF"
};

bool is_alpha_char(char character);
bool is_numeric_char(char character);
bool peek_next(Lexer *lexer, char *file, char find_character); 
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
    .tokens = NULL,
  };

  return lexer;
}

void load_tokens(Lexer *lexer, char *file) {
  while (true) {
    char cur_char = file[lexer->start_index];
    if (cur_char == '\0') {
      break;
    }

    if (is_alpha_char(cur_char) || cur_char == '_') {
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
      case '~': add_token(TOKEN_BITWISE_NOT, lexer); break;
      case '?': add_token(TOKEN_QUESTION_MARK, lexer); break;
      case ':': add_token(TOKEN_COLON, lexer); break;
      case ',': add_token(TOKEN_COMMA, lexer); break;
      case '+': {
          if (peek_next(lexer, file, '+')) {
            lexer->current_index++;
            add_token(TOKEN_INCREMENT, lexer);
            break;
          }

          if (peek_next(lexer, file, '=')) {
            lexer->current_index++;
            add_token(TOKEN_PLUS_EQUAL, lexer);
            break;
          }
          
          add_token(TOKEN_PLUS, lexer);
          break;
      }
      case '-': {
        if (peek_next(lexer, file, '-')) {
          lexer->current_index += 1; 
          add_token(TOKEN_DECREMENT, lexer);
          break;
        }

        if (peek_next(lexer, file, '=')) {
          lexer->current_index += 1; 
          add_token(TOKEN_NEGATION_EQUAL, lexer);
          break;
        }

        add_token(TOKEN_NEGATION, lexer);
        break;
      }
      case '*': {
        if (peek_next(lexer, file, '=')) {
          lexer->current_index += 1;
          add_token(TOKEN_ASTERISK_EQUAL, lexer);
          break;
        }

        add_token(TOKEN_ASTERISK, lexer);
        break;
      }
      case '/': {
        if (peek_next(lexer, file, '=')) {
          lexer->current_index += 1;
          add_token(TOKEN_FORWARD_SLASH_EQUAL, lexer);
          break;
        }

        if (peek_next(lexer, file, '*')) {
          lexer->current_index += 2;

          while ((file[lexer->current_index] != '*' || (file[lexer->current_index] == '*' && !peek_next(lexer, file, '/'))) && file[lexer->current_index] != '\0') {
            if (file[lexer->current_index] == '\n') {
              lexer->line++;
            }
            lexer->current_index += 1;
          }

          lexer->current_index += 1;
          break;
        }   

        if (peek_next(lexer, file, '/')) {
          lexer->current_index += 1;

          while (file[lexer->current_index] != '\n' && file[lexer->current_index] != '\0') {
            lexer->current_index += 1;
          }
          lexer->line++;
          break;
        }   

        add_token(TOKEN_FORWARD_SLASH, lexer);
        break;
      }
      case '%': {
        if (peek_next(lexer, file, '=')) {
          lexer->current_index += 1;
          add_token(TOKEN_PERCENT_EQUAL, lexer);
          break;
        }

        add_token(TOKEN_PERCENT, lexer);
        break;
      }
      case '=': {
        if (peek_next(lexer, file, '=')) {
          lexer->current_index += 1; 
          add_token(TOKEN_RELATIONAL_EQUAL, lexer);
        } else {
          add_token(TOKEN_EQUAL, lexer);
        }
        break;
      }
      case '!': {
        if (peek_next(lexer, file, '=')) {
          lexer->current_index++;
          add_token(TOKEN_RELATIONAL_NOT_EQUAL, lexer); 
        }
        else {
          add_token(TOKEN_LOGICAL_NOT, lexer); 
        }
        break;
      }
      case '<': {
        if (peek_next(lexer, file, '=')) {
          lexer->current_index++;
          add_token(TOKEN_RELATIONAL_LESS_OR_EQUAL, lexer);
          break;
        }
        
        if (!peek_next(lexer, file, '<')) {
          add_token(TOKEN_RELATIONAL_LESS_THAN, lexer);
          break;
        }

        lexer->current_index++;
        
        if (peek_next(lexer, file, '=')) {
          lexer->current_index++;
          add_token(TOKEN_BITWISE_LEFT_SHIFT_EQUAL, lexer);
          break;
        }

        add_token(TOKEN_BITWISE_LEFT_SHIFT, lexer);
        break;          
      }
      case '>': {
        if (peek_next(lexer, file, '=')) {
          lexer->current_index++;
          add_token(TOKEN_RELATIONAL_GREATER_OR_EQUAL, lexer);
          break;
        }
        
        if (!peek_next(lexer, file, '>')) {
          add_token(TOKEN_RELATIONAL_GREATER_THAN, lexer);
          break;
        }

        lexer->current_index++;
        
        if (peek_next(lexer, file, '=')) {
          lexer->current_index++;
          add_token(TOKEN_BITWISE_RIGHT_SHIFT_EQUAL, lexer);
          break;
        }

        add_token(TOKEN_BITWISE_RIGHT_SHIFT, lexer);
        break;          
      }
      case '&': {
        if (peek_next(lexer, file, '&')) {
          lexer->current_index++; 
          add_token(TOKEN_LOGICAL_AND, lexer);
          break;
        }

        if (peek_next(lexer, file, '=')) {
          lexer->current_index++; 
          add_token(TOKEN_BITWISE_AND_EQUAL, lexer);
          break;
        }

        add_token(TOKEN_BITWISE_AND, lexer);
        break;
      }
      case '|':
        if (peek_next(lexer, file, '|')) {
          lexer->current_index += 1; 
          add_token(TOKEN_LOGICAL_OR, lexer);
          break;
        } 

        if (peek_next(lexer, file, '=')) {
          lexer->current_index++; 
          add_token(TOKEN_BITWISE_OR_EQUAL, lexer);
          break;
        }

        add_token(TOKEN_BITWISE_OR, lexer);
        break;
      case '^':
        if (peek_next(lexer, file, '=')) {
          lexer->current_index++; 
          add_token(TOKEN_BITWISE_XOR_EQUAL, lexer);
          break;
        }

        add_token(TOKEN_BITWISE_XOR, lexer);
        break;
      default:
        fprintf(stderr, "ERROR - Lexer: Invalid token '%c' (line %d)\n", cur_char, lexer->line);
        exit(1);
    }

    lexer->current_index++;
    lexer->start_index = lexer->current_index;
  }  

  add_token(TOKEN_EOF, lexer);
}

void print_tokens(Lexer *lexer, char *file) {
  for (int i = 0; i < lexer->token_count; i++) {
    printf("line %d", lexer->tokens[i].line);
    printf("%*s", 6, "");

    long whitespace = 30 - strlen(TokenTypeStr[lexer->tokens[i].type]);
    printf("%s", TokenTypeStr[lexer->tokens[i].type]);
    printf("%*s", (int)whitespace, "");
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
    lexer->token_capacity = size;
    lexer->tokens = realloc(lexer->tokens, size * sizeof(Token));
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
    fprintf(stderr, "ERROR - Lexer: Invalid character '%c' in number (line %d)\n", file[lexer->current_index + 1], lexer->line);
    exit(1);
  } 

  add_token(TOKEN_CONSTANT_INT, lexer); 
}

void add_identifier_token(Lexer *lexer, char *file) {
  while (file[lexer->current_index + 1] != '\0' && (is_alpha_char(file[lexer->current_index + 1]) || is_numeric_char(file[lexer->current_index + 1]) || file[lexer->current_index + 1] == '_')) {
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

TokenType get_identifier_type(Lexer *lexer, char *file) {
  //TODO: Need to support the rest of the keywords
  //TODO: Having start point be at the current index seems wrong
  switch (file[lexer->start_index]) {
    case 'b': return check_keyword(0, 4, "reak", TOKEN_BREAK, lexer, file);
    case 'c': return check_keyword(0, 7, "ontinue", TOKEN_CONTINUE, lexer, file);
    case 'd': return check_keyword(0, 1, "o", TOKEN_DO, lexer, file);
    case 'e': {
      if (lexer->current_index - lexer->start_index > 0) {
        switch (file[lexer->start_index + 1]) {
          case 'l': return check_keyword(0, 3, "lse", TOKEN_ELSE, lexer, file);
          case 'x': return check_keyword(0, 5, "xtern", TOKEN_EXTERN, lexer, file);
        }
      }
    }
    case 'f': return check_keyword(0, 2, "or", TOKEN_FOR, lexer, file);
    case 'g': return check_keyword(0, 3, "oto", TOKEN_GOTO, lexer, file);
    case 'i': {
      if (lexer->current_index - lexer->start_index > 0) {
        switch (file[lexer->start_index + 1]) {
          case 'n': return check_keyword(0, 2, "nt", TOKEN_INT, lexer, file);
          case 'f': return check_keyword(0, 1, "f", TOKEN_IF, lexer, file);
        }
      } 
      break;
     }
    case 'r': return check_keyword(0, 5, "eturn", TOKEN_RETURN, lexer, file);
    case 's': return check_keyword(0, 5, "tatic", TOKEN_STATIC, lexer, file);
    case 'v': return check_keyword(0, 3, "oid", TOKEN_VOID, lexer, file);
    case 'w': return check_keyword(0, 4, "hile", TOKEN_WHILE, lexer, file);
  }

  return TOKEN_IDENTIFIER;
}

bool peek_next(Lexer *lexer, char *file, char find_character) {
  if (file[lexer->current_index + 1] == '\0') {
    return false;
  }

  if (file[lexer->current_index + 1] == find_character) {
    return true;
  }

  return false;
}
