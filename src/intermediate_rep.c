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
#define FUNCTION_IDENTIFIER_INIT_CAPACITY 4
#define BREAK_LABEL "break"
#define CONTINUE_LABEL "continue"
#define START_LABEL "start"
#define END_LABEL "end"

typedef struct {
  Arena postfix_arena;
  int temp_register_id;
  int temp_label_id;
} IREmitStatus;

typedef struct {
  Arena *node_arena;
  IREmitStatus *emit_status;
  DeclarationSymbolTable *declaration_symbol_table;
} IntermediateRep;

typedef enum {
  EXPRESSION_RESULT_PLAIN_OPERAND,
  EXPRESSION_RESULT_DEREFERENCED_POINTER
} ExpressionResultType;

typedef struct {
  ExpressionResultType type;
  IRNode *operand_value;
} ExpressionResult;

static void              add_postfix_operations(IRNode *ir_function, IntermediateRep *intermediate_rep);
static ExpressionResult* emit_function(AstNode *ast_function, IntermediateRep *intemediate_rep);
static ExpressionResult* emit_ast_node(AstNode *node, IRNode *function, IntermediateRep *intermediate_rep); 
static IRNode*           emit_ast_node_and_convert_lvalue(AstNode *node, IRNode *function, IntermediateRep *intermediate_rep); 
static ExpressionResult* emit_return(AstNode *block_item, IRNode *function, IntermediateRep *intermediate_rep);
static void              emit_if(AstNode *block_item, IRNode *function, IntermediateRep *intermediate_rep); 
static void              emit_goto(AstNode *goto_node, IRNode *function, IntermediateRep *intermediate_rep); 
static void              emit_goto_label(AstNode *goto_label_node, IRNode *function, IntermediateRep *intermediate_rep); 
static void              emit_while(AstNode *while_node, IRNode *function, IntermediateRep *intermediate_rep); 
static void              emit_do_while(AstNode *do_node, IRNode *function, IntermediateRep *intermediate_rep); 
static void              emit_for(AstNode *for_node, IRNode *function, IntermediateRep *intermediate_rep); 
static void              emit_continue(int label_id, IRNode *function, IntermediateRep *intermediate_rep);
static void              emit_break(int label_id, IRNode *function, IntermediateRep *intermediate_rep);
static void              emit_block(AstNode *block_node, IRNode *function, IntermediateRep *intermediate_rep); 
static IRNode*           emit_jump(char *label, IRNode *function, IntermediateRep *intermediate_rep);
static IRNode*           emit_jump_if_zero(char *label, IRNode *condition, IRNode *function, IntermediateRep *intermediate_rep); 
static IRNode*           emit_jump_if_not_zero(char *label, IRNode *condition, IRNode *function, IntermediateRep *intermediate_rep); 
static IRNode*           emit_label(char* label, IRNode *function, IntermediateRep *intermediate_rep);
static IRNode*           emit_copy(IRNode *source, IRNode *destination, IRNode *function, IntermediateRep *intermediate_rep); 
static IRNode*           emit_store(IRNode *source, IRNode *destination_pointer, IRNode *function, IntermediateRep *intermediate_rep); 
static IRNode*           emit_get_address(IRNode *source, IRNode *destination, IRNode *function, IntermediateRep *intermediate_rep); 
static IRNode*           emit_truncate(IRNode *source, IRNode *destination, IRNode *function, IntermediateRep *intermediate_rep); 
static IRNode*           emit_sign_extend(IRNode *source, IRNode *destination, IRNode *function, IntermediateRep *intermediate_rep); 
static IRNode*           emit_zero_extend(IRNode *source, IRNode *destination, IRNode *function, IntermediateRep *intermediate_rep); 
static IRNode*           emit_double_to_int(IRNode *source, IRNode *destination, IRNode *function, IntermediateRep *intermediate_rep); 
static IRNode*           emit_double_to_uint(IRNode *source, IRNode *destination, IRNode *function, IntermediateRep *intermediate_rep); 
static IRNode*           emit_int_to_double(IRNode *source, IRNode *destination, IRNode *function, IntermediateRep *intermediate_rep); 
static IRNode*           emit_uint_to_double(IRNode *source, IRNode *destination, IRNode *function, IntermediateRep *intermediate_rep); 
static ExpressionResult* emit_declaration(AstNode *declaration_node, IRNode *function, IntermediateRep *intermediate_rep); 
static ExpressionResult* emit_conditional_expression(AstNode *condition_node, IRNode *function, IntermediateRep *intermediate_rep);
static ExpressionResult* emit_postfix_expression(AstNode *postfix_node, IntermediateRep *intermediate_rep);
static ExpressionResult* emit_unary_expression(AstNode *unary_node, IRNode *function, IntermediateRep *intermediate_rep);
static ExpressionResult* emit_binary_expression(AstNode *binary_node, IRNode *function, IntermediateRep *intermediate_rep);
static ExpressionResult* emit_assignment_expression(AstNode *assignment_node, IRNode *function, IntermediateRep *intermediate_rep);
static ExpressionResult* emit_function_call_expression(AstNode *function_call_node, IRNode *function, IntermediateRep *intermediate_rep); 
static ExpressionResult* emit_cast_expression(AstNode *cast_node, IRNode *function, IntermediateRep *intermediate_rep);
static ExpressionResult* emit_address_of_expression(AstNode *address_of_node, IRNode *function, IntermediateRep *intermediate_rep); 
static ExpressionResult* emit_dereference_expression(AstNode *dereference_node, IRNode *function, IntermediateRep *intermediate_rep); 
static void              emit_symbol_declarations(HashTable *declaration_symbols, IRNode *ir_program,  IntermediateRep *intermediate_rep); 
static void              add_instruction_to_function(IRNode *ir_function, IRNode *ir_instruction); 
static void              add_top_level_declaration_to_program(IRNode *ir_program, IRNode *ir_function); 
static void              add_argument_to_function_call(IRNode *ir_function_call_node, IRNode *argument);
static ExpressionResult* create_expression_result(IRNode *operand, ExpressionResultType result_type, IntermediateRep *intermediate_rep); 
static char*             create_temp_label(IntermediateRep *intermediate_rep); 
static char*             create_temp_register(IntermediateRep *intermediate_rep); 
static char*             create_concat_identifier(char *string, int integer); 
static ExpressionResult* create_ast_constant(AstNode *constant_node, IntermediateRep *intermediate_rep);
static IRNode*           create_int_constant(int value, IntermediateRep *intermediate_rep);
static ExpressionResult* create_variable(char *identifier, IntermediateRep *intermediate_rep);
static void              add_to_node_pointer(IRNode *ir_node, IRNodePointer *ir_node_pointer); 
static void              init_node_pointer(IRNodePointer *ir_node_pointer); 
static TypeNode*         get_node_type(IRNode *node, IntermediateRep *intermediate_rep); 
static void              add_function_parameter_identifier(char *identifier, IRNode *function_node);  

