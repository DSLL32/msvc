#include "output.h"
#include <recon.h>

void initialize_output_buffer_01(void)
{
    __asm { int 3 }
}

void destroy_output_buffer_01(void)
{
    __asm { int 3 }
}
