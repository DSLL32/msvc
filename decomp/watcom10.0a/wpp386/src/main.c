/* Scaffold only: no function has been reconstructed or accepted.
 * Original main candidate: CF payload RVA 0x43fef, not a PE RVA.
 * Native tests use the GCC ABI; original builds preserve the Watcom ABI.
 * CF transport/layout is not yet approved for application acceptance.
 */
#include <recon.h>

#if defined(__WATCOMC__)
/* Watcom 10.0a has no __asm statement. This intrinsic emits INT3 inline:
 * it introduces no function body, call or application-graph vertex. */
void reconstruction_trap( void );
#pragma aux reconstruction_trap = 0xcc;
#endif

int main( void )
{
#if defined(__WATCOMC__)
    reconstruction_trap();
#else
    __asm__ volatile ("int3");
#endif
    return 0;
}
