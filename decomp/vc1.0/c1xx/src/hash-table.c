/* Arena-backed tables shared by declaration lookup and later frontend passes. */
#include "hash-table.h"
#include "options.h"
#include "declarations.h"
#include "member-pointers.h"
#include <recon.h>

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

/* Original 0x4ee60..0x4ee7a. Key and value remain unwritten
   until next_hash_entry finds an entry. */
void __stdcall begin_hash_iteration(HashIterator *iterator, HashTable *table)
{
    iterator->table = table;
    iterator->bucket = 0;
    iterator->next = 0;
}

/* Original 0x4ee80..0x4eed2. Advance within a chain, then scan
   buckets. An exhausted iterator leaves its last key and value intact. */
int __stdcall next_hash_entry(HashIterator *iterator)
{
    while (!iterator->next) {
        if (iterator->bucket >= iterator->table->bucket_count) return 0;
        iterator->next = iterator->table->buckets[iterator->bucket];
        ++iterator->bucket;
    }
    iterator->key = iterator->next->key;
    iterator->value = iterator->next->value;
    iterator->next = iterator->next->next;
    return 1;
}

unsigned __stdcall hash_table_bucket(HashTable *table, void *key);
HashEntry *__stdcall find_hash_entry(HashTable *table, void *key, unsigned bucket);

/* Original 0x4ece0..0x4ed5f. Reuse an equal key's entry, otherwise prepend
   one arena node before assigning its value. Replacement retains the old key. */
HashEntry *__stdcall set_hash_value(HashTable *table, void *key, void *value)
{
    unsigned bucket = hash_table_bucket(table, key);
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

/* Original 0xa0cf0: contextual dispatch by the table hash kind. Each
   callback retains its own original cdecl entry, including identical bodies. */
typedef unsigned (__cdecl *HashKey)(void *key);
unsigned __cdecl hash_aligned_pointer(void *key);
unsigned __cdecl hash_low_pointer_bits(void *key);
unsigned __cdecl hash_symbol_index(void *key);
unsigned __cdecl hash_first_word(void *key);
unsigned __cdecl hash_virtual_thunk_key(void *key);
unsigned __cdecl hash_tail_word_pair(void *key);
unsigned __cdecl hash_declaration_index(void *key);
unsigned __cdecl hash_resolved_declaration(void *key);
unsigned __cdecl hash_word_triplet(void *key);
unsigned __cdecl hash_word_pair(void *key);
HashKey hash_functions[] = {
    hash_aligned_pointer,
    hash_low_pointer_bits,
    hash_symbol_index,
    hash_first_word,
    hash_virtual_thunk_key,
    hash_tail_word_pair,
    hash_declaration_index,
    hash_resolved_declaration,
    hash_word_triplet,
    hash_word_pair,
    0
};

/* Original 0x4eda0..0x4edc6. Hash callbacks return a full unsigned word;
   the bucket count is independently zero-extended from its stored halfword. */
unsigned __stdcall hash_table_bucket(HashTable *table, void *key)
{
    unsigned hash = hash_functions[table->hash_kind](key);
    return hash % table->bucket_count;
}

/* Original 0x4ee40..0x4ee51. Hash the aligned low-page address; the final
   native AND EAX,0xffff supplies the complete zero-extended callback result. */
unsigned __cdecl hash_aligned_pointer(void *key)
{
    return ((unsigned)key & 0xffc) >> 2;
}

/* Original 0x31ef0..0x31ef7. The friendship table hashes the owner's
   index, stored in the first word of either a record or a temporary key. */
unsigned __cdecl hash_first_word(void *key)
{
    return *(unsigned *)key;
}

/* Original 0x31ed0..0x31ee4. Unsigned subtraction followed by a less
   than one test is equality of the two owner-index words. */
int __cdecl equal_first_words(void *left, void *right)
{
    return *(unsigned *)left == *(unsigned *)right;
}

/* Original 0x56140..0x56156. Static-member aliases compare stable symbol
   indices, matching the separate hash_symbol_index callback. */
int __cdecl equal_symbol_indices(void *left, void *right)
{
    return ((CppSymbol *)left)->index == ((CppSymbol *)right)->index;
}

/* Original 0xa0cc8: equality callbacks indexed independently of hashing. */
typedef int (__cdecl *EqualKeys)(void *left, void *right);
int __cdecl equal_pointer(void *left, void *right);
int __cdecl equal_symbol_indices(void *left, void *right);
int __cdecl equal_first_words(void *left, void *right);
int __cdecl equal_virtual_thunk_keys(void *left, void *right);
int __cdecl equal_tail_word_pairs(void *left, void *right);
int __cdecl equal_declaration_indices(void *left, void *right);
int __cdecl equal_resolved_declarations(void *left, void *right);
int __cdecl equal_word_triplets(void *left, void *right);
int __cdecl equal_word_pairs(void *left, void *right);
EqualKeys equality_functions[] = {
    equal_pointer,
    equal_symbol_indices,
    equal_first_words,
    equal_virtual_thunk_keys,
    equal_tail_word_pairs,
    equal_declaration_indices,
    equal_resolved_declarations,
    equal_word_triplets,
    equal_word_pairs,
    0
};

/* Original 0x4edd0..0x4ee17. Select equality once, before walking this
   bucket. The callback receives the search key before the stored key. */
HashEntry *__stdcall find_hash_entry(HashTable *table, void *key, unsigned bucket)
{
    EqualKeys equal = equality_functions[table->equality_kind];
    HashEntry *entry = table->buckets[bucket];
    while (entry) {
        if (equal(key, entry->key)) return entry;
        entry = entry->next;
    }
    return 0;
}

/* Original 0x4ed60..0x4ed96. A missing table is an empty lookup;
   otherwise preserve the separate bucket and entry helper calls. */
void *__stdcall find_hash_value(HashTable *table, void *key)
{
    unsigned bucket;
    HashEntry *entry;
    if (!table) return 0;
    bucket = hash_table_bucket(table, key);
    entry = find_hash_entry(table, key, bucket);
    return entry ? entry->value : 0;
}

/* Original 0x4ee30..0x4ee3f. Pointer-identity equality remains a
   distinct cdecl callback returning a complete zero or one. */
int __cdecl equal_pointer(void *left, void *right)
{
    return left == right ? 1 : 0;
}

/* Original 0x56160..0x56168. Class-member alias tables hash the
   declaration's stable emitted symbol index rather than its address. */
unsigned __cdecl hash_symbol_index(void *key)
{
    return ((CppSymbol *)key)->index;
}

/* Original 0x4ec00..0x4ec0a. Interned-name bindings hash the low
   twelve bits of the pointer without the alignment shift of kind zero. */
unsigned __cdecl hash_low_pointer_bits(void *key)
{
    return (unsigned)key & 0xfff;
}

/* Original 0x4dfb0..0x4dfd2, cdecl8. The virtual-member thunk
   cache compares the two trailing words; the function word is not its key. */
int __cdecl equal_tail_word_pairs(void *left, void *right)
{
    MemberPointerThunkKey *a = left, *b = right;
    return a->displacement == b->displacement && a->kind == b->kind;
}
