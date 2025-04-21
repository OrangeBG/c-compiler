#ifndef LEXER
#define LEXER

typedef enum {
  TOKEN_BITWISE_NOT, 
  TOKEN_CLOSE_BRACE,
  TOKEN_CLOSE_PAREN,
  TOKEN_CONSTANT_INT,
  TOKEN_DECREMENT,
  TOKEN_IDENTIFIER,
  TOKEN_INT,
  TOKEN_NEGATION,
  TOKEN_OPEN_PAREN,
  TOKEN_OPEN_BRACE,
  TOKEN_RETURN,
  TOKEN_SEMICOLON,
  TOKEN_VOID
} TokenType;

extern const char* TokenTypeStr[];

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
void load_tokens(Lexer *lexer, char *file);
void print_tokens(Lexer *lexer, char *file);

#endif
