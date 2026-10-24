/* Permanent zeroed allocation arena, original globals 0x6b650/0x6b7dc. */
#include <stdlib.h>
#include "options.h"
#include "parser.h"
#include "pch-relocate.h"
#include <recon.h>

unsigned char *permanent_next;
unsigned permanent_available;

/* Original 0x44ba0..0x44bea. */
void *__stdcall allocate_permanent(unsigned bytes)
{
    unsigned *clear;
    unsigned i;
    void *result;
    if (permanent_available < bytes) {
        permanent_next = new_permanent_page();
        clear = (unsigned *)permanent_next;
        for (i = 0; i < 0x3bff; ++i)
            clear[i] = 0;
        permanent_next += 4;
        permanent_available = 0xeff8;
    }
    result = permanent_next;
    permanent_next += bytes;
    permanent_available -= bytes;
    return result;
}

/* Pages and relocation maps used to serialize permanent arena pointers. */
PermanentPage permanent_pages[512]; /* original 0x66370 */
int permanent_page_count;           /* original 0x6de00 */
PermanentPage *current_page;         /* original 0x6ddcc */

/* Original 0x44b00..0x44b7a. */
void *new_permanent_page(void)
{
    unsigned char *allocation;
    if (permanent_page_count >= 511)
        fatal_error(60);
    current_page = &permanent_pages[permanent_page_count++];
    allocation = malloc(0xfffc);
    if (!allocation)
        fatal_error(60);
    allocation = (unsigned char *)(((unsigned)allocation + 0xfff) & 0xfffff000);
    current_page->start = allocation;
    current_page->relocations = new_bitset(0xeffc);
    return current_page->start;
}

/* Original 0x323b0..0x32400. */
BitSet *__stdcall new_bitset(unsigned bit_count)
{
    unsigned bytes = (bit_count >> 3) + 2;
    unsigned i;
    BitSet *set = malloc((bit_count >> 3) + 9);
    if (!set)
        fatal_error(60);
    set->bit_count = bit_count;
    for (i = 0; i < bytes; ++i)
        set->bits[i] = 0;
    set->bits[bytes - 1] = 0xff;
    return set;
}

/* Original 0x32400..0x32410, RET4. Bitsets have individual heap ownership. */
void __stdcall free_bitset(BitSet *set)
{
    free(set);
}

/* Original 0x32410..0x32442, RET4. Retain the constructor's sentinel
   byte as well as the represented bits and their final partial byte. */
BitSet *__stdcall clone_bitset(BitSet *source)
{
    BitSet *copy = new_bitset(source->bit_count);
    unsigned bytes = (source->bit_count >> 3) + 2;
    unsigned index;
    copy->bit_count = source->bit_count;
    for (index = 0; index < bytes; ++index) copy->bits[index] = source->bits[index];
    return copy;
}

/* Original 0x32450..0x32479, RET8. The sentinel beyond the final
   represented byte is deliberately retained. The caller supplies equal sizes. */
void __stdcall subtract_bitset(BitSet *target, BitSet *source)
{
    unsigned last = target->bit_count >> 3, index;
    for (index = 0; index <= last; ++index)
        target->bits[index] &= (unsigned char)~source->bits[index];
}

/* Original 0x4a570..0x4a594. */
void *__stdcall checked_malloc(unsigned bytes)
{
    void *result = malloc(bytes);
    if (!result)
        fatal_error(59);
    return result;
}

/* General-purpose arena roots, original 0x6e1c8 and 0x6e248. */
Arena compiler_arena = { 0x3ff4, 0 };
Arena diagnostic_arena;

/* Original 0x4a600..0x4a6ab. */
void __stdcall refill_arena(Arena *arena, ArenaPool *pool)
{
    ArenaBlock *block = arena->free_blocks;
    unsigned i;
    if (block)
        arena->free_blocks = block->next;
    else {
        block = malloc(arena->block_bytes + 12);
        if (!block)
            fatal_error(arena == &diagnostic_arena ? 60 : 59);
        block->bytes = arena->block_bytes;
    }
    block->next = 0;
    for (i = 0; i < block->bytes; ++i)
        block->data[i] = 0;
    if (pool->last) {
        pool->last->next = block;
        pool->last->bytes = arena->block_bytes - pool->remaining;
    }
    pool->last = block;
    if (!pool->first)
        pool->first = block;
    pool->remaining = arena->block_bytes;
    pool->next = block->data;
}

