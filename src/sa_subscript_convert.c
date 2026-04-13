#include "../include/sa_subscript_convert.h"
#include "../include/error.h"

/*
  @Debt: This is a temporary semantic pass to convert subscript expressions to dereferenced binary add expressions. For example: t[0] = 4; is converted to *(t + 0) = 4;
  It's slower to add this semantic pass rather than have something in the type checker that handles subscript expressions. Eventually, this logic should be merged with other semantic checks so that
  we aren't doing a bunch of node passes.
*/

static AstNode* convert_subscript_to_dereference_expression(AstNode *subscript_expression, ParserResults *parser_results);

void sa_subscript_convert(AstNode **ast_node, ParserResults *parser_results) {
  switch ((*ast_node)->type) {
    case AST_PROGRAM:
      for (int i = 0; i < (*ast_node)->data.program.declaration_count; i++) {
        AstNode *declaration_node = (*ast_node)->data.program.declaration_ptrs->node_pointers[i];
        sa_subscript_convert(&declaration_node, parser_results);
      }
      break;
    case AST_FUNCTION_DECLARATION:
      if ((*ast_node)->data.declaration_function.body_block == NULL) {
        break;
      }
      
      sa_subscript_convert(&(*ast_node)->data.declaration_function.body_block, parser_results);
      break;
    case AST_BLOCK: {
      for (int i = 0; i < (*ast_node)->data.block.block_count; i++) {
        AstNode *block_item_node = (*ast_node)->data.block.block_ptrs->node_pointers[i];
        sa_subscript_convert(&block_item_node, parser_results);
      }   
      break;
    }
    case AST_VARIABLE_DECLARATION:
      if (!(*ast_node)->data.declaration_variable.has_expression) {
        return;
      }
      
      sa_subscript_convert(&(*ast_node)->data.declaration_variable.init_expression, parser_results);
      break;
    case AST_INITIALIZER:
      if ((*ast_node)->data.initializer.type == AST_INITIALIZER_SINGLE) {
        sa_subscript_convert(&(*ast_node)->data.initializer.initializer_node.single_init_expression, parser_results);        
        return;
      }

      for (int i = 0; i < (*ast_node)->data.initializer.initializer_node.compound_initializer->count; i++) {
        AstNode *item_ptr = &(*ast_node)->data.initializer.initializer_node.compound_initializer->items[i];
        sa_subscript_convert(&item_ptr, parser_results);        
      }

      break;
    case AST_STATEMENT_COMPOUND:
      sa_subscript_convert(&(*ast_node)->data.statement_compound.block, parser_results);
      break;
    case AST_STATEMENT_RETURN:
      sa_subscript_convert(&(*ast_node)->data.statement_return.expression, parser_results);
      break;
    case AST_STATEMENT_IF:
      sa_subscript_convert(&(*ast_node)->data.statement_if.condition_expression, parser_results);
      sa_subscript_convert(&(*ast_node)->data.statement_if.then_statement, parser_results);
      if ((*ast_node)->data.statement_if.else_statement != NULL) {
        sa_subscript_convert(&(*ast_node)->data.statement_if.else_statement, parser_results);
      }
      break;
    case AST_STATEMENT_DO_WHILE:
      sa_subscript_convert(&(*ast_node)->data.statement_do_while.condition, parser_results);
      sa_subscript_convert(&(*ast_node)->data.statement_do_while.statement_body, parser_results);
      break;
    case AST_STATEMENT_WHILE:
      sa_subscript_convert(&(*ast_node)->data.statement_while.condition, parser_results);
      sa_subscript_convert(&(*ast_node)->data.statement_while.statement_body, parser_results);
      break;
    case AST_STATEMENT_FOR:
      sa_subscript_convert(&(*ast_node)->data.statement_for.condition_expression, parser_results);
      sa_subscript_convert(&(*ast_node)->data.statement_for.post_expression, parser_results);
      sa_subscript_convert(&(*ast_node)->data.statement_for.statement_body, parser_results);
      sa_subscript_convert(&(*ast_node)->data.statement_for.for_loop_init, parser_results);
      break;
    case AST_EXPRESSION_CAST:
      sa_subscript_convert(&(*ast_node)->data.expression_cast.expression, parser_results);
      break;
    case AST_EXPRESSION_ASSIGNMENT: 
      sa_subscript_convert(&(*ast_node)->data.expression_assignment.left_expression, parser_results);
      sa_subscript_convert(&(*ast_node)->data.expression_assignment.right_expression, parser_results);
      break;
    case AST_EXPRESSION_POSTFIX_DECREMENT:
    case AST_EXPRESSION_POSTFIX_INCREMENT:
      sa_subscript_convert(&(*ast_node)->data.expression_increment_decrement.expression, parser_results);
      break;
    case AST_EXPRESSION_BINARY:
      sa_subscript_convert(&(*ast_node)->data.expression_binary.left_expression, parser_results);
      sa_subscript_convert(&(*ast_node)->data.expression_binary.right_expression, parser_results);
      break;
    case AST_EXPRESSION_UNARY:
      sa_subscript_convert(&(*ast_node)->data.expression_unary.expression, parser_results);
      break;
    case AST_EXPRESSION_DEREFERENCE:
      sa_subscript_convert(&(*ast_node)->data.expression_dereference.expression, parser_results);
      break;
    case AST_EXPRESSION_ADDRESS_OF:
      sa_subscript_convert(&(*ast_node)->data.expression_address_of.expression, parser_results);
      break;
    case AST_EXPRESSION_CONDITIONAL:
      sa_subscript_convert(&(*ast_node)->data.expression_conditional.true_expression, parser_results);
      sa_subscript_convert(&(*ast_node)->data.expression_conditional.false_expression, parser_results);
      break;
    case AST_EXPRESSION_FUNCTION_CALL:
      for (int i = 0; i < (*ast_node)->data.expression_function_call.argument_ptrs->count; i++) {
        AstNode **item_ptr = &(*ast_node)->data.expression_function_call.argument_ptrs->node_pointers[i];
        sa_subscript_convert(item_ptr, parser_results);        
      }
      break;
    case AST_EXPRESSION_SUBSCRIPT: {            
      *ast_node = convert_subscript_to_dereference_expression(*ast_node, parser_results);
      break;
    }
    case AST_EXPRESSION_CONSTANT:
    case AST_EXPRESSION_VARIABLE:
    case AST_EXPRESSION_STRING:
    case AST_STATEMENT_NULL:
    case AST_STATEMENT_GOTO:
    case AST_STATEMENT_GOTO_LABEL:
    case AST_STATEMENT_BREAK:
    case AST_STATEMENT_CONTINUE:
      break;
    default:
      panic("Unsupported AST '%s' node", get_ast_node_string((*ast_node)));
  }
}

