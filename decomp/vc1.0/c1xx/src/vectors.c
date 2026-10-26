/* Short-count pointer vectors allocated from the compiler's arenas. */
#include <stdlib.h>
#include "initialization.h"
#include "lexer.h"
#include "parser-state.h"
#include "source-input.h"
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

Vector *__stdcall copy_vector_with_extra(Vector *vector, short extra, int arena);

/* Original 0x4ea00..0x4ea13. Plain copies use zero additional capacity,
   retaining the native three-slot factory behind this two-slot wrapper. */
Vector *__stdcall copy_vector(Vector *vector, int arena)
{
    return copy_vector_with_extra(vector, 0, arena);
}

/* Original 0x4e980..0x4e9fb. Preserve the three count queries and the
   fatal diagnostic's filename/line arguments on a negative extra capacity. */
Vector *__stdcall copy_vector_with_extra(Vector *vector, short extra, int arena)
{
    Vector *copy;
    int capacity = (int)vector_count(vector) + extra;
    unsigned bytes, i;
    if (capacity < (int)vector_count(vector))
        fatal_error(1, pch_diagnostic_name, 106);
    copy = new_vector((short)(vector_count(vector) + extra), arena);
    copy->count = vector->count;
    if (copy->count > 0) {
        bytes = vector_used_bytes(vector);
        for (i = 0; i < bytes; ++i)
            ((unsigned char *)copy->items)[i] = ((unsigned char *)vector->items)[i];
    }
    return copy;
}

/* Original 0x4ea20..0x4ea33. Table-owner lists reserve one additional
   pointer slot through the shared three-argument vector factory. */
Vector *__stdcall copy_vector_with_spare(Vector *vector, int arena)
{
    return copy_vector_with_extra(vector, 1, arena);
}

/* Original 0x4eb60..0x4eba1. Arena/capacity are irrelevant to vector
   equality. Preserve the separate used-byte getter before comparing contents. */
int __stdcall equal_vectors(Vector *left, Vector *right)
{
    unsigned bytes, index;
    unsigned char *left_bytes, *right_bytes;
    if (left->count != right->count) return 0;
    left_bytes = (unsigned char *)left->items;
    right_bytes = (unsigned char *)right->items;
    bytes = vector_used_bytes(left);
    for (index = 0; index < bytes; ++index)
        if (left_bytes[index] != right_bytes[index]) return 0;
    return 1;
}

/* Original 0x4e970..0x4e97c. Counts are signed shorts; conversion to
   unsigned before shifting preserves the native32-bit wrap for all values. */
unsigned __stdcall vector_used_bytes(Vector *vector)
{
    return (unsigned)vector->count << 2;
}

/* Original 0x32700..0x32794. Working-stack pages hold 2040 data bytes,
   a next-page link and a used-byte word. A null stack writes into the shared
   text workspace instead, with the original 2048-byte limit. */
int __stdcall write_work_character(int character, WorkingStack *stack)
{
    unsigned char *cursor, *page;
    if (!stack) {
        if (text_work_cursor >= expression_storage + 2048) fatal_error(64);
        *text_work_cursor++ = (unsigned char)character;
        return character;
    }
    cursor = stack->next;
    if ((int)stack->count >= 2040) {
        page = *(unsigned char **)(stack->read + 2040);
        if (!page) {
            page = malloc(2048);
            *(unsigned char **)(page + 2040) = 0;
            *(unsigned char **)(stack->read + 2040) = page;
        }
        stack->read = page;
        cursor = stack->read;
        *(unsigned *)(cursor + 2044) = 0;
        stack->count = 0;
    }
    *cursor = (unsigned char)character;
    ++stack->count;
    stack->next = cursor + 1;
    return character;
}

/* Keep the original forward word-plus-byte copies inline in the work-stack
   writer. A helper function would add an edge absent from the original. */
#define COPY_WORK_BLOCK(destination_, source_, bytes_) do { \
    copy_target = (unsigned char *)(destination_); \
    copy_source = (const unsigned char *)(source_); \
    copy_bytes = (bytes_); \
    for (copy_words = copy_bytes / 4; copy_words; --copy_words) { \
        *(unsigned *)copy_target = *(const unsigned *)copy_source; \
        copy_target += 4; copy_source += 4; \
    } \
    for (copy_bytes &= 3; copy_bytes; --copy_bytes) \
        *copy_target++ = *copy_source++; \
} while (0)

/* Original 0x32870..0x32997. Fill the current 2040-byte payload, then
   reuse or allocate linked pages. The saved remainder source is intentionally
   unchanged between full pages: the original repeats that same source block. */
