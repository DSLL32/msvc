/* Scaffold only: no function has been reconstructed or accepted.
 * Original main candidate: CF payload RVA 0x43fef, not a PE RVA.
 * Native tests use the GCC ABI; original builds preserve the Watcom ABI.
 * Resolve the CF layout/transport and original compiler probe before building.
 */
#include <recon.h>

int main( void )
{
#if defined(__WATCOMC__)
    __asm { int 3 }
#else
    __asm__ volatile ("int3");
#endif
    return 0;
}
