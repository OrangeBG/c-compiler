#ifndef SA_TYPE_CHECK
#define SA_TYPE_CHECK

#include "../include/parser.h"
#include "declaration_symbol.h"

void sa_type_check(AstNode *ast_nodes, DeclarationSymbolTable *declaration_table, Arena *ast_arena);

#endif
