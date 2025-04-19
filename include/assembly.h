#ifndef ASSEMBLY
#define ASSEMBLY

#include "../include/parser.h"

typedef struct AsmNode AsmNode;

typedef enum AsmNodeType {
  ASM_PROGRAM,
  ASM_FUNCTION,
  ASM_INSTRUCTION,
  ASM_OPERAND
} AsmNodeType;

typedef struct AsmProgram {
  AsmNode *function;
} AsmProgram;

typedef struct AsmFunction {
  char* name;
  int instruction_count;
  AsmNode *instructions[];
} AsmFunction;

typedef enum AsmInstructionType {
  ASM_INSTRUCTION_MOV,
  ASM_INSTRUCTION_RETURN
} AsmInstructionType;

typedef enum AsmOperandType {
  ASM_OPERAND_IMMEDIATE_VALUE,
  ASM_OPERAND_REGISTER
} AsmOperandType;

typedef struct AsmOperandImmediateValue {
  int constant;
} AsmOperandImmediateValue;

typedef struct AsmInstructionOperand {
  AsmOperandType type;
  union {
    AsmOperandImmediateValue *immediate_value;
  };
} AsmInstructionOperand;

typedef struct AsmInstructionMov {
  AsmInstructionOperand *source;
  AsmInstructionOperand *destination;
} AsmInstructionMov;

typedef struct AsmInstruction {
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

void generate_assembly(AstNode *ast_nodes);

#endif
