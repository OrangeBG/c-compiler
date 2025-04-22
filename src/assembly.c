#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "../include/assembly.h"

AsmNode* asm_program(AstProgram *ast_node);
AsmNode* asm_function(AstNode *ast_function); 

AsmNode* generate_assembly(AstNode *ast_nodes) {  
  AsmNode *asm_nodes = asm_program(ast_nodes->ast_program);

  return asm_nodes;
}

void print_assembly(AsmNode *node) {
  switch(node->type) {    
    case ASM_PROGRAM:
      printf("Program \n");
      print_assembly(node->asm_program->function);
      break;
    case ASM_FUNCTION:
      printf("Function: %s\n", node->asm_function->name);
      printf("Inst Count: %d\n", node->asm_function->instruction_count);

      for (int i = 0; i < node->asm_function->instruction_count; i++) {
        switch(node->asm_function->instructions[i]->asm_instruction->type) {
          case ASM_INSTRUCTION_MOV:
            printf("Source: %d\n",node->asm_function->instructions[i]->asm_instruction->instruction_mov->source->immediate_value->constant);
            printf("Destination: TBD\n");
            break;
          case ASM_INSTRUCTION_RETURN:
            printf("Return\n");
            break;
          default:        
            fprintf(stderr, "ERROR - Assembler: No print debug option for '%d' asm instruction type\n", node->asm_function->instructions[i]->asm_instruction->type);
            break;
        } 
      }      
      break;
    case ASM_INSTRUCTION: break; //Do nothing. Handled in ASM_FUNCTION
    case ASM_OPERAND: break;
    default:
      fprintf(stderr, "ERROR - Assembler: No print debug option for '%d' asm node type\n", node->type);
      break;
  }
}

AsmNode* asm_program(AstProgram *ast_node) {
  AsmNode *function = asm_function(ast_node->function);

  AsmProgram *program = malloc(sizeof(AsmProgram));
  program->function = function;

  AsmNode *program_node = malloc(sizeof(AsmNode));
  program_node->type = ASM_PROGRAM;
  program_node->asm_program = program;

  return program_node;
}

AsmNode* asm_function(AstNode *ast_function_node) {
  //TODO: Only supporting a single instruction for now. Will need to support multiple funtion instructions
  AsmFunction *function = malloc(sizeof(AsmFunction));
  function->name = ast_function_node->ast_function->name; 

  switch (ast_function_node->ast_function->statement->type) {
    case AST_RETURN: {
        AsmInstruction *asm_mov_instruction_base = malloc(sizeof(AsmInstruction));
        AsmInstructionMov *asm_mov = malloc(sizeof(AsmInstructionMov));
        AsmNode *asm_node_mov = malloc(sizeof(AsmNode));

        asm_mov_instruction_base->type = ASM_INSTRUCTION_MOV;
        asm_mov_instruction_base->instruction_mov = asm_mov;

        asm_node_mov->type = ASM_INSTRUCTION;
        asm_node_mov->asm_instruction = asm_mov_instruction_base;

        AsmInstructionOperand *src_operand = malloc(sizeof(AsmInstructionOperand));
        AsmInstructionOperand *dest_operand = malloc(sizeof(AsmInstructionOperand));

        AsmOperandImmediateValue *src_immediate_value = malloc(sizeof(AsmOperandImmediateValue));
        src_immediate_value->constant = ast_function_node->ast_function->statement->ast_return->return_node->ast_expression->constant->value->integer;
        
        src_operand->type = ASM_OPERAND_IMMEDIATE_VALUE;
        src_operand->immediate_value = src_immediate_value; 

        asm_mov->source = src_operand;
        asm_mov->destination = dest_operand;

        AsmInstruction *asm_ret_instruction_base = malloc(sizeof(AsmInstruction));     
        AsmNode *asm_node_ret = malloc(sizeof(AsmNode));

        asm_ret_instruction_base->type = ASM_INSTRUCTION_RETURN;        
        asm_node_ret->asm_instruction = asm_ret_instruction_base;

        function->instruction_count = 2;        
        function->instructions[0] = malloc(sizeof(AsmNode));
        function->instructions[1] = malloc(sizeof(AsmNode));
        
        function->instructions[0] = asm_node_mov;
        function->instructions[1] = asm_node_ret;

        break;
      }
    default:
      break;
  }  

  AsmNode *node = malloc(sizeof(AsmNode));
  node->type = ASM_FUNCTION;
  node->asm_function = function;

  return node;
}
  



