#include "../include/sa_loop_labeling.h"
#include "../include/stack.h"



void sa_loop_labeling(AstNode *ast_nodes) {
  Stack loop_label_stack;
  stack_init(&loop_label_stack, 128);
}

void sa_label_loop(AstNode *ast_node, Stack *label_stack) {
  switch (ast_node->type) {
    //BREAK
    //CONTINUE
    //WHILE
    //DO WHILE
    //FOR
    case AST_PROGRAM:    sa_label_loop(ast_node->data.program.function, label_stack); break;
    case AST_FUNCTION:   sa_label_loop(ast_node->data.function.block, label_stack); break;
    case AST_BLOCK: {
      for (int i = 0; i < ast_node->data.block.block_count; i++) {
        sa_label_loop(&ast_node->data.block.block_items[i], label_stack);
      }   
    }
    case AST_DECLARATION:
      if (!ast_node->data.declaration.has_expression) return;
      sa_label_loop(ast_node->data.declaration.expression, label_stack);
      break;
    case AST_STATEMENT_RETURN: sa_label_loop(ast_node->data.return_statement.expression, label_stack); break;
    case AST_STATEMENT_EXPRESSION: sa_label_loop(ast_node->data.expression_statement.expression, label_stack);
    case AST_STATEMENT_IF:
      sa_label_loop(ast_node->data.if_statement.condition_expression);
      break;
    case AST_STATEMENT_GOTO: 
    case AST_STATEMENT_GOTO_LABEL: 
    case AST_STATEMENT_NULL:
      break

  }
}
