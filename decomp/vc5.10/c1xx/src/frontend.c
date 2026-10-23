#include "c1xx.h"
#include <recon.h>

int initialize_compiler_pass(void)
{
    __asm { int 3 }
    return 0;
}

int main(int argc, char **argv)
{
    __asm { int 3 }
    return 0;
}

int cleanup_compiler_pass(void)
{
    __asm { int 3 }
    return 0;
}