IRNode* generate_intermediate_rep(AstNode *ast_node, DeclarationSymbolTable *declaration_symbol_table) {
  Arena *node_arena = malloc(sizeof(Arena));

  //TODO: Hardcoded capacity
  arena_init(node_arena, sizeof(IRNode), sizeof(IRNode) * 1000, false);

  IRNodePointer *node_pointer = malloc(sizeof(IRNodePointer));
  init_node_pointer(node_pointer);

  IRNode *program = arena_alloc(node_arena);

  program->type = IR_PROGRAM;
  program->data.program.top_level_count = 0;
  program->data.program.top_level_ptrs = node_pointer;

  IREmitStatus emit_status = {
    .temp_register_id = 0,
    .temp_label_id = 0
  };

  IntermediateRep intermediate_rep = {
    .declaration_symbol_table = declaration_symbol_table,
    .emit_status = &emit_status,
    .node_arena = node_arena
  };

  for (int i = 0; i < ast_node->data.program.declaration_count; i++) {
    AstNode *declaration_node = ast_node->data.program.declaration_ptrs->node_pointers[i];

    if (declaration_node->type == AST_VARIABLE_DECLARATION || declaration_node->data.declaration_function.body_block == NULL) {
      continue;
    }

    ExpressionResult *top_level_declaration = emit_function(declaration_node, &intermediate_rep);
    add_top_level_declaration_to_program(program, top_level_declaration->operand_value);        
  }

  emit_symbol_declarations(declaration_symbol_table->symbol_table, program, &intermediate_rep);

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
          struct IRInstructionUnary* unary = &function->instruction_ptrs->node_pointers[i]->data.instruction_unary;

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
      switch (ir_node->data.value_constant.type->type) {
        case TYPE_INT:     printf("Constant(type = int, value = %d)", ir_node->data.value_constant.value.int_value); break;
        case TYPE_LONG:    printf("Constant(type = long, value = %ld)", ir_node->data.value_constant.value.long_value); break;          
        case TYPE_UINT:    printf("Constant(type = uint, value = %d)", ir_node->data.value_constant.value.uint_value); break;
        case TYPE_ULONG:   printf("Constant(type = ulong, value = %ld)", ir_node->data.value_constant.value.ulong_value); break;          
        case TYPE_DOUBLE:  printf("Constant(type = double, value = %f)", ir_node->data.value_constant.value.double_value); break;          
        default:
          fprintf(stderr, "ERROR - Intermediate Rep: Unsupported type '%d' when attempting to print constant\n", ir_node->data.value_constant.type->type);
          exit(1);
      }
      break;
    case IR_VALUE_VAR:
      printf("Var(\"%s\")", ir_node->data.value_var.identifier);
      break;
    case IR_VALUE_STATIC_VAR:
      printf("Static Var(\"%s\" Initial Value: ", ir_node->data.static_variable.identifier);

      switch (ir_node->data.static_variable.static_variable_symbol->value_type->type) {
        case TYPE_INT:    printf("%d, type = int, ", ir_node->data.static_variable.static_variable_symbol->static_initial_value.int_value); break;
        case TYPE_LONG:   printf("%ld, type = long, ", ir_node->data.static_variable.static_variable_symbol->static_initial_value.long_value); break;
        case TYPE_UINT:   printf("%d, type = int, ", ir_node->data.static_variable.static_variable_symbol->static_initial_value.uint_value); break;
        case TYPE_ULONG:  printf("%ld, type = long, ", ir_node->data.static_variable.static_variable_symbol->static_initial_value.ulong_value); break;
        case TYPE_DOUBLE: printf("%f, type = double, ", ir_node->data.static_variable.static_variable_symbol->static_initial_value.double_value); break;
        default:
          fprintf(stderr, "ERROR - Intermediate Rep: Unsupported declaration type '%d' when attempting to print static variable\n", ir_node->data.static_variable.static_variable_symbol->value_type->type);
          exit(1);
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
    case IR_INSTRUCTION_GET_ADDRESS:
      printf("Get Address(Source(");
      print_intermediate_ret(ir_node->data.instruction_get_address.source);
      printf(") (Destination(");
      print_intermediate_ret(ir_node->data.instruction_get_address.destination);
      printf(")\n");
      break;
    case IR_INSTRUCTION_LOAD:
      printf("Load(Source(");
      print_intermediate_ret(ir_node->data.instruction_load.source_pointer);
      printf(") (Destination(");
      print_intermediate_ret(ir_node->data.instruction_load.destination);
      printf(")\n");
      break;
    case IR_INSTRUCTION_STORE:
      printf("Store(Source(");
      print_intermediate_ret(ir_node->data.instruction_store.source);
      printf(") (Destination(");
      print_intermediate_ret(ir_node->data.instruction_store.destination_pointer);
      break;
    default:
      fprintf(stderr, "ERROR - IR: No print for type %d\n", ir_node->type);
      exit(1);
  }
}

static ExpressionResult* emit_function(AstNode *ast_function, IntermediateRep *intermediate_rep) {
  IRNode *function = arena_alloc(intermediate_rep->node_arena);
  IRNodePointer *ir_node_pointer = malloc(sizeof(IRNodePointer));
  init_node_pointer(ir_node_pointer);
  
  function->type = IR_FUNCTION;
  function->data.function.identifier = ast_function->data.declaration_function.name;
  function->data.function.instruction_count = 0;
  function->data.function.instruction_ptrs = ir_node_pointer;

  HashTableEntry *found_declaration_entry = hash_table_get_entry(intermediate_rep->declaration_symbol_table->symbol_table, ast_function->data.declaration_function.name);

  if (found_declaration_entry == NULL || found_declaration_entry->key == NULL) {
    fprintf(stderr, "ERROR - IR: Declaration Symbol expected for the following function: '%s'\n", ast_function->data.declaration_function.name);
    exit(1);
  }

  DeclarationSymbol *symbol = found_declaration_entry->value->structure;
  function->data.function.is_global = symbol->data.function_symbol->is_global;

  for (int i = 0; i < ast_function->data.declaration_function.function_type->data.function_type.param_type_count; i++) {
    add_function_parameter_identifier(ast_function->data.declaration_function.parameter_identifiers[i], function);
  }
    
  Arena postfix_arena;
  //@WARNING: Hardcoded postfix arena size
  //TODO: May be better to initialize outside of this function and instead reset the allocated arena
  arena_init(&postfix_arena, sizeof(AstNode), sizeof(AstNode) * 50, true);
  intermediate_rep->emit_status->postfix_arena = postfix_arena;

  emit_ast_node(ast_function->data.declaration_function.body_block, function, intermediate_rep);

  //@Temporary: Add return statement to every function that returns 0. If there is a return statement already for the function, this won't run.
  IRNode *zero_value = create_int_constant(0, intermediate_rep);
  IRNode *return_instruction = arena_alloc(intermediate_rep->node_arena);
  return_instruction->type = IR_INSTRUCTION_RET;
  return_instruction->data.instruction_ret.value = zero_value;

  add_instruction_to_function(function, return_instruction);

  ExpressionResult *function_result = create_expression_result(function, EXPRESSION_RESULT_PLAIN_OPERAND, intermediate_rep);
  return function_result;
}

