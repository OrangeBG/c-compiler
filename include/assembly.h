#ifndef ASSEMBLY
#define ASSEMBLY

#include "../include/intermediate_rep.h"
#include "../include/declaration_symbol.h"
#include "arena.h"
#include "hash_table.h"

typedef struct AsmNode AsmNode;

typedef enum {
  ASM_PROGRAM,
  ASM_FUNCTION,
  ASM_STATIC_VARIABLE,
  ASM_STATIC_CONSTANT,
  ASM_INSTRUCTION_MOV,
  ASM_INSTRUCTION_MOVSX,
  ASM_INSTRUCTION_MOV_ZERO_EXTEND,
  ASM_INSTRUCTION_CVTTSD2SI,
  ASM_INSTRUCTION_CVTSI2SD,
  ASM_INSTRUCTION_RET,
  ASM_INSTRUCTION_UNARY,
  ASM_INSTRUCTION_BINARY,
  ASM_INSTRUCTION_IDIV,
  ASM_INSTRUCTION_DIV,
  ASM_INSTRUCTION_CDQ,
  ASM_INSTRUCTION_CMP,
  ASM_INSTRUCTION_JMP,
  ASM_INSTRUCTION_JMPCC,
  ASM_INSTRUCTION_SETCC,
  ASM_INSTRUCTION_LABEL,
  ASM_INSTRUCTION_PUSH,
  ASM_INSTRUCTION_CALL,
  ASM_OPERAND_IMM,
  ASM_OPERAND_REGISTER,
  ASM_OPERAND_PSEUDO_REGISTER,
  ASM_OPERAND_STACK,
  ASM_OPERAND_DATA
} AsmNodeType;

typedef enum {
  ASM_UNARY_NEG,
  ASM_UNARY_NOT,
  ASM_UNARY_SHR
} AsmUnaryOpType;

typedef enum {
  ASM_BINARY_ADD,
  ASM_BINARY_SUB,
  ASM_BINARY_MULT,
  ASM_BINARY_DIV_DOUBLE,
  ASM_BINARY_BITWISE_AND,
  ASM_BINARY_BITWISE_OR,
  ASM_BINARY_BITWISE_XOR,
  ASM_BINARY_BITWISE_LEFT_SHIFT,
  ASM_BINARY_BITWISE_RIGHT_SHIFT
} AsmBinaryOpType;

typedef enum {
  ASM_REGISTER_AX,
  ASM_REGISTER_CX,
  ASM_REGISTER_DX,
  ASM_REGISTER_DI,
  ASM_REGISTER_SI,
  ASM_REGISTER_R8,
  ASM_REGISTER_R9,
  ASM_REGISTER_R10,
  ASM_REGISTER_R11,
  ASM_REGISTER_SP,
  ASM_REGISTER_XMM0,
  ASM_REGISTER_XMM1,
  ASM_REGISTER_XMM2,
  ASM_REGISTER_XMM3,
  ASM_REGISTER_XMM4,
  ASM_REGISTER_XMM5,
  ASM_REGISTER_XMM6,
  ASM_REGISTER_XMM7,
  ASM_REGISTER_XMM14,    
  ASM_REGISTER_XMM15
} AsmRegisterType;

typedef enum {
  ASM_CONDITION_EQUAL,
  ASM_CONDITION_NOT_EQUAL,
  ASM_CONDITION_GREATER,
  ASM_CONDITION_GREATER_EQUAL,
  ASM_CONDITION_LESS,
  ASM_CONDITION_LESS_EQUAL,
  ASM_CONDITION_ABOVE,
  ASM_CONDITION_ABOVE_EQUAL,
  ASM_CONDITION_BELOW,
  ASM_CONDITION_BELOW_EQUAL
} AsmConditionCode;

typedef enum {
  ASM_TYPE_LONGWORD,
  ASM_TYPE_QUADWORD,
  ASM_TYPE_DOUBLE
} AsmType;

typedef struct {
  int capacity;
  int count;
  AsmNode **asm_pointers;
} AsmNodePointers;

