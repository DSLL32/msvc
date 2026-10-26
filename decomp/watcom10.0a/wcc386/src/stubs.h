#ifndef WCC_RECON_STUBS_H
#define WCC_RECON_STUBS_H
#include <recon.h>
#if defined(__WATCOMC__)
/* Watcom10 INT3 intrinsic emits inline without adding a helper vertex. */
void reconstruction_trap(void);
#pragma aux reconstruction_trap = 0xcc;
#define STUB_TRAP() reconstruction_trap()
#else
#define STUB_TRAP() __asm__ __volatile__("int $3")
#endif
#endif
