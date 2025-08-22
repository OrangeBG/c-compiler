#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include "../include/sa_type_check.h"
#include "../include/arena.h"
#include "../include/parser.h"
#include "../include/declaration_symbol.h"

//TODO: Check to see how we can better optimize these types of buffers. Exact same use of this buffer is in sa_variable_resolution
#define IDENTIFIER_BUFFER 256

static void     function_and_variable_type_check(AstNode *node, HashTable *symbols, AstNode *function_declaration_node, Arena *ast_arena);
static void     type_check_file_scope_variable_declaration(AstNode *variable_declaration_node, HashTable *symbols); 
static void     type_check_block_scope_variable_declaration(AstNode *variable_declaration_node, HashTable *symbols, char *function_name); 
static void     add_function_parameter_to_symbol_table(AstNode *parameter_type, char *parameter_identifier, char *function_name, HashTable *symbols); 
static void     assign_variable_symbol_value_type(VariableSymbol *variable_symbol, AstNode *variable_declaration_node);
static Types    expression_type_check(AstNode *node, HashTable *symbols, AstNode *function_declaration_node, Arena *ast_arena); 
static Types    get_common_real_type(Types type_1, Types type_2);
static AstNode* implicit_expression_type_cast(AstNode *expression, Types expression_type, Types common_type, Arena *ast_arena); 
 
void sa_type_check(AstNode *ast_nodes, HashTable *declaration_symbols, Arena *ast_arena) {
  for (int i = 0; i < ast_nodes->data.program.declaration_count; i++) {
    AstNode *node = ast_nodes->data.program.declaration_ptrs->node_pointers[i];

    if (node->type == AST_FUNCTION_DECLARATION) {
      function_and_variable_type_check(node, declaration_symbols, node, ast_arena);
      continue;
    } 

    if (node->type == AST_VARIABLE_DECLARATION) {
      function_and_variable_type_check(node, declaration_symbols, NULL, ast_arena);
      continue;
    }

    fprintf(stderr, "ERROR - SA Type Check: Unexpected declaration type");
    exit(1);
  } 
}

