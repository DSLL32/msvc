/* Named INT3 frontiers; inline assembly is solely the required breakpoint primitive. */
#include "cl.h"
#include <recon.h>

BOOL WINAPI console_control_handler(DWORD event)
{
    __asm int 3
    return FALSE;
}
































void toggle_pass(Option *option) { __asm int 3 }










int console_rows(void) { __asm int 3
    return 0;
}
