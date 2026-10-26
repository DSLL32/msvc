/* Actual CRT malloc/_nmalloc segment wrapper CF0x6c6ef..0x6c6fd:
 * save ES, set ES=DS, call private App MemAlloc, restore ES, RET0.
 * Win32/native flat addresses need no selector adaptation. This justified
 * compatibility wrapper retains its real App call; it is not a libc allocator.
 * Keep its name distinct from host malloc to avoid process-wide interposition. */
#include "memory.h"
#include <recon.h>

void *malloc_segment_wrapper(unsigned amount)
{
    return MemAlloc(amount);
}

/* Original free wrapper6c6fd..6c70b saves ES, loads ES from DS and calls
 * the compiler's genuine App MemFree. The flat native/Win32 bridge retains
 * that callback and pointer ownership; host free must never receive it. */
void free_segment_wrapper(void *ptr)
{
    MemFree((char *)ptr);
}
