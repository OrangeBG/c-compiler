#ifndef ASSEMBLY
#define ASSEMBLY

#include "../include/parser.h"

typedef struct AsmNode AsmNode;

typedef enum {
  ASM_PROGRAM,
  ASM_FUNCTION,
  ASM_INSTRUCTION_MOV,
  ASM_INSTRUCTION_RET,
  ASM_OPERAND_IMM,
  ASM_OPERAND_REGISTER
} AsmNodeType;

typedef struct AsmNode {
  AsmNodeType type;
  union {
    struct AsmProgram { struct AsmNode *function; } program;
    struct AsmFunction { char* name; int instruction_count; int instruction_capacity; AsmNode *instructions; } function;
    struct AsmInstructionMov { AsmNode *source; AsmNode *destination; } instruction_mov;
    struct AsmOperandImmediate { int value; } operand_imm;
    struct AsmOperandRegister { char *register_name; } operand_register;
  } data;
} AsmNode;

AsmNode *generate_assembly(AstNode *ast_nodes);
void print_assembly(AsmNode *asm_node);

#endif
