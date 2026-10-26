/* Authored terminal helper unit; no original execution or CLI claim. */
#include "wasm.h"
#include <recon.h>
int terminal_setup(void)
{
    /* Real resource setup and an explicit unit banner precondition. */
    Options.banner_printed=1;
    return MsgInit("R:\\toolchains\\watcom10.0a\\.patched\\binb\\WASM.EXE") == 0;
}
int terminal_unit(void)
{
    DiskError();
    return 0; /* A returned nonreturning helper must fail the authored oracle. */
}
