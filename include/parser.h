#ifndef PARSER
#define PARSER

#include "../include/lexer.h"
#include <stdbool.h>

typedef struct AstNode AstNode;

typedef enum {
  AST_PROGRAM,
  // AST_DECLARATION,
  AST_VARIABLE_DECLARATION,
  AST_FUNCTION_DECLARATION,
  AST_FUNCTION_PARAMETER,
  AST_BLOCK,
  AST_STATEMENT_RETURN,
  AST_STATEMENT_EXPRESSION,
  AST_STATEMENT_NULL,
  AST_STATEMENT_IF,
  AST_STATEMENT_GOTO,
  AST_STATEMENT_GOTO_LABEL,
  AST_STATEMENT_BREAK,
  AST_STATEMENT_CONTINUE,
  AST_STATEMENT_WHILE,
  AST_STATEMENT_DO_WHILE,
  AST_STATEMENT_FOR,
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
  AST_EXPRESSION_FUNCTION_CALL
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
  AST_DECLARATION_FUNCTION,
  AST_DECLARATION_VARIABLE
} DeclarationType;

typedef enum {
  AST_PARAMETER_VOID,
  AST_PARAMETER_INT
} ParameterType;

typedef struct AstNode {
  NodeType type;
  union {
    struct Program { AstNode *function_declarations; int function_count; int function_capacity; } program;
    // struct Function { char *name; AstNode *block;} function;
    // struct Declaration { char *identifier; bool has_expression; AstNode *expression; } declaration;
    // struct Declaration { DeclarationType type; AstNode *declaration; } declaration;
    struct FunctionDeclaration { char *name; AstNode *parameters; int parameter_count; int parameter_capacity; AstNode *body_block; } function_declaration;
    struct FunctionParameter { char *name; ParameterType type; } function_parameters;
    struct VariableDeclaration { char *name; bool has_expression; AstNode *init_expression; } variable_declaration;
    struct Block { AstNode *block_items; int block_count; int block_capacity; } block;
    struct ReturnStatement { AstNode *expression; } return_statement;
    struct ExpressionStatement { AstNode *expression; } expression_statement;
    struct IfStatement { AstNode *condition_expression; AstNode *then_statement; AstNode *else_statement;  } if_statement;
    //TODO: Investigate if needed
    // struct CompoundStatement { AstNode *block; } compound_statement;
    struct GotoStatement { char *label; } goto_statement;
    struct GotoLabelStatement { char *label; } goto_label_statement;
    struct WhileStatement { AstNode *condition; AstNode *statement_body; int label_id; } while_statement;
    struct DoWhileStatement { AstNode *statement_body; AstNode *condition; int label_id; } do_while_statement;
    struct ForStatement { AstNode *for_loop_init; AstNode *condition_expression; AstNode *post_expression; AstNode *statement_body; int label_id; } for_statement;
    struct BreakStatement { int label_id; } break_statement;
    struct ContinueStatement { int label_id; } continue_statement;
    struct ConstantExpression { int value; } constant_expression;
    struct VariableExpression { char *identifier; } variable_expression;
    struct UnaryExpression { UnaryOpType op_type; AstNode *expression; } unary_expression;
    struct BinaryExpression { BinaryOpType op_type; AstNode *left_expression; AstNode *right_expression; } binary_expression;
    struct AssignmentExpression { AstNode *left_expression; AstNode *right_expression; } assignement_expression;
    struct IncrementDecrementExpression { AstNode *expression; } increment_decrement_expression;
    struct ConditionalExpression { AstNode *condition; AstNode *true_expression; AstNode *false_expression; } conditional_expression;
    struct FunctionCallExpression { char *identfier; AstNode *arguments; int argument_count; int argument_capacity; } function_call_expression;
  } data;
} AstNode;

AstNode* parse_ast(Token *tokens, int token_count, char *file);   
void print_ast(AstNode *node, int level);

#endif
