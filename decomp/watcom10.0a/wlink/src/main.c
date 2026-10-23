#include <recon.h>

#if defined(__WATCOMC__)
/* Watcom 10.0a expands this intrinsic in place; no extra function vertex. */
void reconstruction_trap( void );
#pragma aux reconstruction_trap = 0xcc;
#endif

/* Initial application frontier, not an accepted implementation. Native formals
 * use GCC's ABI; the original Watcom build retains its register-call ABI. */
int main( int argc, char **argv )
{
    argc = argc;
    argv = argv;
#if defined(__WATCOMC__)
    reconstruction_trap();
#else
    __asm__ __volatile__( "int3" );
#endif
    return 0;
}
