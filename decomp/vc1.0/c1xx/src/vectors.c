/* Short-count pointer vectors allocated from the compiler's arenas. */
#include <stdlib.h>
#include "initialization.h"
#include <recon.h>

/* Original 0x4e810..0x4e8cd. */
Vector *__stdcall new_vector(short capacity, int arena)
{
    Vector *vector;
    ArenaPool *pool;
    unsigned bytes;
    int index = arena;
    if (!arena)
        vector = allocate_permanent(sizeof(Vector));
    else {
        if (index == -1)
            index = 0;
        pool = &compiler_arena.pools[index];
        if (pool->remaining < (int)sizeof(Vector))
            refill_arena(&compiler_arena, pool);
        vector = (Vector *)pool->next;
        pool->remaining -= sizeof(Vector);
        pool->next += sizeof(Vector);
    }
    vector->count = 0;
    vector->capacity = capacity;
    vector->arena = arena;
    if ((unsigned)(int)vector->capacity & 0x3fffffff) {
        bytes = ((int)vector->capacity * 4 + 3) & ~3;
        if (!arena)
            vector->items = allocate_permanent(bytes);
        else {
            index = arena == -1 ? 0 : arena;
            pool = &compiler_arena.pools[index];
            if (pool->remaining < (int)bytes)
                refill_arena(&compiler_arena, pool);
            vector->items = (void **)pool->next;
            pool->remaining -= bytes;
            pool->next += bytes;
        }
    }
    return vector;
}


/* Original 0x4ead0..0x4eaf8. */
void __stdcall append_vector(Vector *vector, void *item)
{
    if (vector->count == vector->capacity)
        grow_vector(vector);
    vector->items[vector->count++] = item;
}


/* Original 0x32690..0x326f1. */
void __stdcall initialize_work_stack(WorkingStack **stack)
{
    WorkingStack *state = *stack;
    if (!state) {
        state = malloc(sizeof(WorkingStack));
        state->storage = malloc(2048);
        *(unsigned *)(state->storage + 2040) = 0;
        *(unsigned *)(state->storage + 2044) = 0;
        *stack = state;
        reset_work_stack(state);
    } else {
        *(unsigned *)(state->storage + 2044) = 0;
        reset_work_stack(state);
    }
}


/* Original 0x32670..0x32686. */
void __stdcall reset_work_stack(WorkingStack *stack)
{
    stack->read = stack->storage;
    stack->count = 0;
    stack->next = stack->storage;
}

/* Original 0x4e800..0x4e80e. An empty arena vector keeps its own factory ABI. */
Vector *__stdcall new_empty_vector(int arena)
{
    return new_vector(0, arena);
}

short __stdcall vector_count(Vector *vector);

/* Original 0x4ea40..0x4eaa4. Copy the pointer collection into the requested
   arena through its two-slot native element-copy callback. */
Vector *__stdcall copy_vector_elements(Vector *vector, int arena, VectorCopyElement copy)
{
    Vector *result = new_vector(vector_count(vector), arena);
    short index = 0;
    void *item;
    while (index < vector->count) {
        item = vector->items[index++];
        item = copy(item, arena);
        append_vector(result, item);
    }
    return result;
}

/* Original 0x4eab0..0x4eac6. The native result is a signed short in AX. */
short __stdcall vector_count(Vector *vector)
{
    if (!vector) return 0;
    return vector->count;
}

/* Original 0x4e8d0..0x4e96a. Capacity is a signed short, including the
   native wrap on growth. Old arena storage is retained after copying. */
Vector *__stdcall grow_vector(Vector *vector)
{
    unsigned char *old_items = (unsigned char *)vector->items;
    ArenaPool *pool;
    unsigned bytes, i;
    int arena;
    if (vector->capacity >= 1000)
        vector->capacity = (short)(vector->capacity + 1000);
    else vector->capacity = (short)(vector->capacity * 2 + 4);
    bytes = ((int)vector->capacity * 4 + 3) & ~3;
    arena = vector->arena;
    if (!arena) vector->items = allocate_permanent(bytes);
    else {
        if (arena == -1) arena = 0;
        pool = &compiler_arena.pools[arena];
        if (pool->remaining < (int)bytes) refill_arena(&compiler_arena, pool);
        vector->items = (void **)pool->next;
        pool->remaining -= bytes;
        pool->next += bytes;
    }
    if (vector->count > 0) {
        bytes = vector_used_bytes(vector);
        for (i = 0; i < bytes; ++i)
            ((unsigned char *)vector->items)[i] = old_items[i];
    }
    return vector;
}

/* Original 0x4eb50..0x4eb5b. Callers establish that an element exists;
   the original helper performs neither a count nor a null check. */
void *__stdcall first_vector_item(Vector *vector)
{
    return vector->items[0];
}
