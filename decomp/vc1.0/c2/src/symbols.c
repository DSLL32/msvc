/* Symbol and scope descriptor allocation. */
#include "backend.h"
#include "symbols.h"
#include "functions.h"
#include "temporaries.h"
#include "listing.h"
#include "function-input.h"
#include "machine.h"
#include "records.h"
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

/* Original [0004e400,0004e436). Choose the original global/local
   scope and pass all storage, arena, alignment and attribute arguments
   through the separately reconstructed temporary-symbol constructor. */
BackendSymbol *__fastcall new_generated_storage(unsigned kind, unsigned bytes,
    unsigned short storage, int local, unsigned char alignment, unsigned short flags)
{
    return (BackendSymbol *)create_temporary(local ? current_scope : &global_scope,
        kind, bytes, (short)storage, (unsigned)local, (signed char)alignment, flags);
}

/* Original [0004e440,0004e4e9). Construct the original kind1 storage
   descriptor, register its serial, then assign target segment/frame storage
   except for automatic temporaries. The fastcall entry pops five stack args. */
TemporarySymbol *__fastcall create_temporary(ScopeHash *scope, unsigned type, long size,
    short storage, unsigned arena, signed char subclass, unsigned short attributes)
{
    TemporarySymbol *temporary = allocate_descriptor(1, arena);
    temporary->symbol.name = 0;
    temporary->symbol.field0d = 0x26;
    temporary->symbol.value = size;
    temporary->attributes = (temporary->attributes & ~15U) | (type & 15);
    temporary->attributes &= ~0x8000U;
    temporary->attributes = (temporary->attributes & ~0x70U) |
                            (((unsigned)(signed char)storage << 4) & 0x70);
    temporary->attributes = (temporary->attributes & ~0x780U) |
                            (((unsigned)subclass << 7) & 0x780);
    temporary->symbol.field0e = attributes;
    temporary->symbol.identifier = next_symbol_identifier();
    put_symbol(scope, &temporary->symbol);
    if (storage != 3) {
        update_symbol_storage(&temporary->symbol);
        if (!(temporary->symbol.field0e & 0x480))
            reserve_symbol_storage(&temporary->symbol);
    }
    return temporary;
}

/* Original [0007e4a8,0007e6a8): module-lifetime external-name cache,
   shared by variable/function requirement paths. No surrounding data copied. */
enum { EXTERNAL_SYMBOL_BUCKETS = 128 };
static SymbolHeader *external_symbols[EXTERNAL_SYMBOL_BUCKETS];
unsigned __fastcall external_name_hash(char *name);
/* Original [0004e620,0004e72d). Require an undecorated runtime function
   once, preserving its global symbol serial, name ownership and debug queue. */
SymbolHeader *__fastcall require_external_symbol(char *name)
{
    unsigned first = external_name_hash(name) & (EXTERNAL_SYMBOL_BUCKETS - 1);
    unsigned bucket = first;
    char *left, *right;
    BackendSymbol *symbol;
    while (external_symbols[bucket]) {
        left = external_symbols[bucket]->name; right = name;
        while (*left == *right && *left) { ++left; ++right; }
        if (*left == *right) return external_symbols[bucket];
        bucket = (bucket + 1) & (EXTERNAL_SYMBOL_BUCKETS - 1);
        if (bucket == first) compiler_internal_error("@(#)p2symtab.c:1.89", 0x55c);
    }
    symbol = allocate_descriptor(4, 0);
    external_symbols[bucket] = &symbol->symbol;
    symbol->symbol.identifier = next_symbol_identifier();
    symbol->symbol.name = arena_strdup(name, 0);
    symbol->symbol.field0e = 0;
    symbol->attributes = (symbol->attributes & ~8U) | 7;
    symbol->output_flags &= ~1U;
    symbol->output_flags &= ~2U;
    symbol->attributes = (symbol->attributes & ~0xb0U) | 0x40;
    queue_symbol_debug(&symbol->symbol);
    symbol->attributes = (symbol->attributes & ~0x600U) | 0x100;
    insert_symbol(&symbol->symbol, 0);
    return &symbol->symbol;
}

/* Original [0004e530,0004e542). Sum signed input bytes modulo32;
   the two runtime-name caches select a bucket from its low seven bits. */
unsigned __fastcall external_name_hash(char *name)
{
    unsigned hash = 0;
    while (*name) hash += (unsigned)(int)(signed char)*name++;
    return hash;
}
