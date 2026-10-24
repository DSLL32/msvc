#include "frontend.h"
#include <recon.h>

/* Vectors and their pointer storage share the selected compiler arena. A zero
 * capacity leaves the storage member untouched, as in the original routine. */
extern "C" CompilerVector *new_option_vector(unsigned capacity, int arena_index)
{
    CompilerArena *arena = &frontend_bss.arenas[arena_index];
    CompilerVector *vector = (CompilerVector *)arena->cursor;
    unsigned bytes;
    unsigned char *items;

    arena->cursor += sizeof(CompilerVector);
    if (arena->cursor > arena->committed_limit)
        arena->CommitExtent();
    vector->count = 0;
    vector->capacity = (short)capacity;
    vector->arena_index = arena_index;
    bytes = (unsigned)((int)vector->capacity * 4);
    if (bytes) {
        items = arena->cursor;
        arena->cursor += (bytes + 3) & ~3;
        if (arena->cursor > arena->committed_limit)
            arena->CommitExtent();
        vector->items = (void **)items;
    }
    return vector;
}
