#ifndef WLINK_STRTAB_H
#define WLINK_STRTAB_H
/* The original table is one growing buffer, before the later block list. */
typedef struct stringtable {
    unsigned size, used;
    char *data;
} stringtable;
extern void InitStringTable( stringtable * );
extern void CharStringTable( stringtable *, char );
extern unsigned char AddStringTable( stringtable *, char *, unsigned );
extern char *GetStringTable( stringtable * );
extern unsigned GetStringTableSize( stringtable * );
#include <recon.h>
#endif
