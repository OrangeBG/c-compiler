#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/sa_type_check.h"
#include "../include/hash_table.h"

//TODO: Check to see how we can better optimize these types of buffers. Exact same use of this buffer is in sa_variable_resolution
#define IDENTIFIER_BUFFER 256

typedef enum {
  TYPE_INT
} ValueType;

typedef enum {
  SYMBOL_VARIABLE,
  SYMBOL_FUNCTION
} SymbolType;

typedef enum {
  ATTRIBUTE_FUNCTION,
  ATTRIBUTE_STATIC,
  ATTRIBUTE_LOCAL
} IdentifierAttributeType;

typedef enum {
  INITIAL_VALUE_TENTATIVE,
  INITIAL_VALUE_INITIAL,
  INITIAL_VALUE_NO_INITIALIZER
} InitialValueType;

typedef struct FunctionSymbol {
  ValueType value_type;
  int param_count;
} FunctionSymbol;

typedef struct VariableSymbol {
  ValueType value_type;
} VariableSymbol;

typedef struct {
  bool defined;
  bool global;
} FunctionAttribute;
 
typedef struct {
  InitialValueType initial_value_type;
  int initial_value;
} StaticAttribute;
 
typedef struct {
  SymbolType symbol_type;
  union {
    FunctionSymbol *function_symbol;
    VariableSymbol *variable_symbol_type;
  } data;
  IdentifierAttributeType identifier_attribute_type;
  union {
    FunctionAttribute *function_attribute;
    StaticAttribute *static_attribute;
  } identifier_attribute;
} TypeCheckSymbol;

void sa_function_and_variable_type_check(AstNode *node, HashTable *symbols, char *function_name);

void sa_type_check(AstNode *ast_nodes) {
  HashTable symbols;
  hash_table_init(&symbols);

  for (int i = 0; i < ast_nodes->data.program.declaration_count; i++) {
    AstNode *function_node = ast_nodes->data.program.declaration_ptrs->node_pointers[i];
    sa_function_and_variable_type_check(function_node, &symbols, function_node->data.function_declaration.name);
  } 
}