void __stdcall write_work_bytes(const void *source, unsigned bytes, unsigned count, WorkingStack *stack)
{
    const unsigned char *remainder, *copy_source;
    unsigned char *page, *next, *copy_target;
    unsigned total = bytes * count;
    unsigned space, copy_bytes, copy_words;
    if (!stack) {
        COPY_WORK_BLOCK(text_work_cursor, source, total);
        text_work_cursor += total;
        return;
    }
    page = stack->read;
    space = 2040 - stack->count;
    if (total <= space) {
        COPY_WORK_BLOCK(stack->next, source, total);
        stack->next += total;
        stack->count += total;
        return;
    }
    COPY_WORK_BLOCK(stack->next, source, space);
    total -= space;
    remainder = (const unsigned char *)source + space;
    while (total) {
        next = *(unsigned char **)(page + 2040);
        if (!next) {
            next = malloc(2048);
            *(unsigned char **)(next + 2040) = 0;
            *(unsigned char **)(page + 2040) = next;
        }
        page = *(unsigned char **)(page + 2040);
        *(unsigned *)(page + 2044) = 0;
        if (total <= 2040) {
            COPY_WORK_BLOCK(page, remainder, total);
            stack->read = page;
            stack->count = total;
            stack->next = page + total;
            total = 0;
        } else {
            COPY_WORK_BLOCK(page, remainder, 2040);
            total -= 2040;
        }
    }
}
#undef COPY_WORK_BLOCK

/* Original 0x32850..0x32861. Mark the final page's valid byte count
   so a later reader can distinguish it from a full payload page. */
void __stdcall finish_work_stack(WorkingStack *stack)
{
    *(unsigned *)(stack->read + 2044) = stack->count;
}

/* Original 0x327a0..0x32844. A zero footer marks a full payload page;
   a nonzero footer is the final page's byte count. Advance pages only while
   a full page cannot satisfy the request, retaining the final-page cursor. */
int __stdcall read_work_stack(WorkingStack *stack, void *buffer, unsigned bytes)
{
    unsigned char *page = stack->read;
    unsigned char *destination = buffer, *source;
    unsigned total = 0, available, chunk;
    if (!page) return 0;
    do {
        source = stack->next;
        available = *(unsigned *)(page + 2044);
        if (!available) {
            available = 2040 - stack->count;
            if (available >= bytes) {
                chunk = bytes;
                stack->count += chunk;
                stack->next = source + chunk;
                page = 0;
            } else {
                chunk = available;
                bytes -= chunk;
                stack->count = 0;
                page = *(unsigned char **)(stack->read + 2040);
                stack->read = page;
                stack->next = page;
            }
        } else {
            chunk = available - stack->count;
            if (bytes <= chunk) chunk = bytes;
            stack->count += chunk;
            stack->next = source + chunk;
            page = 0;
        }
        total += chunk;
        /* The original copy count test is signed despite unsigned page
           arithmetic; retain its behavior for a wrapped/invalid count. */
        while ((int)chunk > 0) {
            *destination++ = *source++;
            --chunk;
        }
    } while (page);
    return (int)total;
}

/* Original 0x329a0..0x32a30, stdcall12. Only absolute seeks are
   supported. Final-page bounds and the offset comparison are signed;
   the original does not reject a negative absolute offset. */
long __stdcall seek_work_stack(WorkingStack *stack, long offset, int origin)
{
    unsigned char *page;
    long remaining = offset;
    int available;
    if (origin || !stack) return -1;
    reset_work_stack(stack);
    page = stack->read;
    while (page) {
        available = *(int *)(page + 2044);
        if (available) {
            if (available <= remaining) return -1;
            break;
        }
        if (remaining < 2040) break;
        page = *(unsigned char **)(page + 2040);
        stack->read = page;
        remaining -= 2040;
    }
    if (!page) return -1;
    stack->count = (unsigned)remaining;
    stack->next = page + remaining;
    return offset;
}

/* Original 0x32a40..0x32ac3, stdcall12/defined AX. Copy each
   published work-page payload to a FILE or another stack. A zero page length
   denotes a full 2040-byte page; the accumulated result wraps as a WORD. */
unsigned short __stdcall write_pch_events(WorkingStack *stack, FILE *file,
    WorkingStack *destination)
{
    unsigned short total = 0;
    unsigned bytes;
    unsigned char *page;
    reset_work_stack(stack);
    for (page = stack->read; page; page = *(unsigned char **)(page + 2040)) {
        bytes = *(unsigned *)(page + 2044);
        if (!bytes) bytes = 2040;
        if (file) fwrite(page, 1, (unsigned short)bytes, file);
        else if (destination) write_work_bytes(page, 1, (unsigned short)bytes, destination);
        total += (unsigned short)bytes;
    }
    return total;
}

/* Original 0x32ad0..0x32b49, stdcall12. Match the stack's page
   payloads, preserving two historical details: the expected pointer is not
   advanced between pages, and extra expected bytes do not prevent success. */
int __stdcall match_work_stack(WorkingStack *stack, void *text, unsigned bytes)
{
    unsigned char *page, *expected = text;
    unsigned declared, count, i;
    reset_work_stack(stack);
    for (page = stack->read; page; page = *(unsigned char **)(page + 2040)) {
        declared = *(unsigned *)(page + 2044);
        if (!declared) declared = 2040;
        count = (unsigned short)declared;
        if (count > bytes) return 0;
        for (i = 0; i < count; ++i)
            if (page[i] != expected[i]) return 0;
        bytes -= count;
    }
    return 1;
}

/* Original0x4eb00..0x4eb49, stdcall8. Compare identities in the
   vector's signed-short logical range, without dereferencing its elements. */
int __stdcall vector_contains(Vector *vector, void *item)
{
    short index;
    for (index = 0; index < vector->count; ++index)
        if (vector->items[index] == item) return 1;
    return 0;
}
