#ifndef SA_TYPE_CHECK
#define SA_TYPE_CHECK

#include "../include/parser.h"
#include "../include/hash_table.h"

void sa_type_check(AstNode *ast_nodes, HashTable *declaration_symbols, Arena *ast_arena);

#endif
