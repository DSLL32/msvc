/* C++ is confined to thiscall declarations and their actual caller. */
#include <windows.h>
#include <stdio.h>
#include "link.h"
#include "link-driver.h"
#include <recon.h>

extern "C" int __stdcall run_linker(int argc, char **argv)
{
    __asm { int 3 }
    return 255;
}
LinkContext *RECON_THISCALL LinkContext::Initialize(int vxd)
{
    __asm { int 3 }
    return this;
}
int RECON_THISCALL LinkContext::FullLink(int *argc)
{
    __asm { int 3 }
    return 255;
}
