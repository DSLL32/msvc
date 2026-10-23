#include "c2.h"
#include <recon.h>

/* All node variants share the link and kind; their remaining fields differ. */
typedef struct C2NodeHeader {
    struct C2NodeHeader *next;
    signed char kind;
} C2NodeHeader;

unsigned long c2_node_bytes[4];
C2Pool c2_pools[4];
C2PoolBlock *c2_free_blocks;

void *__fastcall c2_pool_allocate(int pool, unsigned bytes)
{
    unsigned rounded = (bytes + 3) & ~3U;
    C2Pool *arena = &c2_pools[pool];
    C2PoolBlock *block;
    unsigned char *result;

    if (rounded <= 4088) {
        if ((int)rounded > arena->remaining) {
            block = c2_free_blocks;
            if (block)
                c2_free_blocks = block->next;
            else {
                block = malloc(4096);
                if (!block)
                    c2_fatal(2);
                block->capacity = 4088;
            }
            block->next = arena->blocks;
            arena->blocks = block;
            arena->remaining = block->capacity;
            arena->cursor = block->data;
            memset(block->data, 0, block->capacity);
        }
        result = arena->cursor;
        arena->cursor += rounded;
        arena->remaining -= rounded;
        return result;
    }

    block = malloc(rounded + 8);
    if (!block)
        c2_fatal(2);
    if (arena->blocks) {
        block->next = arena->blocks->next;
        arena->blocks->next = block;
    } else {
        arena->blocks = block;
        block->next = NULL;
    }
    block->capacity = rounded;
    arena->remaining = 0;
    result = block->data;
    memset(result, 0, rounded);
    return result;
}

unsigned __fastcall c2_node_size(int kind)
{
    switch (kind) {
    case 1: case 3: case 11:
        return 64;
    case 2: case 7:
        return 68;
    case 4: case 9:
        return 86;
    case 5: case 8:
        return 70;
    case 14:
        return 110;
    case 16:
        return 90;
    default:
        c2_report_internal("E:\\utc\\src\\\\P2\\p2symtab.c", 1514);
        return 0; /* The internal diagnostic terminates the process. */
    }
}

void *__fastcall c2_allocate_node(signed char kind, int pool)
{
    unsigned bytes = c2_node_size(kind);
    C2NodeHeader *node;

    c2_node_bytes[pool] += bytes;
    node = c2_pool_allocate(pool, bytes);
    node->kind = kind;
    return node;
}
