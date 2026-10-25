/* Pending application entries. No body has been reconstructed or accepted. */
#include "memory.h"
#include <recon.h>

#if defined(__WATCOMC__)
/* Watcom 10.0a's aux intrinsic emits INT3 inline, without a helper vertex. */
void reconstruction_trap(void);
#pragma aux reconstruction_trap = 0xcc;
#endif

void *MemAlloc(unsigned size)
{
    (void)size;
#if defined(__WATCOMC__)
    reconstruction_trap();
#else
    __asm__ __volatile__("int3");
#endif
    return 0;
}

int main(void)
{
#if defined(__WATCOMC__)
    reconstruction_trap();
#else
    __asm__ __volatile__("int3");
#endif
    return 0;
}
