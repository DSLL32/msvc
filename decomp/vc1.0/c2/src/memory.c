/* Compiler memory arenas. */
#include "backend.h"
/* Original [00070ad0,00070ae0). This build does not configure arena limits. */
void *__fastcall configure_arena(unsigned arena, unsigned mode, unsigned block_size)
{
    return NULL;
}

struct Arena {
    void *blocks;
    char *next;
    unsigned remaining;
    int heap_flags;
};
/* Original arena descriptors [00086090,00086110). */
Arena arenas[8];
/* Original [00070bf0,00070c10). Blocks begin with their next pointer. */
void __fastcall free_arena_blocks(Arena *arena)
{
    unsigned *block = arena->blocks, *next;
    while (block) {
        next = (unsigned *)*block;
        free(block);
        block = next;
    }
}
/* Original [00070c10,00070c40). The policy flags survive the reset. */
void __fastcall reset_arena(unsigned arena)
{
    Arena *descriptor = arenas + arena;
    free_arena_blocks(descriptor);
    descriptor->remaining = 0;
    descriptor->blocks = 0;
    descriptor->next = 0;
}
void *__fastcall compiler_allocate(unsigned bytes, int flags);
/* Original [00070bc0,00070bf0). */
void *__fastcall arena_calloc(unsigned arena, unsigned bytes)
{
    void *result = compiler_allocate(bytes, arenas[arena].heap_flags);
    unsigned *words = result;
    unsigned count = bytes >> 2;
    unsigned char *tail;
    while (count--) *words++ = 0;
    tail = (unsigned char *)words;
    count = bytes & 3;
    while (count--) *tail++ = 0;
    return result;
}

#include "options.h"
#include <recon.h>
/* Original [00070b80,00070bc0). Callers in this case use flags == 0. */
void *__fastcall compiler_allocate(unsigned bytes, int flags)
{
    void *block;
    if (!flags) {
        block = malloc(bytes);
        if (!block) compiler_report(2);
    }
    /* The original leaves this local uninitialized when flags is nonzero. */
    return block;
}

/* Original [00070ae0,00070b80). Blocks have a four-byte chain header. */
void *__fastcall arena_alloc(unsigned arena, unsigned bytes)
{
    Arena *descriptor = arenas + arena;
    unsigned rounded = (bytes + 3) & ~3U;
    unsigned *block, *words, count;
    char *result;
    if (rounded <= 1020) {
        if ((int)descriptor->remaining < (int)rounded) {
            block = compiler_allocate(1024, 0);
            *block = (unsigned)descriptor->blocks;
            descriptor->blocks = block;
            descriptor->remaining = 1020;
            descriptor->next = (char *)(block + 1);
            words = block + 1;
            count = 255;
            while (count--) *words++ = 0;
        }
        result = descriptor->next;
        descriptor->next += rounded;
        descriptor->remaining -= rounded;
        return result;
    }
    block = compiler_allocate(rounded + 4, 0);
    if (!descriptor->blocks) {
        descriptor->blocks = block;
        *block = 0;
    } else {
        *block = *(unsigned *)descriptor->blocks;
        *(unsigned *)descriptor->blocks = (unsigned)block;
    }
    words = block + 1;
    count = rounded >> 2;
    while (count--) *words++ = 0;
    descriptor->remaining = 0;
    return block + 1;
}

/* Original [00070c40,00070c90). */
char *__fastcall arena_strdup(char *string, unsigned arena)
{
    unsigned bytes = 0;
    char *result, *destination;
    while (string[bytes]) ++bytes;
    result = arena_alloc(arena, bytes + 1);
    destination = result;
    while ((*destination++ = *string++) != 0) ;
    return result;
}
