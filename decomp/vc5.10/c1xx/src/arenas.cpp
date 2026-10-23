#include "frontend.h"
#include <recon.h>

const ArenaDescriptor arena_descriptors[9] = {
    { 0, 0x3200000, 0x10000, 0x21, 1 },
    { 1, 0xa00000, 0x8000, 0x10, 4 },
    { 2, 0x500000, 0x8000, 0x18, 4 },
    { 3, 0xa00000, 0x8000, 0x18, 4 },
    { 4, 0xa00000, 0x8000, 0x18, 4 },
    { 5, 0x500000, 0x4000, 0x18, 4 },
    { 6, 0x500000, 0x8000, 0x18, 4 },
    { 7, 0x500000, 0x8000, 0x18, 4 },
    { 8, 0x500000, 0x8000, 0x18, 4 }
};

extern "C" void initialize_compiler_environment(void)
{
    unsigned i;
    initialize_default_arena();
    for (i = 0; i < 9; ++i) {
        if (!(arena_descriptors[i].flags & 1)) {
            if (!frontend_bss.arenas[i].Reserve(&arena_descriptors[i], NULL))
                report_compiler_error(60);
        }
    }
    initialize_pool_arenas();
}

extern "C" void initialize_default_arena(void)
{
    unsigned i;
    for (i = 0; i < 9; ++i) {
        if (arena_descriptors[i].flags & 1) {
            if (!frontend_bss.arenas[i].Reserve(&arena_descriptors[i], NULL))
                report_compiler_error(60);
        }
    }
}

int RECON_THISCALL CompilerArena::Reserve(const ArenaDescriptor *descriptor,
        BYTE *address)
{
    __asm { int 3 }
    return 0;
}

extern "C" void initialize_pool_arenas(void)
{
    __asm { int 3 }
}
