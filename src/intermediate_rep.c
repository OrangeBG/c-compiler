#include <stdio.h>
#include <stdlib.h>
#include "../include/intermediate_rep.h"

#define INSTRUCTION_CAPACITY 8

IRNode* ir_function(AstNode *ast_function);
IRNode* ir_value(AstNode *ast_statement, IRNode *ir_function, int temp_identifier_id); 
void check_ir_function_instruction_size(IRNode *asm_function);

IRNode* generate_intermediate_rep(AstNode *ast_node) {
  IRNode *program = malloc(sizeof(IRNode));

  program->type = IR_PROGRAM;
  program->data.program.function = ir_function(ast_node->data.program.function); 

  return program;
}

IRNode* ir_function(AstNode *ast_function) {
  IRNode *function = malloc(sizeof(IRNode));
  function->type = IR_FUNCTION;
  function->data.function.identifier = ast_function->data.function.name;

  if (ast_function->data.function.statement->type == STMT_RETURN) {
    IRNode *value = ir_value(ast_function->data.function.statement->data.return_stmt.expression, function, 0);
  } else {
    fprintf(stderr, "ERROR - IR: Unsupported statement in ir_function");
  } 

  return function;
}

IRNode* ir_value(AstNode *ast_expression, IRNode *ir_function, int temp_identifier_id) {
  switch (ast_expression->type) {
    case EXPR_CONSTANT: {
        IRNode *constant = malloc(sizeof(IRNode));
        constant->type = IR_VALUE_CONSTANT;
        constant->data.value_constant.value = ast_expression->data.constant.value;

        return constant;
      }
      break;
    case EXPR_UNARY: {
        IRNode *source = ir_value(ast_expression->data.unary.expression, ir_function, temp_identifier_id);

        //TODO: Warning, setting hard buffer limit
        char destination_name[10];
        snprintf(destination_name, 10, "tmp.%d", temp_identifier_id); 
        
        IRNode *destination = malloc(sizeof(IRNode));
        destination->type = IR_VALUE_VAR;

        IRUnaryOpType unary_op_type;

        if (ast_expression->data.unary.op_type == COMPLEMENT) {
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

    IRNode *instructions = realloc(asm_function->data.function.instructions, new_size);

    asm_function->data.function.instruction_capacity = new_size;
    asm_function->data.function.instructions = instructions;
  } 
} 
