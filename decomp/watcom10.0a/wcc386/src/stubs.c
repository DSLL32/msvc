#include "memory.h"
#include <recon.h>
#if defined(__WATCOMC__)
/* Watcom10 INT3 intrinsic emits inline without adding a helper vertex. */
void reconstruction_trap(void);
#pragma aux reconstruction_trap = 0xcc;
#define STUB_TRAP() reconstruction_trap()
#else
#define STUB_TRAP() __asm__ __volatile__("int $3")
#endif
void MemInit(void) { STUB_TRAP(); }
unsigned SizeToClass(unsigned size) { (void)size; STUB_TRAP(); return 0; }
void *GetFromFrl(unsigned size,unsigned cls) { (void)size; (void)cls; STUB_TRAP(); return 0; }
void *MemFromSys(unsigned size) { (void)size; STUB_TRAP(); return 0; }
