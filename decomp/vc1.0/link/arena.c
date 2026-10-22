/* Zeroed allocation blocks chained for bulk release by the COMDAT owner. */
#include "linker-types.h"

void *allocate_arena(ArenaBlock **arena,unsigned long size)
{
    ArenaBlock *block=*arena;
    unsigned long capacity, i;
    char *data;
    void *result;
    if (!block || block->available < size) {
        capacity=size > 8192 ? size : 8192;
        block=malloc(sizeof(*block)+capacity);
        if (!block) out_of_memory();
        data=(char *)(block+1);
        for (i=0;i<capacity;++i) data[i]=0;
        block->used=0;
        block->available=capacity;
        block->next=*arena;
        *arena=block;
    }
    result=(char *)(block+1)+block->used;
    block->used+=size;
    block->available-=size;
    return result;
}

/* Original [000219b0,000219d0). Shared checked heap allocation. */
void *allocate_checked(unsigned long size)
{
    void *memory=malloc(size);
    if (!memory) out_of_memory();
    return memory;
}

/* Small symbol/hash nodes share this retained allocation block. */
unsigned long pool_remaining,pool_capacity=4096,pool_saved_remaining;
char *pool_block;

void *allocate_pool_block(unsigned long size)
{
    if (pool_block) realloc(pool_block,pool_capacity-pool_remaining);
    pool_block=calloc(pool_capacity,1);
    if (!pool_block) out_of_memory();
    pool_remaining=pool_capacity-size;
    return pool_block;
}

void free_pointer(void **pointer)
{
    if (*pointer) {
        free(*pointer);
        *pointer=NULL;
    }
}
