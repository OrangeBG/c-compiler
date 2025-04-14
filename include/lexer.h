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
  int start_index;
  int end_index;
  int line;
} Token;

typedef struct {
  int start_index;
  int current_index;
  int line;
  int token_capacity;
  int token_count;
  Token* tokens;
} Lexer;

Lexer init_lexer();
void load_tokens(Lexer* lexer, char* file);
void print_tokens(Lexer* lexer, char* file);
#endif
