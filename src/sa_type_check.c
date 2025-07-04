#include <stdio.h>
#include <stdlib.h>
#include "../include/sa_type_check.h"
#include "../include/hash_table.h"

typedef enum {
  TYPE_INT
} ValueType;

typedef enum {
  SYMBOL_VARIABLE,
  SYMBOL_FUNCTION
} SymbolType;

typedef struct FunctionSymbol {
  bool defined;
  ValueType value_type;
  int param_count;
} FunctionSymbol;

typedef struct VariableSymbol {
  ValueType value_type;
} VariableSymbol;

typedef struct {
  SymbolType type;
  union {
    FunctionSymbol *function_symbol;
    VariableSymbol *variable_symbol_type;
  } data;
} TypeCheckSymbol;

void sa_function_and_variable_type_check(AstNode *node, HashTable *symbols);

void sa_type_check(AstNode *ast_nodes) {
  HashTable symbols;
  hash_table_init(&symbols);

  for (int i = 0; i < ast_nodes->data.program.function_count; i++) {
    sa_function_and_variable_type_check(&ast_nodes->data.program.function_declarations[i], &symbols);
  } 
}

void sa_function_and_variable_type_check(AstNode *node, HashTable *symbols) {
  switch (node->type) {
    case AST_VARIABLE_DECLARATION: {
      char *identifier = node->data.variable_declaration.name;

      //This pass happens after variable resolution, so no need to check to see if the variable is duplicated in the hash table
      TypeCheckSymbol *symbol = malloc(sizeof(TypeCheckSymbol));
      symbol->type = SYMBOL_VARIABLE;

      VariableSymbol *variable_symbol = malloc(sizeof(VariableSymbol));
      variable_symbol->value_type = TYPE_INT;

      symbol->data.variable_symbol_type = variable_symbol;

      HashTableEntry *entry = malloc(sizeof(HashTableEntry));
      entry->key = identifier;
      entry->value.type = HASH_STRUCT;
      entry->value.structure = symbol;

      hash_table_add_entry(symbols, entry); 

      if (node->data.variable_declaration.has_expression) {
        sa_function_and_variable_type_check(node->data.variable_declaration.init_expression, symbols);
      }
      break;
    }
    case AST_FUNCTION_DECLARATION: {
      HashTableEntry *entry = hash_table_get_entry(symbols, node->data.function_declaration.name);
      bool is_defined = false;

      if (entry != NULL && entry->key != NULL) {
        TypeCheckSymbol *existing_function_symbol = (TypeCheckSymbol*)entry->value.structure;

        if (existing_function_symbol->data.function_symbol->value_type != TYPE_INT) {
          fprintf(stderr, "ERROR - SA Type Check: Incompatible function declarations for '%s\n'", entry->key);
          exit(1);
        }

        is_defined = existing_function_symbol->data.function_symbol->defined;

        if (existing_function_symbol->data.function_symbol->defined && node->data.function_declaration.body_block != NULL) {
          fprintf(stderr, "ERROR - SA Type Check: Function defined more than once '%s'\n", entry->key);
          exit(1);
        }

        break;
      }

      TypeCheckSymbol *new_symbol = malloc(sizeof(TypeCheckSymbol));
      new_symbol->type = SYMBOL_FUNCTION;

      FunctionSymbol *function_symbol = malloc(sizeof(FunctionSymbol));
      function_symbol->defined = is_defined;
      function_symbol->value_type = TYPE_INT;

      new_symbol->data.function_symbol = function_symbol;

      HashValue *new_value = malloc(sizeof(HashValue));
      new_value->type = HASH_STRUCT;
      new_value->structure = new_symbol;

      HashTableEntry *new_entry = malloc(sizeof(HashTableEntry));
      new_entry->key = node->data.function_declaration.name;
      new_entry->value.structure = new_value;

      hash_table_add_entry(symbols, new_entry);

        
      for (int i = 0; i < node->data.function_declaration.parameter_count; i++) {
        sa_function_and_variable_type_check(&node->data.function_declaration.parameters[i], symbols);
      }

      if (node->data.function_declaration.body_block != NULL) {
        sa_function_and_variable_type_check(node->data.function_declaration.body_block, symbols);
      }
      break;
    }
    case AST_FUNCTION_PARAMETER: {
      if (node->data.function_parameters.type == AST_PARAMETER_VOID) {
        break;
      }
      
      char *identifier = node->data.function_parameters.name;

      //This pass happens after variable resolution, so no need to check to see if the variable is duplicated in the hash table
      TypeCheckSymbol *symbol = malloc(sizeof(TypeCheckSymbol));
      symbol->type = SYMBOL_VARIABLE;

      VariableSymbol *variable_symbol = malloc(sizeof(VariableSymbol));
      variable_symbol->value_type = TYPE_INT;

      symbol->data.variable_symbol_type = variable_symbol;

      HashTableEntry *entry = malloc(sizeof(HashTableEntry));
      entry->key = identifier;
      entry->value.type = HASH_STRUCT;
      entry->value.structure = symbol;

      hash_table_add_entry(symbols, entry); 
      break;
    }
    case AST_EXPRESSION_FUNCTION_CALL: {
      HashTableEntry *entry = hash_table_get_entry(symbols, node->data.function_call_expression.identfier);

      if (entry != NULL && entry->key != NULL) {
        TypeCheckSymbol *existing_symbol = entry->value.structure;

        if (existing_symbol->type == SYMBOL_VARIABLE) {
          fprintf(stderr, "ERROR - SA Type Check: Variable '%s' is used as a function name", node->data.function_call_expression.identfier);
          exit(1);
        } 

        
      }

      break;
    }
    case AST_BLOCK: {
      for (int i = 0; i < node->data.block.block_count; i++) {   
        sa_function_and_variable_type_check(&node->data.block.block_items[i], symbols);
      }
      break;
    }
    case AST_STATEMENT_IF: {
      sa_function_and_variable_type_check(node->data.if_statement.condition_expression, symbols);
      sa_function_and_variable_type_check(node->data.if_statement.then_statement, symbols);

      if (node->data.if_statement.else_statement != NULL) {
        sa_function_and_variable_type_check(node->data.if_statement.else_statement, symbols);
      }
      break;
    }
    case AST_STATEMENT_RETURN: {
      sa_function_and_variable_type_check(node->data.return_statement.expression, symbols);
      break;
    }
    case AST_STATEMENT_EXPRESSION: {
      sa_function_and_variable_type_check(node->data.expression_statement.expression, symbols);
      break;
    }
    case AST_STATEMENT_FOR: {
      if (node->data.for_statement.for_loop_init != NULL) {
        sa_function_and_variable_type_check(node->data.for_statement.for_loop_init, symbols);
      }

      if (node->data.for_statement.condition_expression != NULL) {
        sa_function_and_variable_type_check(node->data.for_statement.condition_expression, symbols);
      }

      if (node->data.for_statement.post_expression != NULL) {
        sa_function_and_variable_type_check(node->data.for_statement.post_expression, symbols);
      }

      sa_function_and_variable_type_check(node->data.for_statement.statement_body, symbols);
      break;
    }
    case AST_STATEMENT_WHILE: {
      sa_function_and_variable_type_check(node->data.while_statement.condition, symbols);
      sa_function_and_variable_type_check(node->data.while_statement.statement_body, symbols);
      break;
    }
    case AST_STATEMENT_DO_WHILE: {
      sa_function_and_variable_type_check(node->data.do_while_statement.condition, symbols);
      sa_function_and_variable_type_check(node->data.do_while_statement.statement_body, symbols);
      break;
    }
    case AST_EXPRESSION_ASSIGNMENT: {
      sa_function_and_variable_type_check(node->data.assignement_expression.left_expression, symbols);
      sa_function_and_variable_type_check(node->data.assignement_expression.right_expression, symbols);
      break;
    }
    case AST_EXPRESSION_BINARY: {
      sa_function_and_variable_type_check(node->data.binary_expression.left_expression, symbols);
      sa_function_and_variable_type_check(node->data.binary_expression.right_expression, symbols);
      break;
    }
    case AST_EXPRESSION_POSTFIX_INCREMENT:
    case AST_EXPRESSION_POSTFIX_DECREMENT:
    case AST_EXPRESSION_PREFIX_INCREMENT:
    case AST_EXPRESSION_PREFIX_DECREMENT: 
      sa_function_and_variable_type_check(node->data.increment_decrement_expression.expression, symbols);
      break;
    case AST_EXPRESSION_UNARY:
      sa_function_and_variable_type_check(node->data.unary_expression.expression, symbols);
      break;
  }  
}
