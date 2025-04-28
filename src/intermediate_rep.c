#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "../include/intermediate_rep.h"

#define INSTRUCTION_CAPACITY 8

void    check_ir_function_instruction_size(IRNode *asm_function);
IRNode* ir_function(AstNode *ast_function);
IRNode* ir_value(AstNode *ast_statement, IRNode *ir_function, int temp_identifier_id); 

//TODO: Temporary..Replace later
int temp_number = 0;

IRNode* generate_intermediate_rep(AstNode *ast_node) {
  IRNode *program = malloc(sizeof(IRNode));

  program->type = IR_PROGRAM;
  program->data.program.function = ir_function(ast_node->data.program.function); 

  return program;
}

void print_immediate_ret(IRNode *ir_node) {
  switch (ir_node->type) {
    case IR_PROGRAM:
      printf("Program \n");
      print_immediate_ret(ir_node->data.program.function);
      break;
    case IR_FUNCTION: {
        struct IRFunction *function = &ir_node->data.function; 
        printf("Function -> %s\n", function->identifier);

        for (int i = 0; i < function->instruction_count; i++) {
          if (function->instructions[i].type == IR_INSTRUCTION_RET) {
            printf("Return(");
            print_immediate_ret(function->instructions[i].data.instruction_ret.value);
            printf(")");
          } else {      
            struct IRInstructionUnary* unary = &function->instructions[i].data.unary;

            printf("Unary(%s", unary->op_type == IR_UNARY_COMPLEMENT ? "Complement, " : "Negate, ");
            print_immediate_ret(unary->source);            
            printf(",");
            print_immediate_ret(unary->destination);
            printf(")");
            printf("\n");
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
    default: fprintf(stderr, "ERROR - IR: No print for type %d\n", ir_node->type); }
}

IRNode* ir_function(AstNode *ast_function) {
  IRNode *function = malloc(sizeof(IRNode));
  IRNode *instructions = malloc(sizeof(IRNode));
  function->type = IR_FUNCTION;
  function->data.function.identifier = ast_function->data.function.name;
  function->data.function.instruction_count = 0;
  function->data.function.instruction_capacity = 0;
  function->data.function.instructions = instructions;

  if (ast_function->data.function.statement->type == AST_STATEMENT_RETURN) {
    IRNode *value = ir_value(ast_function->data.function.statement->data.return_stmt.expression, function, 0);
    IRNode *return_instruction = malloc(sizeof(IRNode));
    return_instruction->type = IR_INSTRUCTION_RET;
    return_instruction->data.instruction_ret.value = value;

    check_ir_function_instruction_size(function);

    function->data.function.instructions[function->data.function.instruction_count] = *return_instruction; 
    function->data.function.instruction_count++;

  } else {
    fprintf(stderr, "ERROR - IR: Unsupported statement in ir_function");
  } 

  return function;
}

IRNode* ir_value(AstNode *ast_expression, IRNode *ir_function, int temp_identifier_id) {
  switch (ast_expression->type) {
    case AST_EXPRESSION_CONSTANT: {
        IRNode *constant = malloc(sizeof(IRNode));
        constant->type = IR_VALUE_CONSTANT;
        constant->data.value_constant.value = ast_expression->data.constant.value;

        return constant;
      }
      break;
    case AST_EXPRESSION_UNARY: {
        IRNode *source = ir_value(ast_expression->data.unary.expression, ir_function, temp_identifier_id);

        //TODO: Warning, setting hard buffer limit
        char *destination_name = malloc(10);
        snprintf(destination_name, 10, "tmp.%d", temp_number++); 
        
        IRNode *destination = malloc(sizeof(IRNode));
        destination->type = IR_VALUE_VAR;
        destination->data.value_var.identifier = destination_name;

        IRUnaryOpType unary_op_type;

        if (ast_expression->data.unary.op_type == AST_UNARY_COMPLEMENT) {
          unary_op_type = IR_UNARY_COMPLEMENT;
        } else {
          unary_op_type = IR_UNARY_NEGATE;
        }        

        IRNode *unary_instruction = malloc(sizeof(IRNode));         
        unary_instruction->type = IR_INSTRUCTION_UNARY;
        unary_instruction->data.unary.op_type = unary_op_type;
        unary_instruction->data.unary.source = source;
        unary_instruction->data.unary.destination = destination;

        check_ir_function_instruction_size(ir_function);

        ir_function->data.function.instructions[ir_function->data.function.instruction_count] = *unary_instruction; 
        ir_function->data.function.instruction_count++;

        return destination;
      }
      break;
    default:
      break;
  }

  fprintf(stderr, "ERROR - IR: AST expression type not supported %d", ast_expression->type);
  exit(1);    
}

void check_ir_function_instruction_size(IRNode *asm_function) {
  int current_count = asm_function->data.function.instruction_count;
  int current_capacity = asm_function->data.function.instruction_capacity;

  if (current_count == current_capacity) {
    int new_size = current_capacity == 0 ? INSTRUCTION_CAPACITY : current_capacity * INSTRUCTION_CAPACITY;

    IRNode *instructions = realloc(asm_function->data.function.instructions, new_size * sizeof(IRNode));

    asm_function->data.function.instruction_capacity = new_size;
    asm_function->data.function.instructions = instructions;
  } 
} 