typedef enum {
  ASM_SYMBOL_OBJECT_ENTRY,
  ASM_SYMBOL_FUNCTION_ENTRY
} AsmBackendSymbolType;

typedef struct {
  AsmBackendSymbolType type;
  union {
    struct ObjectEntry { AsmType assembly_type; bool is_static; bool is_constant; } object_entry;
    struct FunctionEntry { bool is_defined; } function_entry;
  } data;
} AsmBackendSymbol;

typedef struct {
  HashTable *symbol_table;
  Arena *symbol_arena;
} AsmBackendSymbolTable;

typedef struct AsmNode {
  AsmNodeType type;
  union {
    struct AsmProgram { AsmNodePointers *top_level_pointers; AsmNodePointers *static_constant_pointers; } program;
    struct AsmFunction { char* name; bool is_global; AsmNodePointers *instruction_pointers; int instruction_count; } function;
    struct AsmStaticVariable { char *identifier; bool is_global; int alignment; VariableSymbol *static_variable_symbol; } static_variable;
    struct AsmStaticConstant { char *identifier; int alignment; VariableSymbol *static_init; } static_constant;
    struct AsmInstructionMov { AsmType assembly_type; AsmNode *source; AsmNode *destination; } instruction_mov;
    struct AsmInstructionMovsx { AsmNode *source; AsmNode *destination; } instruction_movsx;
    struct AsmInstructionMovZeroExtend { AsmNode *source; AsmNode *destination; } instruction_mov_zero_extend;
    struct AsmInstructionCvttsd2si { AsmType destination_assembly_type; AsmNode *source_operand; AsmNode *destination_operand; } instruction_cvttsd2si; 
    struct AsmInstructionCvtsi2sd { AsmType source_assembly_type; AsmNode *source_operand; AsmNode *destination_operand; } instruction_cvtsi2sd; 
    struct AsmInstructionUnary { AsmType assembly_type; AsmUnaryOpType unary_op; AsmNode *operand; } instruction_unary;
    struct AsmInstructionBinary { AsmType assembly_type; AsmBinaryOpType binary_op; AsmNode *operand_1; AsmNode *operand_2; } instruction_binary;
    struct AsmInstructionIdiv { AsmType assembly_type; AsmNode *operand; } instruction_idiv;
    struct AsmInstructionDiv { AsmType assembly_type; AsmNode *operand; } instruction_div;
    struct AsmInstructionCmp { AsmType assembly_type; AsmNode *operand_1; AsmNode *operand_2; } instruction_cmp;
    struct AsmInstructionCdq { AsmType assembly_type; } instruction_cdq;
    struct AsmInstructionJmp { char *identifier; } instruction_jmp;
    struct AsmInstructionJmpCC { AsmConditionCode condition_code; char *identifier; } instruction_jmp_cc;
    struct AsmInstructionSetCC { AsmConditionCode condition_code; AsmNode *operand; } instruction_set_cc;
    struct AsmInstructionLabel { char *identifier; } instruction_label;
    struct AsmInstructionPush { AsmNode *operand; } instruction_push;
    struct AsmInstructionCall { char *identifier; } instruction_call;
    //TODO: Assigning long to immediate value to support long constants. May need to rework this when long long is supported. Read top of pg 266
    struct AsmOperandImmediate { long value; } operand_imm;
    struct AsmOperandRegister { AsmRegisterType op_register; } operand_register;
    struct AsmOperandPseudoRegister { char *identifier; } operand_pseudo_register;
    struct AsmOperandStack { int address; } operand_stack;
    struct AsmOperandData { char *identifier; } operand_data;
  } data;
} AsmNode;

AsmNode *generate_assembly(IRNode *ir_nodes, DeclarationSymbolTable *declaration_symbol_table, AsmBackendSymbolTable *backend_symbol_table);
void print_assembly(AsmNode *asm_node);
void backend_symbol_table_init(AsmBackendSymbolTable *backend_symbol_table);
void backend_symbol_table_free(AsmBackendSymbolTable *backend_symbol_table);
void backend_symbol_table_print(AsmBackendSymbolTable *backend_symbol_table);

#endif
