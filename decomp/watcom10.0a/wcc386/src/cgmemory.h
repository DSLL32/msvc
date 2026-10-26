#ifndef WCC_CGMEMORY_H
#define WCC_CGMEMORY_H
#include <recon.h>
enum { MO_FATAL,MO_SUICIDE,MO_OK };
extern unsigned char MemOut;
unsigned char SetMemOut(unsigned char value);
extern unsigned Chunks;
void *CGAlloc(int size);
void CGFreeSize(void *block,int size);
unsigned char MemLow(void);
unsigned char MemCheck(int size);
void FatalError(char *message);
void Suicide(void);
extern int InOptimizer;
extern unsigned FrlSize;
unsigned char FlushSomeOpt(unsigned size);
void MemCoalesce(void);
void CalcMemLimit(void);
#endif
