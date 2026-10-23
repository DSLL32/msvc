#ifndef CVPACK_STRIP_H
#define CVPACK_STRIP_H
#include "image.h"
#include <recon.h>

#pragma pack(2)
typedef struct CoffSymbol {
    unsigned char name[8];
    unsigned long value;
    short section;
    unsigned short type;
    unsigned char storage_class, auxiliary_count;
} CoffSymbol;
#pragma pack()
typedef char CoffSymbolSize[(sizeof(CoffSymbol)==18)?1:-1];
typedef struct CoffFileName {
    unsigned long symbol;
    char *name;
    struct CoffFileName *next;
} CoffFileName;
typedef char CoffFileNameSize[(sizeof(CoffFileName)==12)?1:-1];

extern unsigned long *strip_section_rvas;
extern CoffFileName *strip_file_names;
unsigned short read_coff_symbol(CoffSymbol *,unsigned long);
unsigned short next_coff_symbol(CoffSymbol *,unsigned long *);
int classify_coff_symbol(const CoffSymbol *);
#endif
