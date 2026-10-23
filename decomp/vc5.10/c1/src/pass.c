#include "c1.h"
#include <recon.h>

/* Named next frontiers; no pass implementation accepted yet. */
int __cdecl initialize_compiler_pass(void)
{
    __asm { int 3 }
    return 1;
}

int __cdecl main(int argc, char **argv)
{
    __asm { int 3 }
    return 1;
}

int __cdecl cleanup_compiler_pass(void)
{
    __asm { int 3 }
    return 1;
}
