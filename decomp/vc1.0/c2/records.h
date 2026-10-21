#ifndef RECORDS_H
#define RECORDS_H
#include "functions.h"
typedef struct DebugLink {
    struct DebugLink *next;
    SymbolHeader *symbol;
} DebugLink;
typedef struct DebugScope {
    short line;
    unsigned short field02;
    unsigned field04, field08;
    DebugLink *records;
    unsigned field10;
    struct DebugScope *next;
} DebugScope;

SymbolHeader *__fastcall type_of_symbol(SymbolHeader *symbol, int unwrap);
SymbolHeader *__fastcall read_symbol(SymbolHeader *parent_type);
SymbolHeader *__fastcall lookup_symbol(ScopeHash *scope, unsigned identifier);
SymbolHeader *__fastcall put_symbol(ScopeHash *scope, SymbolHeader *symbol);
void __fastcall queue_symbol_debug(SymbolHeader *symbol);
void __fastcall update_symbol_storage(SymbolHeader *symbol);
void __fastcall reserve_symbol_storage(SymbolHeader *symbol);
void __fastcall record_filename(SymbolHeader *symbol);
void __fastcall copy_symbol_type(SymbolHeader *symbol);
void __fastcall read_qualified_symbol(SymbolHeader *symbol, int arena);
char *__fastcall intern_symbol_name(SymbolHeader *symbol, char *name, int arena);
SymbolHeader *__fastcall lookup_global_symbol(unsigned identifier);
void __fastcall clone_alias_symbol(SymbolHeader *symbol);
char *__fastcall arena_strdup(char *string, unsigned arena);
void __fastcall apply_il_option(int option);
void __cdecl compiler_warning(int level, int code, ...);
#endif
