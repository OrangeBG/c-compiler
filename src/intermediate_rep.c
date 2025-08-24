#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "../include/intermediate_rep.h"
#include "../include/arena.h"
#include "../include/declaration_symbol.h"

#define INSTRUCTION_CAPACITY 8
#define FUNCTION_CAPACITY 8
#define FUNCTION_CALL_CAPACITY 8
#define NODE_POINTER_CAPACITY 8
#define BREAK_LABEL "break"
#define CONTINUE_LABEL "continue"
#define START_LABEL "start"
#define END_LABEL "end"

typedef struct {
  Arena postfix_arena;
  int temp_register_id;
  int temp_label_id;
} IREmitStatus;

void    ir_add_postfix_operations(IRNode *ir_function, IREmitStatus *emit_status, Arena *node_arena, DeclarationSymbolTable *declaration_symbol_table);
IRNode* ir_function(AstNode *ast_function, IREmitStatus *emit_status, Arena *node_arena, DeclarationSymbolTable *declaration_symbol_table);
IRNode* ir_emit_ast_node(AstNode *node, IRNode *function, IREmitStatus *emit_status, Arena *node_arena, DeclarationSymbolTable *declaration_symbol_table); 
IRNode* ir_emit_return(AstNode *block_item, IRNode *function, IREmitStatus *emit_status, Arena *node_arena, DeclarationSymbolTable *declaration_symbol_table);
void    ir_emit_if(AstNode *block_item, IRNode *function, IREmitStatus *emit_status, Arena *node_arena, DeclarationSymbolTable *declaration_symbol_table); 
void    ir_emit_goto(AstNode *goto_node, IRNode *function, Arena *node_arena); 
void    ir_emit_goto_label(AstNode *goto_label_node, IRNode *function, Arena *node_arena); 
void    ir_emit_while(AstNode *while_node, IRNode *function, IREmitStatus *emit_status, Arena *node_arena, DeclarationSymbolTable *declaration_symbol_table); 
void    ir_emit_do_while(AstNode *do_node, IRNode *function, IREmitStatus *emit_status, Arena *node_arena, DeclarationSymbolTable *declaration_symbol_table); 
void    ir_emit_for(AstNode *for_node, IRNode *function, IREmitStatus *emit_status, Arena *node_arena, DeclarationSymbolTable *declaration_symbol_table); 
void    ir_emit_continue(int label_id, IRNode *function, Arena *node_arena);
void    ir_emit_break(int label_id, IRNode *function, Arena *node_arena);
void    ir_emit_block(AstNode *block_node, IRNode *function, IREmitStatus *emit_status, Arena *node_arena, DeclarationSymbolTable *declaration_symbol_table); 
IRNode* ir_emit_jump(char *label, IRNode *function, Arena *node_arena);
IRNode* ir_emit_jump_if_zero(char *label, IRNode *condition, IRNode *function, Arena *node_arena); 
IRNode* ir_emit_jump_if_not_zero(char *label, IRNode *condition, IRNode *function, Arena *node_arena); 
IRNode* ir_emit_label(char* label, IRNode *function, Arena *node_arena);
IRNode* ir_emit_copy(IRNode *source, IRNode *destination, IRNode *function, Arena *node_arena); 
IRNode* ir_emit_declaration(AstNode *declaration_node, IRNode *function, IREmitStatus *emit_status, Arena *node_arena, DeclarationSymbolTable *declaration_symbol_table); 
IRNode* ir_emit_conditional_expression(AstNode *condition_node, IRNode *function, IREmitStatus *emit_status, Arena *node_arena, DeclarationSymbolTable *declaration_symbol_table);
IRNode* ir_emit_postfix_expression(AstNode *postfix_node, IREmitStatus *emit_status, Arena *node_arena);
IRNode* ir_emit_unary_expression(AstNode *unary_node, IRNode *function, IREmitStatus *emit_status, Arena *node_arena, DeclarationSymbolTable *declaration_symbol_table);
IRNode* ir_emit_binary_expression(AstNode *binary_node, IRNode *function, IREmitStatus *emit_status, Arena *node_arena, DeclarationSymbolTable *declaration_symbol_table);
IRNode* ir_emit_assignment_expression(AstNode *assignment_node, IRNode *function, IREmitStatus *emit_status, Arena *node_arena, DeclarationSymbolTable *declaration_symbol_assignment);
IRNode* ir_emit_function_call_expression(AstNode *function_call_node, IRNode *function, IREmitStatus *emit_status, Arena *node_arena, DeclarationSymbolTable *declaration_symbol_table); 
IRNode* ir_emit_cast_expression(AstNode *cast_node, IRNode *function, IREmitStatus *emit_status, Arena *node_arena, DeclarationSymbolTable *declaration_symbol_table);
void    ir_emit_symbol_declarations(HashTable *declaration_symbols, IRNode *ir_program,  Arena *node_arena); 
void    ir_add_instruction_to_function(IRNode *ir_function, IRNode *ir_instruction); 
void    ir_add_top_level_declaration_to_program(IRNode *ir_program, IRNode *ir_function); 
void    ir_add_argument_to_function_call(IRNode *ir_function_call_node, IRNode *argument);
char*   ir_create_temp_label(IREmitStatus *emit_status); 
char*   ir_create_temp_register(IREmitStatus *emit_status); 
char*   ir_create_concat_identifier(char *string, int integer); 
IRNode* ir_create_ast_constant(AstNode *constant_node, Arena *node_arena);
IRNode* ir_create_int_constant(int value, Arena *node_arena);
IRNode* ir_create_variable(char *identifier, Arena *node_arena);
void    ir_add_to_node_pointer(IRNode *ir_node, IRNodePointer *ir_node_pointer); 
void    ir_init_node_pointer(IRNodePointer *ir_node_pointer); 
static IRType  get_node_type(IRNode *node); 

IRNode* generate_intermediate_rep(AstNode *ast_node, DeclarationSymbolTable *declaration_symbol_table) {
  Arena *node_arena = malloc(sizeof(Arena));

  //TODO: Hardcoded capacity
  arena_init(node_arena, sizeof(IRNode), sizeof(IRNode) * 1000, false);

  IRNodePointer *node_pointer = malloc(sizeof(IRNodePointer));
  ir_init_node_pointer(node_pointer);

  IRNode *program = arena_alloc(node_arena);

  program->type = IR_PROGRAM;
  program->data.program.top_level_count = 0;
  program->data.program.top_level_ptrs = node_pointer;

  IREmitStatus emit_status = {
    .temp_register_id = 0,
    .temp_label_id = 0
  };

  for (int i = 0; i < ast_node->data.program.declaration_count; i++) {
    AstNode *declaration_node = ast_node->data.program.declaration_ptrs->node_pointers[i];

    if (declaration_node->type == AST_VARIABLE_DECLARATION ||declaration_node->data.function_declaration.body_block == NULL) {
      continue;
    }

    IRNode *top_level_declaration = ir_function(declaration_node, &emit_status, node_arena, declaration_symbol_table);
    ir_add_top_level_declaration_to_program(program, top_level_declaration);        
  }

  ir_emit_symbol_declarations(declaration_symbol_table->symbol_table, program, node_arena);

  return program;
}

