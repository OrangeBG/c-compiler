#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "../include/lexer.h"
#include "../include/parser.h"
#include "../include/assembly.h"

char* load_file(const char *file_path); 

int main(int argc, const char *argv[]) {
  #ifdef _WIN32
    system("cls");
  #else
    system("clear");
  #endif

  char* file;
   
  if (argc == 1) {
    file = load_file("../test-file.txt");
  }
  else if (argc == 2) {
    file = load_file(argv[1]);
  }
  else {
    fprintf(stderr, "Too many arguments\n");
  }

  Lexer lexer = init_lexer();
  load_tokens(&lexer, file);
  print_tokens(&lexer, file);

  AstNode *ast = parse(lexer.tokens, lexer.token_count, file);

  print_ast(ast, 0);

  //TODO: Can we free the lexer tokens after this?

  generate_assembly(ast);

  return 0;
}

char* load_file(const char *file_path) {
  FILE* file = fopen(file_path, "rb");

  if (file == NULL) {
    fprintf(stderr, "Could not open file: \"%s\".\n", file_path);
    exit(1);
  }

  fseek(file, 0L, SEEK_END);
  size_t file_size = ftell(file);
  rewind(file);

  char* buffer = (char*)malloc(file_size + 1);

  if (buffer == NULL) {
    fprintf(stderr, "Not enough memory to read: \"%s\".\n", file_path);
    exit(1);
  }

  size_t bytes_read = fread(buffer, sizeof(char), file_size, file);
  buffer[bytes_read] = '\0';

  fclose(file);
  return buffer;
}
