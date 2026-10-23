#include "compiler-state.h"
#include <recon.h>

CompilerArenaDescriptor c1_arena_descriptors[6] = {
    { 0, 0x3200000, 0x10000, 0x21, 1 },
    { 1, 0xa00000,  0x8000,  0x10, 4 },
    { 2, 0xa00000,  0x8000,  0x18, 4 },
    { 3, 0x500000,  0x4000,  0x18, 4 },
    { 4, 0x500000,  0x8000,  0x18, 4 },
    { 5, 0x500000,  0x8000,  0x18, 4 }
};

/* Early reservations precede the remaining five descriptor-controlled arenas. */
void __cdecl initialize_compiler_arenas(void)
{
    unsigned index;
    initialize_primary_arenas();
    for (index = 0; index < 6; ++index) {
        if (!(c1_arena_descriptors[index].flags & 1)) {
            if (!c1_bss.arenas[index].Initialize(&c1_arena_descriptors[index], NULL))
                compiler_fatal(60);
        }
    }
    initialize_default_pools();
}

void __cdecl initialize_primary_arenas(void)
{
    unsigned index;
    for (index = 0; index < 6; ++index) {
        if (c1_arena_descriptors[index].flags & 1) {
            if (!c1_bss.arenas[index].Initialize(&c1_arena_descriptors[index], NULL))
                compiler_fatal(60);
        }
    }
}

int RECON_THISCALL CompilerArena::Initialize(CompilerArenaDescriptor *descriptor,
                                          void *address)
{
    this->descriptor = descriptor;
    reserve_bytes = arena_reservation_bytes(descriptor);
    base = (BYTE *)VirtualAlloc(address, reserve_bytes,
                               MEM_RESERVE | ((descriptor->flags & 1) ? MEM_TOP_DOWN : 0),
                               PAGE_READWRITE);
    if (!base)
        return 0;
    mapped_view = 0;
    Reset();
    return 1;
}

/* The compiler's _ftol conversion produces a signed QWORD. The caller uses
 * only its low DWORD before rounding the reservation to the commit unit. */
unsigned __cdecl arena_reservation_bytes(CompilerArenaDescriptor *descriptor)
{
    __int64 converted;
    unsigned unit = descriptor->commit_unit;
    unsigned size;

    converted = (__int64)((double)descriptor->reserve_bytes *
                          c1_data.memory_percent * -0.01);
    size = (unit - (unsigned)converted - 1) & ~(unit - 1);
    return size ? size : unit;
}


void RECON_THISCALL CompilerArena::Reset(void)
{
    committed_bytes = 0;
    cursor = base;
    committed_limit = base;
    reset_value = 0;
}

void __cdecl initialize_default_pools(void)
{
    __asm { int 3 }
}
