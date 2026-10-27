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

/* C linkage is the original caller ABI. C++ expresses its genuine thiscall
 * CommitExtent edge. The allocated root is not cleared here: its zero link
 * comes from the earlier committed arena pages. */
void __cdecl initialize_include_aliases(void)
{
    BYTE *root = c1_bss.arenas[0].cursor;
    c1_bss.arenas[0].cursor += sizeof(CompilerIncludeAliases);
    if (c1_bss.arenas[0].cursor > c1_bss.arenas[0].committed_limit)
        c1_bss.arenas[0].CommitExtent();
    c1_data.options.include_aliases = (CompilerIncludeAliases *)root;
}

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

unsigned char *RECON_THISCALL CompilerArena::AllocatePages(unsigned pages, unsigned *available_bytes)
{
    unsigned char *old_cursor;
    unsigned offset, page_size;

    old_cursor = cursor;
    offset = (unsigned)cursor - (unsigned)base;
    page_size = c1_data.system_info.dwPageSize;
    *available_bytes = (((pages + 1) * page_size + offset - 1) &
                        ~(page_size - 1)) - offset;
    cursor += *available_bytes;
    if (cursor > committed_limit)
        CommitExtent();
    return old_cursor;
}

void RECON_THISCALL CompilerArena::CommitExtent(void)
{
    unsigned bytes, unit;

    if ((int)((unsigned)cursor - (unsigned)base) > (int)reserve_bytes)
        compiler_fatal(76);
    unit = descriptor->commit_unit;
    bytes = ((unsigned)cursor - (unsigned)committed_limit + unit - 1) &
            ~(unit - 1);
    if (!VirtualAlloc(committed_limit, bytes, MEM_COMMIT, PAGE_READWRITE))
        compiler_fatal(60);
    committed_limit += bytes;
    committed_bytes += bytes;
}

BYTE *RECON_THISCALL CompilerArena::AllocateBytes(unsigned bytes)
{
    BYTE *memory = cursor;
    cursor += bytes;
    if (cursor > committed_limit)
        CommitExtent();
    return memory;
}

extern "C" const char *__cdecl get_compiler_message(unsigned);
extern "C" int __cdecl format_buffer(char *, const char *, unsigned, ...);
extern "C" void __cdecl print_pragma_message(const char *);
void RECON_THISCALL CompilerArena::Release(void)
{
    char message[256];
    int released;
    if (!base) return;
    if ((int)committed_bytes > 0 && !VirtualFree(base, committed_bytes, MEM_DECOMMIT)) {
        format_buffer(message, get_compiler_message(910), 256, GetLastError());
        print_pragma_message(message);
    }
    if (mapped_view) released = UnmapViewOfFile(base);
    else released = VirtualFree(base, 0, MEM_RELEASE);
    if (!released) {
        format_buffer(message, get_compiler_message(910), 256, GetLastError());
        print_pragma_message(message);
    }
    base = NULL;
}