void print_intermediate_ret(IRNode *ir_node) {
  switch (ir_node->type) {
    case IR_PROGRAM:
      printf("Program \n");

      for (int i = 0; i < ir_node->data.program.top_level_count; i++) {
        print_intermediate_ret(ir_node->data.program.top_level_ptrs->node_pointers[i]);
      }

      printf("\n");
      break;
    case IR_FUNCTION: {
      struct IRFunction *function = &ir_node->data.function; 
      printf("Function(name = %s, is_global = %d)\n", function->identifier, function->is_global);

      for (int i = 0; i < function->instruction_count; i++) {
        if (function->instruction_ptrs->node_pointers[i]->type == IR_INSTRUCTION_RET) {
          printf("Return(");
          print_intermediate_ret(function->instruction_ptrs->node_pointers[i]->data.instruction_ret.value);
          printf(")\n");
        } else if (function->instruction_ptrs->node_pointers[i]->type == IR_INSTRUCTION_UNARY) {      
          struct IRInstructionUnary* unary = &function->instruction_ptrs->node_pointers[i]->data.unary;

          switch (unary->op_type) {
            case IR_UNARY_NEGATE:     printf("Negate, "); break;
            case IR_UNARY_COMPLEMENT: printf("Complement, "); break;
            case IR_UNARY_NOT:        printf("Not, "); break;
          }
          
          print_intermediate_ret(unary->source);            
          printf(",");
          print_intermediate_ret(unary->destination);
          printf(")");
          printf("\n");
        } else if (function->instruction_ptrs->node_pointers[i]->type == IR_INSTRUCTION_BINARY) {
          struct IRInstructionBinary* binary = &function->instruction_ptrs->node_pointers[i]->data.instruction_binary;

          printf("Binary(");
    
          switch (binary->op_type) {
            case IR_BINARY_ADD:                 printf("Add, "); break;
            case IR_BINARY_SUBTRACT:            printf("Subtract, "); break;
            case IR_BINARY_DIVIDE:              printf("Divide, "); break;
            case IR_BINARY_MULTIPLY:            printf("Multiply, "); break;
            case IR_BINARY_REMAINDER:           printf("Remainder, "); break;
            case IR_BINARY_BITWISE_AND:         printf("Bitwise AND, "); break;
            case IR_BINARY_BITWISE_OR:          printf("Bitwise OR, "); break;
            case IR_BINARY_BITWISE_XOR:         printf("Bitwise XOR, "); break;
            case IR_BINARY_BITWISE_LEFT_SHIFT:  printf("Bitwise Left S., "); break;
            case IR_BINARY_BITWISE_RIGHT_SHIFT: printf("Bitwise Right S., "); break;
            case IR_BINARY_EQUAL:               printf("Equal, "); break;
            case IR_BINARY_NOT_EQUAL:           printf("Not Equal, "); break;
            case IR_BINARY_LESS_THAN:           printf("Less Than, "); break;
            case IR_BINARY_LESS_OR_EQUAL:       printf("Less or Equal, "); break;
            case IR_BINARY_GREATER_THAN:        printf("Greater Than, "); break;
            case IR_BINARY_GREATER_OR_EQUAL:    printf("Greater or Equal, "); break;
          }
          
          print_intermediate_ret(binary->source_1);            
          printf(",");
          print_intermediate_ret(binary->source_2);            
          printf(",");
          print_intermediate_ret(binary->destination);
          printf(")");
          printf("\n");
        } else if (function->instruction_ptrs->node_pointers[i]->type == IR_INSTRUCTION_JUMP_IF_ZERO) {
          printf("Jump If Zero(");
          print_intermediate_ret(function->instruction_ptrs->node_pointers[i]->data.instruction_jump_if_zero.condition);
          printf(", %s)\n", function->instruction_ptrs->node_pointers[i]->data.instruction_jump_if_zero.target);
        } else if (function->instruction_ptrs->node_pointers[i]->type == IR_INSTRUCTION_JUMP_IF_NOT_ZERO) {
          printf("Jump If Not Zero(");
          print_intermediate_ret(function->instruction_ptrs->node_pointers[i]->data.instruction_jump_if_not_zero.condition);
          printf(" , %s)\n", function->instruction_ptrs->node_pointers[i]->data.instruction_jump_if_not_zero.target);
        } else if (function->instruction_ptrs->node_pointers[i]->type == IR_INSTRUCTION_JUMP) {
          printf("Jump(%s)\n", function->instruction_ptrs->node_pointers[i]->data.instruction_jump.target);
        } else if (function->instruction_ptrs->node_pointers[i]->type == IR_INSTRUCTION_COPY) {
          printf("Copy(Source(");
          print_intermediate_ret(function->instruction_ptrs->node_pointers[i]->data.instruction_copy.source);
          printf(") (Destination(");
          print_intermediate_ret(function->instruction_ptrs->node_pointers[i]->data.instruction_copy.destination);
          printf(")\n");
        } else if (function->instruction_ptrs->node_pointers[i]->type == IR_INSTRUCTION_LABEL) {
          printf("Label(%s)\n", function->instruction_ptrs->node_pointers[i]->data.instruction_label.identifier);
        } else if (function->instruction_ptrs->node_pointers[i]->type == IR_INSTRUCTION_TRUNCATE) {
          printf("Truncate(Source(");
          print_intermediate_ret(function->instruction_ptrs->node_pointers[i]->data.instruction_truncate.source);
          printf(") (Destination(");
          print_intermediate_ret(function->instruction_ptrs->node_pointers[i]->data.instruction_truncate.destination);
          printf(")\n");
        } else if (function->instruction_ptrs->node_pointers[i]->type == IR_INSTRUCTION_SIGN_EXTEND) {
          printf("Sign Extend(Source(");
          print_intermediate_ret(function->instruction_ptrs->node_pointers[i]->data.instruction_sign_extend.source);
          printf(") (Destination(");
          print_intermediate_ret(function->instruction_ptrs->node_pointers[i]->data.instruction_sign_extend.destination);
          printf(")\n");
        }
      }
    }
    break;
    case IR_VALUE_CONSTANT:
      switch (ir_node->data.value_constant.type) {
        case IR_TYPE_INT:    printf("Constant(type = int, value = %d)", ir_node->data.value_constant.value.int_value); break;
        case IR_TYPE_LONG:   printf("Constant(type = long, value = %ld)", ir_node->data.value_constant.value.long_value); break;          
      }
      break;
    case IR_VALUE_VAR:
      printf("Var(\"%s\")", ir_node->data.value_var.identifier);
      break;
    case IR_VALUE_STATIC_VAR:
      printf("Static Var(\"%s\" Initial Value: ", ir_node->data.static_variable.identifier);

      switch (ir_node->data.static_variable.type) {
        case IR_TYPE_INT:  printf("%d, type = int, ", ir_node->data.static_variable.initial_value.int_value); break;
        case IR_TYPE_LONG: printf("%ld, type = long, ", ir_node->data.static_variable.initial_value.long_value); break;
      }

      printf("Is Global = %d)\n", ir_node->data.static_variable.is_global);
      break;
    case IR_INSTRUCTION_FUNCTION_CALL:
      printf("Function Call(name=%s ", ir_node->data.instruction_function_call.identifier);

      for (int i = 0; i < ir_node->data.instruction_function_call.arg_count; i++) {
        printf("Argument(");
        print_intermediate_ret(&ir_node->data.instruction_function_call.args[i]);
        printf(")");
      }
      printf(")");
      break;
    default:
      fprintf(stderr, "ERROR - IR: No print for type %d\n", ir_node->type);
      exit(1);
  }
}

