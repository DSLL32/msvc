#ifndef SYMBOL_OUTPUT_H
#define SYMBOL_OUTPUT_H
#include "global-ir.h"
unsigned short __stdcall debug_tag_type(CppSymbol *tag);
void __stdcall reference_debug_type(CType *type);
void flush_debug_types(void);
void __stdcall prepare_symbol_debug_type(CppSymbol *symbol);
#endif
