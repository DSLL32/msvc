#ifndef SYMBOLS_H
#define SYMBOLS_H
#include "c2.h"
/* Common prefix of the hashable descriptor variants. */
typedef struct SymbolHeader {
    struct SymbolHeader *hash_next;
    unsigned field04;
    long value;
    signed char kind;
    unsigned char field0d;
    unsigned short field0e;
    char *name;
    unsigned field14;
    union { void *pointer; char *group; unsigned identifier; } field18;
    unsigned identifier;
} SymbolHeader;
typedef struct ScopeHash {
    struct ScopeHash *previous;
    SymbolHeader *buckets[64];
} ScopeHash;
extern ScopeHash *current_scope;
extern ScopeHash global_scope;
void __fastcall insert_symbol(SymbolHeader *symbol, int local);
#endif
