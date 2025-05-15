#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "../include/lexer.h"
#include "../include/parser.h"
#include "../include/assembly.h"
#include "../include/code_emit.h"
#include "../include/intermediate_rep.h"

char* load_file(const char *file_path); 

int main(int argc, const char *argv[]) {  
  char* file;
  bool print_debug = true;
   
  if (argc == 1) {
    file = load_file("../test-file.txt");
  }
  else if (argc == 2) {
    file = load_file(argv[1]);
  }
  else if (argc == 3) {
    //To disable printing when running the tester
    if (*argv[1] == 't') {
      print_debug = false;
      file = load_file(argv[2]);
    } else {
      fprintf(stderr, "Invalid command '%s'\n", argv[1]);
      exit(1);
    }
  }
  else {
    fprintf(stderr, "Too many arguments\n");
    exit(1);
  }

  if (print_debug) {
    #ifdef _WIN32
      system("cls");
    #else
      system("clear");
    #endif
  }

  Lexer lexer = init_lexer();
  load_tokens(&lexer, file);

  if (print_debug) {
    printf("\n>> LEXER PRINT <<\n\n");
    print_tokens(&lexer, file);
  }

  AstNode *ast = parse_ast(lexer.tokens, lexer.token_count, file);

  if (print_debug) {
    printf("\n>> AST PRINT <<\n\n");
    print_ast(ast, 0);
  }

  IRNode *ir = generate_intermediate_rep(ast);

  if (print_debug) {
    printf("\n>> IR PRINT <<\n\n");
    print_intermediate_ret(ir);
  }

  //TODO: Can we free the lexer tokens after this?

  AsmNode *asm_nodes = generate_assembly(ir);

  if (print_debug) {
    printf("\n>> ASSEMBLY PRINT <<\n\n");
    print_assembly(asm_nodes);

    FILE *assembly_file;
    assembly_file = fopen("assembly.asm", "w");

    save_assembly_file(asm_nodes, assembly_file);

    fclose(assembly_file);    
    system("clang -arch x86_64 -c assembly.asm -o assembly.o");
    
    printf("\n>> CODE EMIT PRINT <<\n\n");
    print_code_emit(asm_nodes);
  }

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
