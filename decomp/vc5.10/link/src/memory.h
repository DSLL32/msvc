#ifndef LINK_MEMORY_H
#define LINK_MEMORY_H
#include <windows.h>
#include <recon.h>
#ifdef __cplusplus
extern "C" {
#endif

typedef struct MemoryPool {
    struct MemoryPool *previous;
    unsigned used,remaining;
} MemoryPool;
typedef char MemoryPoolHeaderSize[(sizeof(MemoryPool)==12)?1:-1];
typedef struct MemoryArena {
    struct MemoryArena *previous;
    unsigned used, remaining;
    BYTE *base;
} MemoryArena;
void __stdcall release_memory_pool(MemoryPool **);
void __stdcall release_memory_arena(MemoryArena *);
void *__stdcall allocate_link_memory(unsigned,unsigned);
void *__stdcall allocate_memory(unsigned);
void *__stdcall reallocate_memory(void *,unsigned);
void *__stdcall allocate_virtual_memory(void *,unsigned,DWORD,DWORD);
void *__stdcall allocate_incremental(unsigned);
void *__stdcall reserve_memory(MemoryArena **,unsigned);
MemoryArena *__stdcall create_memory_arena(unsigned);
typedef char MemoryArenaSize[(sizeof(MemoryArena)==16)?1:-1];

#ifdef __cplusplus
}
#endif
#endif
