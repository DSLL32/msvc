#include "c1xx.h"
#include <recon.h>

/* The lifecycle-only case resolves these exports without invoking them. */
void WINAPI AbortCompilerPass(DWORD reason)
{
    __asm { int 3 }
}

int WINAPI InvokeCompilerPass(int argc, char **argv, int mode)
{
    __asm { int 3 }
    return 0;
}
