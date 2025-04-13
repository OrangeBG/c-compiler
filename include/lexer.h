#ifndef LEXER
#define LEXER

typedef enum {
  TOKEN_KEYWORD,
  TOKEN_IDENTIFIER,
  TOKEN_CONSTANT_INT,
  TOKEN_OPEN_PAREN,
  TOKEN_CLOSE_PAREN,
  TOKEN_OPEN_BRACE,
  TOKEN_CLOSE_BRACE,
  TOKEN_SEMICOLON
} TokenType;

typedef struct {
  TokenType type;
  char* value;
} Token;

typedef struct {
  int capacity;
  int count;
  Token* tokens;
} Tokens;

Tokens* init_tokens();
void load_tokens(Tokens* tokens, char* file);

#endif
