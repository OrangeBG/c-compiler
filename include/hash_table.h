#ifndef HASH_TABLE
#define HASH_TABLE

typedef enum {
  HASH_STRUCT,
  HASH_STRING,
  HASH_INT,
  HASH_TOMBSTONE
} HashType;

typedef struct HashValue {
  HashType type;
  union {
    void *structure;
    char *string;
    int integer;
  };
} HashValue;

typedef struct HashTableEntry {
  char *key;
  HashValue *value;
} HashTableEntry;

typedef struct HashTable {
  int count;
  int capacity;
  HashTableEntry* entries;
} HashTable;

void hash_table_init(HashTable *table);
void hash_table_add_entry(HashTable *table, const HashTableEntry *entry);
void hash_table_delete_entry(const HashTable *table, const char *key); 
void hash_table_print(const HashTable *table);
HashTableEntry* hash_table_get_entry(const HashTable *table, const char *key); 
HashTable*      hash_table_clone(const HashTable *table);

#endif