IRNode* ir_function(AstNode *ast_function, IREmitStatus *emit_status, Arena *node_arena, DeclarationSymbolTable *declaration_symbol_table) {
  IRNode *function = arena_alloc(node_arena);
  IRNodePointer *ir_node_pointer = malloc(sizeof(IRNodePointer));
  ir_init_node_pointer(ir_node_pointer);
  
  function->type = IR_FUNCTION;
  function->data.function.identifier = ast_function->data.function_declaration.name;
  function->data.function.instruction_count = 0;
  function->data.function.instruction_ptrs = ir_node_pointer;

  HashTableEntry *found_declaration_entry = hash_table_get_entry(declaration_symbol_table->symbol_table, ast_function->data.function_declaration.name);

  if (found_declaration_entry == NULL || found_declaration_entry->key == NULL) {
    fprintf(stderr, "ERROR - IR: Declaration Symbol expected for the following function: '%s'\n", ast_function->data.function_declaration.name);
    exit(1);
  }

  DeclarationSymbol *symbol = found_declaration_entry->value->structure;
  function->data.function.is_global = symbol->data.function_symbol->is_global;

  Arena postfix_arena;
  //@WARNING: Hardcoded postfix arena size
  //TODO: May be better to initialize outside of this function and instead reset the allocated arena
  arena_init(&postfix_arena, sizeof(AstNode), sizeof(AstNode) * 50, true);
  emit_status->postfix_arena = postfix_arena;

  ir_emit_ast_node(ast_function->data.function_declaration.body_block, function, emit_status, node_arena, declaration_symbol_table);

  //@Temporary: Add return statement to every function that returns 0. If there is a return statement already for the function, this won't run.
  IRNode *zero_value = ir_create_int_constant(0, node_arena);
  IRNode *return_instruction = arena_alloc(node_arena);
  return_instruction->type = IR_INSTRUCTION_RET;
  return_instruction->data.instruction_ret.value = zero_value;

  ir_add_instruction_to_function(function, return_instruction);

  return function;
}

IRNode* ir_emit_ast_node(AstNode *node, IRNode *function, IREmitStatus *emit_status, Arena *node_arena, DeclarationSymbolTable *declaration_symbol_table) {
  switch (node->type) {
      case AST_BLOCK:                        { ir_emit_block(node, function, emit_status, node_arena, declaration_symbol_table); break; }
      case AST_STATEMENT_IF:                 { ir_emit_if(node, function, emit_status, node_arena, declaration_symbol_table); break; }
      case AST_STATEMENT_GOTO:               { ir_emit_goto(node, function, node_arena); break; }
      case AST_STATEMENT_GOTO_LABEL:         { ir_emit_goto_label(node, function, node_arena); break; }
      case AST_STATEMENT_WHILE:              { ir_emit_while(node, function, emit_status, node_arena, declaration_symbol_table); break; }
      case AST_STATEMENT_DO_WHILE:           { ir_emit_do_while(node, function, emit_status, node_arena, declaration_symbol_table); break; }
      case AST_STATEMENT_FOR:                { ir_emit_for(node, function, emit_status, node_arena, declaration_symbol_table); break; }
      case AST_STATEMENT_CONTINUE:           { ir_emit_continue(node->data.continue_statement.label_id, function, node_arena); break; }
      case AST_STATEMENT_BREAK:              { ir_emit_break(node->data.break_statement.label_id, function, node_arena); break; }
      case AST_STATEMENT_COMPOUND:           { ir_emit_block(node->data.compound_statement.block, function, emit_status, node_arena, declaration_symbol_table); break; }
      case AST_STATEMENT_NULL:               { break; } 
      case AST_STATEMENT_RETURN:             { return ir_emit_return(node, function, emit_status, node_arena, declaration_symbol_table); }
      case AST_EXPRESSION_VARIABLE:          { return ir_create_variable(node->data.variable_expression.identifier, node_arena); }
      case AST_EXPRESSION_CONSTANT:          { return ir_create_ast_constant(node, node_arena); }
      case AST_EXPRESSION_CONDITIONAL:       { return ir_emit_conditional_expression(node, function, emit_status, node_arena, declaration_symbol_table); }
      case AST_EXPRESSION_POSTFIX_INCREMENT: { return ir_emit_postfix_expression(node, emit_status, node_arena); }
      case AST_EXPRESSION_POSTFIX_DECREMENT: { return ir_emit_postfix_expression(node, emit_status, node_arena); }
      case AST_EXPRESSION_PREFIX_INCREMENT:  { return ir_emit_ast_node(node, function, emit_status, node_arena, declaration_symbol_table); }
      case AST_EXPRESSION_PREFIX_DECREMENT:  { return ir_emit_ast_node(node, function, emit_status, node_arena, declaration_symbol_table); }
      case AST_EXPRESSION_UNARY:             { return ir_emit_unary_expression(node, function, emit_status, node_arena, declaration_symbol_table); }
      case AST_EXPRESSION_BINARY:            { return ir_emit_binary_expression(node, function, emit_status, node_arena, declaration_symbol_table); }
      case AST_EXPRESSION_ASSIGNMENT:        { return ir_emit_assignment_expression(node, function, emit_status, node_arena, declaration_symbol_table); }
      case AST_EXPRESSION_FUNCTION_CALL:     { return ir_emit_function_call_expression(node, function, emit_status, node_arena, declaration_symbol_table); }
      case AST_EXPRESSION_CAST:              { return ir_emit_cast_expression(node, function, emit_status, node_arena, declaration_symbol_table); } 
      case AST_VARIABLE_DECLARATION:         { return ir_emit_declaration(node, function, emit_status, node_arena, declaration_symbol_table); }
      case AST_FUNCTION_DECLARATION:         {
          if (node->data.function_declaration.body_block == NULL) break;
          return ir_function(node, emit_status, node_arena, declaration_symbol_table);
      }
      default:
        fprintf(stderr, "ERROR - IR: ASTNode type %d not found for node emit\n", node->type);
        exit(1);
  }

  return NULL;
}

