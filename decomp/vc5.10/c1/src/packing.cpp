#include "compiler-state.h"
#include "packing.h"
#include <recon.h>

/* The original initializer calls a genuine ECX-receiver member. Compile the
 * actual caller and member as C++, without implicit construction or wrappers. */
extern "C" void __cdecl initialize_packing(void)
{
    CompilerPackingFrame *frame;
    unsigned alignment;

    c1_data.options.packing_stack =
        (CompilerPacking *)allocate_packing_memory(sizeof(CompilerPacking));
    frame = c1_data.options.packing_stack->Push();
    alignment = c1_data.options.packing_alignment;
    if (alignment == (unsigned)-1)
        alignment = 8;
    frame->alignment = alignment;
    frame->tag = NULL;
    push_packing(0, NULL);
}

extern "C" void *__cdecl allocate_packing_memory(unsigned bytes)
{
    CompilerArena *arena = &c1_bss.arenas[0];
    BYTE *memory = arena->cursor;

    arena->cursor += (bytes + 3) & ~3U;
    if (arena->cursor > arena->committed_limit)
        arena->CommitExtent();
    return memory;
}

CompilerPackingFrame *RECON_THISCALL CompilerPacking::Push(void)
{
    CompilerPackingFrame *frame = free_frames;

    if (frame == NULL) {
        CompilerArena *arena = &c1_bss.arenas[0];
        frame = (CompilerPackingFrame *)arena->cursor;
        arena->cursor += sizeof(CompilerPackingFrame);
        if (arena->cursor > arena->committed_limit)
            arena->CommitExtent();
    } else {
        free_frames = frame->previous;
        clear_packing_frame(frame);
    }
    frame->previous = active;
    active = frame;
    return frame;
}

extern "C" void __cdecl push_packing(unsigned alignment, const void *tag)
{
    __asm { int 3 }
}

extern "C" void __cdecl clear_packing_frame(CompilerPackingFrame *frame)
{
    __asm { int 3 }
}
