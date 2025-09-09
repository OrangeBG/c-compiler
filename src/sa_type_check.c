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

static void     function_and_variable_type_check(AstNode *node, DeclarationSymbolTable *declaration_table, AstNode *function_declaration_node, Arena *ast_arena);
static void     type_check_file_scope_variable_declaration(AstNode *variable_declaration_node, DeclarationSymbolTable *declaration_table); 
static void     type_check_block_scope_variable_declaration(AstNode *variable_declaration_node, DeclarationSymbolTable *declaration_table, char *function_name); 
static void     add_function_parameter_to_symbol_table(AstNode *parameter_type, char *parameter_identifier, char *function_name, DeclarationSymbolTable *declaration_table); 
static DeclarationSymbolValueType convert_ast_declaration_type_to_symbol_type(AstNode *variable_declaration_node); 
static Types    expression_type_check(AstNode *node, DeclarationSymbolTable *declaration_table, AstNode *function_declaration_node, Arena *ast_arena); 
static Types    get_common_real_type(Types type_1, Types type_2);
static AstNode* implicit_expression_type_cast(AstNode *expression, Types expression_type, Types common_type, Arena *ast_arena); 
 
void sa_type_check(AstNode *ast_nodes, DeclarationSymbolTable *declaration_table, Arena *ast_arena) {
  for (int i = 0; i < ast_nodes->data.program.declaration_count; i++) {
    AstNode *node = ast_nodes->data.program.declaration_ptrs->node_pointers[i];

    if (node->type == AST_FUNCTION_DECLARATION) {
      function_and_variable_type_check(node, declaration_table, node, ast_arena);
      continue;
    } 

    if (node->type == AST_VARIABLE_DECLARATION) {
      function_and_variable_type_check(node, declaration_table, NULL, ast_arena);
      continue;
    }

    fprintf(stderr, "ERROR - SA Type Check: Unexpected declaration type\n");
    exit(1);
  } 
}

