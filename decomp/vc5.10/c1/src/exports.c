#include "c1.h"
#include <recon.h>

/* The lifecycle case resolves these exports without invoking them. */
int WINAPI InvokeCompilerPass(int argc, char **argv, int mode)
{
    __asm { int 3 }
    return 1;
}

void WINAPI AbortCompilerPass(int reason)
{
    __asm { int 3 }
}
