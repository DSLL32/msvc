/* Console control registration, 004057b4..004057c2. */
#include "cl.h"
#include <recon.h>

BOOL install_control_handler(void)
{
    return SetConsoleCtrlHandler(console_control_handler, TRUE);
}
