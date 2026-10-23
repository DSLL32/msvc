#include <windows.h>
#include <string.h>
#include <stdlib.h>
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

void *__stdcall allocate_memory(unsigned bytes)
{
    void *memory=malloc(bytes);
    if (!memory) out_of_memory();
    return memory;
}

void *__stdcall reserve_memory(MemoryArena **arena,unsigned bytes)
{
    MemoryArena *fresh;
    if (!*arena || (*arena)->remaining<bytes) {
        fresh=create_memory_arena((bytes/allocation_page_size+1)*allocation_page_size);
        if (*arena) fresh->previous=*arena;
        *arena=fresh;
    }
    (*arena)->used+=bytes;
    (*arena)->remaining-=bytes;
    return (*arena)->base+(*arena)->used-bytes;
}
MemoryArena *__stdcall create_memory_arena(unsigned bytes)
{
    MemoryArena *arena=(MemoryArena *)allocate_memory(sizeof(MemoryArena));
    arena->base=(BYTE *)VirtualAlloc(NULL,bytes,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);
    if (!arena->base) out_of_memory();
    arena->remaining=bytes;
    arena->used=0;
    arena->previous=NULL;
    return arena;
}

void *__stdcall allocate_virtual_memory(void *address,unsigned bytes,DWORD flags,DWORD protection)
{
    __asm { int 3 }
    return NULL;
}
