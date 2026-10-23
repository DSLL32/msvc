#ifndef HASH_TABLE_H
#define HASH_TABLE_H

#include <recon.h>
typedef struct HashEntry {
    void *key;
    void *value;
    struct HashEntry *next;
} HashEntry;
typedef struct HashTable {
    short arena;
    unsigned short reserved02;
    unsigned comparison_kind, hash_kind;
    unsigned short bucket_count, reserved0e;
    HashEntry **buckets;
    unsigned count;
} HashTable;
HashTable *__stdcall new_hash_table(unsigned short buckets, short arena,
                                  unsigned hash, unsigned compare);
HashEntry *__stdcall insert_hash_value(HashTable *table, void *key, void *value);
unsigned __stdcall hash_bucket_index(HashTable *table, void *key);
HashEntry *__stdcall find_hash_entry(HashTable *table, void *key, unsigned bucket);
unsigned hash_interned_name(void *key);
unsigned hash_pointer_low12(void *key);
unsigned hash_symbol_index(void *key);
int hash_keys_equal(void *left, void *right);
int hash_symbol_indices_equal(void *left, void *right);
extern unsigned (__cdecl *hash_functions[])(void *);
extern int (__cdecl *hash_equal_functions[])(void *, void *);
unsigned __stdcall hash_table_count(HashTable *table);
typedef struct HashWalk {
    HashTable *table;
    HashEntry *entry;
    unsigned bucket;
    void *key, *value;
} HashWalk;
typedef char HashWalk_size[sizeof(HashWalk) == 20 ? 1 : -1];
void __stdcall initialize_hash_walk(HashWalk *walk, HashTable *table);
int __stdcall next_hash_entry(HashWalk *walk);
#endif