void ir_emit_block(AstNode *block_node, IRNode *function, IREmitStatus *emit_status, Arena *node_arena, DeclarationSymbolTable *declaration_symbol_table) {
  for (int i = 0; i < block_node->data.block.block_count; i++) {
    arena_reset(&emit_status->postfix_arena);
    AstNode *block_item_node = block_node->data.block.block_ptrs->node_pointers[i];
    ir_emit_ast_node(block_item_node, function, emit_status, node_arena, declaration_symbol_table);
    ir_add_postfix_operations(function, emit_status, node_arena, declaration_symbol_table);
  }
}

IRNode* ir_emit_return(AstNode *block_item, IRNode *function, IREmitStatus *emit_status, Arena *node_arena, DeclarationSymbolTable *declaration_symbol_table) {
  IRNode *value = ir_emit_ast_node(block_item->data.return_statement.expression, function, emit_status, node_arena, declaration_symbol_table);
  IRNode *return_instruction = arena_alloc(node_arena);

  return_instruction->type = IR_INSTRUCTION_RET;
  return_instruction->data.instruction_ret.value = value;

  ir_add_instruction_to_function(function, return_instruction);
  ir_add_postfix_operations(function, emit_status, node_arena, declaration_symbol_table);

  return return_instruction;
}

void ir_emit_if(AstNode *if_node, IRNode *function, IREmitStatus *emit_status, Arena *node_arena, DeclarationSymbolTable *declaration_symbol_table) {
  IRNode *condition = ir_emit_ast_node(if_node->data.if_statement.condition_expression, function, emit_status, node_arena, declaration_symbol_table);
  char *label_name = ir_create_temp_label(emit_status);

  ir_emit_jump_if_zero(label_name, condition, function, node_arena);

  AstNode *then_statement = if_node->data.if_statement.then_statement;

  ir_emit_ast_node(then_statement, function, emit_status, node_arena, declaration_symbol_table);
  ir_emit_label(label_name, function, node_arena);
}

void ir_emit_goto(AstNode *goto_node, IRNode *function, Arena *node_arena) {
  ir_emit_jump(goto_node->data.goto_label_statement.label, function, node_arena);
}

void ir_emit_goto_label(AstNode *goto_label_node, IRNode *function, Arena *node_arena) {
  ir_emit_label(goto_label_node->data.goto_statement.label, function, node_arena);
}

void ir_emit_while(AstNode *while_node, IRNode *function, IREmitStatus *emit_status, Arena *node_arena, DeclarationSymbolTable *declaration_symbol_table) {
  char *continue_label_identifier = ir_create_concat_identifier(CONTINUE_LABEL, while_node->data.do_while_statement.label_id); 
  char *break_label_identifier = ir_create_concat_identifier(BREAK_LABEL, while_node->data.do_while_statement.label_id); 

  ir_emit_label(continue_label_identifier, function, node_arena);

  IRNode *condition = ir_emit_ast_node(while_node->data.while_statement.condition, function, emit_status, node_arena, declaration_symbol_table);

  ir_emit_jump_if_zero(break_label_identifier, condition, function, node_arena);
  ir_emit_ast_node(while_node->data.while_statement.statement_body, function, emit_status, node_arena, declaration_symbol_table);
  ir_emit_jump(continue_label_identifier, function, node_arena);
  ir_emit_label(break_label_identifier, function, node_arena);
}

void ir_emit_do_while(AstNode *do_node, IRNode *function, IREmitStatus *emit_status, Arena *node_arena, DeclarationSymbolTable *declaration_symbol_table) {
  char *start_label_identifier = ir_create_concat_identifier(START_LABEL, do_node->data.do_while_statement.label_id);
  ir_emit_label(start_label_identifier, function, node_arena);

  ir_emit_ast_node(do_node->data.do_while_statement.statement_body, function, emit_status, node_arena, declaration_symbol_table);

  char *continue_label_identifier = ir_create_concat_identifier(CONTINUE_LABEL, do_node->data.do_while_statement.label_id); 
  ir_emit_label(continue_label_identifier, function, node_arena);

  IRNode *condition = ir_emit_ast_node(do_node->data.do_while_statement.condition, function, emit_status, node_arena, declaration_symbol_table);
  ir_emit_jump_if_not_zero(start_label_identifier, condition, function, node_arena);

  char *break_label_identifier = ir_create_concat_identifier(BREAK_LABEL, do_node->data.do_while_statement.label_id);
  ir_emit_label(break_label_identifier, function, node_arena);
}

void ir_emit_for(AstNode *for_node, IRNode *function, IREmitStatus *emit_status, Arena *node_arena, DeclarationSymbolTable *declaration_symbol_table) {
  if (for_node->data.for_statement.for_loop_init != NULL) {
    ir_emit_ast_node(for_node->data.for_statement.for_loop_init, function, emit_status, node_arena, declaration_symbol_table);
  }  

  char *start_label_identifier = ir_create_concat_identifier(START_LABEL, for_node->data.for_statement.label_id);
  ir_emit_label(start_label_identifier, function, node_arena);

  char *break_label_identifier = ir_create_concat_identifier(BREAK_LABEL, for_node->data.for_statement.label_id);

  if (for_node->data.for_statement.condition_expression != NULL) {
    IRNode *condition = ir_emit_ast_node(for_node->data.for_statement.condition_expression, function, emit_status, node_arena, declaration_symbol_table);
    ir_emit_jump_if_zero(break_label_identifier, condition, function, node_arena);
  }

  ir_emit_ast_node(for_node->data.for_statement.statement_body, function, emit_status, node_arena, declaration_symbol_table);

  char *continue_label_identifier = ir_create_concat_identifier(CONTINUE_LABEL, for_node->data.for_statement.label_id);
  ir_emit_label(continue_label_identifier, function, node_arena);

  if (for_node->data.for_statement.post_expression != NULL) {
    ir_emit_ast_node(for_node->data.for_statement.post_expression, function, emit_status, node_arena, declaration_symbol_table);
  }

  ir_emit_jump(start_label_identifier, function, node_arena);
  ir_emit_label(break_label_identifier, function, node_arena);
}

