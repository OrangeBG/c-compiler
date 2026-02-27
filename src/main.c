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
#include "../include/error.h"

static char* load_file(const char *file_path); 

int main(int argc, const char *argv[]) {  
  char* file;
  bool print_debug = true;
   
  //@Temporary: These arguments are to support immediate testing while developing the compiler
  switch (argc) {
    case 1:    file = load_file("../test-file.c"); break;
    case 2:    load_file(argv[1]); break;
    case 3: {
      if (*argv[1] != 't') {
        input_error("Invalid command '%s'", argv[1]);
      }

      print_debug = false;
      file = load_file(argv[2]);
      break;
    }
    default:
      input_error("Too many arguments");    
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
  parse_ast(&parser_results, lexer.tokens, lexer.tokens->count, file);

  if (print_debug) {
    printf("\n>> AST PRINT <<\n\n");
    AstNode *program_node = arena_get_by_index(parser_results.ast_node_arena, 0);
    print_ast(program_node, 0);
  }

  AstNode *program_node = arena_get_by_index(parser_results.ast_node_arena, 0);
  sa_variable_resolution(program_node);

  SymbolTable symbol_table;
  symbol_table_init(&symbol_table);
  
  sa_type_check(&parser_results, &symbol_table);
  sa_loop_labeling(program_node);
  sa_goto_check(program_node);

  if (print_debug) {
    printf("\n>> SEMANTIC PRINT <<\n\n");
    AstNode *program_node = arena_get_by_index(parser_results.ast_node_arena, 0);
    print_ast(program_node, 0);
  }

  IRNode *ir = generate_intermediate_rep(program_node, &symbol_table);

  if (print_debug) {
    printf("\n>> IR PRINT <<\n\n");
    print_intermediate_ret(ir);
  }

  arena_free(parser_results.ast_node_arena);

  AsmBackendSymbolTable backend_symbol_table;
  backend_symbol_table_init(&backend_symbol_table);

  AsmNode *asm_nodes = generate_assembly(ir, &symbol_table, &backend_symbol_table);
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
    system("clang -c assembly.asm -o assembly.o");
  #else 
    system("clang -arch x86_64 -c assembly.asm -o assembly.o");
  #endif

  return EXIT_SUCCESS;
}

static char* load_file(const char *file_path) {
  FILE* file = fopen(file_path, "rb");

  if (file == NULL) {
    panic("Could not open file: \"%s\"", file_path);
  }

  fseek(file, 0L, SEEK_END);
  size_t file_size = ftell(file);
  rewind(file);

  char* buffer = (char*)malloc(file_size + 1);

  if (buffer == NULL) {
    panic("Not enough memory to read: \"%s\"", file_path);
  }

  size_t bytes_read = fread(buffer, sizeof(char), file_size, file);
  buffer[bytes_read] = '\0';

  fclose(file);
  return buffer;
}
