/* Dormant CRT callback, still unimplemented and unavailable. */
#include "c2-crt.h"
#include <recon.h>

int c2_math_error(struct _exception *exception)
{
    __asm { int 3 }
    return 0;
}
