#ifndef SYMBOL_CONSTANTS_H
#define SYMBOL_CONSTANTS_H
#include "symbols.h"
#include <stddef.h>
#include <recon.h>
/* Kind8 has a50-byte descriptor, ending in the payload's signed short count.
   Its DWORD fields share the ordinary symbol offsets; no trailing DWORD is
   allocated by descriptor_size. The two consumers use only these fields. */
#pragma pack(2)
typedef struct SymbolConstant {
    SymbolHeader symbol;
    unsigned debug_type, attributes, field28;
    unsigned char *value_bytes;
    short value_size;
} SymbolConstant;
#pragma pack()
typedef char SymbolConstantSizeCheck[sizeof(SymbolConstant)==50?1:-1];
#ifdef RECON_NATIVE_TEST
typedef char SymbolConstantPointerOffset[offsetof(SymbolConstant,value_bytes)==0x2c?1:-1];
typedef char SymbolConstantSizeOffset[offsetof(SymbolConstant,value_size)==0x30?1:-1];
#endif
#endif
