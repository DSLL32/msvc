/* RC's CRT predates the region allocator shipped in LIBC.LIB. */
#include "driver.h"
#include <stdlib.h>
HANDLE resource_heap;
int (__cdecl *_pnhHeap)(unsigned int);
void __cdecl _heap_init(void)
{
    resource_heap = GetProcessHeap();
}

/* Original malloc 0x0100f2f0..0x0100f330. */
void * __cdecl malloc(unsigned int size)
{
    void *block;
    if (!size)
        size = 1;
    do {
        block = HeapAlloc(resource_heap, 0, size);
        if (block || !_pnhHeap)
            break;
    } while ((*_pnhHeap)(size));
    return block;
}

void __cdecl free(void *block)
{
    HeapFree(resource_heap, 0, block);
}
void * __cdecl calloc(unsigned int count, unsigned int size)
{
    void *block;
    unsigned int bytes = count * size;
    if (!bytes)
        bytes = 1;
    do {
        block = HeapAlloc(resource_heap, HEAP_ZERO_MEMORY, bytes);
        if (block || !_pnhHeap)
            break;
    } while ((*_pnhHeap)(bytes));
    return block;
}

/* Checked resource allocation, original 0x01007000..0x01007026. */
void * __stdcall resource_alloc(unsigned int bytes)
{
    void *block = calloc(1, bytes);
    if (!block)
        resource_finish("RC : fatal error RW1020: out of memory, needed %d bytes", bytes);
    return block;
}

/* Original 0x01007030..0x01007046. Returning NULL lets callers clear a
 * released resource pointer with the same wrapper. */
void * __stdcall resource_free(void *block)
{
    if (block)
        free(block);
    return 0;
}
