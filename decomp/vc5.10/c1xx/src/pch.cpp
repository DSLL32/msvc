#include "frontend.h"
#include <recon.h>

PchContext *RECON_THISCALL PchContext::Initialize(void)
{
    __asm { int 3 }
    return this;
}

extern "C" void initialize_frontend_37(void)
{
    frontend_bss.pch_context.Initialize();
}
