/* Symbol and scope descriptor allocation. */
#include "backend.h"
#include "symbols.h"
unsigned descriptor_bytes[7];
unsigned __fastcall descriptor_size(int kind);
void *__fastcall arena_alloc(unsigned arena, unsigned bytes);
/* Original [0004dcb0,0004dce0). */
void *__fastcall allocate_descriptor(signed char kind, unsigned arena)
{
    unsigned bytes = descriptor_size(kind);
    SymbolHeader *descriptor;
    descriptor_bytes[arena] += bytes;
    descriptor = arena_alloc(arena, bytes);
    descriptor->kind = kind;
    return descriptor;
}

/* Original [0004dd90,0004de50), including its .text switch table. */
unsigned __fastcall descriptor_size(int kind)
{
    switch (kind) {
    case 1: case 11: return 44;
    case 2: case 7: return 48;
    case 3: case 8: return 50;
    case 4: return 64;
    case 5: return 52;
    case 9: return 72;
    case 14: return 84;
    case 16: return 68;
    case 19: return 13;
    default:
        compiler_internal_error("@(#)p2symtab.c:1.89", 0x47d);
        return 0;
    }
}

ScopeHash *current_scope;
/* Original [0004d1d0,0004d210). */
void __fastcall insert_symbol(SymbolHeader *symbol, int local)
{
    unsigned index = symbol->identifier & 63;
    SymbolHeader **bucket = local ? current_scope->buckets + index : global_scope.buckets + index;
    symbol->hash_next = *bucket;
    *bucket = symbol;
}

/* Original [0004ce90,0004ceb0). */
SymbolHeader *__fastcall put_symbol(ScopeHash *scope, SymbolHeader *symbol)
{
    unsigned index = symbol->identifier & 63;
    symbol->hash_next = scope->buckets[index];
    scope->buckets[index] = symbol;
    return symbol;
}
