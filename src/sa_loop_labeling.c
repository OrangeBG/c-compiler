#include <stdlib.h>
#include "../include/sa_loop_labeling.h"
#include "../include/stack.h"

static void sa_label_loop(AstNode *ast_node, Stack *label_stack, int *current_loop_id); 

void sa_loop_labeling(AstNode *ast_nodes) {
  Stack loop_label_stack;
  stack_init(&loop_label_stack, 128);

  int starting_loop_id = 100;
  sa_label_loop(ast_nodes, &loop_label_stack, &starting_loop_id);
}

static void sa_label_loop(AstNode *ast_node, Stack *label_stack, int *current_loop_id) {
  switch (ast_node->type) {
    case AST_STATEMENT_WHILE: {
      StackValue loop_stack_value = {
        .type = STACK_INT,
        .data.integer = *current_loop_id++
      };
      stack_push(label_stack, &loop_stack_value);

      ast_node->data.statement_while.label_id = loop_stack_value.data.integer;

      sa_label_loop(ast_node->data.statement_while.condition, label_stack, current_loop_id);
      sa_label_loop(ast_node->data.statement_while.statement_body, label_stack, current_loop_id);

      stack_pop(label_stack);
    }
    break;
    case AST_STATEMENT_FOR: {
      StackValue loop_stack_value = {
        .type = STACK_INT,
        .data.integer = *current_loop_id++
      };
      stack_push(label_stack, &loop_stack_value);

      ast_node->data.statement_for.label_id = loop_stack_value.data.integer;

      if (ast_node->data.statement_for.for_loop_init != NULL) {
        sa_label_loop(ast_node->data.statement_for.for_loop_init, label_stack, current_loop_id);
      }

      if (ast_node->data.statement_for.condition_expression != NULL) {
        sa_label_loop(ast_node->data.statement_for.condition_expression, label_stack, current_loop_id);
      }

      if (ast_node->data.statement_for.statement_body != NULL) {
        sa_label_loop(ast_node->data.statement_for.statement_body, label_stack, current_loop_id);
      }

      if (ast_node->data.statement_for.post_expression != NULL) {
        sa_label_loop(ast_node->data.statement_for.post_expression, label_stack, current_loop_id);
      }

      stack_pop(label_stack);
    }
    break;
    case AST_STATEMENT_DO_WHILE: {
        StackValue loop_stack_value = {
          .type = STACK_INT,
          .data.integer = *current_loop_id++
        };
        stack_push(label_stack, &loop_stack_value);

        ast_node->data.statement_do_while.label_id = loop_stack_value.data.integer;

        sa_label_loop(ast_node->data.statement_do_while.condition, label_stack, current_loop_id);
        sa_label_loop(ast_node->data.statement_do_while.statement_body, label_stack, current_loop_id);

        stack_pop(label_stack);
      }
      break;
    case AST_STATEMENT_BREAK: {
        StackValue *current_loop_label = stack_top(label_stack);

        if (current_loop_label == NULL) {
          fprintf(stderr, "ERROR: SA LOOP LABELING - Null loop label value for 'break'\n");
          exit(1);
        }

        ast_node->data.statement_break.label_id = current_loop_label->data.integer;
      }
      break;    
    case AST_STATEMENT_CONTINUE: {
        StackValue *current_loop_label = stack_top(label_stack);

        if (current_loop_label == NULL) {
          fprintf(stderr, "ERROR: SA LOOP LABELING - Null loop label value for 'continue'\n");
          exit(1);
        }

        ast_node->data.statement_continue.label_id = current_loop_label->data.integer;
      }
      break;    
    case AST_PROGRAM:
      for (int i = 0; i < ast_node->data.program.declaration_count; i++) {
        AstNode *declaration_node = ast_node->data.program.declaration_ptrs->node_pointers[i];
        sa_label_loop(declaration_node, label_stack, current_loop_id);
      }
      break;
    case AST_FUNCTION_DECLARATION:
      if (ast_node->data.declaration_function.body_block == NULL) {
        break;
      }
      
      sa_label_loop(ast_node->data.declaration_function.body_block, label_stack, current_loop_id);
      break;
    case AST_BLOCK: {
      for (int i = 0; i < ast_node->data.block.block_count; i++) {
        AstNode *block_item_node = ast_node->data.block.block_ptrs->node_pointers[i];
        sa_label_loop(block_item_node, label_stack, current_loop_id);
      }   
      break;
    }
    case AST_VARIABLE_DECLARATION:
      if (!ast_node->data.declaration_variable.has_expression) return;
      sa_label_loop(ast_node->data.declaration_variable.init_expression, label_stack, current_loop_id);
      break;
    case AST_STATEMENT_RETURN: sa_label_loop(ast_node->data.statement_return.expression, label_stack, current_loop_id); break;
    case AST_STATEMENT_IF:
      sa_label_loop(ast_node->data.statement_if.condition_expression, label_stack, current_loop_id);
      sa_label_loop(ast_node->data.statement_if.then_statement, label_stack, current_loop_id);

      if (ast_node->data.statement_if.else_statement == NULL) return; 

      sa_label_loop(ast_node->data.statement_if.else_statement, label_stack, current_loop_id);      
      break;
    case AST_EXPRESSION_POSTFIX_INCREMENT:
    case AST_EXPRESSION_POSTFIX_DECREMENT:
    case AST_EXPRESSION_PREFIX_INCREMENT:
    case AST_EXPRESSION_PREFIX_DECREMENT:
      sa_label_loop(ast_node->data.expression_increment_decrement.expression, label_stack, current_loop_id);
      break;
    case AST_EXPRESSION_CONDITIONAL:
      sa_label_loop(ast_node->data.expression_conditional.condition, label_stack, current_loop_id);
      sa_label_loop(ast_node->data.expression_conditional.true_expression, label_stack, current_loop_id);
      sa_label_loop(ast_node->data.expression_conditional.false_expression, label_stack, current_loop_id);
      break;
    case AST_EXPRESSION_UNARY:
      sa_label_loop(ast_node->data.expression_unary.expression, label_stack, current_loop_id);
      break;
    case AST_EXPRESSION_BINARY:
      sa_label_loop(ast_node->data.expression_binary.left_expression, label_stack, current_loop_id);
      sa_label_loop(ast_node->data.expression_binary.right_expression, label_stack, current_loop_id);
      break;
    case AST_EXPRESSION_ASSIGNMENT: 
      sa_label_loop(ast_node->data.expression_assignment.left_expression, label_stack, current_loop_id);
      sa_label_loop(ast_node->data.expression_assignment.right_expression, label_stack, current_loop_id);
      break;
    case AST_STATEMENT_COMPOUND:
      sa_label_loop(ast_node->data.statement_compound.block, label_stack, current_loop_id);
      break;
    case AST_STATEMENT_GOTO: 
    case AST_STATEMENT_GOTO_LABEL: 
    case AST_STATEMENT_NULL:
    case AST_EXPRESSION_CONSTANT:
    case AST_EXPRESSION_VARIABLE:
      break;
  }
}
