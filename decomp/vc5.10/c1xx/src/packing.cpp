#include <stddef.h>
#include "frontend.h"
#include "packing.h"
#include <recon.h>

typedef char packing_stack_layout[(sizeof(CompilerPacking) == 8 &&
    offsetof(CompilerPacking, available) == 4) ? 1 : -1];
typedef char packing_frame_layout[(sizeof(PackingFrame) == 12 &&
    offsetof(PackingFrame, alignment) == 4 &&
    offsetof(PackingFrame, identifier) == 8) ? 1 : -1];

extern "C" BYTE *allocate_packing_storage(unsigned bytes);
extern "C" PackingFrame *push_packing(unsigned alignment, void *identifier);

extern "C" void initialize_packing(void)
{
    PackingFrame *frame;
    unsigned alignment;

    frontend_data.options.packing_stack =
        (CompilerPacking *)allocate_packing_storage(sizeof(CompilerPacking));
    frame = frontend_data.options.packing_stack->AcquireFrame();
    alignment = frontend_data.options.packing_alignment == -1 ?
        8 : frontend_data.options.packing_alignment;
    frame->alignment = alignment;
    frame->identifier = NULL;
    push_packing(0, NULL);
}

extern "C" BYTE *allocate_packing_storage(unsigned bytes)
{
    CompilerArena *arena = &frontend_bss.arenas[0];
    BYTE *result = arena->cursor;

    arena->cursor += (bytes + 3) & ~3U;
    if (arena->cursor > arena->committed_limit)
        arena->CommitExtent();
    return result;
}

PackingFrame *RECON_THISCALL CompilerPacking::AcquireFrame(void)
{
    __asm { int 3 }
    return NULL;
}

extern "C" PackingFrame *push_packing(unsigned alignment, void *identifier)
{
    __asm { int 3 }
    return NULL;
}
