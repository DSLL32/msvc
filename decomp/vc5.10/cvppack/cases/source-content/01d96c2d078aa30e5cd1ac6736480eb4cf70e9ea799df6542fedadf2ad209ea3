/* Copy one type into a chained arena [0040ff80,0041000a). */
#include "types.h"
#include <recon.h>

#pragma intrinsic(memcpy)

unsigned char *__fastcall copy_type_arena(TypeArena *arena, const unsigned char *data, unsigned size)
{
    TypeArenaBlock *block;
    unsigned char *result;
    if (size > arena->capacity) return NULL;
    if (size > arena->remaining) {
        block = allocate_cleared(sizeof(TypeArenaBlock));
        block->data = allocate_bytes(arena->capacity);
        if (arena->last) arena->last->next = block;
        else arena->first = block;
        ++arena->block_count;
        arena->last = block;
        arena->cursor = block->data;
        arena->remaining = arena->capacity;
    }
    result = arena->cursor;
    memcpy(result, data, size);
    arena->cursor += size;
    arena->remaining -= size;
    arena->last->used += size;
    return result;
}
