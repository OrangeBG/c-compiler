#include "../include/sa_goto_check.h"
#include "../include/hash_table.h"
#include <stdio.h>

//TODO: Need to add VLA check to goto statements when arrays are added to the compiler. Reference C17 ISO: 6.8.6 Jump statements

typedef struct {
  bool has_label;
} GotoStatus;

enum GotoType {
  GOTO_LABEL,
  GOTO_STATEMENT
}; 

static void check_ast_node(AstNode *ast_node, HashTable *goto_statuses);
static void add_goto_to_table(char* goto_label, enum GotoType type,  HashTable *goto_statuses); 

void sa_goto_check(AstNode *ast_nodes) {

  check_ast_node(ast_nodes, NULL);
}

void check_ast_node(AstNode *ast_node, HashTable *goto_statuses) {
  switch (ast_node->type) {    
    case AST_PROGRAM:
      for (int i = 0; i < ast_node->data.program.declaration_count; i++) {
        AstNode *declaration_node = ast_node->data.program.declaration_ptrs->node_pointers[i];

        if (declaration_node->type != AST_FUNCTION_DECLARATION) {
          continue;
        }
        
        check_ast_node(ast_node->data.program.declaration_ptrs->node_pointers[i], goto_statuses);
      }
      break;
    case AST_FUNCTION_DECLARATION:
      if (ast_node->data.function_declaration.body_block == NULL) {
        break;
      }

      HashTable new_goto_statuses;
      hash_table_init(&new_goto_statuses);

      check_ast_node(ast_node->data.function_declaration.body_block, &new_goto_statuses);

      for (int i = 0; i < new_goto_statuses.capacity; i++) {
        HashTableEntry *entry = &new_goto_statuses.entries[i];

        if (entry == NULL || entry->key == NULL) {
          continue;
        }

        if (((GotoStatus*)entry->value->structure)->has_label == false) {
          fprintf(stderr, "ERROR - SA GOTO CHECK: Undefined goto '%s' label\n", entry->key);
          exit(1);
        }        

        //TODO: Check to see if we are leaking memory with this hash table and need to free it after doing this loop
      }      
      break;
    case AST_BLOCK:
      for (int i = 0; i < ast_node->data.block.block_count; i++) {
        AstNode *block_item_node = ast_node->data.block.block_ptrs->node_pointers[i];

        if (block_item_node->type == AST_BLOCK || block_item_node->type == AST_STATEMENT_COMPOUND || block_item_node->type == AST_STATEMENT_GOTO_LABEL || block_item_node->type == AST_STATEMENT_GOTO) {
          check_ast_node(block_item_node, goto_statuses);
        }
      }   
      break;      
    case AST_STATEMENT_COMPOUND:
      check_ast_node(ast_node->data.compound_statement.block, goto_statuses);
      break;
    case AST_STATEMENT_GOTO_LABEL: {
      HashTableEntry *existing_goto_entry = hash_table_get_entry(goto_statuses, ast_node->data.goto_label_statement.label);

      if (existing_goto_entry == NULL || existing_goto_entry->key == NULL) {
        add_goto_to_table(ast_node->data.goto_label_statement.label, GOTO_LABEL, goto_statuses);
        break;
      }

      if (((GotoStatus*)existing_goto_entry->value->structure)->has_label) {
        fprintf(stderr, "ERROR - SA GOTO CHECK: Duplicate '%s' label not allowed", ast_node->data.goto_label_statement.label); 
        exit(1);
      }
      
      ((GotoStatus*)existing_goto_entry->value->structure)->has_label = true;      

      break;
    }
    case AST_STATEMENT_GOTO: {
      HashTableEntry *existing_goto_entry = hash_table_get_entry(goto_statuses, ast_node->data.goto_label_statement.label);

      if (existing_goto_entry == NULL || existing_goto_entry->key == NULL) {
        add_goto_to_table(ast_node->data.goto_label_statement.label, GOTO_STATEMENT, goto_statuses);
      }      

      break;
    }
    default:
      fprintf(stderr, "ERROR - SA GOTO CHECK: Unresolved AST node type %d", ast_node->type);
      exit(1);
  }
}

static void add_goto_to_table(char* goto_label, enum GotoType type, HashTable *goto_statuses) {
  GotoStatus *status = malloc(sizeof(GotoStatus)); 
  status->has_label = type == GOTO_LABEL ? true : false;
  
  HashValue *value = malloc(sizeof(HashValue));
  value->type = HASH_STRUCT;
  value->structure = status;

  HashTableEntry *entry = malloc(sizeof(HashTableEntry));
  entry->key = goto_label;
  entry->value = value;

  hash_table_add_entry(goto_statuses, entry);
}
