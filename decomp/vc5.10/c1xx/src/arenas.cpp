#include "frontend.h"
#include <recon.h>

BYTE *RECON_THISCALL CompilerArena::Allocate(unsigned bytes)
{
    BYTE *memory = cursor;
    cursor += bytes;
    if (cursor > committed_limit) CommitExtent();
    return memory;
}

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
    this->descriptor = descriptor;
    reserve_bytes = calculate_arena_reservation(descriptor);
    base = (BYTE *)VirtualAlloc(address, reserve_bytes,
            MEM_RESERVE | ((descriptor->flags & 1) ? MEM_TOP_DOWN : 0),
            PAGE_READWRITE);
    if (!base)
        return 0;
    mapped_view = 0;
    Reset();
    return 1;
}

void RECON_THISCALL CompilerArena::Reset(void)
{
    committed_bytes = 0;
    cursor = base;
    committed_limit = base;
    reset_value = 0;
}

/* The existing compiler CRT conversion returns a signed QWORD. The caller
 * consumes its low DWORD and rounds upward to this arena's commit unit. */
extern "C" unsigned calculate_arena_reservation(const ArenaDescriptor *descriptor)
{
    __int64 converted;
    unsigned unit = descriptor->commit_unit;
    unsigned bytes;
    converted = (__int64)((double)descriptor->reserve_bytes *
            frontend_data.option_zm_percent * -0.01);
    bytes = (unit - (unsigned)converted - 1) & ~(unit - 1);
    return bytes ? bytes : unit;
}

extern "C" void initialize_pool_arenas(void)
{
    frontend_bss.pools[9].BindArena(0);
    frontend_bss.pools[8].BindArena(0);
    frontend_bss.pools[0].BindArena(0);
    frontend_bss.pools[1].BindArena(0);
    frontend_bss.pools[3].BindArena(0);
    frontend_bss.pools[4].BindArena(0);
    frontend_bss.pools[5].BindArena(0);
    frontend_bss.pools[2].BindArena(0);
    frontend_bss.pools[6].BindArena(0);
    frontend_bss.pools[7].BindArena(0);
}

unsigned char *RECON_THISCALL CompilerArena::AllocatePages(unsigned pages,
        unsigned *remaining_bytes)
{
    unsigned page_bytes = frontend_data.system_info.dwPageSize;
    unsigned offset = (unsigned)(cursor - base);
    unsigned available;
    BYTE *result = cursor;
    available = (((pages + 1) * page_bytes + offset - 1) &
            ~(page_bytes - 1)) - offset;
    *remaining_bytes = available;
    cursor += available;
    if (cursor > committed_limit)
        CommitExtent();
    return result;
}

void RECON_THISCALL CompilerArena::CommitExtent(void)
{
    unsigned bytes, unit;
    if ((int)((unsigned)cursor - (unsigned)base) > (int)reserve_bytes)
        report_compiler_error(76);
    unit = descriptor->commit_unit;
    bytes = ((unsigned)cursor - (unsigned)committed_limit + unit - 1) &
            ~(unit - 1);
    if (!VirtualAlloc(committed_limit, bytes, MEM_COMMIT, PAGE_READWRITE))
        report_compiler_error(60);
    committed_limit += bytes;
    committed_bytes += bytes;
}

/* The four-byte root is allocated here, but its contents are not initialized
 * or interpreted until a later alias consumer. */
extern "C" void initialize_include_aliases(void)
{
    CompilerArena *arena = &frontend_bss.arenas[0];
    unsigned char *root = arena->cursor;
    arena->cursor += 4;
    if (arena->cursor > arena->committed_limit)
        arena->CommitExtent();
    frontend_data.options.include_aliases = (CompilerIncludeAliases *)root;
}