void ir_emit_continue(int label_id, IRNode *function, Arena *node_arena) {
  char *continue_label_identifier = ir_create_concat_identifier(CONTINUE_LABEL, label_id); 
  ir_emit_jump(continue_label_identifier, function, node_arena);
}

void ir_emit_break(int label_id, IRNode *function, Arena *node_arena) {
  char *break_label_identifier = ir_create_concat_identifier(BREAK_LABEL, label_id); 
  ir_emit_jump(break_label_identifier, function, node_arena);
}

IRNode* ir_emit_declaration(AstNode *declaration_node, IRNode *function, IREmitStatus *emit_status, Arena *node_arena, DeclarationSymbolTable *declaration_symbol_table) {
  if (!declaration_node->data.variable_declaration.has_expression) {
    return NULL;
  }

  IRNode *node = ir_emit_ast_node(declaration_node->data.variable_declaration.init_expression, function, emit_status, node_arena, declaration_symbol_table);    
  ir_add_postfix_operations(function, emit_status, node_arena, declaration_symbol_table);

  return node;
}

IRNode* ir_emit_conditional_expression(AstNode *conditional_node, IRNode *function, IREmitStatus *emit_status, Arena *node_arena, DeclarationSymbolTable *declaration_symbol_table) {
  IRNode *condition = ir_emit_ast_node(conditional_node->data.conditional_expression.condition, function, emit_status, node_arena, declaration_symbol_table);

  char *end_label_name = ir_create_temp_label(emit_status);
  char *false_label_name = ir_create_temp_label(emit_status);

  ir_emit_jump_if_zero(false_label_name, condition, function, node_arena);

  IRNode *true_value = ir_emit_ast_node(conditional_node->data.conditional_expression.true_expression, function, emit_status, node_arena, declaration_symbol_table);

  ir_emit_jump(end_label_name, function, node_arena);
  ir_emit_label(false_label_name, function, node_arena);

  IRNode *false_value = ir_emit_ast_node(conditional_node->data.conditional_expression.false_expression, function, emit_status, node_arena, declaration_symbol_table);      

  ir_emit_label(end_label_name, function, node_arena);

  return condition;
}

IRNode* ir_emit_postfix_expression(AstNode *postfix_node, IREmitStatus *emit_status, Arena *node_arena) {
  AstNode *postfix_arena_node = arena_alloc(&emit_status->postfix_arena);
  *postfix_arena_node = *postfix_node->data.increment_decrement_expression.expression;

  IRNode *variable = arena_alloc(node_arena);
  variable->type = IR_VALUE_VAR;

  AstNode *postfix_expression = postfix_node->data.increment_decrement_expression.expression->data.assignement_expression.left_expression;

  if (postfix_expression->type == AST_EXPRESSION_VARIABLE) {
    variable->data.value_var.identifier = postfix_expression->data.variable_expression.identifier;
  } else if (postfix_expression->type == AST_EXPRESSION_UNARY) {
    variable->data.value_var.identifier = postfix_expression->data.unary_expression.expression->data.variable_expression.identifier;
  } else {
    fprintf(stderr, "ERROR - Intermediate Rep: Could not resolve variable identifier for Postfix expression\n");
    exit(1);
  }

  return variable;
}

IRNode* ir_emit_unary_expression(AstNode *unary_node, IRNode *function, IREmitStatus *emit_status, Arena *node_arena, DeclarationSymbolTable *declaration_symbol_table) {
  IRNode *source = ir_emit_ast_node(unary_node->data.unary_expression.expression, function, emit_status, node_arena, declaration_symbol_table);

  //TODO: Warning, setting hard buffer limit
  char *destination_name = ir_create_temp_register(emit_status);

  IRNode *destination = arena_alloc(node_arena);
  destination->type = IR_VALUE_VAR;
  destination->data.value_var.identifier = destination_name;

  IRUnaryOpType unary_op_type;

  switch (unary_node->data.unary_expression.op_type) {
    case AST_UNARY_COMPLEMENT: unary_op_type = IR_UNARY_COMPLEMENT; break;
    case AST_UNARY_NEGATE:     unary_op_type = IR_UNARY_NEGATE; break;
    case AST_UNARY_NOT:        unary_op_type = IR_UNARY_NOT; break;
  }

  IRNode *unary_instruction = arena_alloc(node_arena);         
  unary_instruction->type = IR_INSTRUCTION_UNARY;
  unary_instruction->data.unary.op_type = unary_op_type;
  unary_instruction->data.unary.source = source;
  unary_instruction->data.unary.destination = destination;

  ir_add_instruction_to_function(function, unary_instruction);

  return destination;
}