static void function_and_variable_type_check(AstNode *node, HashTable *symbols, AstNode *function_declaration_node, Arena *ast_arena) {
  switch (node->type) {
    case AST_VARIABLE_DECLARATION: {
      if (function_declaration_node == NULL) {
        type_check_file_scope_variable_declaration(node, symbols);
      } else {
        type_check_block_scope_variable_declaration(node, symbols, function_declaration_node->data.function_declaration.name);
      }

      if (node->data.variable_declaration.has_expression) {
        function_and_variable_type_check(node->data.variable_declaration.init_expression, symbols, function_declaration_node, ast_arena);
      }
      break;
    }
    case AST_FUNCTION_DECLARATION: {
      HashTableEntry *entry = hash_table_get_entry(symbols, node->data.function_declaration.name);

      if (entry != NULL && entry->key != NULL) {
        DeclarationSymbol *existing_function_symbol = entry->value->structure;

        if (existing_function_symbol->data.function_symbol->value_type != DECLARATION_SYMBOL_TYPE_INT) {
          fprintf(stderr, "ERROR - SA Type Check: Incompatible function declarations for '%s\n'", entry->key);
          exit(1);
        }

        if (existing_function_symbol->data.function_symbol->defined && node->data.function_declaration.body_block != NULL) {
          fprintf(stderr, "ERROR - SA Type Check: Function defined more than once '%s'\n", entry->key);
          exit(1);
        }

        if (existing_function_symbol->data.function_symbol->global == node->data.function_declaration.storage_class_type == AST_STORAGE_CLASS_STATIC) {
          fprintf(stderr, "ERROR - SA Type Check: Static function '%s' declaration follows non-static\n", node->data.function_declaration.name);
          exit(1);
        }

        if (!existing_function_symbol->data.function_symbol->defined) {
          existing_function_symbol->data.function_symbol->defined = node->data.function_declaration.body_block != NULL;
        }

        break;
      }

      FunctionSymbol *function_symbol = malloc(sizeof(FunctionSymbol));

      switch (node->data.function_declaration.function_type->data.type.function_return_type->data.type.type) {
        case AST_TYPE_INT:   function_symbol->value_type = DECLARATION_SYMBOL_TYPE_INT; break;
        case AST_TYPE_LONG:  function_symbol->value_type = DECLARATION_SYMBOL_TYPE_LONG; break;
        case AST_TYPE_VOID:  break;
        default:
          fprintf(stderr, "ERROR - SA Type Check: Unsuported function declaration AST type '%d'\n", node->data.function_declaration.function_type->data.type.type);
          exit(1);          
      }
      
      function_symbol->param_count = 0;
      function_symbol->defined = node->data.function_declaration.body_block != NULL;
      function_symbol->global = (node->data.function_declaration.storage_class_type != AST_STORAGE_CLASS_STATIC || strcmp(node->data.function_declaration.name, "main") == 0);

      DeclarationSymbol *new_symbol = malloc(sizeof(DeclarationSymbol));
      new_symbol->symbol_type = DECLARATION_SYMBOL_FUNCTION;
      new_symbol->data.function_symbol = function_symbol;

      HashValue *new_value = malloc(sizeof(HashValue));
      new_value->type = HASH_STRUCT;
      new_value->structure = new_symbol;

      HashTableEntry *new_entry = malloc(sizeof(HashTableEntry));
      new_entry->key = node->data.function_declaration.name;
      new_entry->value = new_value;

      hash_table_add_entry(symbols, new_entry);
        
      for (int i = 0; i < node->data.function_declaration.parameter_count; i++) {
        AstNode *parameter_type = &node->data.function_declaration.function_type->data.type.function_param_types[i];

        if (parameter_type->data.type.type == AST_TYPE_VOID) {
          continue;
        }

        function_symbol->param_count++;
        add_function_parameter_to_symbol_table(parameter_type, &node->data.function_declaration.parameter_identifiers[i], node->data.function_declaration.name, symbols);
      }

      if (node->data.function_declaration.body_block != NULL) {
        function_and_variable_type_check(node->data.function_declaration.body_block, symbols, node, ast_arena);
      }
      break;
    }
    case AST_EXPRESSION_FUNCTION_CALL: {
      HashTableEntry *entry = hash_table_get_entry(symbols, node->data.function_call_expression.identfier);

      if (entry != NULL && entry->key != NULL) {
        DeclarationSymbol *existing_symbol = entry->value->structure;

        if (existing_symbol->symbol_type == DECLARATION_SYMBOL_VARIABLE) {
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
        function_and_variable_type_check(argument_node, symbols, function_declaration_node, ast_arena);
      }
      break;
    }
    case AST_EXPRESSION_VARIABLE:
    case AST_EXPRESSION_CONSTANT:
    case AST_EXPRESSION_CAST: 
    case AST_EXPRESSION_UNARY:
    case AST_EXPRESSION_BINARY:
    case AST_EXPRESSION_ASSIGNMENT:
    case AST_EXPRESSION_POSTFIX_INCREMENT:
    case AST_EXPRESSION_POSTFIX_DECREMENT:
    case AST_EXPRESSION_PREFIX_INCREMENT:
    case AST_EXPRESSION_PREFIX_DECREMENT: 
    case AST_EXPRESSION_CONDITIONAL:
      expression_type_check(node, symbols, function_declaration_node, ast_arena);
      break;
    case AST_BLOCK: {
      for (int i = 0; i < node->data.block.block_count; i++) {   
        AstNode *block_item_node = node->data.block.block_ptrs->node_pointers[i];
        function_and_variable_type_check(block_item_node, symbols, function_declaration_node, ast_arena);
      }
      break;
    }
    case AST_STATEMENT_IF: {
      function_and_variable_type_check(node->data.if_statement.condition_expression, symbols, function_declaration_node, ast_arena);
      function_and_variable_type_check(node->data.if_statement.then_statement, symbols, function_declaration_node, ast_arena);

      if (node->data.if_statement.else_statement != NULL) {
        function_and_variable_type_check(node->data.if_statement.else_statement, symbols, function_declaration_node, ast_arena);
      }
      break;
    }
    case AST_STATEMENT_RETURN: {
      Types return_expression_type = expression_type_check(node->data.return_statement.expression, symbols, function_declaration_node, ast_arena);
      Types function_return_type = function_declaration_node->data.function_declaration.function_type->data.type.function_return_type->data.type.type;

      if (function_return_type == return_expression_type) {
        break;
      }

      node->data.return_statement.expression = implicit_expression_type_cast(node->data.return_statement.expression, return_expression_type, function_return_type, ast_arena);
      break;
    }
    case AST_STATEMENT_FOR: {
      if (node->data.for_statement.for_loop_init != NULL) {        
        function_and_variable_type_check(node->data.for_statement.for_loop_init, symbols, function_declaration_node, ast_arena);
      }

      if (node->data.for_statement.condition_expression != NULL) {
        function_and_variable_type_check(node->data.for_statement.condition_expression, symbols, function_declaration_node, ast_arena);
      }

      if (node->data.for_statement.post_expression != NULL) {
        function_and_variable_type_check(node->data.for_statement.post_expression, symbols, function_declaration_node, ast_arena);
      }

      function_and_variable_type_check(node->data.for_statement.statement_body, symbols, function_declaration_node, ast_arena);
      break;
    }
    case AST_STATEMENT_WHILE: {
      function_and_variable_type_check(node->data.while_statement.condition, symbols, function_declaration_node, ast_arena);
      function_and_variable_type_check(node->data.while_statement.statement_body, symbols, function_declaration_node, ast_arena);
      break;
    }
    case AST_STATEMENT_DO_WHILE: {
      function_and_variable_type_check(node->data.do_while_statement.condition, symbols, function_declaration_node, ast_arena);
      function_and_variable_type_check(node->data.do_while_statement.statement_body, symbols, function_declaration_node, ast_arena);
      break;
    }
    default:    
      fprintf(stderr, "ERROR - SA Type Check: Unsupported AST type '%d' found in function and variable type check", node->type);
      exit(1);
  }  
}

static void type_check_file_scope_variable_declaration(AstNode *variable_declaration_node, HashTable *symbols) {
  InitialValueType initial_value_type; 
  InitialValue initial_value;
  DeclarationSymbolValueType value_type;

  if (variable_declaration_node->data.variable_declaration.has_expression && variable_declaration_node->data.variable_declaration.init_expression->data.assignement_expression.right_expression->type == AST_EXPRESSION_CONSTANT) {
    initial_value_type = INITIAL_VALUE_INITIALIZED;

    //TODO: Look to see if there is a better way to do this since it's going to grow past ints and longs 
    if (variable_declaration_node->data.variable_declaration.type->data.type.type == AST_TYPE_INT) {
      value_type = DECLARATION_SYMBOL_TYPE_INT;
      initial_value.int_value = variable_declaration_node->data.variable_declaration.init_expression->data.assignement_expression.right_expression->data.constant_expression.int_value;

    } else {
      value_type = DECLARATION_SYMBOL_TYPE_LONG;
      initial_value.long_value = variable_declaration_node->data.variable_declaration.init_expression->data.assignement_expression.right_expression->data.constant_expression.long_value;
    }    
  } else if (!variable_declaration_node->data.variable_declaration.has_expression) {
    if (variable_declaration_node->data.variable_declaration.storage_class_type == AST_STORAGE_CLASS_EXTERN) {
      initial_value_type = INITIAL_VALUE_NO_INITIALIZER;
    } else {
      initial_value_type = INITIAL_VALUE_TENTATIVE;
    }
  } else {
    fprintf(stderr, "ERROR: SA Type Check: Non-constant initializer\n");
    exit(1);
  }

  bool is_global = variable_declaration_node->data.variable_declaration.storage_class_type != AST_STORAGE_CLASS_STATIC;

  HashTableEntry *entry = hash_table_get_entry(symbols, variable_declaration_node->data.variable_declaration.name);

  if (entry != NULL && entry->key != NULL) {
    DeclarationSymbol *existing_variable_symbol = entry->value->structure;

    if (existing_variable_symbol->symbol_type == DECLARATION_SYMBOL_FUNCTION) {
      fprintf(stderr, "ERROR: SA Type Check: Function '%s' redeclared as variable\n", variable_declaration_node->data.variable_declaration.name);
      exit(1);
    }

    if (value_type != existing_variable_symbol->data.variable_symbol->value_type) {
      fprintf(stderr, "ERROR: SA Type Check: Previously declared '%s' variable has type of '%d'\n", variable_declaration_node->data.variable_declaration.name, existing_variable_symbol->data.variable_symbol->value_type);
      exit(1);
    }

    if (variable_declaration_node->data.variable_declaration.storage_class_type == AST_STORAGE_CLASS_EXTERN) {
      existing_variable_symbol->data.variable_symbol->static_is_global = true;
    }
    else if (existing_variable_symbol->data.variable_symbol->static_is_global != is_global) {
      fprintf(stderr, "ERROR: SA Type Check: Function '%s' conflicting variable linkage\n", variable_declaration_node->data.variable_declaration.name);
      exit(1);
    }

    if (existing_variable_symbol->data.variable_symbol->static_initial_type == INITIAL_VALUE_INITIALIZED) {
      if (initial_value_type == INITIAL_VALUE_INITIALIZED) {
        fprintf(stderr, "ERROR: SA Type Check: Function '%s' conflicting file scope variable definitions\n", variable_declaration_node->data.variable_declaration.name);
        exit(1);
      }
    } else {
      existing_variable_symbol->data.variable_symbol->static_initial_type = initial_value_type;
      existing_variable_symbol->data.variable_symbol->static_initial_value = initial_value;
    }

    return;
  }

  DeclarationSymbol *variable_symbol = malloc(sizeof(DeclarationSymbol));
  variable_symbol->symbol_type = DECLARATION_SYMBOL_VARIABLE;

  VariableSymbol *symbol = malloc(sizeof(VariableSymbol));
  symbol->is_automatic_storage_duration = false;

  assign_variable_symbol_value_type(symbol, variable_declaration_node);

  variable_symbol->data.variable_symbol = symbol;
  variable_symbol->data.variable_symbol->static_is_global = variable_declaration_node->data.variable_declaration.storage_class_type != AST_STORAGE_CLASS_STATIC;
  variable_symbol->data.variable_symbol->static_initial_type = initial_value_type;
  variable_symbol->data.variable_symbol->static_initial_value = initial_value;

  HashValue *new_value = malloc(sizeof(HashValue));
  new_value->type = HASH_STRUCT;
  new_value->structure = variable_symbol;

  HashTableEntry *new_entry = malloc(sizeof(HashTableEntry));
  new_entry->key = variable_declaration_node->data.variable_declaration.name;
  new_entry->value = new_value;

  hash_table_add_entry(symbols, new_entry);
}

static void type_check_block_scope_variable_declaration(AstNode *variable_declaration_node, HashTable *symbols, char *function_name) {
  if (variable_declaration_node->data.variable_declaration.storage_class_type == AST_STORAGE_CLASS_EXTERN) {
    if (variable_declaration_node->data.variable_declaration.has_expression) {
      fprintf(stderr, "ERROR - SA Type Check: Initializer on local extern variable declaration '%s'\n", variable_declaration_node->data.variable_declaration.name);
      exit(1);
    }
    
    HashTableEntry *entry = hash_table_get_entry(symbols, variable_declaration_node->data.variable_declaration.name);

    if (entry != NULL && entry->key != NULL) {
      DeclarationSymbol *existing_variable_symbol = entry->value->structure;

      if (existing_variable_symbol->symbol_type == DECLARATION_SYMBOL_FUNCTION) {        
        fprintf(stderr, "ERROR - SA Type Check: Function redeclared as variable");
        exit(1);
      }
    } else {
      DeclarationSymbol *variable_symbol = malloc(sizeof(DeclarationSymbol));
      variable_symbol->symbol_type = DECLARATION_SYMBOL_VARIABLE;

      VariableSymbol *symbol = malloc(sizeof(VariableSymbol));
      symbol->is_automatic_storage_duration = false;

      assign_variable_symbol_value_type(symbol, variable_declaration_node);

      variable_symbol->data.variable_symbol = symbol;

      symbol->static_is_global = true;
      symbol->static_initial_type = INITIAL_VALUE_NO_INITIALIZER;

      HashValue *new_value = malloc(sizeof(HashValue));
      new_value->type = HASH_STRUCT;
      new_value->structure = variable_symbol;

      HashTableEntry *new_entry = malloc(sizeof(HashTableEntry));
      new_entry->key = variable_declaration_node->data.variable_declaration.name;
      new_entry->value = new_value;

      hash_table_add_entry(symbols, new_entry);
    }
    
    return;
  }

  if (variable_declaration_node->data.variable_declaration.storage_class_type == AST_STORAGE_CLASS_STATIC) {
    InitialValue initial_value;
    
    if (!variable_declaration_node->data.variable_declaration.has_expression) {
      switch (variable_declaration_node->data.variable_declaration.type->data.type.type) {
        case AST_TYPE_INT:
          initial_value.int_value = 0; 
          break;
        case AST_TYPE_LONG:
          initial_value.long_value = 0; 
          break;
        default:
          fprintf(stderr, "ERROR - SA Type Check: Unsupported initial value AST Type '%d'", variable_declaration_node->data.variable_declaration.type->data.type.type);
          exit(1);
      }
    } else if (variable_declaration_node->data.variable_declaration.init_expression->data.assignement_expression.right_expression->type == AST_EXPRESSION_CONSTANT) {

      Types constant_expression_type = variable_declaration_node->data.variable_declaration.init_expression->data.assignement_expression.right_expression->data.constant_expression.expression_type->data.type.type;

      switch(variable_declaration_node->data.variable_declaration.type->data.type.type) {
        case AST_TYPE_INT:
          if (constant_expression_type == AST_TYPE_LONG) {
            initial_value.int_value = (int)variable_declaration_node->data.variable_declaration.init_expression->data.assignement_expression.right_expression->data.constant_expression.long_value;
          } else {
            initial_value.int_value = variable_declaration_node->data.variable_declaration.init_expression->data.assignement_expression.right_expression->data.constant_expression.int_value;
          }
          break;
        case AST_TYPE_LONG:
          if (constant_expression_type == AST_TYPE_LONG) {
            initial_value.long_value = (long)variable_declaration_node->data.variable_declaration.init_expression->data.assignement_expression.right_expression->data.constant_expression.int_value;
          } else {
            initial_value.long_value = variable_declaration_node->data.variable_declaration.init_expression->data.assignement_expression.right_expression->data.constant_expression.long_value;
          }
          break;
        default:
          fprintf(stderr, "ERROR - SA Type Check: Unsupported initial value AST Type '%d'",variable_declaration_node->data.variable_declaration.init_expression->data.assignement_expression.right_expression->data.constant_expression.expression_type->data.type.type);
          exit(1);
      }
    } else {
      fprintf(stderr, "ERROR - SA Type Check: Non-constance initializer on local staic variable '%s'\n", variable_declaration_node->data.variable_declaration.name);
      exit(1);
    }

    DeclarationSymbol *variable_symbol = malloc(sizeof(DeclarationSymbol));
    variable_symbol->symbol_type = DECLARATION_SYMBOL_VARIABLE;

    VariableSymbol *symbol = malloc(sizeof(VariableSymbol));
    symbol->is_automatic_storage_duration = false;

    assign_variable_symbol_value_type(symbol, variable_declaration_node);

    variable_symbol->data.variable_symbol = symbol;

    symbol->static_is_global = false;
    symbol->static_initial_type = INITIAL_VALUE_INITIALIZED;
    symbol->static_initial_value = initial_value;

    HashValue *new_value = malloc(sizeof(HashValue));
    new_value->type = HASH_STRUCT;
    new_value->structure = variable_symbol;

    HashTableEntry *new_entry = malloc(sizeof(HashTableEntry));
    new_entry->key = variable_declaration_node->data.variable_declaration.name;
    new_entry->value = new_value;

    hash_table_add_entry(symbols, new_entry);
    
    return;
  }   
  
  DeclarationSymbol *variable_symbol = malloc(sizeof(DeclarationSymbol));
  variable_symbol->symbol_type = DECLARATION_SYMBOL_VARIABLE;

  VariableSymbol *symbol = malloc(sizeof(VariableSymbol));
  symbol->is_automatic_storage_duration = true;

  assign_variable_symbol_value_type(symbol, variable_declaration_node);

  variable_symbol->data.variable_symbol = symbol;

  HashValue *new_value = malloc(sizeof(HashValue));
  new_value->type = HASH_STRUCT;
  new_value->structure = variable_symbol;

  HashTableEntry *new_entry = malloc(sizeof(HashTableEntry));
  new_entry->key = variable_declaration_node->data.variable_declaration.name;
  new_entry->value = new_value;

  hash_table_add_entry(symbols, new_entry);

  return;
} 

static Types expression_type_check(AstNode *node, HashTable *symbols, AstNode *function_declaration_node, Arena *ast_arena) {
  switch (node->type) {
    case AST_EXPRESSION_VARIABLE: {
      HashTableEntry *entry = hash_table_get_entry(symbols, node->data.variable_expression.identifier);

      if (entry == NULL || entry->key == NULL) {
        fprintf(stderr, "ERROR - SA Type Check: Expression variable '%s' not found in declaration symbol table\n", node->data.variable_expression.identifier);
        exit(1);
      }

      DeclarationSymbol* symbol = entry->value->structure; 

      if (symbol->symbol_type == DECLARATION_SYMBOL_FUNCTION) {
        fprintf(stderr, "ERROR - SA Type Check: Function name '%s' is being used as a variable\n", node->data.variable_expression.identifier);
        exit(1);
      }

      AstNode *ast_expression_type = arena_alloc(ast_arena);
      ast_expression_type->type = AST_TYPE;
      
      switch (symbol->data.variable_symbol->value_type) {
        case DECLARATION_SYMBOL_TYPE_INT:   ast_expression_type->data.type.type = AST_TYPE_INT; break;
        case DECLARATION_SYMBOL_TYPE_LONG:  ast_expression_type->data.type.type = AST_TYPE_LONG; break;
      }

      node->data.variable_expression.expression_type = ast_expression_type;

      return ast_expression_type->data.type.type;
    }
    case AST_EXPRESSION_CONSTANT: {
      AstNode *ast_expression_type = arena_alloc(ast_arena);
      ast_expression_type->type = AST_TYPE;
      
      switch (node->data.constant_expression.constant_type) {
        case AST_CONSTANT_TYPE_INT:   ast_expression_type->data.type.type = AST_TYPE_INT; break;
        case AST_CONSTANT_TYPE_LONG:  ast_expression_type->data.type.type = AST_TYPE_LONG; break;
      }

      node->data.constant_expression.expression_type = ast_expression_type;

      return ast_expression_type->data.type.type;
    }
    case AST_EXPRESSION_CAST: {
      Types expression_type = expression_type_check(node, symbols, function_declaration_node, ast_arena);

      AstNode *ast_expression_type_node = arena_alloc(ast_arena);
      ast_expression_type_node->type = AST_TYPE;
      ast_expression_type_node->data.type.type = expression_type;

      node->data.cast_expression.expression_type = ast_expression_type_node;

      return expression_type;
    }
    case AST_EXPRESSION_UNARY: {
      Types expression_type = expression_type_check(node->data.unary_expression.expression, symbols, function_declaration_node, ast_arena);

      AstNode *ast_expression_type_node = arena_alloc(ast_arena);
      ast_expression_type_node->type = AST_TYPE;
      ast_expression_type_node->data.type.type = expression_type;

      node->data.unary_expression.expression_type = ast_expression_type_node;

      if (node->data.unary_expression.op_type == AST_UNARY_NOT) {
        return AST_TYPE_INT;
      }

      return expression_type;
    }
    case AST_EXPRESSION_BINARY: {
      Types left_expression_type = expression_type_check(node->data.binary_expression.left_expression, symbols, function_declaration_node, ast_arena);
      Types right_expression_type = expression_type_check(node->data.binary_expression.right_expression, symbols, function_declaration_node, ast_arena);

      if (node->data.binary_expression.op_type == AST_BINARY_AND || node->data.binary_expression.op_type == AST_BINARY_OR) {
        AstNode *ast_expression_type_node = arena_alloc(ast_arena);
        ast_expression_type_node->type = AST_TYPE;
        ast_expression_type_node->data.type.type = AST_TYPE_INT;

        node->data.binary_expression.expression_type = ast_expression_type_node;
        return AST_TYPE_INT;
      }

      Types common_real_type = get_common_real_type(left_expression_type, right_expression_type);

      node->data.binary_expression.left_expression = implicit_expression_type_cast(node->data.binary_expression.left_expression, left_expression_type, common_real_type, ast_arena);
      node->data.binary_expression.right_expression = implicit_expression_type_cast(node->data.binary_expression.right_expression, right_expression_type, common_real_type, ast_arena);
      
      switch (node->data.binary_expression.op_type) {
        case AST_BINARY_ADD:
        case AST_BINARY_SUBTRACT:
        case AST_BINARY_MULTIPLY:
        case AST_BINARY_DIVIDE:
        case AST_BINARY_REMAINDER:
          return common_real_type;
        default:
          return AST_TYPE_INT;
      }
    }
    case AST_EXPRESSION_ASSIGNMENT: {
      Types left_expression_type = expression_type_check(node->data.assignement_expression.left_expression, symbols, function_declaration_node, ast_arena);
      Types right_expression_type = expression_type_check(node->data.assignement_expression.right_expression, symbols, function_declaration_node, ast_arena);

      //TODO: Need to look into this. I don't think it's working correctly
      node->data.assignement_expression.right_expression = implicit_expression_type_cast(node->data.assignement_expression.right_expression, right_expression_type, left_expression_type, ast_arena);      

      return left_expression_type;
    }
    case AST_EXPRESSION_FUNCTION_CALL: {
      HashTableEntry *entry = hash_table_get_entry(symbols, node->data.function_call_expression.identfier);
      if (entry == NULL && entry->key == NULL) {
        fprintf(stderr, "ERROR - SA Type Check: Called function '%s' not found in symbol table", node->data.function_call_expression.identfier);
        exit(1);
      }

      DeclarationSymbol *existing_symbol = entry->value->structure;

      if (existing_symbol->symbol_type == DECLARATION_SYMBOL_VARIABLE) {
        fprintf(stderr, "ERROR - SA Type Check: Variable '%s' is used as a function name\n", node->data.function_call_expression.identfier);
        exit(1);
      }               

      if (existing_symbol->data.function_symbol->param_count != node->data.function_call_expression.argument_count) {
        fprintf(stderr, "ERROR - SA Type Check: Function '%s' called with incorrect number of arguments\n", node->data.function_call_expression.identfier);
        exit(1);
      }
      
      for (int i = 0; i < node->data.function_call_expression.argument_count; i++) {
        AstNode *argument_node = node->data.function_call_expression.argument_ptrs->node_pointers[i];
        function_and_variable_type_check(argument_node, symbols, function_declaration_node, ast_arena);
      }
    
      AstNode *ast_expression_type_node = arena_alloc(ast_arena);
      ast_expression_type_node->type = AST_TYPE;

      switch (existing_symbol->data.function_symbol->value_type) {
        case DECLARATION_SYMBOL_TYPE_INT:   ast_expression_type_node->data.type.type = AST_TYPE_INT; break;
        case DECLARATION_SYMBOL_TYPE_LONG:  ast_expression_type_node->data.type.type = AST_TYPE_LONG; break;
      }

      node->data.function_call_expression.expression_type = ast_expression_type_node;

      return ast_expression_type_node->data.type.type;
    }
    case AST_EXPRESSION_CONDITIONAL: {
      expression_type_check(node->data.conditional_expression.condition, symbols, function_declaration_node, ast_arena);

      Types true_expression_type = expression_type_check(node->data.conditional_expression.true_expression, symbols, function_declaration_node, ast_arena);
      Types false_expression_type = expression_type_check(node->data.conditional_expression.false_expression, symbols, function_declaration_node, ast_arena);
      
      Types common_real_type = get_common_real_type(true_expression_type, false_expression_type);

      node->data.conditional_expression.true_expression = implicit_expression_type_cast(node->data.conditional_expression.true_expression, true_expression_type, common_real_type, ast_arena);
      node->data.conditional_expression.false_expression = implicit_expression_type_cast(node->data.conditional_expression.false_expression, false_expression_type, common_real_type, ast_arena);
      return common_real_type;
    }
    default:
      fprintf(stderr, "ERROR - SA Type Check: Invalid AST type '%d' found in expression type check", node->type);
      exit(1);
  }
}

static Types get_common_real_type(Types type_1, Types type_2) {
  if (type_1 == type_2) {
    return type_1;
  }

  return AST_TYPE_LONG;
}

static AstNode* implicit_expression_type_cast(AstNode *expression, Types expression_type, Types common_type, Arena *ast_arena) {
  if (expression_type == common_type) {
    return expression;
  }

  AstNode *type_node = arena_alloc(ast_arena);
  type_node->type = AST_TYPE;
  type_node->data.type.type = common_type;

  AstNode *casted_expression = arena_alloc(ast_arena);
  casted_expression->type = AST_EXPRESSION_CAST;
  casted_expression->data.cast_expression.target_type = type_node;
  casted_expression->data.cast_expression.expression = expression;

  
  AstNode *cast_expression_type = NULL;

  switch (expression->type) {
    case AST_EXPRESSION_CONSTANT:      cast_expression_type = expression->data.constant_expression.expression_type; break;
    case AST_EXPRESSION_VARIABLE:      cast_expression_type = expression->data.variable_expression.expression_type; break;
    case AST_EXPRESSION_CAST:          cast_expression_type = expression->data.cast_expression.expression_type; break;
    case AST_EXPRESSION_UNARY:         cast_expression_type = expression->data.unary_expression.expression_type; break;
    case AST_EXPRESSION_BINARY:        cast_expression_type = expression->data.binary_expression.expression_type; break;
    case AST_EXPRESSION_ASSIGNMENT:    cast_expression_type = expression->data.assignement_expression.expression_type; break;
    case AST_EXPRESSION_CONDITIONAL:   cast_expression_type = expression->data.conditional_expression.expression_type; break;
    case AST_EXPRESSION_FUNCTION_CALL: cast_expression_type = expression->data.variable_expression.expression_type; break;
    default:
      fprintf(stderr, "ERROR - Parser: Unsupported cast expression type '%d'", expression->type);
      exit(1);
  }

  casted_expression->data.cast_expression.expression_type = cast_expression_type;
  
  return casted_expression;
}

static void add_function_parameter_to_symbol_table(AstNode *parameter_type, char *parameter_identifier, char *function_name, HashTable *symbols) {
  if (parameter_type->data.type.type != AST_TYPE_FUNCTION) {
    return;
  }

  //This pass happens after variable resolution, so no need to check to see if the variable is duplicated in the hash table
  DeclarationSymbol *symbol = malloc(sizeof(DeclarationSymbol));
  symbol->symbol_type = DECLARATION_SYMBOL_VARIABLE;

  VariableSymbol *variable_symbol = malloc(sizeof(VariableSymbol));
  variable_symbol->value_type = DECLARATION_SYMBOL_TYPE_INT;
  variable_symbol->is_automatic_storage_duration = true;

  symbol->data.variable_symbol = variable_symbol;

  HashTableEntry *entry = malloc(IDENTIFIER_BUFFER);

  char *symbol_key = malloc(IDENTIFIER_BUFFER); 
  snprintf(symbol_key, IDENTIFIER_BUFFER, "%s.%s", function_name, parameter_identifier);
  entry->key = symbol_key;

  HashValue *value = malloc(sizeof(HashValue));
  value->type = HASH_STRUCT;
  value->structure = symbol;

  entry->value = value;

  hash_table_add_entry(symbols, entry); 
}

static void assign_variable_symbol_value_type(VariableSymbol *variable_symbol, AstNode *variable_declaration_node) {
  switch (variable_declaration_node->data.variable_declaration.type->data.type.type) {
    case AST_TYPE_INT:    variable_symbol->value_type = DECLARATION_SYMBOL_TYPE_INT; break;
    case AST_TYPE_LONG:   variable_symbol->value_type = DECLARATION_SYMBOL_TYPE_LONG; break;
    default:
      fprintf(stderr, "ERROR - SA Type Check: Unsupported initial value AST Type '%d'", variable_declaration_node->data.variable_declaration.type->data.type.type);
      exit(1);
  }

}
