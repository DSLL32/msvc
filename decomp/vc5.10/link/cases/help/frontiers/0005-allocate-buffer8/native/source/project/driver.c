#include <windows.h>
#include <recon.h>

/* Original target main, preferred VA 0040de90. Unreconstructed. */
int __cdecl main(int argc, char **argv, char **environment)
{
    /* Named unreconstructed frontier: INT3 is intentional. */
    __asm { int 3 }
    return 255;
}
