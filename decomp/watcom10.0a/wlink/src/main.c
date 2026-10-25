/* Fresh standardized reconstruction: genuine main at original VA4242e0.
 * No previous implementation or acceptance credit is carried into this stub. */
#include "link.h"
#include <recon.h>

#if defined(__WATCOMC__)
void reconstruction_trap( void );
#pragma aux reconstruction_trap = 0xcc;
#define TRAP() reconstruction_trap()
#else
#define TRAP() __asm__ __volatile__( "int3" )
#endif

int main( int argc, char **argv )
{
    argc = argc;
    argv = argv;
    TRAP();
    return 0;
}
