#include <stdlib.h>
#include <stdbool.h>
#include "../include/assembly.h"

AsmNode* asm_program(AstProgram *ast_node);
AsmNode* asm_function(AstNode *ast_function); 

void generate_assembly(AstNode *ast_nodes) {
  AsmNode *asm_nodes = asm_program(ast_nodes->ast_program);
}

AsmNode* asm_program(AstProgram *ast_node) {
  AsmNode *function = malloc(sizeof(AsmFunction)); 

  function = asm_function(ast_node->function);

  return function;
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
        src_immediate_value->constant = ast_function_node->ast_function->statement->ast_return->return_node->ast_constant->value->integer;
        
        src_operand->type = ASM_OPERAND_IMMEDIATE_VALUE;
        src_operand->immediate_value = src_immediate_value; 

        asm_mov->source = src_operand;
        asm_mov->destination = dest_operand;

        AsmInstruction *asm_ret_instruction_base = malloc(sizeof(AsmInstruction));     
        AsmNode *asm_node_ret = malloc(sizeof(AsmNode));

        asm_ret_instruction_base->type = ASM_INSTRUCTION_RETURN;        
        asm_node_ret->asm_instruction = asm_ret_instruction_base;

        AsmNode *instructions[] = {asm_node_mov, asm_node_ret};

        function->instruction_count = 2;        
        function->instructions[0] = instructions[0];
        function->instructions[1] = instructions[1];

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
  



