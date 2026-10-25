/* Narrow projection regression for the newly encountered exception counters. */
#include "frontend.h"
#include <recon.h>

int c1xx_test_exception_counters(void)
{
    if (frontend_data.recursion_depth || frontend_bss.error_count)
        return 0;
    if (initialize_compiler_pass())
        return 0;
    frontend_data.recursion_depth = 17;
    frontend_bss.error_count = 23;
    if (initialize_compiler_pass())
        return 0;
    return frontend_data.recursion_depth == 0 && frontend_bss.error_count == 0;
}