static void function_and_variable_type_check(AstNode *node, DeclarationSymbolTable *declaration_table, AstNode *function_declaration_node, Arena *ast_arena) {
  switch (node->type) {
    case AST_VARIABLE_DECLARATION: {
      if (function_declaration_node == NULL) {
        type_check_file_scope_variable_declaration(node, declaration_table);
      } else {
        type_check_block_scope_variable_declaration(node, declaration_table, function_declaration_node->data.function_declaration.name);
      }

      if (node->data.variable_declaration.has_expression) {
        function_and_variable_type_check(node->data.variable_declaration.init_expression, declaration_table, function_declaration_node, ast_arena);
      }
      break;
    }
    case AST_FUNCTION_DECLARATION: {
      HashTableEntry *entry = hash_table_get_entry(declaration_table->symbol_table, node->data.function_declaration.name);

      if (entry != NULL && entry->key != NULL) {
        DeclarationSymbol *existing_function_symbol = entry->value->structure;

        if (existing_function_symbol->data.function_symbol->value_type != DECLARATION_SYMBOL_TYPE_INT) {
          fprintf(stderr, "ERROR - SA Type Check: Incompatible function declarations for '%s\n'", entry->key);
          exit(1);
        }

        if (existing_function_symbol->data.function_symbol->is_defined && node->data.function_declaration.body_block != NULL) {
          fprintf(stderr, "ERROR - SA Type Check: Function defined more than once '%s'\n", entry->key);
          exit(1);
        }

        if (existing_function_symbol->data.function_symbol->is_global == node->data.function_declaration.storage_class_type == AST_STORAGE_CLASS_STATIC) {
          fprintf(stderr, "ERROR - SA Type Check: Static function '%s' declaration follows non-static\n", node->data.function_declaration.name);
          exit(1);
        }

        if (!existing_function_symbol->data.function_symbol->is_defined) {
          existing_function_symbol->data.function_symbol->is_defined = node->data.function_declaration.body_block != NULL;
        }

        break;
      }

      DeclarationSymbolValueType declaration_value_type;
      
      switch (node->data.function_declaration.function_type->data.type.function_return_type->data.type.type) {
        case AST_TYPE_INT:   declaration_value_type = DECLARATION_SYMBOL_TYPE_INT; break;
        case AST_TYPE_LONG:  declaration_value_type = DECLARATION_SYMBOL_TYPE_LONG; break;
        case AST_TYPE_VOID:  declaration_value_type = DECLARATION_SYMBOL_TYPE_VOID; break;
        default:
          fprintf(stderr, "ERROR - SA Type Check: Unsuported function declaration AST type '%d'\n", node->data.function_declaration.function_type->data.type.type);
          exit(1);          
      }

      int param_count = 0;
      bool is_defined = node->data.function_declaration.body_block != NULL;
      bool is_global = (node->data.function_declaration.storage_class_type != AST_STORAGE_CLASS_STATIC || strcmp(node->data.function_declaration.name, "main") == 0);

      DeclarationSymbol *function_declaration_symbol = add_function_declaration_symbol(declaration_table, node->data.function_declaration.name, declaration_value_type, param_count, is_global, is_defined);
        
      for (int i = 0; i < node->data.function_declaration.parameter_count; i++) {
        AstNode *parameter_type = &node->data.function_declaration.function_type->data.type.function_param_types[i];

        if (parameter_type->data.type.type == AST_TYPE_VOID) {
          continue;
        }

        function_declaration_symbol->data.function_symbol->param_count++;
        add_function_parameter_to_symbol_table(parameter_type, node->data.function_declaration.parameter_identifiers[i], node->data.function_declaration.name, declaration_table);
      }

      if (node->data.function_declaration.body_block != NULL) {
        function_and_variable_type_check(node->data.function_declaration.body_block, declaration_table, node, ast_arena);
      }
      break;
    }
    case AST_EXPRESSION_FUNCTION_CALL: {
      HashTableEntry *entry = hash_table_get_entry(declaration_table->symbol_table, node->data.function_call_expression.identfier);

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
        function_and_variable_type_check(argument_node, declaration_table, function_declaration_node, ast_arena);
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
      expression_type_check(node, declaration_table, function_declaration_node, ast_arena);
      break;
    case AST_BLOCK: {
      for (int i = 0; i < node->data.block.block_count; i++) {   
        AstNode *block_item_node = node->data.block.block_ptrs->node_pointers[i];
        function_and_variable_type_check(block_item_node, declaration_table, function_declaration_node, ast_arena);
      }
      break;
    }
    case AST_STATEMENT_IF: {
      function_and_variable_type_check(node->data.if_statement.condition_expression, declaration_table, function_declaration_node, ast_arena);
      function_and_variable_type_check(node->data.if_statement.then_statement, declaration_table, function_declaration_node, ast_arena);

      if (node->data.if_statement.else_statement != NULL) {
        function_and_variable_type_check(node->data.if_statement.else_statement, declaration_table, function_declaration_node, ast_arena);
      }
      break;
    }
    case AST_STATEMENT_RETURN: {
      Types return_expression_type = expression_type_check(node->data.return_statement.expression, declaration_table, function_declaration_node, ast_arena);
      Types function_return_type = function_declaration_node->data.function_declaration.function_type->data.type.function_return_type->data.type.type;

      if (function_return_type == return_expression_type) {
        break;
      }

      node->data.return_statement.expression = implicit_expression_type_cast(node->data.return_statement.expression, return_expression_type, function_return_type, ast_arena);
      break;
    }
    case AST_STATEMENT_FOR: {
      if (node->data.for_statement.for_loop_init != NULL) {        
        function_and_variable_type_check(node->data.for_statement.for_loop_init, declaration_table, function_declaration_node, ast_arena);
      }

      if (node->data.for_statement.condition_expression != NULL) {
        function_and_variable_type_check(node->data.for_statement.condition_expression, declaration_table, function_declaration_node, ast_arena);
      }

      if (node->data.for_statement.post_expression != NULL) {
        function_and_variable_type_check(node->data.for_statement.post_expression, declaration_table, function_declaration_node, ast_arena);
      }

      function_and_variable_type_check(node->data.for_statement.statement_body, declaration_table, function_declaration_node, ast_arena);
      break;
    }
    case AST_STATEMENT_WHILE: {
      function_and_variable_type_check(node->data.while_statement.condition, declaration_table, function_declaration_node, ast_arena);
      function_and_variable_type_check(node->data.while_statement.statement_body, declaration_table, function_declaration_node, ast_arena);
      break;
    }
    case AST_STATEMENT_DO_WHILE: {
      function_and_variable_type_check(node->data.do_while_statement.condition, declaration_table, function_declaration_node, ast_arena);
      function_and_variable_type_check(node->data.do_while_statement.statement_body, declaration_table, function_declaration_node, ast_arena);
      break;
    }
    case AST_STATEMENT_COMPOUND:      
      function_and_variable_type_check(node->data.compound_statement.block, declaration_table, function_declaration_node, ast_arena);
      break;
    case AST_STATEMENT_GOTO_LABEL:
    case AST_STATEMENT_GOTO:
    case AST_STATEMENT_BREAK:
      break;
    default:    
      fprintf(stderr, "ERROR - SA Type Check: Unsupported AST type '%d' found in function and variable type check\n", node->type);
      exit(1);
  }  
}

static void type_check_file_scope_variable_declaration(AstNode *variable_declaration_node, DeclarationSymbolTable *declaration_table) {
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

  HashTableEntry *entry = hash_table_get_entry(declaration_table->symbol_table, variable_declaration_node->data.variable_declaration.name);

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

  DeclarationSymbolValueType declaration_symbol_value_type = convert_ast_declaration_type_to_symbol_type(variable_declaration_node);
  add_static_variable_declaration_symbol(declaration_table, declaration_symbol_value_type, initial_value, variable_declaration_node->data.variable_declaration.name, is_global, initial_value_type);  
}

static void type_check_block_scope_variable_declaration(AstNode *variable_declaration_node, DeclarationSymbolTable *declaration_table, char *function_name) {
  if (variable_declaration_node->data.variable_declaration.storage_class_type == AST_STORAGE_CLASS_EXTERN) {
    if (variable_declaration_node->data.variable_declaration.has_expression) {
      fprintf(stderr, "ERROR - SA Type Check: Initializer on local extern variable declaration '%s'\n", variable_declaration_node->data.variable_declaration.name);
      exit(1);
    }
    
    HashTableEntry *entry = hash_table_get_entry(declaration_table->symbol_table, variable_declaration_node->data.variable_declaration.name);

    if (entry != NULL && entry->key != NULL) {
      DeclarationSymbol *existing_variable_symbol = entry->value->structure;

      if (existing_variable_symbol->symbol_type == DECLARATION_SYMBOL_FUNCTION) {        
        fprintf(stderr, "ERROR - SA Type Check: Function redeclared as variable\n");
        exit(1);
      }
    } else {
      DeclarationSymbolValueType declaration_symbol_value_type = convert_ast_declaration_type_to_symbol_type(variable_declaration_node);
      add_static_extern_variable_declaration_symbol(declaration_table, declaration_symbol_value_type, variable_declaration_node->data.variable_declaration.name); 
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
          fprintf(stderr, "ERROR - SA Type Check: Unsupported initial value AST Type '%d'\n", variable_declaration_node->data.variable_declaration.type->data.type.type);
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
          fprintf(stderr, "ERROR - SA Type Check: Unsupported initial value AST Type '%d'\n",variable_declaration_node->data.variable_declaration.init_expression->data.assignement_expression.right_expression->data.constant_expression.expression_type->data.type.type);
          exit(1);
      }
    } else {
      fprintf(stderr, "ERROR - SA Type Check: Non-constance initializer on local staic variable '%s'\n", variable_declaration_node->data.variable_declaration.name);
      exit(1);
    }

    DeclarationSymbolValueType value_type = convert_ast_declaration_type_to_symbol_type(variable_declaration_node);
    add_static_variable_declaration_symbol(declaration_table, value_type, initial_value, variable_declaration_node->data.variable_declaration.name, false, INITIAL_VALUE_INITIALIZED);
    
    return;
  }   

  DeclarationSymbolValueType value_type = convert_ast_declaration_type_to_symbol_type(variable_declaration_node);
  add_automatic_variable_declaration_symbol(declaration_table, value_type, variable_declaration_node->data.variable_declaration.name);

  return;
} 

