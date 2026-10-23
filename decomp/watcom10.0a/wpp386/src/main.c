/* Scaffold only: no function has been reconstructed or accepted.
 * Original main candidate: CF payload RVA 0x43fef, not a PE RVA.
 * Resolve the Watcom register ABI before enabling a reconstruction build.
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
