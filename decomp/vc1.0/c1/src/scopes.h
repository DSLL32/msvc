#ifndef SCOPES_H
#define SCOPES_H
#include "ir.h"
#include <recon.h>

typedef struct SymbolScope {
    struct SymbolScope *next, *parent;
    FrontendSymbol **buckets;
    unsigned short bucket_mask, flags;
    unsigned field10;
} SymbolScope;
typedef struct ScopeCleanupEntry {
    struct ScopeCleanupEntry *next;
    union { unsigned char depth; FrontendSymbol *symbol; } value;
} ScopeCleanupEntry;
extern ScopeCleanupEntry *scope_cleanup_stack;
extern SymbolScope *global_scope, *current_scope, *lookup_scope, *found_symbol_scope;
extern SymbolScope *completed_scopes;
extern SymbolScope **scope_directory;
extern unsigned scope_serial;
extern unsigned char scope_depth, found_scope_depth;
void __stdcall push_symbol_scope(SymbolScope *scope);
void __stdcall activate_symbol_scope(SymbolScope *scope);
void deactivate_symbol_scope(void);
void pop_symbol_scope(void);
FrontendSymbol *__stdcall lookup_symbol_chain(FrontendSymbol *head, char *name);
void __stdcall create_scope_buckets(SymbolScope *scope, FrontendSymbol *symbol);
#endif
