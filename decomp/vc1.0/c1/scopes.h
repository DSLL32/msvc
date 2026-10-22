#ifndef SCOPES_H
#define SCOPES_H
#include "ir.h"

typedef struct SymbolScope {
    struct SymbolScope *next, *parent;
    FrontendSymbol **buckets;
    unsigned short bucket_mask, flags;
    unsigned field10;
} SymbolScope;
extern SymbolScope *global_scope, *current_scope, *lookup_scope;
extern SymbolScope **scope_directory;
extern unsigned scope_serial;
extern unsigned char scope_depth;
void __stdcall push_symbol_scope(SymbolScope *scope);
void __stdcall activate_symbol_scope(SymbolScope *scope);
void deactivate_symbol_scope(void);
void pop_symbol_scope(void);
FrontendSymbol *__stdcall lookup_symbol_chain(FrontendSymbol *head, char *name);
void __stdcall create_scope_buckets(SymbolScope *scope, FrontendSymbol *symbol);
#endif