static ExpressionResult* emit_ast_node(AstNode *node, IRNode *function, IntermediateRep *intermediate_rep) {
  switch (node->type) {
      case AST_BLOCK:                        { emit_block(node, function, intermediate_rep); break; }
      case AST_STATEMENT_IF:                 { emit_if(node, function, intermediate_rep); break; }
      case AST_STATEMENT_GOTO:               { emit_goto(node, function, intermediate_rep); break; }
      case AST_STATEMENT_GOTO_LABEL:         { emit_goto_label(node, function, intermediate_rep); break; }
      case AST_STATEMENT_WHILE:              { emit_while(node, function, intermediate_rep); break; }
      case AST_STATEMENT_DO_WHILE:           { emit_do_while(node, function, intermediate_rep); break; }
      case AST_STATEMENT_FOR:                { emit_for(node, function, intermediate_rep); break; }
      case AST_STATEMENT_CONTINUE:           { emit_continue(node->data.statement_continue.label_id, function, intermediate_rep); break; }
      case AST_STATEMENT_BREAK:              { emit_break(node->data.statement_break.label_id, function, intermediate_rep); break; }
      case AST_STATEMENT_COMPOUND:           { emit_block(node->data.statement_compound.block, function, intermediate_rep); break; }
      case AST_STATEMENT_NULL:               { break; } 
      case AST_STATEMENT_RETURN:             { return emit_return(node, function, intermediate_rep); }
      case AST_EXPRESSION_VARIABLE:          { return create_variable(node->data.expression_variable.identifier, intermediate_rep); }
      case AST_EXPRESSION_CONSTANT:          { return create_ast_constant(node, intermediate_rep); }
      case AST_EXPRESSION_CONDITIONAL:       { return emit_conditional_expression(node, function, intermediate_rep); }
      case AST_EXPRESSION_POSTFIX_INCREMENT: { return emit_postfix_expression(node, intermediate_rep); }
      case AST_EXPRESSION_POSTFIX_DECREMENT: { return emit_postfix_expression(node, intermediate_rep); }
      case AST_EXPRESSION_PREFIX_INCREMENT:  { return emit_ast_node(node, function, intermediate_rep); }
      case AST_EXPRESSION_PREFIX_DECREMENT:  { return emit_ast_node(node, function, intermediate_rep); }
      case AST_EXPRESSION_UNARY:             { return emit_unary_expression(node, function, intermediate_rep); }
      case AST_EXPRESSION_BINARY:            { return emit_binary_expression(node, function, intermediate_rep); }
      case AST_EXPRESSION_ASSIGNMENT:        { return emit_assignment_expression(node, function, intermediate_rep); }
      case AST_EXPRESSION_FUNCTION_CALL:     { return emit_function_call_expression(node, function, intermediate_rep); }
      case AST_EXPRESSION_CAST:              { return emit_cast_expression(node, function, intermediate_rep); } 
      case AST_EXPRESSION_ADDRESS_OF:        { return emit_address_of_expression(node, function, intermediate_rep); }
      case AST_EXPRESSION_DEREFERENCE:       { return emit_dereference_expression(node, function, intermediate_rep); }
      case AST_VARIABLE_DECLARATION:         { return emit_declaration(node, function, intermediate_rep); }
      case AST_FUNCTION_DECLARATION:         {
          if (node->data.declaration_function.body_block == NULL) break;
          return emit_function(node, intermediate_rep);
      }
      default:
        fprintf(stderr, "ERROR - IR: ASTNode type %d not found for node emit\n", node->type);
        exit(1);
  }

  return NULL;
}

static IRNode* emit_ast_node_and_convert_lvalue(AstNode *node, IRNode *function, IntermediateRep *intermediate_rep) {
  ExpressionResult *result = emit_ast_node(node, function, intermediate_rep);

  if (result->type == EXPRESSION_RESULT_PLAIN_OPERAND) {
    return result->operand_value;
  }  

  char *destination_name = create_temp_register(intermediate_rep);
  TypeNode *result_type = get_node_type(result->operand_value, intermediate_rep);

  add_automatic_variable_declaration_symbol(intermediate_rep->declaration_symbol_table, result_type, destination_name);

  IRNode *destination = arena_alloc(intermediate_rep->node_arena);
  destination->type = IR_VALUE_VAR;
  destination->data.value_var.identifier = destination_name;

  IRNode *load_instruction = arena_alloc(intermediate_rep->node_arena);
  load_instruction->type = IR_INSTRUCTION_LOAD;
  load_instruction->data.instruction_load.source_pointer = result->operand_value;
  load_instruction->data.instruction_load.destination = destination;
  
  add_instruction_to_function(function, load_instruction);

  return load_instruction;
}
 
static void emit_block(AstNode *block_node, IRNode *function, IntermediateRep *intermediate_rep) {
  for (int i = 0; i < block_node->data.block.block_count; i++) {
    arena_reset(&intermediate_rep->emit_status->postfix_arena);
    AstNode *block_item_node = block_node->data.block.block_ptrs->node_pointers[i];
    emit_ast_node(block_item_node, function, intermediate_rep);
    add_postfix_operations(function, intermediate_rep);
  }
}

static ExpressionResult* emit_return(AstNode *block_item, IRNode *function, IntermediateRep *intermediate_rep) {
  IRNode *value = emit_ast_node_and_convert_lvalue(block_item->data.statement_return.expression, function, intermediate_rep);
  IRNode *return_instruction = arena_alloc(intermediate_rep->node_arena);

  return_instruction->type = IR_INSTRUCTION_RET;
  return_instruction->data.instruction_ret.value = value;

  add_instruction_to_function(function, return_instruction);
  add_postfix_operations(function, intermediate_rep);

  ExpressionResult *return_result = create_expression_result(return_instruction, EXPRESSION_RESULT_PLAIN_OPERAND, intermediate_rep);

  return return_result;
}

static void emit_if(AstNode *if_node, IRNode *function, IntermediateRep *intermediate_rep) {
  IRNode *condition = emit_ast_node_and_convert_lvalue(if_node->data.statement_if.condition_expression, function, intermediate_rep);
  char *label_name = create_temp_label(intermediate_rep);

  emit_jump_if_zero(label_name, condition, function, intermediate_rep);

  AstNode *then_statement = if_node->data.statement_if.then_statement;

  emit_ast_node(then_statement, function, intermediate_rep);
  emit_label(label_name, function, intermediate_rep);
}

static void emit_goto(AstNode *goto_node, IRNode *function, IntermediateRep *intermediate_rep) {
  emit_jump(goto_node->data.statement_goto_label.label, function, intermediate_rep);
}

static void emit_goto_label(AstNode *goto_label_node, IRNode *function, IntermediateRep *intermediate_rep) {
  emit_label(goto_label_node->data.statement_goto.label, function, intermediate_rep);
}

static void emit_while(AstNode *while_node, IRNode *function, IntermediateRep *intermediate_rep) {
  char *continue_label_identifier = create_concat_identifier(CONTINUE_LABEL, while_node->data.statement_do_while.label_id); 
  char *break_label_identifier = create_concat_identifier(BREAK_LABEL, while_node->data.statement_do_while.label_id); 

  emit_label(continue_label_identifier, function, intermediate_rep);

  IRNode *condition = emit_ast_node_and_convert_lvalue(while_node->data.statement_while.condition, function, intermediate_rep);

  emit_jump_if_zero(break_label_identifier, condition, function, intermediate_rep);
  emit_ast_node(while_node->data.statement_while.statement_body, function, intermediate_rep);
  emit_jump(continue_label_identifier, function, intermediate_rep);
  emit_label(break_label_identifier, function, intermediate_rep);
}

