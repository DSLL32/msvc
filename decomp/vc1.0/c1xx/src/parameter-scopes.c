/* Parameter-declaration scopes and their active symbol chain. */
#include "parameter-scopes.h"

CppSymbol *active_parameter_symbols; /* original 0x8c400 */

/* Original 0x36670..0x366f9. The scope's low flag byte selects parameter
   lookup. Its high flags and other untouched fields retain their arena data. */
void __stdcall begin_parameter_scope(CppSymbol *first)
{
    ArenaPool *pool = &compiler_arena.pools[1];
    SymbolScope *scope;
    if (pool->remaining < (int)sizeof(SymbolScope)) refill_arena(&compiler_arena, pool);
    scope = (SymbolScope *)pool->next;
    pool->remaining -= sizeof(SymbolScope);
    pool->next += sizeof(SymbolScope);
    scope->bucket_mask = 0;
    scope->flags = (scope->flags & 0xff00) | 1;
    if (first) {
        if (pool->remaining < (int)sizeof(CppSymbol *)) refill_arena(&compiler_arena, pool);
        scope->buckets = (CppSymbol **)pool->next;
        pool->remaining -= sizeof(CppSymbol *);
        pool->next += sizeof(CppSymbol *);
        active_parameter_symbols = first;
        scope->buckets[0] = first;
    }
    push_symbol_scope(scope);
    record_scope_directory_entry(scope);
}

/* Original 0x36750..0x36785. The list value is its last element; the
   parameter link, separate from scope linkage, makes a circular chain. */
CppSymbol *__stdcall append_parameter_declaration(CppSymbol *list, CppSymbol *declaration)
{
    if (!declaration) return list;
    if (!list) declaration->detail.parameter_next = declaration;
    else prepend_declarator(list, declaration);
    return declaration;
}

/* Original 0x37750..0x37811. The grammar's ellipsis marker is a full symbol
   record with storage kind four; retain untouched arena bytes in other slots. */
CppSymbol *make_ellipsis_parameter(void)
{
    ArenaPool *pool = &compiler_arena.pools[1];
    CppSymbol *symbol;
    unsigned char kind;
    declaration_kind = 1;
    if (pool->remaining < (int)sizeof(CppSymbol)) refill_arena(&compiler_arena, pool);
    symbol = (CppSymbol *)pool->next;
    pool->remaining -= sizeof(CppSymbol);
    pool->next += sizeof(CppSymbol);
    symbol->kind = 11;
    symbol->storage[0] = (symbol->storage[0] & 0xfc) | 4;
    kind = symbol->storage[0] & 7;
    symbol->storage[0] = declaration_storage[0];
    symbol->storage[1] = declaration_storage[1];
    symbol->storage[2] = declaration_storage[2];
    symbol->storage[0] = (symbol->storage[0] & ~7) | kind;
    symbol->name = name_ellipsis;
    symbol->references = 1;
    if (next_symbol_index >= 0x7fffffffU) fatal_error(55);
    if (next_symbol_index >= browser_symbol_limit && browser_filename) browser_symbol_rollover();
    symbol->index = next_symbol_index++;
    symbol->type = type_0080;
    return symbol;
}
