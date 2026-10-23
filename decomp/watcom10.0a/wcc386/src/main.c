/* Unreconstructed conventional main; no application acceptance yet.
 * Original CF entry candidate: 0x1a99b. The CRT passes argc/argv registers,
 * but this function does not consume either value. */
#include <recon.h>

#if defined(__WATCOMC__)
/* Watcom 10.0a has no __asm statement. This aux intrinsic emits the INT3
 * inline in main; it is not an additional function or reconstructed logic. */
void reconstruction_trap(void);
#pragma aux reconstruction_trap = 0xcc;
#endif

int main(void)
{
#if defined(RECON_NATIVE_TEST)
    __asm__ __volatile__("int $3");
#else
    reconstruction_trap();
#endif
    return 0;
}
