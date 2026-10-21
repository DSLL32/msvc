#ifndef SCOPES_H
#define SCOPES_H
#include "ir.h"

typedef struct FrontendSymbol FrontendSymbol;
typedef struct SymbolScope {
    struct SymbolScope *next, *parent;
    FrontendSymbol **buckets;
    unsigned short bucket_mask, flags;
    unsigned field10;
} SymbolScope;
extern SymbolScope *global_scope, *current_scope, *lookup_scope;
extern SymbolScope **scope_directory;
extern unsigned scope_serial;
void __stdcall create_scope_buckets(SymbolScope *scope, FrontendSymbol *symbol);
#endif
