#include <recon.h>

/* Initial application frontier, not an accepted implementation. Native formals
 * use GCC's ABI; the original Watcom build retains its register-call ABI. */
int main( int argc, char **argv )
{
    argc = argc;
    argv = argv;
#if defined(__WATCOMC__)
    __asm { int 3 }
#else
    __asm__ __volatile__( "int3" );
#endif
    return 0;
}
