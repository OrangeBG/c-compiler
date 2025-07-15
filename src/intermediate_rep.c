#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "../include/intermediate_rep.h"
#include "../include/arena.h"

#define INSTRUCTION_CAPACITY 8
#define FUNCTION_CAPACITY 8
#define FUNCTION_CALL_CAPACITY 8
#define BREAK_LABEL "break"
#define CONTINUE_LABEL "continue"
#define START_LABEL "start"
#define END_LABEL "end"

typedef struct {
  Arena postfix_arena;
  int temp_register_id;
  int temp_label_id;
} IREmitStatus;

void    check_ir_function_instruction_size(IRNode *ir_function);
void    ir_add_postfix_operations(IRNode *ir_function, IREmitStatus *emit_status);
IRNode* ir_function(AstNode *ast_function, IREmitStatus *emit_status);
IRNode* ir_emit_ast_node(AstNode *node, IRNode *function, IREmitStatus *emit_status); 
IRNode* ir_emit_return(AstNode *block_item, IRNode *function, IREmitStatus *emit_status);
void    ir_emit_if(AstNode *block_item, IRNode *function, IREmitStatus *emit_status); 
void    ir_emit_goto(AstNode *goto_node, IRNode *function); 
void    ir_emit_goto_label(AstNode *goto_label_node, IRNode *function); 
void    ir_emit_while(AstNode *while_node, IRNode *function, IREmitStatus *emit_status); 
void    ir_emit_do_while(AstNode *do_node, IRNode *function, IREmitStatus *emit_status); 
void    ir_emit_for(AstNode *for_node, IRNode *function, IREmitStatus *emit_status); 
void    ir_emit_continue(int label_id, IRNode *function);
void    ir_emit_break(int label_id, IRNode *function);
void    ir_emit_block(AstNode *block_node, IRNode *function, IREmitStatus *emit_status); 
IRNode* ir_emit_jump(char *label, IRNode *function);
IRNode* ir_emit_jump_if_zero(char *label, IRNode *condition, IRNode *function); 
IRNode* ir_emit_jump_if_not_zero(char *label, IRNode *condition, IRNode *function); 
IRNode* ir_emit_label(char* label, IRNode *function);
IRNode* ir_emit_copy(IRNode *source, IRNode *destination, IRNode *function); 
IRNode* ir_emit_declaration(AstNode *declaration_node, IRNode *function, IREmitStatus *emit_status); 
IRNode* ir_emit_conditional_expression(AstNode *condition_node, IRNode *function, IREmitStatus *emit_status);
IRNode* ir_emit_conditional_expression(AstNode *condition_node, IRNode *function, IREmitStatus *emit_status);
IRNode* ir_emit_postfix_expression(AstNode *postfix_node, IREmitStatus *emit_status);
IRNode* ir_emit_unary_expression(AstNode *unary_node, IRNode *function, IREmitStatus *emit_status);
IRNode* ir_emit_binary_expression(AstNode *binary_node, IRNode *function, IREmitStatus *emit_status);
IRNode* ir_emit_assignment_expression(AstNode *assignment_node, IRNode *function, IREmitStatus *emit_status);
IRNode* ir_emit_function_call_expression(AstNode *function_call_node, IRNode *function, IREmitStatus *emit_status); 
void    ir_add_instruction_to_function(IRNode *ir_function, IRNode *ir_instruction); 
void    ir_add_function_to_program(IRNode *ir_program, IRNode *ir_function);
void    ir_add_argument_to_function_call(IRNode *ir_function_call_node, IRNode *argument);
char*   ir_create_temp_label(IREmitStatus *emit_status); 
char*   ir_create_temp_register(IREmitStatus *emit_status); 
char*   ir_create_concat_identifier(char *string, int integer); 
IRNode* ir_create_constant(int value);
IRNode* ir_create_variable(char *identifier);

IRNode* generate_intermediate_rep(AstNode *ast_node) {
  IRNode *program = malloc(sizeof(IRNode));

  program->type = IR_PROGRAM;
  program->data.program.function_capacity = 0;
  program->data.program.function_count = 0;

  IREmitStatus emit_status = {
    .temp_register_id = 0,
    .temp_label_id = 0
  };

  for (int i = 0; i < ast_node->data.program.function_count; i++) {
    AstNode *function_node = ast_node->data.program.function_ptrs->node_pointers[i];
    //We only need to process function definitions, not function declarations
    if (function_node->data.function_declaration.body_block == NULL) {
      continue;
    }
    
    IRNode *function = ir_function(function_node, &emit_status);
    ir_add_function_to_program(program, function);
  }

  return program;
}

