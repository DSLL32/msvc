#ifndef WCC_MEMORY_H
#define WCC_MEMORY_H
#include <recon.h>
/* Original47c77: unsigned32 size in EAX, pointer result in EAX.
 * GCC native convention is authorized; original Watcom uses -3r. */
extern int Initialized;
extern unsigned AllocSize,MemorySize;
void *MemAlloc(unsigned size);
void MemInit(void);
/* Original66a2e returns an eight-bit bool in AL, not an int in EAX. */
unsigned char GetEnvVar(char *,char *,int);
int myatoi(char *);
void myitoa(int,char *);
char *CopyStr(char *,char *);
unsigned Length(char *);
extern int write(int,const void *,unsigned);
unsigned SizeToClass(unsigned size);
void *GetFromFrl(unsigned size,unsigned cls);
void *MemFromSys(unsigned size);
#endif