IRNode* ir_emit_binary_expression(AstNode *binary_node, IRNode *function, IREmitStatus *emit_status, Arena *node_arena, DeclarationSymbolTable *declaration_symbol_table) {
  IRNode *source_1 = ir_emit_ast_node(binary_node->data.binary_expression.left_expression, function, emit_status, node_arena, declaration_symbol_table);
  IRNode *source_2 = ir_emit_ast_node(binary_node->data.binary_expression.right_expression, function, emit_status, node_arena, declaration_symbol_table);

  //TODO: Warning, setting hard buffer limit
  char *destination_name = ir_create_temp_register(emit_status);

  IRNode *destination = arena_alloc(node_arena);
  destination->type = IR_VALUE_VAR;
  destination->data.value_var.identifier = destination_name;

  if (binary_node->data.binary_expression.op_type == AST_BINARY_AND || binary_node->data.binary_expression.op_type == AST_BINARY_OR) {
    char *label_name = ir_create_temp_label(emit_status);

    IRNode *jmp_instruction_v1 = arena_alloc(node_arena);

    if (binary_node->data.binary_expression.op_type == AST_BINARY_AND) { 
      jmp_instruction_v1->type = IR_INSTRUCTION_JUMP_IF_ZERO;  
      jmp_instruction_v1->data.instruction_jump_if_zero.condition = source_1;
      jmp_instruction_v1->data.instruction_jump_if_zero.target = label_name;
    } else {
      jmp_instruction_v1->type = IR_INSTRUCTION_JUMP_IF_NOT_ZERO;  
      jmp_instruction_v1->data.instruction_jump_if_not_zero.condition = source_1;
      jmp_instruction_v1->data.instruction_jump_if_not_zero.target = label_name;
    }

    ir_add_instruction_to_function(function, jmp_instruction_v1);

    IRNode *jmp_instruction_v2 = arena_alloc(node_arena);;

    if (binary_node->data.binary_expression.op_type == AST_BINARY_AND) { 
      jmp_instruction_v2->type = IR_INSTRUCTION_JUMP_IF_ZERO;  
      jmp_instruction_v2->data.instruction_jump_if_zero.condition = source_2;
      jmp_instruction_v2->data.instruction_jump_if_zero.target = label_name;
    } else {
      jmp_instruction_v2->type = IR_INSTRUCTION_JUMP_IF_NOT_ZERO;  
      jmp_instruction_v2->data.instruction_jump_if_not_zero.condition = source_2;
      jmp_instruction_v2->data.instruction_jump_if_not_zero.target = label_name;
    }

    ir_add_instruction_to_function(function, jmp_instruction_v2);

    IRNode *result_1 = ir_create_int_constant(1, node_arena);

    ir_emit_copy(result_1, destination, function, node_arena);
    ir_emit_jump(END_LABEL, function, node_arena);
    ir_emit_label(label_name, function, node_arena);

    IRNode *result_0 = ir_create_int_constant(0, node_arena);

    ir_emit_copy(result_0, destination, function, node_arena);
    ir_emit_label(END_LABEL, function, node_arena);

    return destination;
  }

  IRBinaryOpType binary_op_type;

  switch (binary_node->data.binary_expression.op_type) {
    case AST_BINARY_ADD:                  binary_op_type = IR_BINARY_ADD; break;
    case AST_BINARY_SUBTRACT:             binary_op_type = IR_BINARY_SUBTRACT; break;
    case AST_BINARY_DIVIDE:               binary_op_type = IR_BINARY_DIVIDE; break;
    case AST_BINARY_MULTIPLY:             binary_op_type = IR_BINARY_MULTIPLY; break;
    case AST_BINARY_REMAINDER:            binary_op_type = IR_BINARY_REMAINDER; break;
    case AST_BINARY_BITWISE_AND:          binary_op_type = IR_BINARY_BITWISE_AND; break;
    case AST_BINARY_BITWISE_OR:           binary_op_type = IR_BINARY_BITWISE_OR; break;
    case AST_BINARY_BITWISE_XOR:          binary_op_type = IR_BINARY_BITWISE_XOR; break;            
    case AST_BINARY_BITWISE_LEFT_SHIFT:   binary_op_type = IR_BINARY_BITWISE_LEFT_SHIFT; break;
    case AST_BINARY_BITWISE_RIGHT_SHIFT:  binary_op_type = IR_BINARY_BITWISE_RIGHT_SHIFT; break;
    case AST_BINARY_EQUAL:                binary_op_type = IR_BINARY_EQUAL; break;
    case AST_BINARY_NOT_EQUAL:            binary_op_type = IR_BINARY_NOT_EQUAL; break;
    case AST_BINARY_LESS_THAN:            binary_op_type = IR_BINARY_LESS_THAN; break;
    case AST_BINARY_LESS_OR_EQUAL:        binary_op_type = IR_BINARY_LESS_OR_EQUAL; break;
    case AST_BINARY_GREATER_THAN:         binary_op_type = IR_BINARY_GREATER_THAN; break;
    case AST_BINARY_GREATER_OR_EQUAL:     binary_op_type = IR_BINARY_GREATER_OR_EQUAL; break;
    default: break;
  }      

  IRNode *binary_instruction = arena_alloc(node_arena);         
  binary_instruction->type = IR_INSTRUCTION_BINARY;
  binary_instruction->data.instruction_binary.op_type = binary_op_type;
  binary_instruction->data.instruction_binary.source_1 = source_1;
  binary_instruction->data.instruction_binary.source_2 = source_2;
  binary_instruction->data.instruction_binary.destination = destination;

  ir_add_instruction_to_function(function, binary_instruction);

  return destination;
}

IRNode* ir_emit_assignment_expression(AstNode *assignment_node, IRNode *function, IREmitStatus *emit_status, Arena *node_arena, DeclarationSymbolTable *declaration_symbol_table) {
  //TODO: Keep this for now. Need to assess why conditional expressions are handled differently when the source node is 'ast_expression_assignment'. There's already an emit_conditional(). 
  if (assignment_node->data.assignement_expression.right_expression->type == AST_EXPRESSION_CONDITIONAL) {
    IRNode *condition = ir_emit_ast_node(assignment_node->data.assignement_expression.right_expression->data.conditional_expression.condition, function, emit_status, node_arena, declaration_symbol_table);

    char *end_label_name = ir_create_temp_label(emit_status);
    char *false_label_name = ir_create_temp_label(emit_status);

    ir_emit_jump_if_zero(false_label_name, condition, function, node_arena);
    
    IRNode *true_value = ir_emit_ast_node(assignment_node->data.assignement_expression.right_expression->data.conditional_expression.true_expression, function, emit_status, node_arena, declaration_symbol_table);

    IRNode *variable = arena_alloc(node_arena);
    variable->type = IR_VALUE_VAR;
    variable->data.value_var.identifier = assignment_node->data.assignement_expression.left_expression->data.variable_expression.identifier;

    ir_emit_copy(true_value, variable, function, node_arena);
    ir_emit_jump(end_label_name, function, node_arena);
    ir_emit_label(false_label_name, function, node_arena);
  
    IRNode *false_value = ir_emit_ast_node(assignment_node->data.assignement_expression.right_expression->data.conditional_expression.false_expression, function, emit_status, node_arena, declaration_symbol_table);

    ir_emit_copy(false_value, variable, function, node_arena);
    ir_emit_label(end_label_name, function, node_arena);

    return NULL;
  }
    
  IRNode *result = ir_emit_ast_node(assignment_node->data.assignement_expression.right_expression, function, emit_status, node_arena, declaration_symbol_table);

  IRNode *variable = arena_alloc(node_arena);
  variable->type = IR_VALUE_VAR;

  if (assignment_node->data.assignement_expression.left_expression->type == AST_EXPRESSION_VARIABLE) {
    variable->data.value_var.identifier = assignment_node->data.assignement_expression.left_expression->data.variable_expression.identifier;
  } else if (assignment_node->data.assignement_expression.left_expression->type == AST_EXPRESSION_UNARY) {
    variable->data.value_var.identifier = assignment_node->data.assignement_expression.left_expression->data.unary_expression.expression->data.variable_expression.identifier;
  } else {
    fprintf(stderr, "ERROR - Intermediate Rep: Could not resolve variable identifier for Expression Assignment\n");
    exit(1);
  }

  ir_emit_copy(result, variable, function, node_arena);
  
  return result;
}

