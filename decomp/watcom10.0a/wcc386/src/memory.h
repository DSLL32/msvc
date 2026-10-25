#ifndef WCC_MEMORY_H
#define WCC_MEMORY_H
#include <recon.h>
/* Original47c77: unsigned32 size in EAX, pointer result in EAX.
 * GCC native convention is authorized; original Watcom uses -3r. */
extern int Initialized;
void *MemAlloc(unsigned size);
void MemInit(void);
unsigned SizeToClass(unsigned size);
void *GetFromFrl(unsigned size,unsigned cls);
void *MemFromSys(unsigned size);
#endif
