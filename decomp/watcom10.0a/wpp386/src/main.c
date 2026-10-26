/* WPP386 10.0a main, CF0x43fef..0x44022. The released 2002 DLL driver
 * has a different graph; only names/roles are correlated to that reference. */
#include "compiler.h"
#include <recon.h>

/* Original symbol for CF0xb905c remains unknown. Its initial DWORD and
 * main's explicit assignment are both16; no surrounding data is copied. */
unsigned StartupWord = 16;

int main(int argc, char **argv)
{
    int status;

    (void)argc;
    (void)argv;
    CtxSetContext(0);
    StartupWord = 16;
    ExitPointAcquireRtn(&ExitPoint_program);
    status = front_end(0);
    ExitPointReleaseRtn(&ExitPoint_program);
    return status;
}