void print_intermediate_ret(IRNode *ir_node) {
  switch (ir_node->type) {
    case IR_PROGRAM:
      printf("Program \n");

      for (int i = 0; i < ir_node->data.program.function_count; i++) {
        print_intermediate_ret(&ir_node->data.program.functions[i]);
      }

      printf("\n");
      break;
    case IR_FUNCTION: {
        struct IRFunction *function = &ir_node->data.function; 
        printf("Function -> %s\n", function->identifier);

        for (int i = 0; i < function->instruction_count; i++) {
          if (function->instructions[i].type == IR_INSTRUCTION_RET) {
            printf("Return(");
            print_intermediate_ret(function->instructions[i].data.instruction_ret.value);
            printf(")\n");
          } else if (function->instructions[i].type == IR_INSTRUCTION_UNARY) {      
            struct IRInstructionUnary* unary = &function->instructions[i].data.unary;

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
          } else if (function->instructions[i].type == IR_INSTRUCTION_BINARY) {
            struct IRInstructionBinary* binary = &function->instructions[i].data.instruction_binary;

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
          } else if (function->instructions[i].type == IR_INSTRUCTION_JUMP_IF_ZERO) {
            printf("Jump If Zero(");
            print_intermediate_ret(function->instructions[i].data.instruction_jump_if_zero.condition);
            printf(", %s)\n", function->instructions[i].data.instruction_jump_if_zero.target);
          } else if (function->instructions[i].type == IR_INSTRUCTION_JUMP_IF_NOT_ZERO) {
            printf("Jump If Not Zero(");
            print_intermediate_ret(function->instructions[i].data.instruction_jump_if_not_zero.condition);
            printf(" , %s)\n", function->instructions[i].data.instruction_jump_if_not_zero.target);
          } else if (function->instructions[i].type == IR_INSTRUCTION_JUMP) {
            printf("Jump(%s)\n", function->instructions[i].data.instruction_jump.target);
          } else if (function->instructions[i].type == IR_INSTRUCTION_COPY) {
            printf("Copy(Source(");
            print_intermediate_ret(function->instructions[i].data.instruction_copy.source);
            printf(") (Destination(");
            print_intermediate_ret(function->instructions[i].data.instruction_copy.destination);
            printf(")\n");
          } else if (function->instructions[i].type == IR_INSTRUCTION_LABEL) {
            printf("Label(%s)\n", function->instructions[i].data.instruction_label.identifier);
          }
        }
      }
      break;
    case IR_VALUE_CONSTANT:
      printf("Constant(%d)", ir_node->data.value_constant.value);
      break;
    case IR_VALUE_VAR:
      printf("Var(\"%s\")", ir_node->data.value_var.identifier);
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

IRNode* ir_function(AstNode *ast_function, IREmitStatus *emit_status) {
  IRNode *function = malloc(sizeof(IRNode));
  IRNode *instructions = malloc(sizeof(IRNode));
  function->type = IR_FUNCTION;
  function->data.function.identifier = ast_function->data.function_declaration.name;
  function->data.function.instruction_count = 0;
  function->data.function.instruction_capacity = 0;
  function->data.function.instructions = instructions;

  Arena postfix_arena;
  //@WARNING: Hardcoded postfix arena size
  //TODO: May be better to initialize outside of this function and instead reset the allocated arena
  arena_init(&postfix_arena, sizeof(AstNode), sizeof(AstNode) * 50, true);
  emit_status->postfix_arena = postfix_arena;

  ir_emit_ast_node(ast_function->data.function_declaration.body_block, function, emit_status);

  //@Temporary: Add return statement to every function that returns 0. If there is a return statement already for the function, this won't run.
  IRNode *zero_value = ir_create_constant(0);
  IRNode *return_instruction = malloc(sizeof(IRNode));
  return_instruction->type = IR_INSTRUCTION_RET;
  return_instruction->data.instruction_ret.value = zero_value;

  ir_add_instruction_to_function(function, return_instruction);

  return function;
}

IRNode* ir_emit_ast_node(AstNode *node, IRNode *function, IREmitStatus *emit_status) {
  switch (node->type) {
      case AST_BLOCK:                        { ir_emit_block(node, function, emit_status); break; }
      case AST_STATEMENT_IF:                 { ir_emit_if(node, function, emit_status); break; }
      case AST_STATEMENT_GOTO:               { ir_emit_goto(node, function); break; }
      case AST_STATEMENT_GOTO_LABEL:         { ir_emit_goto_label(node, function); break; }
      case AST_STATEMENT_WHILE:              { ir_emit_while(node, function, emit_status); break; }
      case AST_STATEMENT_DO_WHILE:           { ir_emit_do_while(node, function, emit_status); break; }
      case AST_STATEMENT_FOR:                { ir_emit_for(node, function, emit_status); break; }
      case AST_STATEMENT_CONTINUE:           { ir_emit_continue(node->data.continue_statement.label_id, function); break; }
      case AST_STATEMENT_BREAK:              { ir_emit_break(node->data.break_statement.label_id, function); break; }
      case AST_STATEMENT_NULL:               { break; } 
      case AST_STATEMENT_RETURN:             { return ir_emit_return(node, function, emit_status); }
      // case AST_DECLARATION:                  { return ir_emit_declaration(node, function, emit_status); }
      case AST_EXPRESSION_VARIABLE:          { return ir_create_variable(node->data.variable_expression.identifier); }
      case AST_EXPRESSION_CONSTANT:          { return ir_create_constant(node->data.constant_expression.value); }
      case AST_EXPRESSION_CONDITIONAL:       { return ir_emit_conditional_expression(node, function, emit_status); }
      case AST_EXPRESSION_POSTFIX_INCREMENT: { return ir_emit_postfix_expression(node, emit_status); }
      case AST_EXPRESSION_POSTFIX_DECREMENT: { return ir_emit_postfix_expression(node, emit_status); }
      case AST_EXPRESSION_PREFIX_INCREMENT:  { return ir_emit_ast_node(node, function, emit_status); }
      case AST_EXPRESSION_PREFIX_DECREMENT:  { return ir_emit_ast_node(node, function, emit_status); }
      case AST_EXPRESSION_UNARY:             { return ir_emit_unary_expression(node, function, emit_status); }
      case AST_EXPRESSION_BINARY:            { return ir_emit_binary_expression(node, function, emit_status); }
      case AST_EXPRESSION_ASSIGNMENT:        { return ir_emit_assignment_expression(node, function, emit_status); }
      case AST_EXPRESSION_FUNCTION_CALL:     { return ir_emit_function_call_expression(node, function, emit_status); }
      case AST_VARIABLE_DECLARATION:         { return ir_emit_declaration(node, function, emit_status); }
      case AST_FUNCTION_DECLARATION:         {
          if (node->data.function_declaration.body_block == NULL) break;
          return ir_function(node, emit_status);
      }
      default:
        fprintf(stderr, "ERROR - IR: ASTNode type %d not found for node emit\n", node->type);
        exit(1);
  }

  return NULL;
}

void ir_emit_block(AstNode *block_node, IRNode *function, IREmitStatus *emit_status) {
  for (int i = 0; i < block_node->data.block.block_count; i++) {
    arena_reset(&emit_status->postfix_arena);
    AstNode *block_item_node = block_node->data.block.block_ptrs->node_pointers[i];
    ir_emit_ast_node(block_item_node, function, emit_status);
    ir_add_postfix_operations(function, emit_status);
  }
}

IRNode* ir_emit_return(AstNode *block_item, IRNode *function, IREmitStatus *emit_status) {
  IRNode *value = ir_emit_ast_node(block_item->data.return_statement.expression, function, emit_status);
  IRNode *return_instruction = malloc(sizeof(IRNode));

  return_instruction->type = IR_INSTRUCTION_RET;
  return_instruction->data.instruction_ret.value = value;

  ir_add_instruction_to_function(function, return_instruction);
  ir_add_postfix_operations(function, emit_status);

  return return_instruction;
}

void ir_emit_if(AstNode *if_node, IRNode *function, IREmitStatus *emit_status) {
  IRNode *condition = ir_emit_ast_node(if_node->data.if_statement.condition_expression, function, emit_status);
  char *label_name = ir_create_temp_label(emit_status);

  ir_emit_jump_if_zero(label_name, condition, function);

  AstNode *then_statement = if_node->data.if_statement.then_statement;

  ir_emit_ast_node(then_statement, function, emit_status);
  ir_emit_label(label_name, function);
}

void ir_emit_goto(AstNode *goto_node, IRNode *function) {
  ir_emit_jump(goto_node->data.goto_label_statement.label, function);
}

void ir_emit_goto_label(AstNode *goto_label_node, IRNode *function) {
  ir_emit_label(goto_label_node->data.goto_statement.label, function);
}

void ir_emit_while(AstNode *while_node, IRNode *function, IREmitStatus *emit_status) {
  char *continue_label_identifier = ir_create_concat_identifier(CONTINUE_LABEL, while_node->data.do_while_statement.label_id); 
  char *break_label_identifier = ir_create_concat_identifier(BREAK_LABEL, while_node->data.do_while_statement.label_id); 

  ir_emit_label(continue_label_identifier, function);

  IRNode *condition = ir_emit_ast_node(while_node->data.while_statement.condition, function, emit_status);

  ir_emit_jump_if_zero(break_label_identifier, condition, function);
  ir_emit_ast_node(while_node->data.while_statement.statement_body, function, emit_status);
  ir_emit_jump(continue_label_identifier, function);
  ir_emit_label(break_label_identifier, function);
}

void ir_emit_do_while(AstNode *do_node, IRNode *function, IREmitStatus *emit_status) {
  char *start_label_identifier = ir_create_concat_identifier(START_LABEL, do_node->data.do_while_statement.label_id);
  ir_emit_label(start_label_identifier, function);

  ir_emit_ast_node(do_node->data.do_while_statement.statement_body, function, emit_status);

  char *continue_label_identifier = ir_create_concat_identifier(CONTINUE_LABEL, do_node->data.do_while_statement.label_id); 
  ir_emit_label(continue_label_identifier, function);

  IRNode *condition = ir_emit_ast_node(do_node->data.do_while_statement.condition, function, emit_status);
  ir_emit_jump_if_not_zero(start_label_identifier, condition, function);

  char *break_label_identifier = ir_create_concat_identifier(BREAK_LABEL, do_node->data.do_while_statement.label_id);
  ir_emit_label(break_label_identifier, function);
}

void ir_emit_for(AstNode *for_node, IRNode *function, IREmitStatus *emit_status) {
  if (for_node->data.for_statement.for_loop_init != NULL) {
    ir_emit_ast_node(for_node->data.for_statement.for_loop_init, function, emit_status);
  }  

  char *start_label_identifier = ir_create_concat_identifier(START_LABEL, for_node->data.for_statement.label_id);
  ir_emit_label(start_label_identifier, function);

  char *break_label_identifier = ir_create_concat_identifier(BREAK_LABEL, for_node->data.for_statement.label_id);

  if (for_node->data.for_statement.condition_expression != NULL) {
    IRNode *condition = ir_emit_ast_node(for_node->data.for_statement.condition_expression, function, emit_status);
    ir_emit_jump_if_zero(break_label_identifier, condition, function);
  }

  ir_emit_ast_node(for_node->data.for_statement.statement_body, function, emit_status);

  char *continue_label_identifier = ir_create_concat_identifier(CONTINUE_LABEL, for_node->data.for_statement.label_id);
  ir_emit_label(continue_label_identifier, function);

  if (for_node->data.for_statement.post_expression != NULL) {
    ir_emit_ast_node(for_node->data.for_statement.post_expression, function, emit_status);
  }

  ir_emit_jump(start_label_identifier, function);
  ir_emit_label(break_label_identifier, function);
}

void ir_emit_continue(int label_id, IRNode *function) {
  char *continue_label_identifier = ir_create_concat_identifier(CONTINUE_LABEL, label_id); 
  ir_emit_jump(continue_label_identifier, function);
}

void ir_emit_break(int label_id, IRNode *function) {
  char *break_label_identifier = ir_create_concat_identifier(BREAK_LABEL, label_id); 
  ir_emit_jump(break_label_identifier, function);
}

IRNode* ir_emit_declaration(AstNode *declaration_node, IRNode *function, IREmitStatus *emit_status) {
  if (!declaration_node->data.variable_declaration.has_expression) {
    return NULL;
  }

  IRNode *node = ir_emit_ast_node(declaration_node->data.variable_declaration.init_expression, function, emit_status);    
  ir_add_postfix_operations(function, emit_status);

  return node;
}

IRNode* ir_emit_conditional_expression(AstNode *conditional_node, IRNode *function, IREmitStatus *emit_status) {
  IRNode *condition = ir_emit_ast_node(conditional_node->data.conditional_expression.condition, function, emit_status);

  char *end_label_name = ir_create_temp_label(emit_status);
  char *false_label_name = ir_create_temp_label(emit_status);

  ir_emit_jump_if_zero(false_label_name, condition, function);

  IRNode *true_value = ir_emit_ast_node(conditional_node->data.conditional_expression.true_expression, function, emit_status);

  ir_emit_jump(end_label_name, function);
  ir_emit_label(false_label_name, function);

  IRNode *false_value = ir_emit_ast_node(conditional_node->data.conditional_expression.false_expression, function, emit_status);      

  ir_emit_label(end_label_name, function);

  return condition;
}

IRNode* ir_emit_postfix_expression(AstNode *postfix_node, IREmitStatus *emit_status) {
  AstNode *postfix_arena_node = arena_alloc(&emit_status->postfix_arena);
  *postfix_arena_node = *postfix_node->data.increment_decrement_expression.expression;

  IRNode *variable = malloc(sizeof(IRNode));
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

IRNode* ir_emit_unary_expression(AstNode *unary_node, IRNode *function, IREmitStatus *emit_status) {
  IRNode *source = ir_emit_ast_node(unary_node->data.unary_expression.expression, function, emit_status);

  //TODO: Warning, setting hard buffer limit
  char *destination_name = ir_create_temp_register(emit_status);

  IRNode *destination = malloc(sizeof(IRNode));
  destination->type = IR_VALUE_VAR;
  destination->data.value_var.identifier = destination_name;

  IRUnaryOpType unary_op_type;

  switch (unary_node->data.unary_expression.op_type) {
    case AST_UNARY_COMPLEMENT: unary_op_type = IR_UNARY_COMPLEMENT; break;
    case AST_UNARY_NEGATE:     unary_op_type = IR_UNARY_NEGATE; break;
    case AST_UNARY_NOT:        unary_op_type = IR_UNARY_NOT; break;
  }

  IRNode *unary_instruction = malloc(sizeof(IRNode));         
  unary_instruction->type = IR_INSTRUCTION_UNARY;
  unary_instruction->data.unary.op_type = unary_op_type;
  unary_instruction->data.unary.source = source;
  unary_instruction->data.unary.destination = destination;

  ir_add_instruction_to_function(function, unary_instruction);

  return destination;
}

IRNode* ir_emit_binary_expression(AstNode *binary_node, IRNode *function, IREmitStatus *emit_status) {
  IRNode *source_1 = ir_emit_ast_node(binary_node->data.binary_expression.left_expression, function, emit_status);
  IRNode *source_2 = ir_emit_ast_node(binary_node->data.binary_expression.right_expression, function, emit_status);

  //TODO: Warning, setting hard buffer limit
  char *destination_name = ir_create_temp_register(emit_status);

  IRNode *destination = malloc(sizeof(IRNode));
  destination->type = IR_VALUE_VAR;
  destination->data.value_var.identifier = destination_name;

  if (binary_node->data.binary_expression.op_type == AST_BINARY_AND || binary_node->data.binary_expression.op_type == AST_BINARY_OR) {
    char *label_name = ir_create_temp_label(emit_status);

    IRNode *jmp_instruction_v1 = malloc(sizeof(IRNode));

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

    IRNode *jmp_instruction_v2 = malloc(sizeof(IRNode));

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

    IRNode *result_1 = ir_create_constant(1);

    ir_emit_copy(result_1, destination, function);
    ir_emit_jump(END_LABEL, function);
    ir_emit_label(label_name, function);

    IRNode *result_0 = ir_create_constant(0);

    ir_emit_copy(result_0, destination, function);
    ir_emit_label(END_LABEL, function);

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

  IRNode *binary_instruction = malloc(sizeof(IRNode));         
  binary_instruction->type = IR_INSTRUCTION_BINARY;
  binary_instruction->data.instruction_binary.op_type = binary_op_type;
  binary_instruction->data.instruction_binary.source_1 = source_1;
  binary_instruction->data.instruction_binary.source_2 = source_2;
  binary_instruction->data.instruction_binary.destination = destination;

  ir_add_instruction_to_function(function, binary_instruction);

  return destination;
}

IRNode* ir_emit_assignment_expression(AstNode *assignment_node, IRNode *function, IREmitStatus *emit_status) {
  //TODO: Keep this for now. Need to assess why conditional expressions are handled differently when the source node is 'ast_expression_assignment'. There's already an emit_conditional(). 
  if (assignment_node->data.assignement_expression.right_expression->type == AST_EXPRESSION_CONDITIONAL) {
    IRNode *condition = ir_emit_ast_node(assignment_node->data.assignement_expression.right_expression->data.conditional_expression.condition, function, emit_status);

    char *end_label_name = ir_create_temp_label(emit_status);
    char *false_label_name = ir_create_temp_label(emit_status);

    ir_emit_jump_if_zero(false_label_name, condition, function);
    
    IRNode *true_value = ir_emit_ast_node(assignment_node->data.assignement_expression.right_expression->data.conditional_expression.true_expression, function, emit_status);
    IRNode *variable = malloc(sizeof(IRNode));
    variable->type = IR_VALUE_VAR;
    variable->data.value_var.identifier = assignment_node->data.assignement_expression.left_expression->data.variable_expression.identifier;

    ir_emit_copy(true_value, variable, function);
    ir_emit_jump(end_label_name, function);
    ir_emit_label(false_label_name, function);
  
    IRNode *false_value = ir_emit_ast_node(assignment_node->data.assignement_expression.right_expression->data.conditional_expression.false_expression, function, emit_status);

    ir_emit_copy(false_value, variable, function);
    ir_emit_label(end_label_name, function);

    return NULL;
  }
    
  IRNode *result = ir_emit_ast_node(assignment_node->data.assignement_expression.right_expression, function, emit_status);
  IRNode *variable = malloc(sizeof(IRNode));
  variable->type = IR_VALUE_VAR;

  if (assignment_node->data.assignement_expression.left_expression->type == AST_EXPRESSION_VARIABLE) {
    variable->data.value_var.identifier = assignment_node->data.assignement_expression.left_expression->data.variable_expression.identifier;
  } else if (assignment_node->data.assignement_expression.left_expression->type == AST_EXPRESSION_UNARY) {
    variable->data.value_var.identifier = assignment_node->data.assignement_expression.left_expression->data.unary_expression.expression->data.variable_expression.identifier;
  } else {
    fprintf(stderr, "ERROR - Intermediate Rep: Could not resolve variable identifier for Expression Assignment\n");
    exit(1);
  }

  ir_emit_copy(result, variable, function);
  
  return result;
}

IRNode* ir_emit_function_call_expression(AstNode *function_call_node, IRNode *function, IREmitStatus *emit_status) {
  IRNode *ir_function_call = malloc(sizeof(IRNode));
  ir_function_call->type = IR_INSTRUCTION_FUNCTION_CALL;
  ir_function_call->data.instruction_function_call.identifier = function_call_node->data.function_call_expression.identfier;
  ir_function_call->data.instruction_function_call.arg_capacity = 0;
  ir_function_call->data.instruction_function_call.arg_count = 0;
  ir_function_call->data.instruction_function_call.args = NULL;
  ir_function_call->data.instruction_function_call.destination = function;

  for (int i = 0; i < function_call_node->data.function_call_expression.argument_count; i++) {
    AstNode *argument_node = function_call_node->data.function_call_expression.argument_ptrs->node_pointers[i];

    IRNode *argument = ir_emit_ast_node(argument_node, function, emit_status);

    ir_add_argument_to_function_call(ir_function_call, argument);    
  }

  ir_add_instruction_to_function(function, ir_function_call);

  return ir_function_call;
} 

IRNode* ir_emit_jump(char *label, IRNode *function) {
  IRNode *jmp_instruction = malloc(sizeof(IRNode));
  jmp_instruction->type = IR_INSTRUCTION_JUMP;
  jmp_instruction->data.instruction_jump.target = label;

  ir_add_instruction_to_function(function, jmp_instruction);

  return jmp_instruction;
}

IRNode* ir_emit_jump_if_zero(char *label, IRNode *condition, IRNode *function) {
  IRNode *jump_if_zero = malloc(sizeof(IRNode));
  jump_if_zero->type = IR_INSTRUCTION_JUMP_IF_ZERO;
  jump_if_zero->data.instruction_jump_if_zero.condition = condition;
  jump_if_zero->data.instruction_jump_if_zero.target = label;

  ir_add_instruction_to_function(function, jump_if_zero);

  return jump_if_zero;
}

IRNode* ir_emit_jump_if_not_zero(char *label, IRNode *condition, IRNode *function) {
  IRNode *jmp_if_not_zero = malloc(sizeof(IRNode));
  jmp_if_not_zero->type = IR_INSTRUCTION_JUMP_IF_NOT_ZERO;
  jmp_if_not_zero->data.instruction_jump_if_not_zero.condition = condition;
  jmp_if_not_zero->data.instruction_jump_if_not_zero.target = label;

  ir_add_instruction_to_function(function, jmp_if_not_zero);
  return jmp_if_not_zero;
}

void check_ir_function_instruction_size(IRNode *ir_function) {
  int current_count = ir_function->data.function.instruction_count;
  int current_capacity = ir_function->data.function.instruction_capacity;

  if (current_count == current_capacity) {
    int new_size = current_capacity == 0 ? INSTRUCTION_CAPACITY : current_capacity * INSTRUCTION_CAPACITY;

    IRNode *instructions = realloc(ir_function->data.function.instructions, new_size * sizeof(IRNode));

    ir_function->data.function.instruction_capacity = new_size;
    ir_function->data.function.instructions = instructions;
  } 
} 

IRNode* ir_emit_label(char *label, IRNode *function) {
  IRNode *label_instruction = malloc(sizeof(IRNode));
  label_instruction->type = IR_INSTRUCTION_LABEL;
  label_instruction->data.instruction_label.identifier = label;

  ir_add_instruction_to_function(function, label_instruction);

  return label_instruction;
}

IRNode* ir_emit_copy(IRNode *source, IRNode *destination, IRNode *function) {
  IRNode *copy_instruction = malloc(sizeof(IRNode));
  copy_instruction->type = IR_INSTRUCTION_COPY;
  copy_instruction->data.instruction_copy.source = source;
  copy_instruction->data.instruction_copy.destination = destination;      

  ir_add_instruction_to_function(function, copy_instruction);

  return copy_instruction;
}

void ir_add_postfix_operations(IRNode *ir_function, IREmitStatus *emit_status) {
  if (emit_status->postfix_arena.offset == 0) {
    return;
  }

  for (int i = 0; i < emit_status->postfix_arena.offset; i += emit_status->postfix_arena.base_size) {    
    AstNode *node = (AstNode*)((char *)emit_status->postfix_arena.allocation);
    ir_emit_ast_node(node, ir_function, emit_status);    
  }
}

void ir_add_instruction_to_function(IRNode *ir_function, IRNode *ir_instruction) {
  check_ir_function_instruction_size(ir_function);
  ir_function->data.function.instructions[ir_function->data.function.instruction_count] = *ir_instruction; 
  ir_function->data.function.instruction_count++;
}

void ir_add_function_to_program(IRNode *ir_program, IRNode *ir_function) {
  int current_count = ir_program->data.program.function_count;
  int current_capacity = ir_program->data.program.function_capacity;
  
  if (current_count == current_capacity) {
    int new_size = current_capacity == 0 ? FUNCTION_CAPACITY : current_capacity * FUNCTION_CAPACITY;

    IRNode *functions = realloc(ir_program->data.program.functions, new_size * sizeof(IRNode));

    ir_program->data.program.function_capacity = new_size;
    ir_program->data.program.functions = functions;
  } 

  ir_program->data.program.functions[ir_program->data.program.function_count] = *ir_function;
  ir_program->data.program.function_count++;
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

IRNode* ir_create_constant(int value) {
  IRNode *constant = malloc(sizeof(IRNode));
  constant->type = IR_VALUE_CONSTANT;
  constant->data.value_constant.value = value;

  return constant;
}

IRNode* ir_create_variable(char *identifier) {
  IRNode *variable = malloc(sizeof(IRNode));
  variable->type = IR_VALUE_VAR;
  variable->data.value_var.identifier = identifier;

  return variable;
}

char* ir_create_concat_identifier(char *string, int integer) {
  char *identifier = malloc(64);
  snprintf(identifier, 64, "%s.%d", string, integer);

  return identifier;
}
