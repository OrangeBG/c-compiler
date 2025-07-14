#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "../include/hash_table.h"

#define HASH_TABLE_INITIAL_CAPACITY 8
#define HASH_TABLE_MAX_LOAD 0.75

uint32_t        hash_key(char *key);
void            hash_table_expand(HashTable *table, int new_capacity); 
HashTableEntry* hash_table_get_with_entries(HashTableEntry *entries, int capacity, char *key); 

void hash_table_init(HashTable *table) {
  table->capacity = 0;
  table->count = 0;
  table->entries = NULL;
}

void hash_table_print(HashTable *table) {
  printf("HashTable:\n");

  for (int i = 0; i < table->capacity; i++) {
    if (table->entries[i].key == NULL) {
      continue;
    }
    
    printf("index: %d\tkey: %s \t", i, table->entries[i].key);    
    
    switch(table->entries[i].value->type) {
      case HASH_STRUCT:
        printf("value: struct\n");
        break;
      case HASH_STRING:
        printf("value: %s\n", table->entries[i].value->string);
        break;
      case HASH_INT:
        printf("value: %d\n", table->entries[i].value->integer);
        break;
      case HASH_TOMBSTONE:
        printf("value: Tombstone\n");
        break;
    }   
  }
}

void hash_table_delete_entry(HashTable *table, char *key) {
  HashTableEntry *entry = hash_table_get_entry(table, key);

  if (entry == NULL) {
    return;
  }

  entry->key = NULL;
  entry->value->type = HASH_TOMBSTONE;
}

void hash_table_add_entry(HashTable *table, HashTableEntry *entry) {
  HashTableEntry *found_entry = hash_table_get_entry(table, entry->key);

  if (found_entry != NULL && found_entry->key != NULL) {
    fprintf(stderr, "ERROR - Hash Table: Added to table with existing '%s' key\n", entry->key);
    exit(1);
  }

  if (table->count + 1 > table->capacity * HASH_TABLE_MAX_LOAD) {
    int new_capacity = table->capacity < HASH_TABLE_INITIAL_CAPACITY ? HASH_TABLE_INITIAL_CAPACITY : table->capacity * 2;
    hash_table_expand(table, new_capacity);        

    //find and assign the entry from the new table
    found_entry = hash_table_get_entry(table, entry->key);
  }

  found_entry->key = entry->key;
  found_entry->value = entry->value;    
  table->count++;
}

void hash_table_expand(HashTable *table, int new_capacity) {
  HashTableEntry *new_entries = malloc(sizeof(HashTableEntry) * new_capacity);

  for (int i = 0; i < new_capacity; i++) {
    new_entries[i].key = NULL;
  }

  table->count = 0;

  for (int i = 0; i < table->capacity; i++) {
    HashTableEntry old_entry = table->entries[i];

    if (old_entry.key == NULL || old_entry.value->type == HASH_TOMBSTONE) {
      continue;
    }

    HashTableEntry *new_entry = hash_table_get_with_entries(new_entries, new_capacity, old_entry.key); 

    new_entry->key = old_entry.key;
    new_entry->value = old_entry.value;
    
    table->count++;
  }

  table->capacity = new_capacity;
  table->entries = new_entries;  
}

HashTableEntry* hash_table_get_entry(HashTable *table, char *key) {
  if (table->capacity == 0) {
    return NULL;
  }

  HashTableEntry *entry = hash_table_get_with_entries(table->entries, table->capacity, key);

  return entry;
}

//Uses FNV-1A hash algorithm
uint32_t hash_key(char *key) {
  int key_length = strlen(key);
  uint32_t hash = 2166136261u;

  for (int i = 0; i < key_length; i++) {
    hash ^= (uint8_t)key[i];
    hash *= 16777619;
  }

  return hash;
}

//Using Linear probing
HashTableEntry* hash_table_get_with_entries(HashTableEntry *entries, int capacity, char *key) {
  uint32_t hash = hash_key(key);
  int index = hash % capacity;
  
  while (true) {
    HashTableEntry *entry = &entries[index]; 

    if (entry->key == NULL && (entry->value == NULL || entry->value->type != HASH_TOMBSTONE)) {
      return entry;
    }
    
    if (entry->key != NULL && strcmp(entry->key, key) == 0) {
      return entry;
    }

    index = (index + 1) % capacity;
  }
}

HashTable* hash_table_clone(HashTable *table) {
  HashTable *table_copy = malloc(sizeof(HashTable));

  if (table->count == 0) {
      hash_table_init(table_copy);
      return table_copy;
  }
  
  HashTableEntry *entries_copy = malloc(sizeof(HashTableEntry) * table->capacity);

  memcpy(entries_copy, table->entries, sizeof(HashTableEntry) * table->capacity);

  table_copy->capacity = table->capacity;
  table_copy->count = table->count;
  table_copy->entries = entries_copy;
  
  return table_copy;
}
