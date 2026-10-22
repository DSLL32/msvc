/* Permanent zeroed allocation arena, original globals 0x9f1e0/0x9f41c. */
#include <stdlib.h>
#include "options.h"

static unsigned char *permanent_next;
static unsigned permanent_available;

/* Original 0x6ad50..0x6ad9a. */
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
typedef struct PermanentPage {
    unsigned char *start;
    BitSet *relocations;
} PermanentPage;
static PermanentPage permanent_pages[512]; /* original 0x98180 */
static int permanent_page_count;           /* original 0xa2340 */
static PermanentPage *current_page;         /* original 0xa230c */

/* Original 0x6acb0..0x6ad2a. */
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

/* Original 0x4e700..0x4e750. */
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

/* Original 0x72d20..0x72d44. */
void *__stdcall checked_malloc(unsigned bytes)
{
    void *result = malloc(bytes);
    if (!result)
        fatal_error(59);
    return result;
}

Arena compiler_arena = { 0x3ff4, 0 }; /* original 0xa2740 */
Arena diagnostic_arena; /* original 0xa27c0 */

/* Original 0x72db0..0x72e5b. */
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

/* Original 0x74490..0x744f4. Copy unrounded bytes into a word-aligned allocation. */
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
