/* Arena-backed tables shared by declaration lookup and later frontend passes. */
#include "hash-table.h"
#include "options.h"

/* Original 0x4ec10..0x4ecd5. */
HashTable *__stdcall new_hash_table(unsigned short buckets, short arena,
                                    unsigned hash_kind, unsigned equality_kind)
{
    HashTable *table;
    ArenaPool *pool;
    unsigned bytes;
    if (!arena)
        table = allocate_permanent(sizeof(HashTable));
    else {
        pool = &compiler_arena.pools[arena == -1 ? 0 : arena];
        if (pool->remaining < (int)sizeof(HashTable))
            refill_arena(&compiler_arena, pool);
        table = (HashTable *)pool->next;
        pool->next += sizeof(HashTable);
        pool->remaining -= sizeof(HashTable);
    }
    table->arena = arena;
    table->hash_kind = hash_kind;
    table->equality_kind = equality_kind;
    table->bucket_count = buckets;
    bytes = (buckets * sizeof(HashEntry *) + 3) & ~3;
    if (!arena)
        table->buckets = allocate_permanent(bytes);
    else {
        pool = &compiler_arena.pools[arena == -1 ? 0 : arena];
        if (pool->remaining < (int)bytes)
            refill_arena(&compiler_arena, pool);
        table->buckets = (HashEntry **)pool->next;
        pool->next += bytes;
        pool->remaining -= bytes;
    }
    table->count = 0;
    return table;
}
