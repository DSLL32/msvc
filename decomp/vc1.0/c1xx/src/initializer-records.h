#ifndef INITIALIZER_RECORDS_H
#define INITIALIZER_RECORDS_H
#include "object-initializers.h"
#include <recon.h>
extern unsigned char initializer_record_marker;
int __stdcall find_initializer_address(IrToken *value, CppSymbol **symbol, int *offset);
unsigned __stdcall initializer_address_size(CppSymbol *symbol);
#endif
