#ifndef SYMBOL_BINDINGS_H
#define SYMBOL_BINDINGS_H
#include "declarations.h"
#include "hash-table.h"
#include <recon.h>
/* Original binding allocation is exactly two pointer words. */
typedef struct SymbolBinding {
    CppSymbol *object, *member;
} SymbolBinding;
typedef char SymbolBindingSizeMustBe8[(sizeof(SymbolBinding) == 8) ? 1 : -1];
extern HashTable *global_symbol_bindings, *function_symbol_bindings;
void __stdcall record_symbol_binding(int local, CppSymbol *alias,
    CppSymbol *object, CppSymbol *member);
SymbolBinding *__stdcall new_symbol_binding(CppSymbol *object, CppSymbol *member,
    short arena);
#endif
