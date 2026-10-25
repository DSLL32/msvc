/* Unreconstructed application entry: CF RVA cb28..cb5d.
 * The original Watcom build uses argc EAX, argv EDX; the authorized native
 * build uses GCC i386 C formals. No application behavior is accepted yet. */
#include "wasm.h"
#include <recon.h>

#if defined(__WATCOMC__)
/* Watcom 10.0a's intrinsic emits the required frontier INT3 without a call. */
void reconstruction_trap(void);
#pragma aux reconstruction_trap = 0xcc;
#endif

int main(int argc, char **argv)
{
    (void)argc;
    (void)argv;
#if defined(RECON_NATIVE_TEST)
    /* Required unreconstructed-function trap; no implementation in assembly. */
    __asm__ __volatile__("int $3");
#else
    reconstruction_trap();
#endif
    return 0;
}
