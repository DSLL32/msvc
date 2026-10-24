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
    __asm { int 3 }
    return NULL;
}

CompilerPackingFrame *RECON_THISCALL CompilerPacking::Push(void)
{
    __asm { int 3 }
    return NULL;
}

extern "C" void __cdecl push_packing(unsigned alignment, const void *tag)
{
    __asm { int 3 }
}