static AstNode* convert_subscript_to_dereference_expression(AstNode *subscript_expression, ParserResults *parser_results) {
  AstNode *binary_expression = arena_alloc(parser_results->ast_node_arena);
  binary_expression->type = AST_EXPRESSION_BINARY;
  binary_expression->data.expression_binary.op_type = AST_BINARY_ADD;
  binary_expression->line_number = subscript_expression->line_number;
  
  AstNode *dereference_expression = arena_alloc(parser_results->ast_node_arena);
  dereference_expression->type = AST_EXPRESSION_DEREFERENCE;
  dereference_expression->line_number = subscript_expression->line_number;
  dereference_expression->data.expression_dereference.expression = binary_expression;

  if (subscript_expression->data.expression_subscript.expression_1->type == AST_EXPRESSION_SUBSCRIPT) {
    AstNode* inner_deref = convert_subscript_to_dereference_expression(subscript_expression->data.expression_subscript.expression_1, parser_results);

    binary_expression->data.expression_binary.left_expression = inner_deref;
    binary_expression->data.expression_binary.right_expression = subscript_expression->data.expression_subscript.expression_2;

    return dereference_expression;

  } else if (subscript_expression->data.expression_subscript.expression_2->type == AST_EXPRESSION_SUBSCRIPT) {
    AstNode* inner_deref = convert_subscript_to_dereference_expression(subscript_expression->data.expression_subscript.expression_2, parser_results);

    binary_expression->data.expression_binary.left_expression = inner_deref;
    binary_expression->data.expression_binary.right_expression = subscript_expression->data.expression_subscript.expression_1;

    return dereference_expression;
  }

  binary_expression->data.expression_binary.left_expression = subscript_expression->data.expression_subscript.expression_1;
  binary_expression->data.expression_binary.right_expression = subscript_expression->data.expression_subscript.expression_2;

  return dereference_expression;
}
