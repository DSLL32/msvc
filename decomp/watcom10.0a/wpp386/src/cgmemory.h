#ifndef WPP_CGMEMORY_H
#define WPP_CGMEMORY_H
#include <recon.h>
enum { MO_FATAL,MO_SUICIDE,MO_OK };
void *CGAlloc(int size);
unsigned char MemLow(void);
unsigned char ChkMemLimit(unsigned limit);
unsigned MemInUse(void);
unsigned char FlushSomeOpt(unsigned size);
unsigned char MemCheck(int size);
void FatalError(char *message);
void Suicide(void);
#endif
