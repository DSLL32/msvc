#include "compiler-state.h"
#include <recon.h>

/* The original receives this in ECX and one stack argument, with RET4.
 * VC5 C cannot express that member convention. This is the actual member
 * and its target caller, not an ABI wrapper or synthetic constructor. */
CompilerPool *RECON_THISCALL CompilerPool::Initialize(unsigned minimum_pages)
{
    __asm { int 3 }
    return this;
}

extern "C" void __cdecl initialize_state_126ef(void)
{
    c1_bss.pools[0].Initialize(1);
}
