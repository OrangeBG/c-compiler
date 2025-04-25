#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "../include/assembly.h"

#define INSTRUCTION_CAPACITY 8

AsmNode* asm_program(AstNode *ast_node);
AsmNode* asm_function(AstNode *ast_function); 
void asm_add_statement_instructions(AstNode *ast_statement, AsmNode *asm_function); 
void check_function_instruction_size(AsmNode *asm_function); 

AsmNode* generate_assembly(AstNode *ast_nodes) {  
  AsmNode *program = malloc(sizeof(AsmNode));

  program->type = ASM_PROGRAM;
  program->data.program.function = asm_function(ast_nodes->data.program.function); 

  return program;
}

AsmNode* asm_function(AstNode *ast_function) {
  AsmNode *function = malloc(sizeof(AsmNode));
  function->type = ASM_FUNCTION;
  function->data.function.name = ast_function->data.function.name;

  AsmNode *instructions = malloc(sizeof(AsmNode));
  
  function->data.function.instruction_count = 0;
  function->data.function.instruction_capacity = 0;
  function->data.function.instructions = instructions;

  asm_add_statement_instructions(ast_function->data.function.statement, function);

  return function;
}

void asm_add_statement_instructions(AstNode *ast_statement, AsmNode *asm_function) {
  switch (ast_statement->type) {
    case STMT_RETURN: {
        AsmNode *ret_node = malloc(sizeof(AsmNode));
        ret_node->type = ASM_INSTRUCTION_RET;

        check_function_instruction_size(asm_function);
        
        AsmNode *mov_node = malloc(sizeof(AsmNode));
        mov_node->type = ASM_INSTRUCTION_MOV;

        check_function_instruction_size(asm_function);        

        AsmNode *source_node = malloc(sizeof(AsmNode));
        source_node->type = ASM_OPERAND_IMM;
        source_node->data.operand_imm.value = ast_statement->data.return_stmt.expression->data.constant.value;

        AsmNode *destination_node = malloc(sizeof(AsmNode));
        destination_node->type = ASM_OPERAND_REGISTER;
        destination_node->data.operand_register.register_name = "eax";

        mov_node->data.instruction_mov.source = source_node;
        mov_node->data.instruction_mov.destination = destination_node;

        asm_function->data.function.instructions[asm_function->data.function.instruction_count] = *mov_node;
        asm_function->data.function.instruction_count++;
        asm_function->data.function.instructions[asm_function->data.function.instruction_count] = *ret_node;
        asm_function->data.function.instruction_count ++;
      }      
      break;
    default:
      break;
  }

}

void check_function_instruction_size(AsmNode *asm_function) {
  int current_count = asm_function->data.function.instruction_count;
  int current_capacity = asm_function->data.function.instruction_capacity;

  if (current_count == current_capacity) {
    int new_size = current_capacity == 0 ? INSTRUCTION_CAPACITY : current_capacity * INSTRUCTION_CAPACITY;

    AsmNode *instructions = realloc(asm_function->data.function.instructions, new_size);

    asm_function->data.function.instruction_capacity = new_size;
    asm_function->data.function.instructions = instructions;
  } 
} 

void print_assembly(AsmNode *node) {
  switch(node->type) {    
    case ASM_PROGRAM:
      printf("Program \n");
      print_assembly(node->data.program.function);
      break;
    case ASM_FUNCTION:
      printf("Function: %s\n", node->data.function.name);
      printf("Inst Count: %d\n", node->data.function.instruction_count);

      for (int i = 0; i < node->data.function.instruction_count; i++) {
        switch(node->data.function.instructions[i].type) {
          case ASM_INSTRUCTION_MOV:
            printf("Source: %d\n",node->data.function.instructions[i].data.instruction_mov.source->data.operand_imm.value);
            printf("Destination: %s\n", node->data.function.instructions[i].data.instruction_mov.destination->data.operand_register.register_name);
            break;
          case ASM_INSTRUCTION_RET:
            printf("Return\n");
            break;
          default:        
            fprintf(stderr, "ERROR - Assembler: No print debug option for '%d' asm instruction type\n", node->data.function.instructions[i].type);
            break;
        } 
      }      
      break;
    default:
      fprintf(stderr, "ERROR - Assembler: No print debug option for '%d' asm node type\n", node->type);
      break;
  }
}
