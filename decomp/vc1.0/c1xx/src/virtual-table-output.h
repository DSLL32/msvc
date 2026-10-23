#ifndef VIRTUAL_TABLE_OUTPUT_H
#define VIRTUAL_TABLE_OUTPUT_H
#include "class-finalization.h"
#include "object-initializers.h"
#include <recon.h>
void __stdcall prepare_virtual_table_functions(VirtualTable *table, CppSymbol *tag);
CppSymbol *__stdcall resolve_virtual_table_function(VirtualCallEntry *entry, CppSymbol *tag, VirtualTable *table);
IrToken *__stdcall pure_virtual_table_expression(CppSymbol *function);
int __stdcall virtual_base_table_entry_offset(VirtualBaseEntry *entry, CppSymbol *tag, VirtualTable *table);
unsigned virtual_base_entry_width(void);
int __stdcall virtual_entry_this_displacement(VirtualCallEntry *entry, CppSymbol *tag, VirtualTable *table);
/* Cache key shared by this-adjustment and virtual-base adjustment thunks. */
typedef struct VirtualThunkKey {
    unsigned kind;
    CppSymbol *function;
    int displacement, virtual_displacement;
} VirtualThunkKey;
typedef char VirtualThunkKeySizeMustBe16[(sizeof(VirtualThunkKey) == 16) ? 1 : -1];
typedef char VirtualThunkFunctionOffsetMustBe4[(offsetof(VirtualThunkKey, function) == 4) ? 1 : -1];
typedef char VirtualThunkDisplacementOffsetMustBe8[(offsetof(VirtualThunkKey, displacement) == 8) ? 1 : -1];
typedef char VirtualThunkVirtualDisplacementOffsetMustBe12[(offsetof(VirtualThunkKey, virtual_displacement) == 12) ? 1 : -1];
extern HashTable *virtual_adjustment_thunks;
VirtualThunkKey *__stdcall new_virtual_thunk_key(unsigned kind, CppSymbol *function,
    int displacement, int virtual_displacement, int arena);
VirtualThunkKey *__stdcall copy_virtual_thunk_key(VirtualThunkKey *key, int arena);
CppSymbol *__stdcall emit_virtual_adjustment_thunk(VirtualThunkKey *key);
CppSymbol *__stdcall get_virtual_adjustment_thunk(unsigned kind, CppSymbol *function, int displacement, int virtual_displacement);
unsigned __stdcall virtual_adjustment_offset(VirtualAdjustment *adjustment, CppSymbol *tag);
unsigned __stdcall virtual_base_subobject_offset(CppSymbol *base, CppSymbol *derived);
#endif
