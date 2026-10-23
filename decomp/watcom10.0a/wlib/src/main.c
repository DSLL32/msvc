/* Unreconstructed application entry, original VA 0x412a67.
   Native tests use GCC's ABI; the Win32 build retains Watcom's ABI.
   The trap is a frontier marker, not accepted application implementation. */
#include <recon.h>

#if defined(__WATCOMC__)
/* Watcom 10.0a intrinsic emits INT3 inline, without a function or call edge. */
void reconstruction_trap( void );
#pragma aux reconstruction_trap = 0xcc;
#endif

int main( int argc, char **argv )
{
#if defined(__WATCOMC__)
    reconstruction_trap();
#else
    __asm__ volatile ("int3");
#endif
    return 0;
}
