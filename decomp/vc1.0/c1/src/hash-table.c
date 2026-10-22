/* Arena-backed hash tables used by enum definitions and other symbol maps. */
#include "options.h"
#include "hash-table.h"
#include <recon.h>

/* Original 0x328c0..0x32985. Both the header and its initially empty bucket
   vector use the same arena; arena 0 is permanent, -1 selects pool 0. */
HashTable *__stdcall new_hash_table(unsigned short buckets, short arena,
                                  unsigned hash, unsigned compare)
{
    HashTable *table;
    ArenaPool *pool;
    int allocation_arena = arena;
    unsigned bytes = ((unsigned)buckets * 4 + 3) & ~3;
    if (!allocation_arena) table = allocate_permanent(sizeof(HashTable));
    else {
        if (allocation_arena == -1) allocation_arena = 0;
        pool = &compiler_arena.pools[allocation_arena];
        if (pool->remaining < (int)sizeof(HashTable)) refill_arena(&compiler_arena, pool);
        table = (HashTable *)pool->next;
        pool->remaining -= sizeof(HashTable);
        pool->next += sizeof(HashTable);
    }
    table->arena = arena;
    table->hash_kind = hash;
    table->comparison_kind = compare;
    table->bucket_count = buckets;
    if (!arena) table->buckets = allocate_permanent(bytes);
    else {
        allocation_arena = arena == -1 ? 0 : arena;
        pool = &compiler_arena.pools[allocation_arena];
        if (pool->remaining < (int)bytes) refill_arena(&compiler_arena, pool);
        table->buckets = (HashEntry **)pool->next;
        pool->remaining -= bytes;
        pool->next += bytes;
    }
    table->count = 0;
    return table;
}

typedef char HashTable_must_be_24_bytes[(sizeof(HashTable) == 24) ? 1 : -1];

/* Original 0x32990..0x32a10. Replace an existing key's value or link a
   new arena-owned entry at the head of its collision chain. */
HashEntry *__stdcall insert_hash_value(HashTable *table, void *key, void *value)
{
    unsigned bucket = hash_bucket_index(table, key);
    HashEntry *entry = find_hash_entry(table, key, bucket);
    ArenaPool *pool;
    int arena;
    if (!entry) {
        arena = table->arena;
        if (!arena) entry = allocate_permanent(sizeof(HashEntry));
        else {
            if (arena == -1) arena = 0;
            pool = &compiler_arena.pools[arena];
            if (pool->remaining < (int)sizeof(HashEntry)) refill_arena(&compiler_arena, pool);
            entry = (HashEntry *)pool->next;
            pool->remaining -= sizeof(HashEntry);
            pool->next += sizeof(HashEntry);
        }
        entry->key = key;
        entry->next = table->buckets[bucket];
        table->buckets[bucket] = entry;
        ++table->count;
    }
    entry->value = value;
    return entry;
}

/* Original dispatch tables at 0x6ce60(hash) and 0x6ce58(equality). */
unsigned (__cdecl *hash_functions[])(void *) = {
    hash_interned_name, hash_pointer_low12, hash_symbol_index
};
int (__cdecl *hash_equal_functions[])(void *, void *) = {
    hash_keys_equal, hash_symbol_indices_equal
};

/* Original 0x32a50..0x32a77. */
unsigned __stdcall hash_bucket_index(HashTable *table, void *key)
{
    return (*hash_functions[table->hash_kind])(key) % table->bucket_count;
}

/* Original 0x32af0..0x32b02. Interned names are aligned and identified
   by their addresses; use their position within the allocation page. */
unsigned hash_interned_name(void *key)
{
    return ((unsigned)key & 0xffc) >> 2;
}

/* Original 0x32a80..0x32ac8. Equality callbacks return true on a match. */
HashEntry *__stdcall find_hash_entry(HashTable *table, void *key, unsigned bucket)
{
    int (__cdecl *equal)(void *, void *) = hash_equal_functions[table->comparison_kind];
    HashEntry *entry = table->buckets[bucket];
    while (entry) {
        if ((*equal)(key, entry->key)) return entry;
        entry = entry->next;
    }
    return 0;
}

/* Original 0x32ae0..0x32af0. Interned keys compare by pointer identity. */
int hash_keys_equal(void *left, void *right)
{
    return left == right;
}

typedef char HashEntry_must_be_12_bytes[(sizeof(HashEntry) == 12) ? 1 : -1];