IRNode* ir_emit_function_call_expression(AstNode *function_call_node, IRNode *function, IREmitStatus *emit_status, Arena *node_arena, DeclarationSymbolTable *declaration_symbol_table) {
  IRNode *ir_function_call = arena_alloc(node_arena);
  ir_function_call->type = IR_INSTRUCTION_FUNCTION_CALL;
  ir_function_call->data.instruction_function_call.identifier = function_call_node->data.function_call_expression.identfier;
  ir_function_call->data.instruction_function_call.arg_capacity = 0;
  ir_function_call->data.instruction_function_call.arg_count = 0;
  ir_function_call->data.instruction_function_call.args = NULL;

  char *destination_name = ir_create_temp_register(emit_status);

  IRNode *destination = arena_alloc(node_arena);
  destination->type = IR_VALUE_VAR;
  destination->data.value_var.identifier = destination_name;

  ir_function_call->data.instruction_function_call.destination = destination;

  for (int i = 0; i < function_call_node->data.function_call_expression.argument_count; i++) {
    AstNode *argument_node = function_call_node->data.function_call_expression.argument_ptrs->node_pointers[i];

    IRNode *argument = ir_emit_ast_node(argument_node, function, emit_status, node_arena, declaration_symbol_table);

    ir_add_argument_to_function_call(ir_function_call, argument);    
  }

  ir_add_instruction_to_function(function, ir_function_call);

  return destination;
} 

IRNode* ir_emit_cast_expression(AstNode *cast_node, IRNode *function, IREmitStatus *emit_status, Arena *node_arena, DeclarationSymbolTable *declaration_symbol_table) {
  IRNode *cast_expression = ir_emit_ast_node(cast_node->data.cast_expression.expression, function, emit_status, node_arena, declaration_symbol_table);
  IRType expression_type = get_node_type(cast_expression); 

  if (cast_node->data.cast_expression.target_type->data.type.type == cast_node->data.cast_expression.expression_type->data.type.type) {
    return cast_expression;
  }

  char *temp_destination = ir_create_temp_register(emit_status);

  IRNode *var_destination_node = arena_alloc(node_arena);
  var_destination_node->type = IR_VALUE_VAR;
  var_destination_node->data.value_var.identifier = temp_destination; 
  
  switch (expression_type) {
    case IR_TYPE_INT: {
      IRNode *truncate_instruction = arena_alloc(node_arena);
      truncate_instruction->type = IR_INSTRUCTION_TRUNCATE;
      truncate_instruction->data.instruction_truncate.destination = var_destination_node;
      truncate_instruction->data.instruction_truncate.source = cast_expression;

      ir_add_instruction_to_function(function, truncate_instruction);
      break;
    }
    case IR_TYPE_LONG: {
      IRNode *sign_extend_instruction = arena_alloc(node_arena);
      sign_extend_instruction->type = IR_INSTRUCTION_SIGN_EXTEND;
      sign_extend_instruction->data.instruction_sign_extend.destination = var_destination_node;
      sign_extend_instruction->data.instruction_sign_extend.source = cast_expression;

      ir_add_instruction_to_function(function, sign_extend_instruction);
      break;
    }
    default:
      fprintf(stderr, "ERROR - IR: Unsupported cast expression type '%d'", expression_type);
      exit(1);
  }
  
  return var_destination_node;
}

IRNode* ir_emit_jump(char *label, IRNode *function, Arena *node_arena) {
  IRNode *jmp_instruction = arena_alloc(node_arena);
  jmp_instruction->type = IR_INSTRUCTION_JUMP;
  jmp_instruction->data.instruction_jump.target = label;

  ir_add_instruction_to_function(function, jmp_instruction);

  return jmp_instruction;
}

IRNode* ir_emit_jump_if_zero(char *label, IRNode *condition, IRNode *function, Arena *node_arena) {
  IRNode *jump_if_zero = arena_alloc(node_arena);
  jump_if_zero->type = IR_INSTRUCTION_JUMP_IF_ZERO;
  jump_if_zero->data.instruction_jump_if_zero.condition = condition;
  jump_if_zero->data.instruction_jump_if_zero.target = label;

  ir_add_instruction_to_function(function, jump_if_zero);

  return jump_if_zero;
}

IRNode* ir_emit_jump_if_not_zero(char *label, IRNode *condition, IRNode *function, Arena *node_arena) {
  IRNode *jmp_if_not_zero = arena_alloc(node_arena);
  jmp_if_not_zero->type = IR_INSTRUCTION_JUMP_IF_NOT_ZERO;
  jmp_if_not_zero->data.instruction_jump_if_not_zero.condition = condition;
  jmp_if_not_zero->data.instruction_jump_if_not_zero.target = label;

  ir_add_instruction_to_function(function, jmp_if_not_zero);
  return jmp_if_not_zero;
}

IRNode* ir_emit_label(char *label, IRNode *function, Arena *node_arena) {
  IRNode *label_instruction = arena_alloc(node_arena);
  label_instruction->type = IR_INSTRUCTION_LABEL;
  label_instruction->data.instruction_label.identifier = label;

  ir_add_instruction_to_function(function, label_instruction);

  return label_instruction;
}

IRNode* ir_emit_copy(IRNode *source, IRNode *destination, IRNode *function, Arena *node_arena) {
  IRNode *copy_instruction = arena_alloc(node_arena);
  copy_instruction->type = IR_INSTRUCTION_COPY;
  copy_instruction->data.instruction_copy.source = source;
  copy_instruction->data.instruction_copy.destination = destination;      

  ir_add_instruction_to_function(function, copy_instruction);

  return copy_instruction;
}

