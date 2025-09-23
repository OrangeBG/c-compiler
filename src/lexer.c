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
  "TOKEN_CONSTANT_LONG",
  "TOKEN_CONSTANT_UNSIGNED_INT",
  "TOKEN_CONSTANT_UNSIGNED_LONG",
  "TOKEN_CONTINUE",
  "TOKEN_DECREMENT",
  "TOKEN_DO",
  "TOKEN_DOUBLE",
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
  "TOKEN_LONG",
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
  "TOKEN_SIGNED",
  "TOKEN_STATIC",
  "TOKEN_UNSIGNED",
  "TOKEN_VOID", 
  "TOKEN_WHILE",
  "TOKEN_EOF"
};

static bool is_alpha_char(char character);
static bool is_numeric_char(char character);
static bool peek_next(Lexer *lexer, char *file, char find_character); 
static void add_token(TokenType type, Lexer *lexer);
static void add_number_token(Lexer *lexer, char *file); 
static void add_identifier_token(Lexer *lexer, char *file); 
static TokenType check_keyword(int start, int length, char *rest, TokenType type, Lexer *lexer, char *file); 
static TokenType get_identifier_type(Lexer *lexer, char *file); 
 
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

static void add_token(TokenType type, Lexer *lexer) {  
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

static bool is_alpha_char(char character) {
  if ((character >= 'a' && character <= 'z') || (character >= 'A' && character <= 'Z')) {
    return true;
  }

  return false;
}

static bool is_numeric_char(char character) {
  if (character >= '0' && character <= '9') {
    return true;
  } 

  return false;
}

static void add_number_token(Lexer *lexer, char *file) {
  while (file[lexer->current_index + 1] != '\0' && is_numeric_char(file[lexer->current_index + 1])) {
    lexer->current_index++;
  }

  if ((file[lexer->current_index + 1] == 'U' || file[lexer->current_index + 1] == 'u' || file[lexer->current_index + 1] == 'L' || file[lexer->current_index + 1] == 'l') && (file[lexer->current_index + 2] == 'U' || file[lexer->current_index + 2] == 'u' || file[lexer->current_index + 2] == 'L' || file[lexer->current_index + 2] == 'l')) {
    add_token(TOKEN_CONSTANT_UNSIGNED_LONG, lexer);
    lexer->current_index += 2;
    if (is_alpha_char(file[lexer->current_index + 1])) {
      fprintf(stderr, "ERROR - Lexer: Invalid character '%c' in number (line %d)\n", file[lexer->current_index + 1], lexer->line);
      exit(1);
    } 
    return;
  }

  if (file[lexer->current_index + 1] == 'L' || file[lexer->current_index + 1] == 'l') {
    add_token(TOKEN_CONSTANT_LONG, lexer);
    lexer->current_index++;
    if (is_alpha_char(file[lexer->current_index + 1])) {
      fprintf(stderr, "ERROR - Lexer: Invalid character '%c' in number (line %d)\n", file[lexer->current_index + 1], lexer->line);
      exit(1);
    } 
    return;
  }

  if (file[lexer->current_index + 1] == 'U' || file[lexer->current_index + 1] == 'u') {
    add_token(TOKEN_CONSTANT_UNSIGNED_INT, lexer);
    lexer->current_index++;
    if (is_alpha_char(file[lexer->current_index + 1])) {
      fprintf(stderr, "ERROR - Lexer: Invalid character '%c' in number (line %d)\n", file[lexer->current_index + 1], lexer->line);
      exit(1);
    } 
    return;
  }

  if (is_alpha_char(file[lexer->current_index + 1])) {
    fprintf(stderr, "ERROR - Lexer: Invalid character '%c' in number (line %d)\n", file[lexer->current_index + 1], lexer->line);
    exit(1);
  } 

  add_token(TOKEN_CONSTANT_INT, lexer); 
}

static void add_identifier_token(Lexer *lexer, char *file) {
  while (file[lexer->current_index + 1] != '\0' && (is_alpha_char(file[lexer->current_index + 1]) || is_numeric_char(file[lexer->current_index + 1]) || file[lexer->current_index + 1] == '_')) {
    lexer->current_index++;
  }
  
  TokenType type = get_identifier_type(lexer, file);

  add_token(type, lexer);
}

static TokenType check_keyword(int start, int length, char *rest, TokenType type, Lexer *lexer, char *file) { 
  if (lexer->current_index - lexer->start_index == (start + length) - 1 && memcmp(&file[lexer->start_index + start], rest, length) == 0) {
    return type;
  }

  return TOKEN_IDENTIFIER;
}

static TokenType get_identifier_type(Lexer *lexer, char *file) {
  switch (file[lexer->start_index]) {
    case 'b': return check_keyword(1, 4, "reak", TOKEN_BREAK, lexer, file);
    case 'c': return check_keyword(1, 7, "ontinue", TOKEN_CONTINUE, lexer, file);
    case 'd': {
      if (lexer->current_index - lexer->start_index > 0) {
        switch (file[lexer->start_index + 1]) {
          case 'o': { 
            if ((lexer->current_index + 1) - lexer->start_index > 0) {
              switch (file[lexer->start_index + 2]) {
                case 'u': return check_keyword(3, 3, "ble", TOKEN_DOUBLE, lexer, file);              
              }
            }
            return check_keyword(2, 0, "", TOKEN_DO, lexer, file);
          }
        }
      }
    }
    case 'e': {
      if (lexer->current_index - lexer->start_index > 0) {
        switch (file[lexer->start_index + 1]) {
          case 'l': return check_keyword(2, 2, "se", TOKEN_ELSE, lexer, file);
          case 'x': return check_keyword(2, 4, "tern", TOKEN_EXTERN, lexer, file);
        }
      }
    }
    case 'f': return check_keyword(1, 2, "or", TOKEN_FOR, lexer, file);
    case 'g': return check_keyword(1, 3, "oto", TOKEN_GOTO, lexer, file);
    case 'i': {
      if (lexer->current_index - lexer->start_index > 0) {
        switch (file[lexer->start_index + 1]) {
          case 'n': return check_keyword(2, 1, "t", TOKEN_INT, lexer, file);
          case 'f': return TOKEN_IF;
        }
      } 
      break;
     }
    case 'l': return check_keyword(1, 3, "ong", TOKEN_LONG, lexer, file);
    case 'r': return check_keyword(1, 5, "eturn", TOKEN_RETURN, lexer, file);
    case 's': {
      if (lexer->current_index - lexer->start_index > 0) {
        switch (file[lexer->start_index + 1]) {
          case 'i': return check_keyword(2, 4, "gned", TOKEN_SIGNED, lexer, file);
          case 't': return check_keyword(2, 4, "atic", TOKEN_STATIC, lexer, file);
        }
      } 
      break;
    }
    case 'u': return check_keyword(1, 7, "nsigned", TOKEN_UNSIGNED, lexer, file);
    case 'v': return check_keyword(1, 3, "oid", TOKEN_VOID, lexer, file);
    case 'w': return check_keyword(1, 4, "hile", TOKEN_WHILE, lexer, file);
  }

  return TOKEN_IDENTIFIER;
}

static bool peek_next(Lexer *lexer, char *file, char find_character) {
  if (file[lexer->current_index + 1] == '\0') {
    return false;
  }

  if (file[lexer->current_index + 1] == find_character) {
    return true;
  }

  return false;
}
