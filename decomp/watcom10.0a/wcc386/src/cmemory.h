#ifndef WCC_CMEMORY_H
#define WCC_CMEMORY_H
#include <recon.h>
typedef struct mem_block {
    unsigned len;
    struct mem_block *prev,*next;
} MCB;
extern char *PCH_Start,*PCH_End;
void Ccoalesce(MCB *block);
#endif