void ir_emit_symbol_declarations(HashTable *declaration_symbols, IRNode *ir_program,  Arena *node_arena) {
  for (int i = 0; i < declaration_symbols->capacity; i++) {
    HashTableEntry *entry = &declaration_symbols->entries[i];

    if (entry == NULL || entry->key == NULL) {
      continue;
    }

    DeclarationSymbol *declaration_symbol= entry->value->structure;

    if (declaration_symbol->symbol_type != DECLARATION_SYMBOL_VARIABLE || declaration_symbol->data.variable_symbol->is_automatic_storage_duration) {
      continue;
    }
    
    IRNode *static_node = arena_alloc(node_arena);

    static_node->type = IR_VALUE_STATIC_VAR;
    static_node->data.static_variable.identifier = entry->key;
    static_node->data.static_variable.is_global = declaration_symbol->data.variable_symbol->static_is_global;

    if (declaration_symbol->data.variable_symbol->value_type == DECLARATION_SYMBOL_TYPE_INT) {
      if (declaration_symbol->data.variable_symbol->static_initial_type == INITIAL_VALUE_TENTATIVE) {     
        static_node->data.static_variable.initial_value.int_value = 0;
      } else {
        static_node->data.static_variable.initial_value.int_value = declaration_symbol->data.variable_symbol->static_initial_value.int_value;
      }
    } else {
      if (declaration_symbol->data.variable_symbol->static_initial_type == INITIAL_VALUE_TENTATIVE) {     
        static_node->data.static_variable.initial_value.long_value = 0;
      } else {
        static_node->data.static_variable.initial_value.long_value = declaration_symbol->data.variable_symbol->static_initial_value.long_value;
      }
    } 

    ir_add_top_level_declaration_to_program(ir_program, static_node);    
  }
}

void ir_add_postfix_operations(IRNode *ir_function, IREmitStatus *emit_status, Arena *node_arena, DeclarationSymbolTable *declaration_symbol_table) {
  if (emit_status->postfix_arena.offset == 0) {
    return;
  }

  for (int i = 0; i < emit_status->postfix_arena.offset; i += emit_status->postfix_arena.base_size) {    
    AstNode *node = (AstNode*)((char *)emit_status->postfix_arena.allocation);
    ir_emit_ast_node(node, ir_function, emit_status, node_arena, declaration_symbol_table);    
  }
}

void ir_add_instruction_to_function(IRNode *ir_function, IRNode *ir_instruction) {
  ir_add_to_node_pointer(ir_instruction, ir_function->data.function.instruction_ptrs);
  ir_function->data.function.instruction_count++;
}

void ir_add_top_level_declaration_to_program(IRNode *ir_program, IRNode *ir_function) {
  ir_add_to_node_pointer(ir_function, ir_program->data.program.top_level_ptrs);
  ir_program->data.program.top_level_count++;
}

void ir_add_argument_to_function_call(IRNode *ir_function_call_node, IRNode *argument) {
  int current_count = ir_function_call_node->data.instruction_function_call.arg_count;
  int current_capacity = ir_function_call_node->data.instruction_function_call.arg_capacity;
  
  if (current_count == current_capacity) {
    int new_size = current_capacity == 0 ? FUNCTION_CALL_CAPACITY : current_capacity * FUNCTION_CALL_CAPACITY;

    IRNode *functions = realloc(ir_function_call_node->data.instruction_function_call.args, new_size * sizeof(IRNode));

    ir_function_call_node->data.instruction_function_call.arg_capacity = new_size;
    ir_function_call_node->data.instruction_function_call.args = functions;
  } 

  ir_function_call_node->data.instruction_function_call.args[ir_function_call_node->data.instruction_function_call.arg_count] = *argument; 
  ir_function_call_node->data.instruction_function_call.arg_count++;
}

char* ir_create_temp_label(IREmitStatus *emit_status) {
  char *label_name = malloc(20);
  snprintf(label_name, 10, "%d", emit_status->temp_label_id++); 

  return label_name;
}

char* ir_create_temp_register(IREmitStatus *emit_status) {
  char *register_name = malloc(20);
  snprintf(register_name, 10, "tmp.%d", emit_status->temp_register_id++); 

  return register_name;
}

IRNode* ir_create_int_constant(int value, Arena *node_arena) {
  IRNode *constant = arena_alloc(node_arena);
  constant->type = IR_VALUE_CONSTANT;
  constant->data.value_constant.value.int_value = value;

  return constant;
}

IRNode* ir_create_ast_constant(AstNode *ast_constant, Arena *node_arena) {
  IRNode *constant = arena_alloc(node_arena);
  constant->type = IR_VALUE_CONSTANT;

  switch (ast_constant->data.constant_expression.expression_type->data.type.type) {
    case AST_TYPE_INT:
      constant->data.value_constant.value.int_value = ast_constant->data.constant_expression.int_value;
      break;
    case AST_TYPE_LONG:
      constant->data.value_constant.value.int_value = ast_constant->data.constant_expression.long_value;
      break;
    default:
      fprintf(stderr, "ERROR - IR: Attempted to create an unsupported Constant type (%d)", ast_constant->data.constant_expression.expression_type->data.type.type);
      exit(1);
  }

  return constant;
}

IRNode* ir_create_variable(char *identifier, Arena *node_arena) {
  IRNode *variable = arena_alloc(node_arena);
  variable->type = IR_VALUE_VAR;
  variable->data.value_var.identifier = identifier;

  return variable;
}

char* ir_create_concat_identifier(char *string, int integer) {
  //TODO: Hardcoded value
  char *identifier = malloc(64);
  snprintf(identifier, 64, "%s.%d", string, integer);

  return identifier;
}

void ir_add_to_node_pointer(IRNode *ir_node, IRNodePointer *ir_node_pointer) {
  if (ir_node_pointer == NULL) {
    return;
  }
  
  if (ir_node_pointer->count == ir_node_pointer->capacity) {
    int new_size = ir_node_pointer->capacity == 0 ? NODE_POINTER_CAPACITY : ir_node_pointer->capacity * 2;

    IRNode **realloc_pointers = realloc(ir_node_pointer->node_pointers, new_size * sizeof(IRNode**));

    ir_node_pointer->capacity = new_size;
    ir_node_pointer->node_pointers = realloc_pointers;
  } 

  ir_node_pointer->node_pointers[ir_node_pointer->count] = ir_node;
  ir_node_pointer->count++;
}

void ir_init_node_pointer(IRNodePointer *ir_node_pointer) {
  if (ir_node_pointer == NULL) {
    return;
  }
  
  ir_node_pointer->capacity = 0;
  ir_node_pointer->count = 0;
  ir_node_pointer->node_pointers = NULL;
}

static IRType get_node_type(IRNode *node) {
  switch (node->type) {
    case IR_VALUE_CONSTANT:   return node->data.value_constant.type; break;
    case IR_VALUE_STATIC_VAR: return node->data.static_variable.type; break;
    default:
      fprintf(stderr, "ERROR - IR: Unsupported node type '%d' for get_node_type", node->type);
      exit(1);
  }
}
