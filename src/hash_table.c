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

void hash_table_delete_entry(HashTable *table, char *key) {
  HashTableEntry *entry = hash_table_get_entry(table, key);

  if (entry == NULL) {
    return;
  }

  entry->key = NULL;
  entry->value.type = HASH_TOMBSTONE;
}

void hash_table_add_entry(HashTable *table, HashTableEntry *entry) {
  uint32_t hash = hash_key(entry->key);

  if (table->count + 1 > table->capacity * HASH_TABLE_MAX_LOAD) {
    int new_capacity = table->capacity < HASH_TABLE_INITIAL_CAPACITY ? HASH_TABLE_INITIAL_CAPACITY : table->capacity * 2;
    hash_table_expand(table, new_capacity);    
  }

  uint32_t index = hash % table->capacity;
  table->entries[index] = *entry;  
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

    if (old_entry.value.type == HASH_TOMBSTONE) {
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
  if (table->count == 0) {
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

    if (entry->key == NULL && entry->value.type != HASH_TOMBSTONE) {
      return entry;
    }
    
    if (entry->key != NULL && strcmp(entry->key, key) == 0) {
      return entry;
    }

    index = (index + 1) % capacity;
  }
}
