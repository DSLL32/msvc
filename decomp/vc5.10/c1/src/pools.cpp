#include "compiler-state.h"
#include <recon.h>

/* The original receives this in ECX and one stack argument, with RET4.
 * VC5 C cannot express that member convention. This is the actual member
 * and its target caller, not an ABI wrapper or synthetic constructor. */
CompilerPool *RECON_THISCALL CompilerPool::Initialize(unsigned minimum_pages)
{
    cursor = NULL;
    remaining = 0;
    this->minimum_pages = minimum_pages;
    arena = NULL;
    return this;
}

extern "C" void __cdecl initialize_state_126ef(void)
{
    c1_bss.pools[0].Initialize(1);
}

extern "C" void __cdecl initialize_state_12712(void)
{
    c1_bss.pools[1].Initialize(1);
}

extern "C" void __cdecl initialize_state_1271f(void)
{
    c1_bss.pools[2].Initialize(1);
}

extern "C" void __cdecl initialize_state_1272c(void)
{
    c1_bss.pools[3].Initialize(1);
}

extern "C" void __cdecl initialize_state_12739(void)
{
    c1_bss.pools[4].Initialize(1);
}

extern "C" void __cdecl initialize_state_12746(void)
{
    c1_bss.pools[5].Initialize(1);
}

extern "C" void __cdecl initialize_state_12753(void)
{
    c1_bss.pools[6].Initialize(1);
}

extern "C" void __cdecl initialize_state_12760(void)
{
    c1_bss.pools[7].Initialize(1);
}

extern "C" void __cdecl initialize_state_1276d(void)
{
    c1_bss.pools[8].Initialize(1);
}

/* The observed default order differs from static initializer order. */
extern "C" void __cdecl initialize_default_pools(void)
{
    c1_bss.pools[8].SetArena(0);
    c1_bss.pools[7].SetArena(0);
    c1_bss.pools[0].SetArena(0);
    c1_bss.pools[1].SetArena(0);
    c1_bss.pools[2].SetArena(0);
    c1_bss.pools[3].SetArena(0);
    c1_bss.pools[4].SetArena(0);
    c1_bss.pools[5].SetArena(0);
    c1_bss.pools[6].SetArena(0);
}

void RECON_THISCALL CompilerPool::SetArena(unsigned arena_index)
{
    arena = &c1_bss.arenas[arena_index];
    Refill(1);
}

unsigned char *RECON_THISCALL CompilerPool::Refill(unsigned minimum_bytes)
{
    unsigned pages;

    pages = (minimum_bytes - 1) / c1_data.system_info.dwPageSize + 1;
    if (minimum_pages > pages)
        pages = minimum_pages;
    cursor = arena->AllocatePages(pages, &remaining);
    return cursor;
}


/* This C-ABI allocator calls the existing genuine thiscall arena member;
 * compiling the actual function as C++ preserves that call without a wrapper.
 * The count slot is consumed as a signed WORD, as stored in the header. */
extern "C" CompilerVector *__cdecl new_option_vector(unsigned count, int mode)
{
    CompilerArena *arena = &c1_bss.arenas[mode];
    CompilerVector *vector = (CompilerVector *)arena->cursor;
    unsigned char *items;
    int bytes;

    arena->cursor += sizeof(CompilerVector);
    if (arena->cursor > arena->committed_limit)
        arena->CommitExtent();
    vector->count = 0;
    vector->capacity = (short)count;
    vector->arena_kind = mode;
    bytes = (int)vector->capacity * sizeof(void *);
    if (bytes) {
        items = arena->cursor;
        arena->cursor += (bytes + 3) & ~3;
        if (arena->cursor > arena->committed_limit)
            arena->CommitExtent();
        vector->items = (void **)items;
    }
    return vector;
}
