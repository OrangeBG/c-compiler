#ifndef ASSEMBLY
#define ASSEMBLY

#include "../include/intermediate_rep.h"

typedef struct AsmNode AsmNode;

typedef enum {
  ASM_PROGRAM,
  ASM_FUNCTION,
  ASM_INSTRUCTION_MOV,
  ASM_INSTRUCTION_RET,
  ASM_INSTRUCTION_UNARY,
  ASM_INSTRUCTION_BINARY,
  ASM_INSTRUCTION_ALLOCATE_STACK,
  ASM_INSTRUCTION_IDIV,
  ASM_INSTRUCTION_CDQ,
  ASM_INSTRUCTION_CMP,
  ASM_INSTRUCTION_JMP,
  ASM_INSTRUCTION_JMPCC,
  ASM_INSTRUCTION_SETCC,
  ASM_INSTRUCTION_LABEL,
  ASM_OPERAND_IMM,
  ASM_OPERAND_REGISTER,
  ASM_OPERAND_PSEUDO_REGISTER,
  ASM_OPERAND_STACK
} AsmNodeType;

typedef enum {
  ASM_UNARY_NEG,
  ASM_UNARY_NOT
} AsmUnaryOpType;

typedef enum {
  ASM_BINARY_ADD,
  ASM_BINARY_SUB,
  ASM_BINARY_MULT,
  ASM_BINARY_BITWISE_AND,
  ASM_BINARY_BITWISE_OR,
  ASM_BINARY_BITWISE_XOR,
  ASM_BINARY_BITWISE_LEFT_SHIFT,
  ASM_BINARY_BITWISE_RIGHT_SHIFT
} AsmBinaryOpType;

typedef enum {
  ASM_REGISTER_AX,
  ASM_REGISTER_DX,
  ASM_REGISTER_R10,
  ASM_REGISTER_R11
} AsmRegisterType;

typedef enum {
  E,
  NE,
  G,
  GE,
  L,
  LE
} AsmConditionCode;

typedef struct AsmNode {
  AsmNodeType type;
  union {
    struct AsmProgram { struct AsmNode *function; } program;
    struct AsmFunction { char* name; int instruction_count; int instruction_capacity; AsmNode *instructions; } function;
    struct AsmInstructionMov { AsmNode *source; AsmNode *destination; } instruction_mov;
    struct AsmInstructionUnary { AsmUnaryOpType operator; AsmNode *operand;  } instruction_unary;
    struct AsmInstructionBinary { AsmBinaryOpType operator; AsmNode *operand_1; AsmNode *operand_2;  } instruction_binary;
    struct AsmInstructionIdiv { AsmNode *operand; } instruction_idiv;
    struct AsmInstructionCmp { AsmNode *operand_1; AsmNode *operand_2; } instruction_cmp;
    struct AsmInstructionJmp { char *identifier; } instruction_jmp;
    struct AsmInstructionJmpCC { AsmConditionCode condition_code; char *identifier; } instruction_jmp_cc;
    struct AsmInstructionSetCC { AsmConditionCode condition_code; AsmNode *operand; } instruction_set_cc;
    struct AsmInstructionLabel { char *identifier; } instruction_label;
    struct AsmInstructionAllocateStack { int bytes_to_subtract;  } instruction_allocate_stack;
    struct AsmOperandImmediate { int value; } operand_imm;
    struct AsmOperandRegister { AsmRegisterType op_register; } operand_register;
    struct AsmOperandPseudoRegister { char *identifier;  } operand_pseudo_register;
    struct AsmOperandStack { int address;  } operand_stack;
  } data;
} AsmNode;

AsmNode *generate_assembly(IRNode *ir_nodes);
void print_assembly(AsmNode *asm_node);

#endif
