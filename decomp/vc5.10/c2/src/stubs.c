#include "c2.h"
#include <recon.h>

/* Unknown child functions remain named trapping frontiers. */
void c2_set_argv(void)
{
    __asm { int 3 }
}

void c2_set_default_precision(void)
{
    __asm { int 3 }
}

int c2_math_error(struct _exception *exception)
{
    __asm { int 3 }
    return 0;
}

int c2_main(int argc, char **argv, char **environment)
{
    __asm { int 3 }
    return 0;
}
