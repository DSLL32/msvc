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
