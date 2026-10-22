/* Permanent zeroed allocation arena, original globals 0x6b650/0x6b7dc. */
#include <stdlib.h>
#include "options.h"
#include "parser.h"

static unsigned char *permanent_next;
static unsigned permanent_available;

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
typedef struct PermanentPage {
    unsigned char *start;
    BitSet *relocations;
} PermanentPage;
static PermanentPage permanent_pages[512]; /* original 0x66370 */
static int permanent_page_count;           /* original 0x6de00 */
static PermanentPage *current_page;         /* original 0x6ddcc */

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
