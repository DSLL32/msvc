#include "frontend.h"
#include "hash-table.h"
#include <recon.h>

extern "C" unsigned __cdecl hash_table_key(CompilerHashTable *, const void *); /*31ef*/
extern "C" CompilerHashEntry *__cdecl find_hash_entry(CompilerHashTable *, const void *, unsigned); /*3214*/

extern "C" CompilerHashEntry *__cdecl insert_hash_entry(CompilerHashTable *table,
    const void *key, void *value)
{
    unsigned bucket = hash_table_key(table, key);
    CompilerHashEntry *entry;
    CompilerArena *arena;
    CompilerPool *pool;
    if (bucket >= table->bucket_count) { __asm { int 3 } }
    entry = find_hash_entry(table, key, bucket);
    if (!entry) {
        if (table->arena_kind) {
            if (table->arena_kind >= 9) { __asm { int 3 } }
            arena = &frontend_bss.arenas[table->arena_kind];
            entry = (CompilerHashEntry *)arena->cursor;
            arena->cursor += 12;
            if (arena->cursor > arena->committed_limit) arena->CommitExtent();
        } else {
            pool = &frontend_bss.pools[4]; /* initializer28, actual C06E8 */
            if (pool->remaining < 12) pool->Refill(12);
            entry = (CompilerHashEntry *)pool->cursor;
            pool->cursor += 12;
            pool->remaining -= 12;
        }
        entry->key = key;
        entry->next = table->buckets[bucket];
        table->buckets[bucket] = entry;
        ++table->entry_count;
    }
    entry->value = value;
    return entry;
}
extern "C" unsigned __cdecl hash_pointer_key(const void *); /*325b*/
typedef unsigned (__cdecl *CompilerHashFunction)(const void *);
/* Only selector0 is encountered. This one callback slot does not assert
 * the original dispatch family's total capacity or wider domains. */
extern "C" CompilerHashFunction const compiler_hash_functions[1];
CompilerHashFunction const compiler_hash_functions[1] = { hash_pointer_key };
extern "C" unsigned __cdecl hash_table_key(CompilerHashTable *table, const void *key)
{
    unsigned hash;
    if (table->hash_kind != 0 || !table->bucket_count) { __asm { int 3 } }
    hash = compiler_hash_functions[table->hash_kind](key);
    return hash % table->bucket_count;
}
extern "C" unsigned __cdecl hash_pointer_key(const void *key)
{
    return ((unsigned)key >> 2) & 0xfff;
}
extern "C" int __cdecl pointer_keys_equal(const void *, const void *); /*1db74*/
typedef int (__cdecl *CompilerKeyComparator)(const void *, const void *);
/* This is only the encountered comparison selector0, not the capacity of
 * the original callback family at CF0D0. */
extern "C" CompilerKeyComparator const compiler_key_comparators[1];
CompilerKeyComparator const compiler_key_comparators[1] = { pointer_keys_equal };
extern "C" CompilerHashEntry *__cdecl find_hash_entry(CompilerHashTable *table,
    const void *key, unsigned bucket)
{
    CompilerKeyComparator compare;
    CompilerHashEntry *entry;
    if (table->comparison_kind != 0 || bucket >= table->bucket_count) { __asm { int 3 } }
    compare = compiler_key_comparators[table->comparison_kind];
    for (entry = table->buckets[bucket]; entry; entry = entry->next) {
        if (compare(key, entry->key)) return entry;
    }
    return NULL;
}
extern "C" int __cdecl pointer_keys_equal(const void *left, const void *right)
{
    return left == right;
}
