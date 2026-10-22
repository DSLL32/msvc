/* Symbol and scope descriptor allocation. */
#include "backend.h"
#include "symbols.h"
#include "functions.h"
#include "temporaries.h"
#include "listing.h"
unsigned descriptor_bytes[7];
static ScopeHash *symbol_iteration_scope; /*83280*/
static int symbol_iteration_bucket; /*8327c*/
static SymbolHeader *symbol_iteration_current; /*7e6a8*/
/* Original [0004e330,0004e370). Release arena-one symbol scratch and
   disconnect its temporary-register and current-scope roots. */
void release_function_symbols(void)
{
    descriptor_bytes[1] = 0;
    reset_arena(1);
    temporary_registers = NULL;
    current_symbol = NULL;
    current_scope = NULL;
}
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

#include "options.h"
#include <recon.h>
extern unsigned next_serial;
/* Original [0004df20,0004df50). */
unsigned next_symbol_identifier(void)
{
    if (!next_serial) compiler_report(0x37);
    return next_serial++;
}

/* Each function scope chains to the module's 64-bucket scope. */
ScopeHash *new_scope(void)
{
    ScopeHash *scope;
    unsigned i;
    scope = arena_alloc(0, sizeof(ScopeHash));
    scope->previous = &global_scope;
    for (i = 0; i != 64; ++i) scope->buckets[i] = 0;
    return scope;
}

/* Original [0004d210,0004d240). Search the enclosing scopes in order. */
SymbolHeader *__fastcall lookup_symbol(ScopeHash *scope, unsigned identifier)
{
    SymbolHeader *symbol;
    unsigned bucket;
    if (!scope) return 0;
    bucket = identifier & 63;
    do {
        symbol = scope->buckets[bucket];
        while (symbol) {
            if (symbol->identifier == identifier) return symbol;
            symbol = symbol->hash_next;
        }
        scope = scope->previous;
    } while (scope);
    return 0;
}

/* Original [0004e3a0,0004e3d0). */
void __fastcall activate_scope(ScopeHash *scope)
{
    if (scope->previous != &global_scope)
        compiler_internal_error("@(#)p2symtab.c:1.89", 0x4a8);
    current_scope = scope;
}

/* Original [0004dce0,0004dd30). Resolve an identifier in the active function
   scope before falling back to the module table; zero is the null symbol. */
SymbolHeader *__fastcall lookup_global_symbol(unsigned identifier)
{
    unsigned bucket;
    SymbolHeader *symbol;
    if (!identifier) return 0;
    bucket = identifier & 63;
    if (current_scope)
        for (symbol = current_scope->buckets[bucket]; symbol; symbol = symbol->hash_next)
            if (symbol->identifier == identifier) return symbol;
    for (symbol = global_scope.buckets[bucket]; symbol; symbol = symbol->hash_next)
        if (symbol->identifier == identifier) return symbol;
    return 0;
}

/* Original [0004e7f0,0004e830). Visit exactly the selected scope. The
   stateful iterator verifies the preceding symbol on each continuation. */
void __fastcall for_each_symbol(int local, SymbolVisitor visit)
{
    ScopeHash *scope = local ? current_scope : &global_scope;
    SymbolHeader *symbol;
    if (!scope) return;
    for (symbol = next_scope_symbol(scope, 0); symbol;
         symbol = next_scope_symbol(scope, symbol)) visit(symbol);
}

/* Original [0004e730,0004e7f0). Shared continuation state belongs to one
   scope at a time; nested or stale iterator continuations are internal errors. */
SymbolHeader *__fastcall next_scope_symbol(ScopeHash *scope, SymbolHeader *previous)
{
    if (!previous) {
        symbol_iteration_scope = scope;
        symbol_iteration_bucket = -1;
        symbol_iteration_current = 0;
    }
    if (symbol_iteration_current != previous || symbol_iteration_scope != scope)
        compiler_internal_error("@(#)p2symtab.c:1.89", 0x58c);
    if (previous && previous->hash_next) {
        symbol_iteration_current = previous->hash_next;
        return symbol_iteration_current;
    }
    for (++symbol_iteration_bucket; symbol_iteration_bucket < 64; ++symbol_iteration_bucket) {
        if (scope->buckets[symbol_iteration_bucket]) {
            symbol_iteration_current = scope->buckets[symbol_iteration_bucket];
            return symbol_iteration_current;
        }
    }
    return 0;
}
