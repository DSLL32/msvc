/* Named application entry stub, not an accepted reconstruction.
 * The original CF CRT calls main at payload RVA 0xcb28 with argc in EAX
 * and argv in EDX. Native GCC formals use its ordinary i386 ABI. */
#include <recon.h>

#if defined(__WATCOMC__)
/* Watcom 10.0a's aux intrinsic emits INT3 inline without a helper call.
 * Its parser does not support an MSVC-style __asm block. */
void reconstruction_trap(void);
#pragma aux reconstruction_trap = 0xcc;
#endif

int main(int argc, char **argv)
{
    (void)argc;
    (void)argv;
#if defined(RECON_NATIVE_TEST)
    __asm__ __volatile__("int $3");
#else
    reconstruction_trap();
#endif
    return 0;
}