static void emit_do_while(AstNode *do_node, IRNode *function, IntermediateRep *intermediate_rep) {
  char *start_label_identifier = create_concat_identifier(START_LABEL, do_node->data.statement_do_while.label_id);
  emit_label(start_label_identifier, function, intermediate_rep);

  emit_ast_node(do_node->data.statement_do_while.statement_body, function, intermediate_rep);

  char *continue_label_identifier = create_concat_identifier(CONTINUE_LABEL, do_node->data.statement_do_while.label_id); 
  emit_label(continue_label_identifier, function, intermediate_rep);

  IRNode *condition = emit_ast_node_and_convert_lvalue(do_node->data.statement_do_while.condition, function, intermediate_rep);
  emit_jump_if_not_zero(start_label_identifier, condition, function, intermediate_rep);

  char *break_label_identifier = create_concat_identifier(BREAK_LABEL, do_node->data.statement_do_while.label_id);
  emit_label(break_label_identifier, function, intermediate_rep);
}

static void emit_for(AstNode *for_node, IRNode *function, IntermediateRep *intermediate_rep) {
  if (for_node->data.statement_for.for_loop_init != NULL) {
    emit_ast_node(for_node->data.statement_for.for_loop_init, function, intermediate_rep);
  }  

  char *start_label_identifier = create_concat_identifier(START_LABEL, for_node->data.statement_for.label_id);
  emit_label(start_label_identifier, function, intermediate_rep);

  char *break_label_identifier = create_concat_identifier(BREAK_LABEL, for_node->data.statement_for.label_id);

  if (for_node->data.statement_for.condition_expression != NULL) {
    IRNode *condition = emit_ast_node_and_convert_lvalue(for_node->data.statement_for.condition_expression, function, intermediate_rep);
    emit_jump_if_zero(break_label_identifier, condition, function, intermediate_rep);
  }

  emit_ast_node(for_node->data.statement_for.statement_body, function, intermediate_rep);

  char *continue_label_identifier = create_concat_identifier(CONTINUE_LABEL, for_node->data.statement_for.label_id);
  emit_label(continue_label_identifier, function, intermediate_rep);

  if (for_node->data.statement_for.post_expression != NULL) {
    emit_ast_node(for_node->data.statement_for.post_expression, function, intermediate_rep);
  }

  emit_jump(start_label_identifier, function, intermediate_rep);
  emit_label(break_label_identifier, function, intermediate_rep);
}

static void emit_continue(int label_id, IRNode *function, IntermediateRep *intermediate_rep) {
  char *continue_label_identifier = create_concat_identifier(CONTINUE_LABEL, label_id); 
  emit_jump(continue_label_identifier, function, intermediate_rep);
}

static void emit_break(int label_id, IRNode *function, IntermediateRep *intermediate_rep) {
  char *break_label_identifier = create_concat_identifier(BREAK_LABEL, label_id); 
  emit_jump(break_label_identifier, function, intermediate_rep);
}

static ExpressionResult* emit_declaration(AstNode *declaration_node, IRNode *function, IntermediateRep *intermediate_rep) {
  if (!declaration_node->data.declaration_variable.has_expression) {
    return NULL;
  }

  IRNode *node = emit_ast_node_and_convert_lvalue(declaration_node->data.declaration_variable.init_expression, function, intermediate_rep);    
  add_postfix_operations(function, intermediate_rep);

  ExpressionResult *expression_result = create_expression_result(node, EXPRESSION_RESULT_PLAIN_OPERAND, intermediate_rep);
  return expression_result;
}

static ExpressionResult* emit_conditional_expression(AstNode *conditional_node, IRNode *function, IntermediateRep *intermediate_rep) {
  IRNode *condition = emit_ast_node_and_convert_lvalue(conditional_node->data.expression_conditional.condition, function, intermediate_rep);

  char *end_label_name = create_temp_label(intermediate_rep);
  char *false_label_name = create_temp_label(intermediate_rep);

  emit_jump_if_zero(false_label_name, condition, function, intermediate_rep);

  IRNode *true_value = emit_ast_node_and_convert_lvalue(conditional_node->data.expression_conditional.true_expression, function, intermediate_rep);

  emit_jump(end_label_name, function, intermediate_rep);
  emit_label(false_label_name, function, intermediate_rep);

  IRNode *false_value = emit_ast_node_and_convert_lvalue(conditional_node->data.expression_conditional.false_expression, function, intermediate_rep);      

  emit_label(end_label_name, function, intermediate_rep);

  ExpressionResult *conditional_result = create_expression_result(condition, EXPRESSION_RESULT_PLAIN_OPERAND, intermediate_rep);
  return conditional_result;
}

static ExpressionResult* emit_postfix_expression(AstNode *postfix_node, IntermediateRep *intermediate_rep) {
  AstNode *postfix_arena_node = arena_alloc(&intermediate_rep->emit_status->postfix_arena);
  *postfix_arena_node = *postfix_node->data.expression_increment_decrement.expression;

  IRNode *variable = arena_alloc(intermediate_rep->node_arena);
  variable->type = IR_VALUE_VAR;

  AstNode *postfix_expression = postfix_node->data.expression_increment_decrement.expression->data.expression_assignment.left_expression;

  if (postfix_expression->type == AST_EXPRESSION_VARIABLE) {
    variable->data.value_var.identifier = postfix_expression->data.expression_variable.identifier;
  } else if (postfix_expression->type == AST_EXPRESSION_UNARY) {
    variable->data.value_var.identifier = postfix_expression->data.expression_unary.expression->data.expression_variable.identifier;
  } else {
    fprintf(stderr, "ERROR - Intermediate Rep: Could not resolve variable identifier for Postfix expression\n");
    exit(1);
  }

  ExpressionResult *variable_result = create_expression_result(variable, EXPRESSION_RESULT_PLAIN_OPERAND, intermediate_rep);
  return variable_result;
}

static ExpressionResult* emit_unary_expression(AstNode *unary_node, IRNode *function, IntermediateRep *intermediate_rep) {
  IRNode *source = emit_ast_node_and_convert_lvalue(unary_node->data.expression_unary.expression, function, intermediate_rep);

  //TODO: Warning, setting hard buffer limit
  char *destination_name = create_temp_register(intermediate_rep);

  add_automatic_variable_declaration_symbol(intermediate_rep->declaration_symbol_table, unary_node->data.expression_unary.expression_type, destination_name);

  IRNode *destination = arena_alloc(intermediate_rep->node_arena);
  destination->type = IR_VALUE_VAR;
  destination->data.value_var.identifier = destination_name;

  IRUnaryOpType unary_op_type;

  switch (unary_node->data.expression_unary.op_type) {
    case AST_UNARY_COMPLEMENT: unary_op_type = IR_UNARY_COMPLEMENT; break;
    case AST_UNARY_NEGATE:     unary_op_type = IR_UNARY_NEGATE; break;
    case AST_UNARY_NOT:        unary_op_type = IR_UNARY_NOT; break;
    default:
      fprintf(stderr, "ERROR - Intermediate Rep: Unsupported unary op type");
      exit(1);
  }

  IRNode *unary_instruction = arena_alloc(intermediate_rep->node_arena);         
  unary_instruction->type = IR_INSTRUCTION_UNARY;
  unary_instruction->data.instruction_unary.op_type = unary_op_type;
  unary_instruction->data.instruction_unary.source = source;
  unary_instruction->data.instruction_unary.destination = destination;

  add_instruction_to_function(function, unary_instruction);

  ExpressionResult *destination_result = create_expression_result(destination, EXPRESSION_RESULT_PLAIN_OPERAND, intermediate_rep);

  return destination_result;
}

