#ifndef SCOPES_H
#define SCOPES_H
#include <stddef.h>
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
    union { unsigned char depth; unsigned saved_depth; FrontendSymbol *symbol; } value;
} ScopeCleanupEntry;
/* Register-parameter cleanup writes a whole DWORD; other entries use the
   low depth byte or a symbol pointer at the same original payload slot. */
typedef char ScopeCleanupEntry_layout[(sizeof(ScopeCleanupEntry) == 8 &&
    offsetof(ScopeCleanupEntry, value) == 4) ? 1 : -1];
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
