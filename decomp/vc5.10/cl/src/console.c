/* Console control registration, 004057b4..004057c2. */
#include "cl.h"
#include <recon.h>

BOOL install_control_handler(void)
{
    return SetConsoleCtrlHandler(console_control_handler, TRUE);
}

/* 00405800..00405816: preprocessor output and diagnostics share stdout. */
void prepare_execution(void)
{
    if (keep_preprocessor_output)
        _dup2(1, 2);
}
