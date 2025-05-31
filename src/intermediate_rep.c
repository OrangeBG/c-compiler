#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "../include/intermediate_rep.h"
#include "../include/arena.h"

#define INSTRUCTION_CAPACITY 8

void    check_ir_function_instruction_size(IRNode *asm_function);
void    ir_add_postfix_operations(IRNode *asm_function, Arena *ast_postfix_arena);
IRNode* ir_function(AstNode *ast_function);
IRNode* ir_value(AstNode *ast_expression, IRNode *ir_function, int temp_identifier_id, Arena *postfix_arena); 

IRNode* generate_intermediate_rep(AstNode *ast_node) {
  IRNode *program = malloc(sizeof(IRNode));

  program->type = IR_PROGRAM;
  program->data.program.function = ir_function(ast_node->data.program.function); 

  return program;
}

void print_intermediate_ret(IRNode *ir_node) {
  switch (ir_node->type) {
    case IR_PROGRAM:
      printf("Program \n");
      print_intermediate_ret(ir_node->data.program.function);
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

  Arena postfix_arena;

  //@WARNING: Hardcoded postfix arena size
  arena_init(&postfix_arena, sizeof(AstNode), sizeof(AstNode) * 50);

  for (int i = 0; i < ast_function->data.function.block_count; i++) {
    arena_reset(&postfix_arena);
    
    AstNode *block_item = &ast_function->data.function.blocks[i];

    //@Debt: Figure out a better way to store/handle postfix operand(s)
    // char *postfix;

    if (block_item->type == AST_DECLARATION) {
      if (!block_item->data.declaration.has_expression) {
        continue;
      }

      //TODO: Probably should rename to something like generate ir
      ir_value(block_item->data.declaration.expression, function, 0, &postfix_arena);    
      ir_add_postfix_operations(function, &postfix_arena);
      
      continue;
    }

    //If not a declaration, then it's a statement
    if (block_item->type == AST_STATEMENT_NULL) {
      continue;
    }

    if (block_item->type == AST_STATEMENT_RETURN) {
      IRNode *value = ir_value(block_item->data.return_statement.expression, function, 0, &postfix_arena);
      IRNode *return_instruction = malloc(sizeof(IRNode));

      return_instruction->type = IR_INSTRUCTION_RET;
      return_instruction->data.instruction_ret.value = value;

      check_ir_function_instruction_size(function);

      function->data.function.instructions[function->data.function.instruction_count] = *return_instruction; 
      function->data.function.instruction_count++;
      //TODO: Resolve postfix
      continue;
    }

    ir_value(block_item, function, 0, &postfix_arena);

    //TODO: Resolve postfix

    arena_free(&postfix_arena);
  }    

  //@Temporary: Add return statement to every function that returns 0. If there is a return statement already for the function, this won't run.
  IRNode *zero_value = malloc(sizeof(IRNode));
  zero_value->type = IR_VALUE_CONSTANT;
  zero_value->data.value_constant.value = 0;

  IRNode *return_instruction = malloc(sizeof(IRNode));
  return_instruction->type = IR_INSTRUCTION_RET;
  return_instruction->data.instruction_ret.value = zero_value;

  check_ir_function_instruction_size(function);

  function->data.function.instructions[function->data.function.instruction_count] = *return_instruction;
  function->data.function.instruction_count++;

  return function;
}

IRNode* ir_value(AstNode *ast_expression, IRNode *ir_function, int temp_identifier_id, Arena *postfix_arena) {
  //TODO: Testing static variables to maintain static storage duration
  static int temp_number;
  static int temp_label;

  switch (ast_expression->type) {
    case AST_EXPRESSION_VARIABLE: {
      IRNode *variable = malloc(sizeof(IRNode));
      variable->type = IR_VALUE_VAR;
      variable->data.value_var.identifier = ast_expression->data.variable_expression.identifier;
      return variable;
    }
    case AST_EXPRESSION_ASSIGNMENT: {
      IRNode *result = ir_value(ast_expression->data.assignement_expression.right_expression, ir_function, temp_identifier_id, postfix_arena);

      IRNode *copy_instruction = malloc(sizeof(IRNode));
      copy_instruction->type = IR_INSTRUCTION_COPY;
      copy_instruction->data.instruction_copy.source = result;

      IRNode *variable = malloc(sizeof(IRNode));
      variable->type = IR_VALUE_VAR;
      variable->data.value_var.identifier = ast_expression->data.assignement_expression.left_expression->data.variable_expression.identifier;

      copy_instruction->data.instruction_copy.destination = variable;      

      check_ir_function_instruction_size(ir_function);

      ir_function->data.function.instructions[ir_function->data.function.instruction_count] = *copy_instruction; 
      ir_function->data.function.instruction_count++;
      
      return result;
    }
    case AST_EXPRESSION_CONSTANT: {
      IRNode *constant = malloc(sizeof(IRNode));
      constant->type = IR_VALUE_CONSTANT;
      constant->data.value_constant.value = ast_expression->data.constant_expression.value;

      return constant;
    }
    break;
    case AST_EXPRESSION_POSTFIX_INCREMENT: {
      // *postfix_variable = ast_expression->data.postfix_expression.expression->data.variable_expression.identifier;
      AstNode *postfix_node = arena_alloc(postfix_arena);
      *postfix_node = *ast_expression->data.postfix_expression.expression;

      IRNode *variable = malloc(sizeof(IRNode));
      variable->type = IR_VALUE_VAR;
      // variable->data.value_var.identifier = ast_expression->data.variable_expression.identifier;
      variable->data.value_var.identifier = ast_expression->data.postfix_expression.expression->data.assignement_expression.left_expression->data.variable_expression.identifier;
      return variable;
    }
    case AST_EXPRESSION_UNARY: {
        IRNode *source = ir_value(ast_expression->data.unary_expression.expression, ir_function, temp_identifier_id, postfix_arena);

        //TODO: Warning, setting hard buffer limit
        char *destination_name = malloc(10);
        snprintf(destination_name, 10, "tmp.%d", temp_number++); 
        
        IRNode *destination = malloc(sizeof(IRNode));
        destination->type = IR_VALUE_VAR;
        destination->data.value_var.identifier = destination_name;

        IRUnaryOpType unary_op_type;

        switch (ast_expression->data.unary_expression.op_type) {
          case AST_UNARY_COMPLEMENT: unary_op_type = IR_UNARY_COMPLEMENT; break;
          case AST_UNARY_NEGATE:     unary_op_type = IR_UNARY_NEGATE; break;
          case AST_UNARY_NOT:        unary_op_type = IR_UNARY_NOT; break;
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
    case AST_EXPRESSION_BINARY: {      
        IRNode *source_1 = ir_value(ast_expression->data.binary_expression.left_expression, ir_function, temp_identifier_id, postfix_arena);
        IRNode *source_2 = ir_value(ast_expression->data.binary_expression.right_expression, ir_function, temp_identifier_id, postfix_arena);

        //TODO: Warning, setting hard buffer limit
        char *destination_name = malloc(10);
        snprintf(destination_name, 10, "tmp.%d", temp_number++); 
        
        IRNode *destination = malloc(sizeof(IRNode));
        destination->type = IR_VALUE_VAR;
        destination->data.value_var.identifier = destination_name;

        if (ast_expression->data.binary_expression.op_type == AST_BINARY_AND || ast_expression->data.binary_expression.op_type == AST_BINARY_OR) {
          char *label_name = malloc(10);
          snprintf(label_name, 10, "%d", temp_label++); 

          IRNode *jmp_instruction_v1 = malloc(sizeof(IRNode));

          if (ast_expression->data.binary_expression.op_type == AST_BINARY_AND) { 
            jmp_instruction_v1->type = IR_INSTRUCTION_JUMP_IF_ZERO;  
            jmp_instruction_v1->data.instruction_jump_if_zero.condition = source_1;
            jmp_instruction_v1->data.instruction_jump_if_zero.target = label_name;
          } else {
            jmp_instruction_v1->type = IR_INSTRUCTION_JUMP_IF_NOT_ZERO;  
            jmp_instruction_v1->data.instruction_jump_if_not_zero.condition = source_1;
            jmp_instruction_v1->data.instruction_jump_if_not_zero.target = label_name;
          }

          check_ir_function_instruction_size(ir_function);

          ir_function->data.function.instructions[ir_function->data.function.instruction_count] = *jmp_instruction_v1; 
          ir_function->data.function.instruction_count++;

          IRNode *jmp_instruction_v2 = malloc(sizeof(IRNode));

          if (ast_expression->data.binary_expression.op_type == AST_BINARY_AND) { 
            jmp_instruction_v2->type = IR_INSTRUCTION_JUMP_IF_ZERO;  
            jmp_instruction_v2->data.instruction_jump_if_zero.condition = source_2;
            jmp_instruction_v2->data.instruction_jump_if_zero.target = label_name;
          } else {
            jmp_instruction_v2->type = IR_INSTRUCTION_JUMP_IF_NOT_ZERO;  
            jmp_instruction_v2->data.instruction_jump_if_not_zero.condition = source_2;
            jmp_instruction_v2->data.instruction_jump_if_not_zero.target = label_name;
          }

          check_ir_function_instruction_size(ir_function);

          ir_function->data.function.instructions[ir_function->data.function.instruction_count] = *jmp_instruction_v2; 
          ir_function->data.function.instruction_count++;

          IRNode *result_1 = malloc(sizeof(IRNode));
          result_1->type = IR_VALUE_CONSTANT;
          result_1->data.value_constant.value = 1;

          IRNode *copy_1 = malloc(sizeof(IRNode));
          copy_1->type = IR_INSTRUCTION_COPY;
          copy_1->data.instruction_copy.destination = destination;
          copy_1->data.instruction_copy.source = result_1;
          
          check_ir_function_instruction_size(ir_function);

          ir_function->data.function.instructions[ir_function->data.function.instruction_count] = *copy_1; 
          ir_function->data.function.instruction_count++;

          IRNode *jmp_instruction = malloc(sizeof(IRNode));
          jmp_instruction->type = IR_INSTRUCTION_JUMP;
          jmp_instruction->data.instruction_jump.target = "end";

          check_ir_function_instruction_size(ir_function);

          ir_function->data.function.instructions[ir_function->data.function.instruction_count] = *jmp_instruction; 
          ir_function->data.function.instruction_count++;

          IRNode *label = malloc(sizeof(IRNode));
          label->type = IR_INSTRUCTION_LABEL;
          label->data.instruction_label.identifier = label_name;

          check_ir_function_instruction_size(ir_function);

          ir_function->data.function.instructions[ir_function->data.function.instruction_count] = *label; 
          ir_function->data.function.instruction_count++;

          IRNode *result_0 = malloc(sizeof(IRNode));
          result_0->type = IR_VALUE_CONSTANT;
          result_0->data.value_constant.value = 0;

          IRNode *copy_2 = malloc(sizeof(IRNode));
          copy_2->type = IR_INSTRUCTION_COPY;
          copy_2->data.instruction_copy.destination = destination;
          copy_2->data.instruction_copy.source = result_0;
          
          check_ir_function_instruction_size(ir_function);

          ir_function->data.function.instructions[ir_function->data.function.instruction_count] = *copy_2; 
          ir_function->data.function.instruction_count++;

          IRNode *label_end = malloc(sizeof(IRNode));
          label_end->type = IR_INSTRUCTION_LABEL;
          label_end->data.instruction_label.identifier = "end";

          check_ir_function_instruction_size(ir_function);

          ir_function->data.function.instructions[ir_function->data.function.instruction_count] = *label_end; 
          ir_function->data.function.instruction_count++;

          return destination;
        }

        IRBinaryOpType binary_op_type;

        switch (ast_expression->data.binary_expression.op_type) {
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

        check_ir_function_instruction_size(ir_function);

        ir_function->data.function.instructions[ir_function->data.function.instruction_count] = *binary_instruction; 
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

void ir_add_postfix_operations(IRNode *ir_function, Arena *ast_postfix_arena) {
  if (ast_postfix_arena->offset == 0) {
    return;
  }

  for (int i = 0; i < ast_postfix_arena->offset; i += ast_postfix_arena->base_size) {    
    AstNode *node = (AstNode*)((char *)ast_postfix_arena->allocation);
    ir_value(node, ir_function, 0, ast_postfix_arena);    
  }
}
