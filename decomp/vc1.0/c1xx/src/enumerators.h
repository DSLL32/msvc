#ifndef ENUMERATORS_H
#define ENUMERATORS_H
#include "declarations.h"
#include <recon.h>
/* Enumerators use the common symbol prefix, followed by their value at+24;
   they do not allocate the longer function/object declaration tail. */
typedef struct EnumeratorSymbol {
    unsigned char kind, flags;
    unsigned short declaration_flags;
    CppSymbol *next;
    char *name;
    CType *type;
    unsigned index, attributes, value;
} EnumeratorSymbol;
typedef char EnumeratorSymbolSizeMustBe28[(sizeof(EnumeratorSymbol) == 28) ? 1 : -1];
typedef char EnumeratorValueOffsetMustBe24[(offsetof(EnumeratorSymbol, value) == 24) ? 1 : -1];
#endif
