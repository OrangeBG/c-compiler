#include <stdlib.h>
#include "../include/sa_loop_labeling.h"
#include "../include/stack.h"

void sa_label_loop(AstNode *ast_node, Stack *label_stack, int *current_loop_id); 

void sa_loop_labeling(AstNode *ast_nodes) {
  Stack loop_label_stack;
  stack_init(&loop_label_stack, 128);

  int starting_loop_id = 100;
  sa_label_loop(ast_nodes, &loop_label_stack, &starting_loop_id);
}

void sa_label_loop(AstNode *ast_node, Stack *label_stack, int *current_loop_id) {
  switch (ast_node->type) {
    case AST_STATEMENT_WHILE: {
      StackValue loop_stack_value = {
        .type = STACK_INT,
        .data.integer = *current_loop_id++
      };
      stack_push(label_stack, &loop_stack_value);

      ast_node->data.while_statement.label_id = loop_stack_value.data.integer;

      sa_label_loop(ast_node->data.while_statement.condition, label_stack, current_loop_id);
      sa_label_loop(ast_node->data.while_statement.statement_body, label_stack, current_loop_id);

      stack_pop(label_stack);
    }
    break;
    case AST_STATEMENT_FOR: {
      StackValue loop_stack_value = {
        .type = STACK_INT,
        .data.integer = *current_loop_id++
      };
      stack_push(label_stack, &loop_stack_value);

      ast_node->data.for_statement.label_id = loop_stack_value.data.integer;

      if (ast_node->data.for_statement.for_loop_init != NULL) {
        sa_label_loop(ast_node->data.for_statement.for_loop_init, label_stack, current_loop_id);
      }

      if (ast_node->data.for_statement.condition_expression != NULL) {
        sa_label_loop(ast_node->data.for_statement.condition_expression, label_stack, current_loop_id);
      }

      if (ast_node->data.for_statement.statement_body != NULL) {
        sa_label_loop(ast_node->data.for_statement.statement_body, label_stack, current_loop_id);
      }

      if (ast_node->data.for_statement.post_expression != NULL) {
        sa_label_loop(ast_node->data.for_statement.post_expression, label_stack, current_loop_id);
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

        ast_node->data.do_while_statement.label_id = loop_stack_value.data.integer;

        sa_label_loop(ast_node->data.do_while_statement.condition, label_stack, current_loop_id);
        sa_label_loop(ast_node->data.do_while_statement.statement_body, label_stack, current_loop_id);

        stack_pop(label_stack);
      }
      break;
    case AST_STATEMENT_BREAK: {
        StackValue *current_loop_label = stack_top(label_stack);

        if (current_loop_label == NULL) {
          fprintf(stderr, "ERROR: SA LOOP LABELING - Null loop label value for 'break'\n");
          exit(1);
        }

        ast_node->data.break_statement.label_id = current_loop_label->data.integer;
      }
      break;    
    case AST_STATEMENT_CONTINUE: {
        StackValue *current_loop_label = stack_top(label_stack);

        if (current_loop_label == NULL) {
          fprintf(stderr, "ERROR: SA LOOP LABELING - Null loop label value for 'continue'\n");
          exit(1);
        }

        ast_node->data.continue_statement.label_id = current_loop_label->data.integer;
      }
      break;    
    case AST_PROGRAM:
      for (int i = 0; i < ast_node->data.program.declaration_count; i++) {
        AstNode *declaration_node = ast_node->data.program.declaration_ptrs->node_pointers[i];
        sa_label_loop(declaration_node, label_stack, current_loop_id);
      }
      break;
    case AST_FUNCTION_DECLARATION:
      if (ast_node->data.function_declaration.body_block == NULL) {
        break;
      }
      
      sa_label_loop(ast_node->data.function_declaration.body_block, label_stack, current_loop_id);
      break;
    case AST_BLOCK: {
      for (int i = 0; i < ast_node->data.block.block_count; i++) {
        AstNode *block_item_node = ast_node->data.block.block_ptrs->node_pointers[i];
        sa_label_loop(block_item_node, label_stack, current_loop_id);
      }   
      break;
    }
    case AST_VARIABLE_DECLARATION:
      if (!ast_node->data.variable_declaration.has_expression) return;
      sa_label_loop(ast_node->data.variable_declaration.init_expression, label_stack, current_loop_id);
      break;
    case AST_STATEMENT_RETURN: sa_label_loop(ast_node->data.return_statement.expression, label_stack, current_loop_id); break;
    case AST_STATEMENT_IF:
      sa_label_loop(ast_node->data.if_statement.condition_expression, label_stack, current_loop_id);
      sa_label_loop(ast_node->data.if_statement.then_statement, label_stack, current_loop_id);

      if (ast_node->data.if_statement.else_statement == NULL) return; 

      sa_label_loop(ast_node->data.if_statement.else_statement, label_stack, current_loop_id);      
      break;
    case AST_EXPRESSION_POSTFIX_INCREMENT:
    case AST_EXPRESSION_POSTFIX_DECREMENT:
    case AST_EXPRESSION_PREFIX_INCREMENT:
    case AST_EXPRESSION_PREFIX_DECREMENT:
      sa_label_loop(ast_node->data.increment_decrement_expression.expression, label_stack, current_loop_id);
      break;
    case AST_EXPRESSION_CONDITIONAL:
      sa_label_loop(ast_node->data.conditional_expression.condition, label_stack, current_loop_id);
      sa_label_loop(ast_node->data.conditional_expression.true_expression, label_stack, current_loop_id);
      sa_label_loop(ast_node->data.conditional_expression.false_expression, label_stack, current_loop_id);
      break;
    case AST_EXPRESSION_UNARY:
      sa_label_loop(ast_node->data.unary_expression.expression, label_stack, current_loop_id);
      break;
    case AST_EXPRESSION_BINARY:
      sa_label_loop(ast_node->data.binary_expression.left_expression, label_stack, current_loop_id);
      sa_label_loop(ast_node->data.binary_expression.right_expression, label_stack, current_loop_id);
      break;
    case AST_EXPRESSION_ASSIGNMENT: 
      sa_label_loop(ast_node->data.assignement_expression.left_expression, label_stack, current_loop_id);
      sa_label_loop(ast_node->data.assignement_expression.right_expression, label_stack, current_loop_id);
      break;
    case AST_STATEMENT_COMPOUND:
      sa_label_loop(ast_node->data.compound_statement.block, label_stack, current_loop_id);
      break;
    case AST_STATEMENT_GOTO: 
    case AST_STATEMENT_GOTO_LABEL: 
    case AST_STATEMENT_NULL:
    case AST_EXPRESSION_CONSTANT:
    case AST_EXPRESSION_VARIABLE:
      break;
  }
}
