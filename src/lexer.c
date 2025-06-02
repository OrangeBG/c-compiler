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
  "TOKEN_CLOSE_BRACE",
  "TOKEN_CLOSE_PAREN",
  "TOKEN_COLON",
  "TOKEN_CONSTANT_INT",
  "TOKEN_DECREMENT",
  "TOKEN_ELSE",
  "TOKEN_EQUAL",
  "TOKEN_FORWARD_SLASH",
  "TOKEN_FORWARD_SLASH_EQUAL",
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
  "TOKEN_VOID", 
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
      case '~': add_token(TOKEN_BITWISE_NOT, lexer); break;
      case '?': add_token(TOKEN_QUESTION_MARK, lexer); break;
      case ':': add_token(TOKEN_COLON, lexer); break;
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

        // lexer->start_index++;
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

        // lexer->start_index++;
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
    // lexer->start_index++;
    // lexer->current_index = lexer->start_index;
  }  

  add_token(TOKEN_EOF, lexer);
}

void print_tokens(Lexer *lexer, char *file) {
  for (int i = 0; i < lexer->token_count; i++) {
    printf("line %d     ", lexer->tokens[i].line);
    switch (lexer->tokens[i].type) {       
      case TOKEN_ASTERISK: printf("Asterisk   "); break;
      case TOKEN_ASTERISK_EQUAL: printf("Asterisk   "); break;
      case TOKEN_BITWISE_AND: printf("Ampersand  "); break;
      case TOKEN_BITWISE_AND_EQUAL: printf("Ampersand Equal"); break;
      case TOKEN_BITWISE_NOT: printf("Tilde      "); break;
      case TOKEN_BITWISE_OR: printf("Pipe       "); break;
      case TOKEN_BITWISE_OR_EQUAL: printf("Pipe Equal "); break;
      case TOKEN_BITWISE_XOR: printf("Caret      "); break;
      case TOKEN_BITWISE_XOR_EQUAL: printf("Caret Equal"); break;
      case TOKEN_BITWISE_RIGHT_SHIFT: printf("R. Shift   "); break;
      case TOKEN_BITWISE_RIGHT_SHIFT_EQUAL: printf("R. Shift Equal"); break;
      case TOKEN_BITWISE_LEFT_SHIFT: printf("L. Shift   "); break;
      case TOKEN_BITWISE_LEFT_SHIFT_EQUAL: printf("L. Shift Equal"); break;
      case TOKEN_CLOSE_BRACE: printf("Close Brace"); break;
      case TOKEN_CLOSE_PAREN: printf("Close Paren"); break;
      case TOKEN_COLON: printf("Colon      "); break;
      case TOKEN_CONSTANT_INT: printf("Constant   "); break;
      case TOKEN_DECREMENT: printf("Decrement  "); break;
      case TOKEN_ELSE: printf("Else       "); break;
      case TOKEN_EQUAL: printf("Equal      "); break;
      case TOKEN_FORWARD_SLASH: printf("Forward Slash"); break;
      case TOKEN_FORWARD_SLASH_EQUAL: printf("Forward Slash Equal"); break;
      case TOKEN_IDENTIFIER: printf("Identifier ");  break;
      case TOKEN_IF: printf("If         "); break;
      case TOKEN_INCREMENT: printf("Increment  "); break;
      case TOKEN_INT: printf("Int        ");  break;
      case TOKEN_NEGATION: printf("Negation   "); break;
      case TOKEN_NEGATION_EQUAL: printf("Negation Equal"); break;
      case TOKEN_OPEN_PAREN: printf("Open Paren "); break;
      case TOKEN_OPEN_BRACE: printf("Open Brace "); break;
      case TOKEN_PERCENT: printf("Percent    "); break;
      case TOKEN_PERCENT_EQUAL: printf("Percent Equal"); break;
      case TOKEN_PLUS: printf("Plus       "); break;
      case TOKEN_PLUS_EQUAL: printf("Plus Equal "); break;
      case TOKEN_QUESTION_MARK: printf("Question Mark"); break;
      case TOKEN_LOGICAL_AND: printf("And        "); break;
      case TOKEN_LOGICAL_OR: printf("Or         "); break;
      case TOKEN_LOGICAL_NOT: printf("Not        "); break;
      case TOKEN_RELATIONAL_EQUAL: printf("Rel. Equal "); break;
      case TOKEN_RELATIONAL_NOT_EQUAL: printf("Not Equal  "); break;
      case TOKEN_RELATIONAL_LESS_THAN: printf("Less Than  "); break;
      case TOKEN_RELATIONAL_LESS_OR_EQUAL: printf("Less or Equal"); break;
      case TOKEN_RELATIONAL_GREATER_THAN: printf("Greater Than"); break;
      case TOKEN_RELATIONAL_GREATER_OR_EQUAL: printf("Greater or Equal"); break;
      case TOKEN_RETURN: printf("Return     "); break;
      case TOKEN_SEMICOLON: printf("Semicolon  "); break;
      case TOKEN_VOID: printf("Void       "); break;
      case TOKEN_EOF: printf("\n"); return;
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

TokenType get_identifier_type(Lexer *lexer, char *file) {
  //TODO: Need to support the rest of the keywords
  //TODO: Having start point be at the current index seems wrong
  switch (file[lexer->start_index]) {
    case 'e': return check_keyword(0, 3, "lse", TOKEN_ELSE, lexer, file);
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
    case 'v': return check_keyword(0, 3, "oid", TOKEN_VOID, lexer, file);
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