void sa_function_and_variable_type_check(AstNode *node, HashTable *symbols, char *function_name) {
  switch (node->type) {
    case AST_VARIABLE_DECLARATION: {
      InitialValueType initial_value_type;
      int initial_value = 0;

      if (node->data.variable_declaration.init_expression->type == AST_EXPRESSION_CONSTANT) {
        initial_value_type = INITIAL_VALUE_INITIAL;
        initial_value = node->data.variable_declaration.init_expression->data.constant_expression.value;
      } else if (node->data.variable_declaration.init_expression == NULL) {
        if (node->data.variable_declaration.storage_class_type == AST_STORAGE_CLASS_EXTERN) {
          initial_value_type = INITIAL_VALUE_NO_INITIALIZER;
        } else {
          initial_value_type = INITIAL_VALUE_TENTATIVE;
        }
      } else {
        fprintf(stderr, "ERROR: SA Type Check: Non-constant initializer");
        exit(1);
      }

      bool is_global = node->data.variable_declaration.storage_class_type != AST_STORAGE_CLASS_STATIC;

      HashTableEntry *entry = hash_table_get_entry(symbols, node->data.variable_declaration.name);

      if (entry != NULL && entry->key != NULL) {
        TypeCheckSymbol *existing_variable_symbol = entry->value->structure;

        if (existing_variable_symbol->data.variable_symbol_type != TYPE_INT) {
          fprintf(stderr, "ERROR: SA Type Check: Function '%s' redeclared as variable", node->data.variable_declaration.name);
          exit(1);
        }

      }



      break;
      // char *identifier = node->data.variable_declaration.name;

      // //This pass happens after variable resolution, so no need to check to see if the variable is duplicated in the hash table
      // TypeCheckSymbol *symbol = malloc(sizeof(TypeCheckSymbol));
      // symbol->symbol_type = SYMBOL_VARIABLE;

      // VariableSymbol *variable_symbol = malloc(sizeof(VariableSymbol));
      // variable_symbol->value_type = TYPE_INT;

      // symbol->data.variable_symbol_type = variable_symbol;

      // HashTableEntry *entry = malloc(sizeof(HashTableEntry));
      // char *symbol_key = malloc(IDENTIFIER_BUFFER); 
      // snprintf(symbol_key, IDENTIFIER_BUFFER, "%s.%s", function_name,  identifier);
      // entry->key = symbol_key;

      // HashValue *value = malloc(sizeof(HashValue));
      // value->type = HASH_STRUCT;
      // value->structure = symbol;

      // entry->value = value;

      // hash_table_add_entry(symbols, entry); 

      // if (node->data.variable_declaration.has_expression) {
      //   sa_function_and_variable_type_check(node->data.variable_declaration.init_expression, symbols, function_name);
      // }
      // break;
    }
    case AST_FUNCTION_DECLARATION: {
      HashTableEntry *entry = hash_table_get_entry(symbols, node->data.function_declaration.name);

      if (entry != NULL && entry->key != NULL) {
        TypeCheckSymbol *existing_function_symbol = entry->value->structure;

        if (existing_function_symbol->data.function_symbol->value_type != TYPE_INT) {
          fprintf(stderr, "ERROR - SA Type Check: Incompatible function declarations for '%s\n'", entry->key);
          exit(1);
        }

        // is_defined = existing_function_symbol->data.function_symbol->defined;

        if (existing_function_symbol->identifier_attribute.function_attribute->defined && node->data.function_declaration.body_block != NULL) {
          fprintf(stderr, "ERROR - SA Type Check: Function defined more than once '%s'\n", entry->key);
          exit(1);
        }

        if (existing_function_symbol->identifier_attribute.function_attribute->global == node->data.function_declaration.storage_class_type == AST_STORAGE_CLASS_STATIC) {
          fprintf(stderr, "ERROR - SA Type Check: Static function '%s' declaration follows non-static", node->data.function_declaration.name);
          exit(1);
        }

        if (!existing_function_symbol->identifier_attribute.function_attribute->defined) {
          existing_function_symbol->identifier_attribute.function_attribute->defined = node->data.function_declaration.body_block != NULL;
        }

        break;
      }

      FunctionSymbol *function_symbol = malloc(sizeof(FunctionSymbol));
      function_symbol->value_type = TYPE_INT;
      function_symbol->param_count = 0;

      TypeCheckSymbol *new_symbol = malloc(sizeof(TypeCheckSymbol));
      new_symbol->symbol_type = SYMBOL_FUNCTION;
      new_symbol->data.function_symbol = function_symbol;
      new_symbol->identifier_attribute_type = ATTRIBUTE_FUNCTION;

      FunctionAttribute *function_attribute = malloc(sizeof(FunctionAttribute));
      function_attribute->defined = node->data.function_declaration.body_block != NULL;
      function_attribute->global = node->data.function_declaration.storage_class_type != AST_STORAGE_CLASS_STATIC;

      new_symbol->identifier_attribute.function_attribute = function_attribute;

      HashValue *new_value = malloc(sizeof(HashValue));
      new_value->type = HASH_STRUCT;
      new_value->structure = new_symbol;

      HashTableEntry *new_entry = malloc(sizeof(HashTableEntry));
      new_entry->key = node->data.function_declaration.name;
      new_entry->value = new_value;

      hash_table_add_entry(symbols, new_entry);

        
      for (int i = 0; i < node->data.function_declaration.parameter_count; i++) {
        AstNode *parameter_node = node->data.function_declaration.parameter_ptrs->node_pointers[i];

        if (parameter_node->data.function_parameters.type == AST_PARAMETER_VOID) {
          continue;
        }

        function_symbol->param_count++;
        sa_function_and_variable_type_check(parameter_node, symbols, function_name);
      }

      if (node->data.function_declaration.body_block != NULL) {
        sa_function_and_variable_type_check(node->data.function_declaration.body_block, symbols, function_name);
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
      symbol->symbol_type = SYMBOL_VARIABLE;

      VariableSymbol *variable_symbol = malloc(sizeof(VariableSymbol));
      variable_symbol->value_type = TYPE_INT;

      symbol->data.variable_symbol_type = variable_symbol;

      HashTableEntry *entry = malloc(IDENTIFIER_BUFFER);

      char *symbol_key = malloc(IDENTIFIER_BUFFER); 
      snprintf(symbol_key, IDENTIFIER_BUFFER, "%s.%s", function_name,  identifier);
      entry->key = symbol_key;

      HashValue *value = malloc(sizeof(HashValue));
      value->type = HASH_STRUCT;
      value->structure = symbol;

      entry->value = value;

      hash_table_add_entry(symbols, entry); 
      break;
    }
    case AST_EXPRESSION_FUNCTION_CALL: {
      HashTableEntry *entry = hash_table_get_entry(symbols, node->data.function_call_expression.identfier);

      if (entry != NULL && entry->key != NULL) {
        TypeCheckSymbol *existing_symbol = entry->value->structure;

        if (existing_symbol->symbol_type == SYMBOL_VARIABLE) {
          fprintf(stderr, "ERROR - SA Type Check: Variable '%s' is used as a function name\n", node->data.function_call_expression.identfier);
          exit(1);
        }               

        if (existing_symbol->data.function_symbol->param_count != node->data.function_call_expression.argument_count) {
          fprintf(stderr, "ERROR - SA Type Check: Function '%s' called with incorrect number of arguments\n", node->data.function_call_expression.identfier);
          exit(1);
        }
      }

      for (int i = 0; i < node->data.function_call_expression.argument_count; i++) {
        AstNode *argument_node = node->data.function_call_expression.argument_ptrs->node_pointers[i];
        sa_function_and_variable_type_check(argument_node, symbols, function_name);
      }
      break;
    }
    case AST_EXPRESSION_VARIABLE: {
      HashTableEntry *entry = hash_table_get_entry(symbols, node->data.variable_expression.identifier);

      if (entry == NULL || entry->key == NULL) {
        break;
      }

      TypeCheckSymbol* symbol = entry->value->structure; 

      if (symbol->symbol_type == SYMBOL_FUNCTION) {
        fprintf(stderr, "ERROR - SA Type Check: Function name '%s' is being used as a variable\n", node->data.variable_expression.identifier);
        exit(1);
      }

      break;
    }
    case AST_BLOCK: {
      for (int i = 0; i < node->data.block.block_count; i++) {   
        AstNode *block_item_node = node->data.block.block_ptrs->node_pointers[i];
        sa_function_and_variable_type_check(block_item_node, symbols, function_name);
      }
      break;
    }
    case AST_STATEMENT_IF: {
      sa_function_and_variable_type_check(node->data.if_statement.condition_expression, symbols, function_name);
      sa_function_and_variable_type_check(node->data.if_statement.then_statement, symbols, function_name);

      if (node->data.if_statement.else_statement != NULL) {
        sa_function_and_variable_type_check(node->data.if_statement.else_statement, symbols, function_name);
      }
      break;
    }
    case AST_STATEMENT_RETURN: {
      sa_function_and_variable_type_check(node->data.return_statement.expression, symbols, function_name);
      break;
    }
    case AST_STATEMENT_EXPRESSION: {
      sa_function_and_variable_type_check(node->data.expression_statement.expression, symbols, function_name);
      break;
    }
    case AST_STATEMENT_FOR: {
      if (node->data.for_statement.for_loop_init != NULL) {
        sa_function_and_variable_type_check(node->data.for_statement.for_loop_init, symbols, function_name);
      }

      if (node->data.for_statement.condition_expression != NULL) {
        sa_function_and_variable_type_check(node->data.for_statement.condition_expression, symbols, function_name);
      }

      if (node->data.for_statement.post_expression != NULL) {
        sa_function_and_variable_type_check(node->data.for_statement.post_expression, symbols, function_name);
      }

      sa_function_and_variable_type_check(node->data.for_statement.statement_body, symbols, function_name);
      break;
    }
    case AST_STATEMENT_WHILE: {
      sa_function_and_variable_type_check(node->data.while_statement.condition, symbols, function_name);
      sa_function_and_variable_type_check(node->data.while_statement.statement_body, symbols, function_name);
      break;
    }
    case AST_STATEMENT_DO_WHILE: {
      sa_function_and_variable_type_check(node->data.do_while_statement.condition, symbols, function_name);
      sa_function_and_variable_type_check(node->data.do_while_statement.statement_body, symbols, function_name);
      break;
    }
    case AST_EXPRESSION_ASSIGNMENT: {
      sa_function_and_variable_type_check(node->data.assignement_expression.left_expression, symbols, function_name);
      sa_function_and_variable_type_check(node->data.assignement_expression.right_expression, symbols, function_name);
      break;
    }
    case AST_EXPRESSION_BINARY: {
      sa_function_and_variable_type_check(node->data.binary_expression.left_expression, symbols, function_name);
      sa_function_and_variable_type_check(node->data.binary_expression.right_expression, symbols, function_name);
      break;
    }
    case AST_EXPRESSION_POSTFIX_INCREMENT:
    case AST_EXPRESSION_POSTFIX_DECREMENT:
    case AST_EXPRESSION_PREFIX_INCREMENT:
    case AST_EXPRESSION_PREFIX_DECREMENT: 
      sa_function_and_variable_type_check(node->data.increment_decrement_expression.expression, symbols, function_name);
      break;
    case AST_EXPRESSION_UNARY:
      sa_function_and_variable_type_check(node->data.unary_expression.expression, symbols, function_name);
      break;
  }  
}
