#include "frontend.h"
#include <recon.h>

/* The actual target member receives this in ECX and one stack argument,
 * returns this in EAX, and pops the argument with RET4. VC5 C cannot express
 * this convention; this member and its real caller therefore use C++. */
CompilerPool *RECON_THISCALL CompilerPool::Initialize(unsigned minimum_pages)
{
    cursor = NULL;
    remaining = 0;
    this->minimum_pages = minimum_pages;
    arena = NULL;
    return this;
}

extern "C" void initialize_frontend_24(void)
{
    frontend_bss.pools[0].Initialize(1);
}

extern "C" void initialize_frontend_25(void)
{
    frontend_bss.pools[1].Initialize(1);
}

extern "C" void initialize_frontend_26(void)
{
    frontend_bss.pools[2].Initialize(1);
}

extern "C" void initialize_frontend_27(void)
{
    frontend_bss.pools[3].Initialize(1);
}

extern "C" void initialize_frontend_28(void)
{
    frontend_bss.pools[4].Initialize(1);
}

extern "C" void initialize_frontend_29(void)
{
    frontend_bss.pools[5].Initialize(1);
}

extern "C" void initialize_frontend_30(void)
{
    frontend_bss.pools[6].Initialize(1);
}

extern "C" void initialize_frontend_31(void)
{
    frontend_bss.pools[7].Initialize(1);
}

extern "C" void initialize_frontend_32(void)
{
    frontend_bss.pools[8].Initialize(1);
}
