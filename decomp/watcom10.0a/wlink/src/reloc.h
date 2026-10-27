#ifndef WLINK_RELOC_H
#define WLINK_RELOC_H
#include "link.h"
#include <recon.h>
#pragma pack(1)
struct reloc_info {
    struct reloc_info *next;
    unsigned short sizeleft;
    union { void *addr; unsigned long spill; } loc;
};
#pragma pack()
extern unsigned long SpillAlloc( unsigned );
extern void SpillWrite( unsigned long, unsigned long, void *, unsigned );
extern void SpillRead( unsigned long, unsigned long, void *, unsigned );
extern unsigned char SwapOutRelocs( void );
extern unsigned char SwapOutVirt( void );
extern unsigned char CacheRelease( void );
extern unsigned char FreeUpMemory( void );
extern unsigned char DumpObjCache( void );
extern unsigned char DiscardDicts( void );
extern unsigned char PermShrink( void );
extern unsigned char SpillRelocList( reloc_info * );
extern unsigned char SpillSections( section * );
extern unsigned char SpillAreas( ovl_area * );
extern int TempFile;
extern unsigned long TmpFSize;
#endif
