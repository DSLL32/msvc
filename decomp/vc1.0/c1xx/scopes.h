#ifndef SCOPES_H
#define SCOPES_H
#include "initialization.h"

typedef struct SymbolScope {
    struct SymbolScope *next, *parent;
    CppSymbol **buckets;
    unsigned short bucket_mask, flags;
    unsigned qc_context;
} SymbolScope;
extern SymbolScope *global_scope, *current_scope, *lookup_scope;
extern SymbolScope **scope_directory;
extern unsigned char scope_directory_index;
void retire_scope_directory_entry(void);
void pop_symbol_scope(void);
void __stdcall push_symbol_scope(SymbolScope *scope);
void __stdcall record_scope_directory_entry(SymbolScope *scope);
void __stdcall initialize_scope_buckets(SymbolScope *scope, CppSymbol *symbol);
typedef char SymbolScopeSizeMustBe20[(sizeof(SymbolScope) == 20) ? 1 : -1];
#endif
