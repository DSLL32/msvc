/* Copy one type into a chained arena [0040ff80,0041000a). */
#include "types.h"
#include <recon.h>

#pragma intrinsic(memcpy, memset)

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

/* Fill a chained type arena [00410010,004100b9), ordinary cdecl. */
unsigned char *fill_type_arena(TypeArena *arena, unsigned value, unsigned size)
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
    memset(result, value, size);
    arena->cursor += size;
    arena->remaining -= size;
    arena->last->used += size;
    return result;
}

/* Reusable temporary type-record storage [0040fae0,0040fb53).
 * Only its requested prefix is initialized by callers. */
static unsigned char *type_temporary_buffer;
static unsigned short type_temporary_capacity;
unsigned char *allocate_type_buffer(unsigned size)
{
    if (size > type_temporary_capacity) {
        if (type_temporary_buffer) free(type_temporary_buffer);
        type_temporary_capacity = (unsigned short)size;
        if (size <= 256) type_temporary_capacity = 256;
        type_temporary_buffer = malloc(type_temporary_capacity);
        if (!type_temporary_buffer) fatal_error(1001, NULL, NULL);
    }
    return type_temporary_buffer;
}
