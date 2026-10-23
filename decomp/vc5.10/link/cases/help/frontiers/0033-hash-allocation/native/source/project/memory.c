#include <windows.h>
#include <string.h>
#include "files.h"
#include "memory.h"
#include "link.h"
#include <recon.h>

DWORD allocation_page_size, maximum_physical_memory;

void __cdecl initialize_memory(void)
{
    SYSTEM_INFO system;
    MEMORYSTATUS memory;
    if (!allocation_page_size) {
        GetSystemInfo(&system);
        allocation_page_size = system.dwPageSize;
    }
    GlobalMemoryStatus(&memory);
    maximum_physical_memory = 0x04000000;
    if (memory.dwTotalPhys >= 0x04000000)
        maximum_physical_memory = memory.dwTotalPhys;
}

extern BYTE incremental_enabled;
void *__stdcall allocate_incremental(unsigned);

#pragma intrinsic(memset)
void *__stdcall allocate_link_memory(unsigned count,unsigned size)
{
    unsigned bytes=count*size;
    void *memory;
    if (!incremental_enabled) return zero_allocate(bytes);
    memory=allocate_incremental(bytes);
    memset(memory,0,bytes);
    return memory;
}
void *__stdcall allocate_incremental(unsigned bytes) { __asm { int 3 } return NULL; }

void *__stdcall allocate_memory(unsigned bytes) { __asm { int 3 } return NULL; }

void *__stdcall reserve_memory(MemoryArena **arena,unsigned bytes)
{ __asm { int 3 } return NULL; }
