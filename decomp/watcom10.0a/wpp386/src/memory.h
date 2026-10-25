#ifndef WPP_MEMORY_H
#define WPP_MEMORY_H
#include <recon.h>
/* Original CF 0x81ba3: unsigned32 size in EAX, pointer result in EAX.
 * Native tests use GCC's ordinary ABI; original Watcom builds use -3r. */
void *MemAlloc(unsigned size);
#endif
