/* Short-count pointer vectors allocated from the compiler's arenas. */
#include <stdlib.h>
#include "initialization.h"

/* Original 0x324c0..0x3257d. */
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

/* Original 0x32780..0x327a8. */
void __stdcall append_vector(Vector *vector, void *item)
{
    if (vector->count == vector->capacity)
        grow_vector(vector);
    vector->items[vector->count++] = item;
}

/* Original 0x29cf0..0x29d4f. */
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

/* Original 0x29cd0..0x29ce6. */
void __stdcall reset_work_stack(WorkingStack *stack)
{
    stack->read = stack->storage;
    stack->count = 0;
    stack->next = stack->storage;
}
