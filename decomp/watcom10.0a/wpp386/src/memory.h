#ifndef WPP_MEMORY_H
#define WPP_MEMORY_H
#include <recon.h>
typedef struct frl frl;
typedef struct next_prev {
    frl *next, *prev;
} next_prev;
struct frl {
    unsigned length;
    int class;
    next_prev link[8];
};
extern frl _FreeList;
/* Original CF 0x81ba3: unsigned32 size in EAX, pointer result in EAX.
 * Native tests use GCC's ordinary ABI; original Watcom builds use -3r. */
void *MemAlloc(unsigned size);
extern int Initialized;
extern unsigned AllocSize, MemorySize;
void MemInit(void);
unsigned char GetEnvVar(char *name, char *buffer, int length);
int myatoi(char *text);
void myitoa(int value, char *text);
char *CopyStr(char *source, char *dest);
unsigned Length(char *text);
extern int write(int handle, const void *buffer, unsigned amount);
int SizeToClass(unsigned size);
void *GetFromFrl(unsigned size, int cls);
void *MemFromSys(unsigned size);
#endif
