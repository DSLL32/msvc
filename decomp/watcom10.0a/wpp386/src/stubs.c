/* Pending application entries. */
#include "memory.h"
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
int SizeToClass(unsigned size) { (void)size; STUB_TRAP(); return 0; }
void *GetFromFrl(unsigned size, int cls) { (void)size; (void)cls; STUB_TRAP(); return 0; }
void *MemFromSys(unsigned size) { (void)size; STUB_TRAP(); return 0; }
int myatoi(char *text) { (void)text; STUB_TRAP(); return 0; }
void myitoa(int value, char *text) { (void)value; (void)text; STUB_TRAP(); }
char *CopyStr(char *source, char *dest) { (void)source; (void)dest; STUB_TRAP(); return 0; }
unsigned Length(char *text) { (void)text; STUB_TRAP(); return 0; }

int main(int argc, char **argv)
{
    (void)argc; (void)argv;
#if defined(__WATCOMC__)
    reconstruction_trap();
#else
    __asm__ __volatile__("int3");
#endif
    return 0;
}
