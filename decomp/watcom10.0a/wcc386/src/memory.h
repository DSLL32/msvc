#ifndef WCC_MEMORY_H
#define WCC_MEMORY_H
#include <string.h>
#include <recon.h>
/* Original47c77: unsigned32 size in EAX, pointer result in EAX.
 * GCC native convention is authorized; original Watcom uses -3r. */
/* Authentic cvars.h flat-model compiler intrinsic. Its comparison result
 * belongs to ECX and it preserves the original no-helper call graph. The
 * original compiler cannot express that register/result ABI in ordinary C. */
#if defined(__WATCOMC__)
extern int far_strcmp(char *,char *,int);
#pragma aux far_strcmp = 0xf3 0xa6 0x74 0x01 0x41 \
    parm caller [edi] [esi] [ecx] value [ecx] modify exact [esi edi ecx];
/* Released cvars.h intrinsics: the old flat-model source emits these at
 * the typedef-name copy site, with no additional function-graph vertex. */
extern int far_strlen_plus1(char *);
#pragma aux far_strlen_plus1 = 0x29 0xc0 0x4f 0x47 0x40 0x80 0x3f 0x00 0x75 0xf9 \
    parm caller [edi] value [eax] modify exact [edi eax];
extern void far_memcpy(char *,char *,int);
#pragma aux far_memcpy = 0xf3 0xa4 \
    parm caller [edi] [esi] [ecx] modify exact [esi edi ecx];
#else
/* Reached consumers use equality only, not the ECX mismatch magnitude. */
#define far_strcmp(a,b,n) memcmp(a,b,n)
#define far_strlen_plus1(p) (strlen(p)+1)
#define far_memcpy(a,b,n) memcpy(a,b,n)
#endif
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
void MemCoalesce(void);
unsigned MemSize(void);
unsigned MemInUse(void);
/* Original66a2e returns an eight-bit bool in AL, not an int in EAX. */
unsigned char GetEnvVar(char *,char *,int);
int myatoi(char *);
void myitoa(int,char *);
char *CopyStr(char *,char *);
char *Copy(char *source,char *destination,unsigned length);
unsigned Length(char *);
extern int write(int,const void *,unsigned);
unsigned SizeToClass(unsigned size);
void *GetFromFrl(unsigned size,int cls);
void *MemFromSys(unsigned size);
unsigned AddBlock(char *block,int paragraphs);
void MemFree(char *block);
void UnHook(FreeList *unhook,FreeList *neighbours);
#endif
