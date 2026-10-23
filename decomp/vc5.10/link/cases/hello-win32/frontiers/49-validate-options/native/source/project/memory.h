#ifndef LINK_MEMORY_H
#define LINK_MEMORY_H
#include <windows.h>
#include <recon.h>
#ifdef __cplusplus
extern "C" {
#endif

typedef struct MemoryArena {
    struct MemoryArena *previous;
    unsigned used, remaining;
    BYTE *base;
} MemoryArena;
void *__stdcall allocate_link_memory(unsigned,unsigned);
void *__stdcall allocate_memory(unsigned);
void *__stdcall reserve_memory(MemoryArena **,unsigned);
MemoryArena *__stdcall create_memory_arena(unsigned);
typedef char MemoryArenaSize[(sizeof(MemoryArena)==16)?1:-1];

#ifdef __cplusplus
}
#endif
#endif
