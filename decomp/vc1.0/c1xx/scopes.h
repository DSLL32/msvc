#ifndef SCOPES_H
#define SCOPES_H
#include "initialization.h"

typedef struct SymbolScope {
    struct SymbolScope *next, *parent;
    CppSymbol **buckets;
    unsigned short bucket_mask, flags;
    unsigned field10;
} SymbolScope;
extern SymbolScope *global_scope, *current_scope, *lookup_scope;
extern SymbolScope **scope_directory;
void retire_scope_directory_entry(void);
void pop_symbol_scope(void);
#endif
