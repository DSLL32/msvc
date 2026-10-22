/* Short-count pointer vectors allocated from the compiler's arenas. */
#include <stdlib.h>
#include "work-stack.h"
#include "pch.h"

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

/* Original 0x29cf0..0x29d51. */
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

/* Original 0x2a000..0x2a091. Only absolute seeks are supported. The
   terminal block has a recorded size; zero records a full linked block. */
long __stdcall seek_work_stack(WorkingStack *stack, long offset, int origin)
{
    WorkingBlock *block;
    long remaining = offset;
    if (origin || !stack) return -1;
    reset_work_stack(stack);
    block = (WorkingBlock *)stack->read;
    while (block) {
        if (block->used_bytes) {
            if ((long)block->used_bytes <= remaining) return -1;
            stack->count = remaining;
            stack->next = block->bytes + remaining;
            return offset;
        }
        if (remaining < 2040) {
            stack->count = remaining;
            stack->next = block->bytes + remaining;
            return offset;
        }
        block = block->next;
        stack->read = (unsigned char *)block;
        remaining -= 2040;
    }
    return -1;
}

/* Original 0x29e00..0x29ea5. A zero footer denotes a complete block;
   a recorded terminal size ends the read even if more bytes were requested. */
int __stdcall read_work_stack(WorkingStack *stack, void *buffer, unsigned bytes)
{
    WorkingBlock *block;
    unsigned char *source, *destination = buffer;
    unsigned available, chunk, total = 0;
    int count;
    block = (WorkingBlock *)stack->read;
    while (block) {
        source = stack->next;
        if (!block->used_bytes) {
            available = 2040U - stack->count;
            if (available >= bytes) {
                chunk = bytes;
                stack->count += chunk;
                stack->next += chunk;
                block = 0;
            } else {
                chunk = available;
                bytes -= chunk;
                stack->count = 0;
                block = block->next;
                stack->read = stack->next = (unsigned char *)block;
            }
        } else {
            chunk = block->used_bytes - stack->count;
            if (bytes <= chunk) chunk = bytes;
            stack->count += chunk;
            stack->next += chunk;
            block = 0;
        }
        total += chunk;
        count = (int)chunk;
        while (count > 0) { *destination++ = *source++; --count; }
    }
    return total;
}

/* Original 0x326f0..0x32754. Copy a vector to another arena through its
   element callback, retaining the original signed short iteration count. */
Vector *__stdcall copy_vector(Vector *source, int arena,
                              void *(__stdcall *copy_item)(void *, int))
{
    Vector *copy = new_vector(vector_count(source), arena);
    short index = 0;
    void *item;
    while (index < source->count) {
        item = source->items[index++];
        append_vector(copy, copy_item(item, arena));
    }
    return copy;
}

/* Original 0x32760..0x32776. */
short __stdcall vector_count(Vector *vector)
{
    return vector ? vector->count : 0;
}
