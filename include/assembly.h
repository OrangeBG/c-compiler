#ifndef ASSEMBLY
#define ASSEMBLY

#include "../include/parser.h"

typedef struct AsmNode AsmNode;

typedef enum {
  ASM_PROGRAM,
  ASM_FUNCTION,
  ASM_INSTRUCTION,
  ASM_OPERAND
} AsmNodeType;

typedef struct {
  AsmNode *function;
} AsmProgram;

typedef struct {
  char* name;
  int instruction_count;
  //TODO: On OSX, I need to specify the array count or it fails. This does not happen on windows.
  AsmNode *instructions[2];
} AsmFunction;

typedef enum {
  ASM_INSTRUCTION_MOV,
  ASM_INSTRUCTION_RETURN
} AsmInstructionType;

typedef enum {
  ASM_OPERAND_IMMEDIATE_VALUE,
  ASM_OPERAND_REGISTER
} AsmOperandType;

typedef struct {
  int constant;
} AsmOperandImmediateValue;

typedef struct {
  AsmOperandType type;
  union {
    AsmOperandImmediateValue *immediate_value;
  };
} AsmInstructionOperand;

typedef struct {
  AsmInstructionOperand *source;
  AsmInstructionOperand *destination;
} AsmInstructionMov;

typedef struct {
  AsmInstructionType type;
  union {
    AsmInstructionMov *instruction_mov;
  };
  
} AsmInstruction;

typedef struct AsmNode {
  AsmNodeType type;
  union {
    AsmProgram *asm_program;
    AsmFunction *asm_function;
    AsmInstruction *asm_instruction;
  };
} AsmNode;

AsmNode *generate_assembly(AstNode *ast_nodes);
void print_assembly(AsmNode *asm_node);

#endif
