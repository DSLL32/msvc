#ifndef WCC_MEMORY_H
#define WCC_MEMORY_H
#include <recon.h>
/* Original47c77: unsigned32 size in EAX, pointer result in EAX.
 * GCC native convention is authorized; original Watcom uses -3r. */
extern int Initialized;
extern unsigned AllocSize,MemorySize;
typedef struct free_list FreeList;
typedef struct free_links {
    FreeList *next;
    FreeList *prev;
} FreeLinks;
struct free_list {
    unsigned length;
    int class;
    FreeLinks link[8];
};
extern FreeList _FreeList;
/* The original compiler lowers small DWORD fills to this shipped helper.
 * Its third argument is ECX rather than the default Watcom EBX; no body is
 * reconstructed here. EAX returns the advanced pointer. */
void *__STOSD(void *destination,unsigned value,unsigned count);
#if defined(__WATCOMC__)
#pragma aux __STOSD "__STOSD" parm [eax] [edx] [ecx] value [eax] modify [ecx];
#endif
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
void *GetFromFrl(unsigned size,int cls);
void *MemFromSys(unsigned size);
unsigned AddBlock(char *block,int paragraphs);
void MemFree(char *block);
void UnHook(FreeList *unhook,FreeList *neighbours);
#endif
