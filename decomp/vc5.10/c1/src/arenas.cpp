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
    __asm { int 3 }
    return 0;
}

void __cdecl initialize_default_pools(void)
{
    __asm { int 3 }
}
