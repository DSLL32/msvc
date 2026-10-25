#ifndef C2_SYMBOL_STORAGE_H
#define C2_SYMBOL_STORAGE_H
#include "c2.h"
#include <recon.h>

extern int local_storage_alignment, argument_stack_offset;
extern int argument_layout_kind, argument_alignment;
long __fastcall allocate_local_storage(unsigned bytes, unsigned alignment);
void __fastcall reserve_section_symbol(C2Symbol *symbol);
#endif
