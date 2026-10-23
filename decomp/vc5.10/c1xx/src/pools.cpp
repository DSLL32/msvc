#include "frontend.h"
#include <recon.h>

/* The actual target member receives this in ECX and one stack argument,
 * returns this in EAX, and pops the argument with RET4. VC5 C cannot express
 * this convention; this member and its real caller therefore use C++. */
CompilerPool *RECON_THISCALL CompilerPool::Initialize(unsigned minimum_pages)
{
    __asm { int 3 }
    return this;
}

extern "C" void initialize_frontend_24(void)
{
    frontend_bss.pools[0].Initialize(1);
}
