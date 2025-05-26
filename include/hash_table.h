#ifndef HASH_TABLE
#define HASH_TABLE

typedef enum {
  HASH_STRING,
  HASH_INT,
  HASH_TOMBSTONE
} HashType;

typedef struct HashValue {
  HashType type;
  union {
    char* string;
    int integer;
  };
} HashValue;

typedef struct HashTableEntry {
  char* key;
  HashValue value;
} HashTableEntry;

typedef struct HashTable {
  int count;
  int capacity;
  HashTableEntry* entries;
} HashTable;

void hash_table_init(HashTable *table);
void hash_table_add_entry(HashTable *table, HashTableEntry *entry);
void hash_table_delete_entry(HashTable *table, char *key); 
void hash_table_print(HashTable *table);
HashTableEntry* hash_table_get_entry(HashTable *table, char *key); 

#endif
