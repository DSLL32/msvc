/* Runtime variants in RCPP.EXE; these differ from the shipped LIBC.LIB. */
#include "rcpp.h"
#include <recon.h>
HANDLE rc_process_heap;
/* Original [010089f0,010089fb); process heap shared by the allocation routines. */
void rc_heap_initialize(void)
{
    rc_process_heap = GetProcessHeap();
}

/* Original [01008960,010089a0). Unlike the shipped region/block allocator,
 * this runtime allocates directly from the Windows process heap. */
int (__cdecl *rc_new_handler)(unsigned);
void *rc_malloc(unsigned size)
{
    void *memory;
    if (!size) size = 1;
    do {
        memory = HeapAlloc(rc_process_heap, 0, size);
        if (memory || !rc_new_handler) return memory;
    } while (rc_new_handler(size));
    return memory;
}

/* Original [010089a0,010089b3). All linked stdio buffers and native allocations
 * share the process heap; a later LIBC region-heap free cannot release them. */
void free(void *memory)
{
    HeapFree(rc_process_heap, 0, memory);
}