static ExpressionResult* emit_binary_expression(AstNode *binary_node, IRNode *function, IntermediateRep *intermediate_rep) {
  IRNode *source_1 = emit_ast_node_and_convert_lvalue(binary_node->data.expression_binary.left_expression, function, intermediate_rep);
  IRNode *source_2 = emit_ast_node_and_convert_lvalue(binary_node->data.expression_binary.right_expression, function, intermediate_rep);

  //TODO: Warning, setting hard buffer limit
  char *destination_name = create_temp_register(intermediate_rep);

  IRNode *destination = arena_alloc(intermediate_rep->node_arena);
  destination->type = IR_VALUE_VAR;
  destination->data.value_var.identifier = destination_name;

  if (binary_node->data.expression_binary.op_type == AST_BINARY_AND || binary_node->data.expression_binary.op_type == AST_BINARY_OR) {
    //@Temp: Malloc'ing node to satisfy the need to padd it into the add function. Look into a way to add to the type arena
    TypeNode *int_type_node = malloc(sizeof(TypeNode));
    int_type_node->type = TYPE_INT;

    add_automatic_variable_declaration_symbol(intermediate_rep->declaration_symbol_table, int_type_node, destination_name);    

    char *label_name = create_temp_label(intermediate_rep);

    IRNode *jmp_instruction_v1 = arena_alloc(intermediate_rep->node_arena);

    if (binary_node->data.expression_binary.op_type == AST_BINARY_AND) { 
      jmp_instruction_v1->type = IR_INSTRUCTION_JUMP_IF_ZERO;  
      jmp_instruction_v1->data.instruction_jump_if_zero.condition = source_1;
      jmp_instruction_v1->data.instruction_jump_if_zero.target = label_name;
    } else {
      jmp_instruction_v1->type = IR_INSTRUCTION_JUMP_IF_NOT_ZERO;  
      jmp_instruction_v1->data.instruction_jump_if_not_zero.condition = source_1;
      jmp_instruction_v1->data.instruction_jump_if_not_zero.target = label_name;
    }

    add_instruction_to_function(function, jmp_instruction_v1);

    IRNode *jmp_instruction_v2 = arena_alloc(intermediate_rep->node_arena);

    if (binary_node->data.expression_binary.op_type == AST_BINARY_AND) { 
      jmp_instruction_v2->type = IR_INSTRUCTION_JUMP_IF_ZERO;  
      jmp_instruction_v2->data.instruction_jump_if_zero.condition = source_2;
      jmp_instruction_v2->data.instruction_jump_if_zero.target = label_name;
    } else {
      jmp_instruction_v2->type = IR_INSTRUCTION_JUMP_IF_NOT_ZERO;  
      jmp_instruction_v2->data.instruction_jump_if_not_zero.condition = source_2;
      jmp_instruction_v2->data.instruction_jump_if_not_zero.target = label_name;
    }

    add_instruction_to_function(function, jmp_instruction_v2);

    IRNode *result_1 = create_int_constant(1, intermediate_rep);

    emit_copy(result_1, destination, function, intermediate_rep);
    emit_jump(END_LABEL, function, intermediate_rep);
    emit_label(label_name, function, intermediate_rep);

    IRNode *result_0 = create_int_constant(0, intermediate_rep);

    emit_copy(result_0, destination, function, intermediate_rep);
    emit_label(END_LABEL, function, intermediate_rep);

    ExpressionResult *destination_result = create_expression_result(destination, EXPRESSION_RESULT_PLAIN_OPERAND, intermediate_rep);
    return destination_result;
  }

  add_automatic_variable_declaration_symbol(intermediate_rep->declaration_symbol_table, binary_node->data.expression_binary.expression_type, destination_name);

  IRBinaryOpType binary_op_type;

  switch (binary_node->data.expression_binary.op_type) {
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

  IRNode *binary_instruction = arena_alloc(intermediate_rep->node_arena);         
  binary_instruction->type = IR_INSTRUCTION_BINARY;
  binary_instruction->data.instruction_binary.op_type = binary_op_type;
  binary_instruction->data.instruction_binary.source_1 = source_1;
  binary_instruction->data.instruction_binary.source_2 = source_2;
  binary_instruction->data.instruction_binary.destination = destination;

  add_instruction_to_function(function, binary_instruction);

  ExpressionResult *destination_result = create_expression_result(destination, EXPRESSION_RESULT_PLAIN_OPERAND, intermediate_rep);
  return destination_result;
}

static ExpressionResult* emit_assignment_expression(AstNode *assignment_node, IRNode *function, IntermediateRep *intermediate_rep) {
  //TODO: Keep this for now. Need to assess why conditional expressions are handled differently when the source node is 'ast_expression_assignment'. There's already an emit_conditional(). 
  if (assignment_node->data.expression_assignment.right_expression->type == AST_EXPRESSION_CONDITIONAL) {
    IRNode *condition = emit_ast_node_and_convert_lvalue(assignment_node->data.expression_assignment.right_expression->data.expression_conditional.condition, function, intermediate_rep);

    char *end_label_name = create_temp_label(intermediate_rep);
    char *false_label_name = create_temp_label(intermediate_rep);

    emit_jump_if_zero(false_label_name, condition, function, intermediate_rep);
    
    IRNode *true_value = emit_ast_node_and_convert_lvalue(assignment_node->data.expression_assignment.right_expression->data.expression_conditional.true_expression, function, intermediate_rep);

    IRNode *variable = arena_alloc(intermediate_rep->node_arena);
    variable->type = IR_VALUE_VAR;
    variable->data.value_var.identifier = assignment_node->data.expression_assignment.left_expression->data.expression_variable.identifier;

    emit_copy(true_value, variable, function, intermediate_rep);
    emit_jump(end_label_name, function, intermediate_rep);
    emit_label(false_label_name, function, intermediate_rep);
  
    IRNode *false_value = emit_ast_node_and_convert_lvalue(assignment_node->data.expression_assignment.right_expression->data.expression_conditional.false_expression, function, intermediate_rep);

    emit_copy(false_value, variable, function, intermediate_rep);
    emit_label(end_label_name, function, intermediate_rep);

    return NULL;
  }

  IRNode *right_value = emit_ast_node_and_convert_lvalue(assignment_node->data.expression_assignment.right_expression, function, intermediate_rep);
  ExpressionResult *left_result = emit_ast_node(assignment_node->data.expression_assignment.left_expression, function, intermediate_rep);


  if (left_result->type == EXPRESSION_RESULT_PLAIN_OPERAND) {   
    emit_copy(right_value, left_result->operand_value, function, intermediate_rep);
    return left_result;
  }

  emit_store(right_value, left_result->operand_value, function, intermediate_rep);   

  ExpressionResult *plain_right_operand = create_expression_result(right_value, EXPRESSION_RESULT_PLAIN_OPERAND, intermediate_rep);
  return plain_right_operand;
}

static ExpressionResult* emit_function_call_expression(AstNode *function_call_node, IRNode *function, IntermediateRep *intermediate_rep) {
  IRNode *ir_function_call = arena_alloc(intermediate_rep->node_arena);
  ir_function_call->type = IR_INSTRUCTION_FUNCTION_CALL;
  ir_function_call->data.instruction_function_call.identifier = function_call_node->data.expression_function_call.identfier;
  ir_function_call->data.instruction_function_call.arg_capacity = 0;
  ir_function_call->data.instruction_function_call.arg_count = 0;
  ir_function_call->data.instruction_function_call.args = NULL;

  char *destination_name = create_temp_register(intermediate_rep);

  add_automatic_variable_declaration_symbol(intermediate_rep->declaration_symbol_table, function_call_node->data.expression_function_call.expression_type, destination_name);

  IRNode *destination = arena_alloc(intermediate_rep->node_arena);
  destination->type = IR_VALUE_VAR;
  destination->data.value_var.identifier = destination_name;

  ir_function_call->data.instruction_function_call.destination = destination;

  for (int i = 0; i < function_call_node->data.expression_function_call.argument_count; i++) {
    AstNode *argument_node = function_call_node->data.expression_function_call.argument_ptrs->node_pointers[i];

    IRNode *argument = emit_ast_node_and_convert_lvalue(argument_node, function, intermediate_rep);

    add_argument_to_function_call(ir_function_call, argument);    
  }

  add_instruction_to_function(function, ir_function_call);

  ExpressionResult *destination_result = create_expression_result(destination, EXPRESSION_RESULT_PLAIN_OPERAND, intermediate_rep);

  return destination_result;
} 

static ExpressionResult* emit_cast_expression(AstNode *cast_node, IRNode *function, IntermediateRep *intermediate_rep) {
  IRNode *cast_expression = emit_ast_node_and_convert_lvalue(cast_node->data.expression_cast.expression, function, intermediate_rep);
  TypeNode *expression_type = get_node_type(cast_expression, intermediate_rep); 
  TypeNode *target_type = cast_node->data.expression_cast.target_type;

  if (expression_type->type == target_type->type) {
    ExpressionResult *cast_result = create_expression_result(cast_expression, EXPRESSION_RESULT_PLAIN_OPERAND, intermediate_rep);
    return cast_result;
  }

  char *temp_destination = create_temp_register(intermediate_rep);
  add_automatic_variable_declaration_symbol(intermediate_rep->declaration_symbol_table, target_type, temp_destination);

  ExpressionResult *var_destination_node = create_variable(temp_destination, intermediate_rep);

  if (get_type_size(target_type->type) == get_type_size(expression_type->type)) {
    emit_copy(cast_expression, var_destination_node->operand_value, function, intermediate_rep);
  } else if (expression_type->type == TYPE_DOUBLE && target_type->type == TYPE_INT) {
    emit_double_to_int(cast_expression, var_destination_node->operand_value, function, intermediate_rep);
  } else if (expression_type->type == TYPE_DOUBLE && target_type->type == TYPE_UINT) {
    emit_double_to_uint(cast_expression, var_destination_node->operand_value, function, intermediate_rep);
  } else if (expression_type->type == TYPE_INT && target_type->type == TYPE_DOUBLE) {
    emit_int_to_double(cast_expression, var_destination_node->operand_value, function, intermediate_rep);
  } else if (expression_type->type == TYPE_UINT && target_type->type == TYPE_DOUBLE) {
    emit_uint_to_double(cast_expression, var_destination_node->operand_value, function, intermediate_rep);
  } else if (expression_type->type == TYPE_POINTER) {
    emit_sign_extend(cast_expression, var_destination_node->operand_value, function, intermediate_rep);
  } else if (target_type->type == TYPE_POINTER) {
    emit_truncate(cast_expression, var_destination_node->operand_value, function, intermediate_rep);    
  } else if (get_type_size(target_type->type) < get_type_size(expression_type->type)) {
    emit_truncate(cast_expression, var_destination_node->operand_value, function, intermediate_rep);    
  } else if (is_type_signed(expression_type->type)) {
    emit_sign_extend(cast_expression, var_destination_node->operand_value, function, intermediate_rep);
  } else {
    emit_zero_extend(cast_expression, var_destination_node->operand_value, function, intermediate_rep);
  }  

  return var_destination_node;
}

static ExpressionResult* emit_address_of_expression(AstNode *address_of_node, IRNode *function, IntermediateRep *intermediate_rep) {
  ExpressionResult *address_of_result = emit_ast_node(address_of_node->data.expression_address_of.expression, function, intermediate_rep);

  if (address_of_result->type == EXPRESSION_RESULT_PLAIN_OPERAND) {   
    char *destination_name = create_temp_register(intermediate_rep);
    TypeNode *result_type = get_node_type(address_of_result->operand_value, intermediate_rep);

    add_automatic_variable_declaration_symbol(intermediate_rep->declaration_symbol_table, result_type, destination_name);

    IRNode *destination = arena_alloc(intermediate_rep->node_arena);
    destination->type = IR_VALUE_VAR;
    destination->data.value_var.identifier = destination_name;

    emit_get_address(address_of_result->operand_value, destination, function, intermediate_rep);

    ExpressionResult *destination_result = create_expression_result(destination, EXPRESSION_RESULT_PLAIN_OPERAND, intermediate_rep);
    return destination_result;
  }

  ExpressionResult *pointer_result = create_expression_result(address_of_result->operand_value, EXPRESSION_RESULT_PLAIN_OPERAND, intermediate_rep);

  return pointer_result;  
}

static ExpressionResult* emit_dereference_expression(AstNode *dereference_node, IRNode *function, IntermediateRep *intermediate_rep) {
  IRNode *dereference_instruction = emit_ast_node_and_convert_lvalue(dereference_node->data.expression_dereference.expression, function, intermediate_rep);

  ExpressionResult *dereference_result = create_expression_result(dereference_instruction, EXPRESSION_RESULT_DEREFERENCED_POINTER, intermediate_rep);
  return dereference_result;
}

static IRNode* emit_jump(char *label, IRNode *function, IntermediateRep *intermediate_rep) {
  IRNode *jmp_instruction = arena_alloc(intermediate_rep->node_arena);
  jmp_instruction->type = IR_INSTRUCTION_JUMP;
  jmp_instruction->data.instruction_jump.target = label;

  add_instruction_to_function(function, jmp_instruction);

  return jmp_instruction;
}

static IRNode* emit_jump_if_zero(char *label, IRNode *condition, IRNode *function, IntermediateRep *intermediate_rep) {
  IRNode *jump_if_zero = arena_alloc(intermediate_rep->node_arena);
  jump_if_zero->type = IR_INSTRUCTION_JUMP_IF_ZERO;
  jump_if_zero->data.instruction_jump_if_zero.condition = condition;
  jump_if_zero->data.instruction_jump_if_zero.target = label;

  add_instruction_to_function(function, jump_if_zero);

  return jump_if_zero;
}

static IRNode* emit_jump_if_not_zero(char *label, IRNode *condition, IRNode *function, IntermediateRep *intermediate_rep) {
  IRNode *jmp_if_not_zero = arena_alloc(intermediate_rep->node_arena);
  jmp_if_not_zero->type = IR_INSTRUCTION_JUMP_IF_NOT_ZERO;
  jmp_if_not_zero->data.instruction_jump_if_not_zero.condition = condition;
  jmp_if_not_zero->data.instruction_jump_if_not_zero.target = label;

  add_instruction_to_function(function, jmp_if_not_zero);
  return jmp_if_not_zero;
}

static IRNode* emit_label(char *label, IRNode *function, IntermediateRep *intermediate_rep) {
  IRNode *label_instruction = arena_alloc(intermediate_rep->node_arena);
  label_instruction->type = IR_INSTRUCTION_LABEL;
  label_instruction->data.instruction_label.identifier = label;

  add_instruction_to_function(function, label_instruction);

  return label_instruction;
}

static IRNode* emit_copy(IRNode *source, IRNode *destination, IRNode *function, IntermediateRep *intermediate_rep) {
  IRNode *copy_instruction = arena_alloc(intermediate_rep->node_arena);
  copy_instruction->type = IR_INSTRUCTION_COPY;
  copy_instruction->data.instruction_copy.source = source;
  copy_instruction->data.instruction_copy.destination = destination;      

  add_instruction_to_function(function, copy_instruction);

  return copy_instruction;
}

static IRNode* emit_store(IRNode *source, IRNode *destination_pointer, IRNode *function, IntermediateRep *intermediate_rep) {
  IRNode *store_instruction = arena_alloc(intermediate_rep->node_arena);
  store_instruction->type = IR_INSTRUCTION_STORE;
  store_instruction->data.instruction_store.source = source;
  store_instruction->data.instruction_store.destination_pointer = destination_pointer;

  return store_instruction;
} 

static IRNode* emit_get_address(IRNode *source, IRNode *destination, IRNode *function, IntermediateRep *intermediate_rep) {
  IRNode *get_address_instruction = arena_alloc(intermediate_rep->node_arena);
  get_address_instruction->type = IR_INSTRUCTION_GET_ADDRESS;
  get_address_instruction->data.instruction_get_address.source = source;
  get_address_instruction->data.instruction_get_address.destination = destination;

  add_instruction_to_function(function, get_address_instruction);

  return get_address_instruction;
}

static IRNode* emit_truncate(IRNode *source, IRNode *destination, IRNode *function, IntermediateRep *intermediate_rep) {
  IRNode *truncate_instruction = arena_alloc(intermediate_rep->node_arena);
  truncate_instruction->type = IR_INSTRUCTION_TRUNCATE;
  truncate_instruction->data.instruction_truncate.source = source;
  truncate_instruction->data.instruction_truncate.destination = destination;      

  add_instruction_to_function(function, truncate_instruction);

  return truncate_instruction;
}

static IRNode* emit_sign_extend(IRNode *source, IRNode *destination, IRNode *function, IntermediateRep *intermediate_rep) {
  IRNode *sign_extend_instruction = arena_alloc(intermediate_rep->node_arena);
  sign_extend_instruction->type = IR_INSTRUCTION_SIGN_EXTEND;
  sign_extend_instruction->data.instruction_sign_extend.source = source;
  sign_extend_instruction->data.instruction_sign_extend.destination = destination;      

  add_instruction_to_function(function, sign_extend_instruction);

  return sign_extend_instruction;
}

static IRNode* emit_zero_extend(IRNode *source, IRNode *destination, IRNode *function, IntermediateRep *intermediate_rep) {
  IRNode *zero_extend_instruction = arena_alloc(intermediate_rep->node_arena);
  zero_extend_instruction->type = IR_INSTRUCTION_ZERO_EXTEND;
  zero_extend_instruction->data.instruction_zero_extend.source = source;
  zero_extend_instruction->data.instruction_zero_extend.destination = destination;      

  add_instruction_to_function(function, zero_extend_instruction);

  return zero_extend_instruction;
}

static IRNode* emit_double_to_int(IRNode *source, IRNode *destination, IRNode *function, IntermediateRep *intermediate_rep) {
  IRNode *double_to_int_instruction = arena_alloc(intermediate_rep->node_arena);
  double_to_int_instruction->type = IR_INSTRUCTION_DOUBLE_TO_INT;
  double_to_int_instruction->data.instruction_double_to_int.source = source;
  double_to_int_instruction->data.instruction_double_to_int.destination = destination;      

  add_instruction_to_function(function, double_to_int_instruction);

  return double_to_int_instruction;
}

static IRNode* emit_double_to_uint(IRNode *source, IRNode *destination, IRNode *function, IntermediateRep *intermediate_rep) {
  IRNode *double_to_uint_instruction = arena_alloc(intermediate_rep->node_arena);
  double_to_uint_instruction->type = IR_INSTRUCTION_DOUBLE_TO_UINT;
  double_to_uint_instruction->data.instruction_double_to_uint.source = source;
  double_to_uint_instruction->data.instruction_double_to_uint.destination = destination;      

  add_instruction_to_function(function, double_to_uint_instruction);

  return double_to_uint_instruction;
}

static IRNode* emit_int_to_double(IRNode *source, IRNode *destination, IRNode *function, IntermediateRep *interediate_rep) {
  IRNode *int_to_double_instruction = arena_alloc(interediate_rep->node_arena);
  int_to_double_instruction->type = IR_INSTRUCTION_INT_TO_DOUBLE;
  int_to_double_instruction->data.instruction_int_to_double.source = source;
  int_to_double_instruction->data.instruction_int_to_double.destination = destination;      

  add_instruction_to_function(function, int_to_double_instruction);

  return int_to_double_instruction;
}

static IRNode* emit_uint_to_double(IRNode *source, IRNode *destination, IRNode *function, IntermediateRep *intermediate_rep) {
  IRNode *uint_to_double_instruction = arena_alloc(intermediate_rep->node_arena);
  uint_to_double_instruction->type = IR_INSTRUCTION_UINT_TO_DOUBLE;
  uint_to_double_instruction->data.instruction_uint_to_double.source = source;
  uint_to_double_instruction->data.instruction_uint_to_double.destination = destination;      

  add_instruction_to_function(function, uint_to_double_instruction);

  return uint_to_double_instruction;
}

static void emit_symbol_declarations(HashTable *declaration_symbols, IRNode *ir_program,  IntermediateRep *intermediate_rep) {
  for (int i = 0; i < declaration_symbols->capacity; i++) {
    HashTableEntry *entry = &declaration_symbols->entries[i];

    if (entry == NULL || entry->key == NULL) {
      continue;
    }

    DeclarationSymbol *declaration_symbol= entry->value->structure;

    if (declaration_symbol->symbol_type != DECLARATION_SYMBOL_VARIABLE || declaration_symbol->data.variable_symbol->is_automatic_storage_duration) {
      continue;
    }
    
    IRNode *static_node = arena_alloc(intermediate_rep->node_arena);

    static_node->type = IR_VALUE_STATIC_VAR;
    static_node->data.static_variable.identifier = entry->key;
    static_node->data.static_variable.is_global = declaration_symbol->data.variable_symbol->static_is_global;
    static_node->data.static_variable.static_variable_symbol = declaration_symbol->data.variable_symbol;

    add_top_level_declaration_to_program(ir_program, static_node);    
  }
}

static void add_postfix_operations(IRNode *ir_function, IntermediateRep *intermediate_rep) {
  if (intermediate_rep->emit_status->postfix_arena.offset == 0) {
    return;
  }

  for (int i = 0; i < intermediate_rep->emit_status->postfix_arena.offset; i += intermediate_rep->emit_status->postfix_arena.base_size) {    
    AstNode *node = (AstNode*)((char *)intermediate_rep->emit_status->postfix_arena.allocation);
    emit_ast_node(node, ir_function, intermediate_rep);    
  }
}

static void add_instruction_to_function(IRNode *ir_function, IRNode *ir_instruction) {
  add_to_node_pointer(ir_instruction, ir_function->data.function.instruction_ptrs);
  ir_function->data.function.instruction_count++;
}

static void add_top_level_declaration_to_program(IRNode *ir_program, IRNode *ir_function) {
  add_to_node_pointer(ir_function, ir_program->data.program.top_level_ptrs);
  ir_program->data.program.top_level_count++;
}

static void add_argument_to_function_call(IRNode *ir_function_call_node, IRNode *argument) {
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

static char* create_temp_label(IntermediateRep *intermediate_rep) {
  char *label_name = malloc(20);
  snprintf(label_name, 10, "%d", intermediate_rep->emit_status->temp_label_id++); 

  return label_name;
}

static char* create_temp_register(IntermediateRep *intermediate_rep) {
  char *register_name = malloc(20);
  snprintf(register_name, 10, "tmp.%d", intermediate_rep->emit_status->temp_register_id++); 

  return register_name;
}

static IRNode* create_int_constant(int value, IntermediateRep *intermediate_rep) {
  IRNode *constant = arena_alloc(intermediate_rep->node_arena);
  constant->type = IR_VALUE_CONSTANT;
  constant->data.value_constant.value.int_value = value;
  // constant->data.value_constant.type = TYPE_INT;

  //TODO: Malloc'ing here. Find a better way to handle this
  TypeNode *int_type_node = malloc(sizeof(TypeNode));
  int_type_node->type = TYPE_INT;

  constant->data.value_constant.type = int_type_node;

  return constant;
}

static ExpressionResult* create_ast_constant(AstNode *ast_constant, IntermediateRep *intermediate_rep) {
  IRNode *constant = arena_alloc(intermediate_rep->node_arena);
  constant->type = IR_VALUE_CONSTANT;
  constant->data.value_constant.type = ast_constant->data.expression_constant.expression_type;

  switch (ast_constant->data.expression_constant.expression_type->type) {
    case TYPE_INT:
      constant->data.value_constant.value.int_value = ast_constant->data.expression_constant.int_value;
      break;
    case TYPE_UINT:
      constant->data.value_constant.value.uint_value = ast_constant->data.expression_constant.uint_value;
      break;      
    case TYPE_LONG:
      constant->data.value_constant.value.long_value = ast_constant->data.expression_constant.long_value;
      break;
    case TYPE_ULONG:
      constant->data.value_constant.value.ulong_value = ast_constant->data.expression_constant.ulong_value;
      break;
    case TYPE_DOUBLE:
      constant->data.value_constant.value.double_value = ast_constant->data.expression_constant.double_value;
      break;
    default:
      fprintf(stderr, "ERROR - IR: Attempted to create an unsupported Constant type (%d)\n", ast_constant->data.expression_constant.expression_type->type);
      exit(1);
  }

  ExpressionResult *constant_result = create_expression_result(constant, EXPRESSION_RESULT_PLAIN_OPERAND, intermediate_rep);
  return constant_result;
}

static ExpressionResult* create_variable(char *identifier, IntermediateRep *intermediate_rep) {
  IRNode *variable = arena_alloc(intermediate_rep->node_arena);
  variable->type = IR_VALUE_VAR;
  variable->data.value_var.identifier = identifier;

  ExpressionResult *variable_result = create_expression_result(variable, EXPRESSION_RESULT_PLAIN_OPERAND, intermediate_rep);
  return variable_result;
}

static char* create_concat_identifier(char *string, int integer) {
  //TODO: Hardcoded value
  char *identifier = malloc(64);
  snprintf(identifier, 64, "%s.%d", string, integer);

  return identifier;
}

static ExpressionResult* create_expression_result(IRNode *operand, ExpressionResultType result_type, IntermediateRep *intermediate_rep) {
  ExpressionResult *result_node = arena_alloc(intermediate_rep->node_arena);
  result_node->type = result_type;
  result_node->operand_value = operand;

  return result_node;
}

static void add_to_node_pointer(IRNode *ir_node, IRNodePointer *ir_node_pointer) {
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

static void init_node_pointer(IRNodePointer *ir_node_pointer) {
  if (ir_node_pointer == NULL) {
    return;
  }
  
  ir_node_pointer->capacity = 0;
  ir_node_pointer->count = 0;
  ir_node_pointer->node_pointers = NULL;
}

static TypeNode* get_node_type(IRNode *node, IntermediateRep *intermediate_rep) {
  switch (node->type) {
    case IR_VALUE_CONSTANT:   return node->data.value_constant.type; break;
    case IR_VALUE_STATIC_VAR: return node->data.static_variable.static_variable_symbol->value_type;
    case IR_VALUE_VAR: {
      //TODO: Error check to make sure we actually have a variable symbol.
      //TODO: It's odd that static variables have a variable symbol within the struct but not ir_value_var's. Look into this.
      HashTableEntry *entry = hash_table_get_entry(intermediate_rep->declaration_symbol_table->symbol_table, node->data.value_var.identifier);
      DeclarationSymbol *declaration_symbol = entry->value->structure;

      return declaration_symbol->data.variable_symbol->value_type;
    }
    default:
      fprintf(stderr, "ERROR - IR: Unsupported node type '%d' for get_node_type\n", node->type);
      exit(1);
  }
}

static void add_function_parameter_identifier(char *identifier, IRNode *function_node) {  
  if (function_node->data.function.parameter_count == function_node->data.function.parameter_identifier_capacity) {
    int size = function_node->data.function.parameter_identifier_capacity == 0 ? FUNCTION_IDENTIFIER_INIT_CAPACITY : function_node->data.function.parameter_identifier_capacity * 2;
    function_node->data.function.parameter_identifier_capacity = size;
    function_node->data.function.parameter_identifiers = realloc(function_node->data.function.parameter_identifiers, size * sizeof(char*));
  }

  function_node->data.function.parameter_identifiers[function_node->data.function.parameter_count] = identifier;
  function_node->data.function.parameter_count++;
}
