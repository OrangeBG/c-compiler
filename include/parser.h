#ifndef PARSER
#define PARSER

#include "../include/lexer.h"
#include "../include/arena.h"
#include "../include/types.h"
#include <stdbool.h>

typedef struct AstNode AstNode;

typedef enum {
  AST_PROGRAM,
  AST_VARIABLE_DECLARATION,
  AST_FUNCTION_DECLARATION,
  AST_BLOCK,
  AST_STATEMENT_RETURN,
  AST_STATEMENT_NULL,
  AST_STATEMENT_IF,
  AST_STATEMENT_GOTO,
  AST_STATEMENT_GOTO_LABEL,
  AST_STATEMENT_BREAK,
  AST_STATEMENT_CONTINUE,
  AST_STATEMENT_WHILE,
  AST_STATEMENT_DO_WHILE,
  AST_STATEMENT_FOR,
  AST_STATEMENT_COMPOUND,
  AST_EXPRESSION_CAST,
  AST_EXPRESSION_BINARY,
  AST_EXPRESSION_CONSTANT,
  AST_EXPRESSION_UNARY,
  AST_EXPRESSION_VARIABLE,
  AST_EXPRESSION_ASSIGNMENT,
  AST_EXPRESSION_CONDITIONAL,
  AST_EXPRESSION_POSTFIX_INCREMENT,
  AST_EXPRESSION_POSTFIX_DECREMENT,
  AST_EXPRESSION_PREFIX_INCREMENT,
  AST_EXPRESSION_PREFIX_DECREMENT,
  AST_EXPRESSION_FUNCTION_CALL,
  AST_EXPRESSION_DEREFERENCE,
  AST_EXPRESSION_ADDRESS_OF,
  AST_EXPRESSION_SUBSCRIPT
} NodeType;

typedef enum {
  AST_UNARY_COMPLEMENT,
  AST_UNARY_NEGATE,
  AST_UNARY_NOT,
  AST_UNARY_PREFIX_INCREMENT,
  AST_UNARY_PREFIX_DECREMENT
} UnaryOpType;

typedef enum {
  AST_BINARY_ADD,
  AST_BINARY_AND,
  AST_BINARY_OR,
  AST_BINARY_EQUAL,
  AST_BINARY_NOT_EQUAL,
  AST_BINARY_LESS_THAN,
  AST_BINARY_LESS_OR_EQUAL,
  AST_BINARY_GREATER_THAN,
  AST_BINARY_GREATER_OR_EQUAL,
  AST_BINARY_SUBTRACT,
  AST_BINARY_MULTIPLY,
  AST_BINARY_DIVIDE,
  AST_BINARY_REMAINDER,
  AST_BINARY_BITWISE_AND,
  AST_BINARY_BITWISE_OR,
  AST_BINARY_BITWISE_XOR,
  AST_BINARY_BITWISE_LEFT_SHIFT,
  AST_BINARY_BITWISE_RIGHT_SHIFT
} BinaryOpType;

typedef enum {
  AST_STORAGE_CLASS_NONE,
  AST_STORAGE_CLASS_STATIC,
  AST_STORAGE_CLASS_EXTERN
} StorageClassType;

typedef enum {
  AST_CONSTANT_TYPE_INT,
  AST_CONSTANT_TYPE_UINT,
  AST_CONSTANT_TYPE_LONG,
  AST_CONSTANT_TYPE_ULONG,
  AST_CONSTANT_TYPE_DOUBLE
} ConstantType;

typedef enum {
  AST_INITIALIZER_SINGLE,
  AST_INITIALIZER_COMPOUND
} InitializerType;

typedef struct {
  int capacity;
  int count;
  AstNode **node_pointers;
} NodePointer;

typedef struct {
 Arena *ast_node_arena;
 Arena *type_node_arena;
} ParserResults;

typedef struct AstNode {
  int line_number;
  NodeType type;
  union {
    struct Program { NodePointer *declaration_ptrs; int declaration_count; } program;
    //TODO: Seems bad to have param count and have function_type.data.type.function_param_type_count representing the same thing
    struct FunctionDeclaration { char *name; StorageClassType storage_class_type; char **parameter_identifiers; int parameter_identifier_capacity; int parameter_identifier_count; AstNode *body_block; TypeNode *function_type; } declaration_function;
    struct VariableDeclaration { char *name; TypeNode *type;  StorageClassType storage_class_type; bool has_expression; AstNode *init_expression; } declaration_variable;
    struct Initializer { InitializerType type; union { AstNode *single_init_expression; AstNode *compound_initializer; int compound_count; int compound_capacity; } initializer_node; } initializer;
    struct Block { NodePointer *block_ptrs; int block_count; } block;
    struct ReturnStatement { AstNode *expression; } statement_return;
    struct IfStatement { AstNode *condition_expression; AstNode *then_statement; AstNode *else_statement; } statement_if;
    struct CompoundStatement { AstNode *block; } statement_compound;
    struct GotoStatement { char *label; } statement_goto;
    struct GotoLabelStatement { char *label; } statement_goto_label;
    struct WhileStatement { AstNode *condition; AstNode *statement_body; int label_id; } statement_while;
    struct DoWhileStatement { AstNode *statement_body; AstNode *condition; int label_id; } statement_do_while;
    struct ForStatement { AstNode *for_loop_init; AstNode *condition_expression; AstNode *post_expression; AstNode *statement_body; int label_id; } statement_for;
    struct BreakStatement { int label_id; } statement_break;
    struct ContinueStatement { int label_id; } statement_continue;
    //TODO: Look into making the constant values into a union
    struct ConstantExpression { ConstantType constant_type; int int_value; long long_value; unsigned int uint_value; unsigned long ulong_value; TypeNode *expression_type; double double_value; } expression_constant;
    struct VariableExpression { char *identifier; TypeNode *expression_type; } expression_variable;
    struct UnaryExpression { UnaryOpType op_type; AstNode *expression; TypeNode *expression_type; } expression_unary;
    struct BinaryExpression { BinaryOpType op_type; AstNode *left_expression; AstNode *right_expression; TypeNode *expression_type; } expression_binary;
    struct AssignmentExpression { AstNode *left_expression; AstNode *right_expression; TypeNode *expression_type; } expression_assignment;
    struct IncrementDecrementExpression { AstNode *expression; TypeNode *expression_type; } expression_increment_decrement;
    struct ConditionalExpression { AstNode *condition; AstNode *true_expression; AstNode *false_expression; TypeNode *expression_type; } expression_conditional;
    struct FunctionCallExpression { char *identfier; NodePointer *argument_ptrs; TypeNode *expression_type; int argument_count; } expression_function_call;
    struct CastExpression { TypeNode *target_type; AstNode *expression; } expression_cast;
    struct DereferenceExpression { AstNode *expression; } expression_dereference;
    struct AddressOfExpression { AstNode *expression; } expression_address_of;
    struct SubscriptExpression { AstNode *expression_1; AstNode *expression_2; } expression_subscript;
  } data;
} AstNode;

void parse_ast(ParserResults *results, Token *tokens, int token_count, char *file);   
void print_ast(const AstNode *node, int whitespace);

#endif