static Types expression_type_check(AstNode *node, DeclarationSymbolTable *declaration_table, AstNode *function_declaration_node, Arena *ast_arena) {
  switch (node->type) {
    case AST_EXPRESSION_VARIABLE: {
      HashTableEntry *entry = hash_table_get_entry(declaration_table->symbol_table, node->data.variable_expression.identifier);

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
      //@Bug: I think this is not right. Use the following as an example: long gg = (long)5;. Expression type returned is int
      Types expression_type = expression_type_check(node->data.cast_expression.expression, declaration_table, function_declaration_node, ast_arena);

      AstNode *ast_expression_type_node = arena_alloc(ast_arena);
      ast_expression_type_node->type = AST_TYPE;
      ast_expression_type_node->data.type.type = expression_type;

      node->data.cast_expression.expression_type = ast_expression_type_node;

      return expression_type;
    }
    case AST_EXPRESSION_UNARY: {
      Types expression_type = expression_type_check(node->data.unary_expression.expression, declaration_table, function_declaration_node, ast_arena);

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
      Types left_expression_type = expression_type_check(node->data.binary_expression.left_expression, declaration_table, function_declaration_node, ast_arena);
      Types right_expression_type = expression_type_check(node->data.binary_expression.right_expression, declaration_table, function_declaration_node, ast_arena);

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
      
      AstNode *ast_expression_type_node = arena_alloc(ast_arena);
      ast_expression_type_node->type = AST_TYPE;
      ast_expression_type_node->data.type.type = common_real_type;

      node->data.binary_expression.expression_type = ast_expression_type_node;
      
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
      Types left_expression_type = expression_type_check(node->data.assignement_expression.left_expression, declaration_table, function_declaration_node, ast_arena);
      Types right_expression_type = expression_type_check(node->data.assignement_expression.right_expression, declaration_table, function_declaration_node, ast_arena);

      //TODO: Need to look into this. I don't think it's working correctly
      node->data.assignement_expression.right_expression = implicit_expression_type_cast(node->data.assignement_expression.right_expression, right_expression_type, left_expression_type, ast_arena);      

      return left_expression_type;
    }
    case AST_EXPRESSION_FUNCTION_CALL: {
      HashTableEntry *entry = hash_table_get_entry(declaration_table->symbol_table, node->data.function_call_expression.identfier);
      if (entry == NULL && entry->key == NULL) {
        fprintf(stderr, "ERROR - SA Type Check: Called function '%s' not found in symbol table\n", node->data.function_call_expression.identfier);
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
        function_and_variable_type_check(argument_node, declaration_table, function_declaration_node, ast_arena);
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
      expression_type_check(node->data.conditional_expression.condition, declaration_table, function_declaration_node, ast_arena);

      Types true_expression_type = expression_type_check(node->data.conditional_expression.true_expression, declaration_table, function_declaration_node, ast_arena);
      Types false_expression_type = expression_type_check(node->data.conditional_expression.false_expression, declaration_table, function_declaration_node, ast_arena);
      
      Types common_real_type = get_common_real_type(true_expression_type, false_expression_type);

      node->data.conditional_expression.true_expression = implicit_expression_type_cast(node->data.conditional_expression.true_expression, true_expression_type, common_real_type, ast_arena);
      node->data.conditional_expression.false_expression = implicit_expression_type_cast(node->data.conditional_expression.false_expression, false_expression_type, common_real_type, ast_arena);
      return common_real_type;
    }
    case AST_EXPRESSION_PREFIX_INCREMENT:
    case AST_EXPRESSION_POSTFIX_INCREMENT:
    case AST_EXPRESSION_PREFIX_DECREMENT:
    case AST_EXPRESSION_POSTFIX_DECREMENT: {
      expression_type_check(node->data.increment_decrement_expression.expression, declaration_table, function_declaration_node, ast_arena);
      break;
    }
    default:
      fprintf(stderr, "ERROR - SA Type Check: Invalid AST type '%d' found in expression type check\n", node->type);
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
      fprintf(stderr, "ERROR - Parser: Unsupported cast expression type '%d'\n", expression->type);
      exit(1);
  }

  casted_expression->data.cast_expression.expression_type = cast_expression_type;
  
  return casted_expression;
}

static void add_function_parameter_to_symbol_table(AstNode *parameter_type, char *parameter_identifier, char *function_name, DeclarationSymbolTable *declaration_table) {
  if (parameter_type->data.type.type == AST_TYPE_VOID) {
    return;
  }

  char *symbol_key = malloc(IDENTIFIER_BUFFER); 
  snprintf(symbol_key, IDENTIFIER_BUFFER, "%s", parameter_identifier);

  //TODO: Should include Long type
  add_automatic_variable_declaration_symbol(declaration_table, DECLARATION_SYMBOL_TYPE_INT, symbol_key);
}

static DeclarationSymbolValueType convert_ast_declaration_type_to_symbol_type(AstNode *variable_declaration_node) {
  switch (variable_declaration_node->data.variable_declaration.type->data.type.type) {
    case AST_TYPE_INT:    return DECLARATION_SYMBOL_TYPE_INT; break;
    case AST_TYPE_LONG:   return DECLARATION_SYMBOL_TYPE_LONG; break;
    case AST_TYPE_VOID:   return DECLARATION_SYMBOL_TYPE_VOID; break;
    default:
      fprintf(stderr, "ERROR - SA Type Check: Unsupported AST Declaration Type '%d' when attempting to convert to Declaration Symbol Value Type\n", variable_declaration_node->data.variable_declaration.type->data.type.type);
      exit(1);
  }
}
