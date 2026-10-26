#ifndef WPP_PENDING_H
#define WPP_PENDING_H
#include <recon.h>
#if defined(__WATCOMC__)
/* Watcom 10.0a's aux intrinsic emits INT3 inline, without a helper vertex. */
void reconstruction_trap(void);
#pragma aux reconstruction_trap = 0xcc;
#endif

#if defined(__WATCOMC__)
#define STUB_TRAP() reconstruction_trap()
#else
#define STUB_TRAP() __asm__ __volatile__("int3")
#endif
#endif