/* Original 0x4bce0..0x4bd44. */
void *__stdcall copy_arena_bytes(void *source, int arena, unsigned bytes)
{
    unsigned allocation = (bytes + 3) & ~3;
    unsigned char *destination;
    ArenaPool *pool;
    unsigned i;
    if (!arena)
        destination = allocate_permanent(allocation);
    else {
        if (arena == -1)
            arena = 0;
        pool = &compiler_arena.pools[arena];
        if (pool->remaining < (int)allocation)
            refill_arena(&compiler_arena, pool);
        destination = pool->next;
        pool->remaining -= allocation;
        pool->next += allocation;
    }
    for (i = 0; i < bytes; ++i)
        destination[i] = ((unsigned char *)source)[i];
    return destination;
}

/* Original 0x6d9cc/0x6d9d0. Release bookkeeping cleared on a nonempty
   pool release; no other references exist in this compiler image. */
static unsigned arena_release_state[2];

/* Original 0x4a5a0..0x4a600. The original root range contains only the
   compiler arena; its diagnostic arena is the excluded upper bound. */
void __stdcall release_arena(int arena)
{
    ArenaPool *pool = &compiler_arena.pools[arena];
    ArenaBlock *last = pool->last;
    if (last) {
        last->bytes = compiler_arena.block_bytes - pool->remaining;
        if (last->bytes) {
            last->next = compiler_arena.free_blocks;
            compiler_arena.free_blocks = pool->first;
            pool->first = 0;
            pool->last = 0;
            pool->next = 0;
            pool->remaining = 0;
            arena_release_state[1] = 0;
            arena_release_state[0] = 0;
        }
    }
}

/* Original 0x33020..0x3306d. The generic two-word work entry is used
   by both label stacks and scope-cleanup lists; its payload is caller-owned. */
void *__stdcall push_work_list(void **head, int arena)
{
    typedef struct WorkListEntry {
        struct WorkListEntry *next;
        void *value;
    } WorkListEntry;
    WorkListEntry *entry;
    ArenaPool *pool;
    if (!arena) entry = allocate_permanent(sizeof(WorkListEntry));
    else {
        if (arena == -1) arena = 0;
        pool = &compiler_arena.pools[arena];
        if (pool->remaining < (int)sizeof(WorkListEntry)) refill_arena(&compiler_arena, pool);
        entry = (WorkListEntry *)pool->next;
        pool->remaining -= sizeof(WorkListEntry);
        pool->next += sizeof(WorkListEntry);
    }
    entry->next = *head;
    *head = entry;
    return entry;
}

/* Original 0x4bc50..0x4bcd3. Round the allocation to four bytes while
   copying only the original string and its terminator. */
char *__stdcall copy_string(char *text, int arena)
{
    unsigned length, bytes, i;
    char *copy;
    ArenaPool *pool;
    for (length = 0; text[length]; ++length) ;
    bytes = (length + 4) & ~3;
    if (!arena) copy = allocate_permanent(bytes);
    else {
        if (arena == -1) arena = 0;
        pool = &compiler_arena.pools[arena];
        if (pool->remaining < (int)bytes) refill_arena(&compiler_arena, pool);
        copy = (char *)pool->next;
        pool->remaining -= bytes;
        pool->next += bytes;
    }
    for (i = 0; (copy[i] = text[i]) != 0; ++i) ;
    return copy;
}

/* Original 0x33070..0x3307d, RET4. Work entries belong to their arena;
   popping only advances the nonempty list and returns the new head. */
void *__stdcall pop_work_list(void **head)
{
    void *next = *(void **)*head;
    *head = next;
    return next;
}

/* Original0x4a6b0..0x4a6df, RET4. The macro allocator allows exactly
   one ordinary malloc retry before its fatal out-of-memory diagnostic. */
void *__stdcall retry_malloc(unsigned bytes)
{
    void *memory = malloc(bytes);
    if (!memory) {
        memory = malloc(bytes);
        if (!memory) {
            fatal_error(60);
            return 0;
        }
    }
    return memory;
}
