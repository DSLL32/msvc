#ifndef WORK_STACK_H
#define WORK_STACK_H
#include "initialization.h"
/* Segmented storage shared by source replay and initializer records. */
typedef struct WorkingBlock {
    unsigned char bytes[2040];
    struct WorkingBlock *next;
    unsigned used_bytes;
} WorkingBlock;
typedef char WorkingBlock_size[sizeof(WorkingBlock) == 2048 ? 1 : -1];
typedef char WorkingStack_size[sizeof(WorkingStack) == 16 ? 1 : -1];
long __stdcall seek_work_stack(WorkingStack *stack, long offset, int origin);
int __stdcall read_work_stack(WorkingStack *stack, void *buffer, unsigned bytes);
#endif
