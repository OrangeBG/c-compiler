#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include "../include/lexer.h"
#include "../include/parser.h"
#include "../include/assembly.h"
#include "../include/code_emit.h"
#include "../include/intermediate_rep.h"
#include "../include/sa_variable_resolution.h"
#include "../include/sa_loop_labeling.h"
#include "../include/sa_type_check.h"
#include "../include/sa_goto_check.h"

static char* load_file(const char *file_path); 

int main(int argc, const char *argv[]) {  
  char* file;
  bool print_debug = true;
   
  //@Temporary: These arguments are to support immediate testing while developing the compiler
  if (argc == 1) {
    file = load_file("../test-file.c");
  } else if (argc == 2) {
    file = load_file(argv[1]);
  } else if (argc == 3) {
    //To disable printing when running the tester
    if (*argv[1] == 't') {
      print_debug = false;
      file = load_file(argv[2]);
    } else {
      fprintf(stderr, "Invalid command '%s'\n", argv[1]);
      exit(1);
    }
  } else {
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

  ParserResults parser_results;
  parse_ast(&parser_results, lexer.tokens, lexer.token_count, file);

  if (print_debug) {
    printf("\n>> AST PRINT <<\n\n");
    AstNode *program_node = arena_get_by_index(parser_results.ast_node_arena, 0);
    print_ast(program_node, 0);
  }

  AstNode *program_node = arena_get_by_index(parser_results.ast_node_arena, 0);
  sa_variable_resolution(program_node);

  DeclarationSymbolTable declaration_symbol_table;
  declaration_symbol_table_init(&declaration_symbol_table);
  
  sa_type_check(&parser_results, &declaration_symbol_table);
  sa_loop_labeling(program_node);
  sa_goto_check(program_node);

  if (print_debug) {
    printf("\n>> SEMANTIC PRINT <<\n\n");
    AstNode *program_node = arena_get_by_index(parser_results.ast_node_arena, 0);
    print_ast(program_node, 0);
  }

  IRNode *ir = generate_intermediate_rep(program_node, &declaration_symbol_table);

  if (print_debug) {
    printf("\n>> IR PRINT <<\n\n");
    print_intermediate_ret(ir);
  }

  arena_free(parser_results.ast_node_arena);

  AsmBackendSymbolTable backend_symbol_table;
  backend_symbol_table_init(&backend_symbol_table);

  AsmNode *asm_nodes = generate_assembly(ir, &declaration_symbol_table, &backend_symbol_table);
  if (print_debug) {
    printf("\n>> ASSEMBLY PRINT <<\n\n");
    print_assembly(asm_nodes);
  }

  FILE *assembly_file;
  assembly_file = fopen("assembly.asm", "w+");
  save_assembly_file(asm_nodes, assembly_file);
    
  if (print_debug) {
    printf("\n>> CODE EMIT PRINT <<\n\n");
    rewind(assembly_file);
    print_code_emit(assembly_file);
  }
    
  fclose(assembly_file);  

  #ifdef __x86_64__
    // system("as assembly.asm -o assembly.o");
    system("clang -c assembly.asm -o assembly.o");
  #else 
    system("clang -arch x86_64 -c assembly.asm -o assembly.o");
  #endif

  return EXIT_SUCCESS;
}

static char* load_file(const char *file_path) {
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
