#ifndef HASH_TABLE_H
#define HASH_TABLE_H

typedef struct HashEntry {
    void *key, *value;
    struct HashEntry *next;
} HashEntry;
typedef struct HashTable {
    short arena;
    unsigned equality_kind, hash_kind;
    unsigned short bucket_count;
    HashEntry **buckets;
    unsigned count;
} HashTable;
enum HashKind { HASH_FIRST_WORD = 3 };
enum EqualityKind { EQUAL_FIRST_WORD = 2 };
HashTable *__stdcall new_hash_table(unsigned short buckets, short arena,
                                    unsigned hash_kind, unsigned equality_kind);
void *__stdcall find_hash_value(HashTable *table, void *key);
HashEntry *__stdcall set_hash_value(HashTable *table, void *key, void *value);
typedef char HashTableSizeMustBe24[(sizeof(HashTable) == 24) ? 1 : -1];
#endif
