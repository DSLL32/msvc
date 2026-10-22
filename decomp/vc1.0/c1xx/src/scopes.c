/* Frontend symbol scopes. */
#include "scopes.h"
#include "parser.h"
#include <recon.h>

SymbolScope *global_scope, *current_scope, *lookup_scope;
SymbolScope **scope_directory;

/* Original 0x50e10..0x50e66. */
void initialize_global_scope(void)
{
    global_scope = allocate_permanent(sizeof(SymbolScope));
    global_scope->buckets = allocate_permanent(4096);
    global_scope->bucket_mask = 1023;
    current_scope = global_scope;
    lookup_scope = global_scope;
    scope_directory = allocate_permanent(512);
    scope_directory[0] = global_scope;
}

CppSymbol *translation_sentinel, *translation_placeholder; /* original 0x9f1c4/c8 */
char *anonymous_symbol_name; /* original 0x9f22c */
Vector *translation_symbols; /* original 0x9f1f0 */

/* Original 0x54fd0..0x5506f. */
void initialize_translation_state(void)
{
    translation_sentinel = allocate_permanent(sizeof(CppSymbol));
    translation_sentinel->kind = 8;
    translation_sentinel->detail.value = 0;
    translation_sentinel->type = type_0000;
    translation_sentinel->attributes |= 0x20;
    translation_sentinel->flags |= 4;
    translation_sentinel->index = 1;
    translation_placeholder = allocate_permanent(sizeof(CppSymbol));
    translation_placeholder->kind = 8;
    translation_placeholder->name = anonymous_symbol_name;
    translation_placeholder->type = type_0000;
    translation_placeholder->index = 10;
    translation_placeholder->attributes |= 0x20;
    translation_placeholder->flags |= 4;
    translation_symbols = new_vector(1, 0);
}

/* Original 0x564b0..0x564c4. */
void __stdcall push_symbol_scope(SymbolScope *scope)
{
    scope->next = current_scope;
    current_scope = scope;
}

unsigned char scope_directory_index; /* original 0xa0d1c */

/* Original 0x564e0..0x5652a. Unmaterialized scopes do not enter the directory. */
void __stdcall record_scope_directory_entry(SymbolScope *scope)
{
    if (scope_directory[scope_directory_index] != scope && scope->buckets) {
        ++scope_directory_index;
        if (scope_directory_index >= 128) fatal_error(61);
        scope_directory[scope_directory_index] = scope;
    }
}

/* Original 0x54a20..0x54ac9. Deferred scopes get sixteen zeroed arena
   buckets when their first declaration arrives. QC can open a control record
   before allocation, except for ordinary parameter declarations. */
void __stdcall initialize_scope_buckets(SymbolScope *scope, CppSymbol *symbol)
{
    ArenaPool *pool;
    unsigned kind;
    if (options.configuration.option_qc && !symbol) scope->qc_context = current_qc_context;
    if (scope->buckets) return;
    kind = symbol ? symbol->kind & 31 : 0;
    if (options.configuration.option_qc && normalized_Ol && declaration_mode > 2 &&
        (!symbol || (kind != 10 && kind != 11)) && !scope->qc_context)
        begin_qc_control(8);
    pool = &compiler_arena.pools[1];
    if (pool->remaining < 16 * (int)sizeof(CppSymbol *)) refill_arena(&compiler_arena, pool);
    scope->buckets = (CppSymbol **)pool->next;
    pool->remaining -= 16 * sizeof(CppSymbol *);
    pool->next += 16 * sizeof(CppSymbol *);
    record_scope_directory_entry(scope);
}

/* Original 0x56530..0x56555. A materialized scope owns the current directory
   slot. The byte index retains the original decrement/wrap behavior. */
void retire_scope_directory_entry(void)
{
    if (current_scope->buckets) {
        scope_directory[scope_directory_index] = 0;
        --scope_directory_index;
    }
}

/* Original 0x564d0..0x564dc. Directory retirement is a separate operation. */
void pop_symbol_scope(void)
{
    current_scope = current_scope->next;
}

/* Original 0x33370..0x333aa. Make temporary qualified scopes visible for
   the definition. Only the innermost converted scope receives bit 0x100; a parameter-scope marker (0xfd) ends this walk. */
void activate_qualified_definition_scopes(void)
{
    SymbolScope *scope;
    unsigned first = 1, kind;
    for (scope = current_scope; scope != global_scope; scope = scope->next) {
        kind = scope->flags & 255;
        if (kind == 0xfd) return;
        if (kind >= 0xfa) {
            scope->flags = (scope->flags & ~0x1ff) | (first << 8) | 0xff;
            first = 0;
        }
    }
}

/* Original 0x333b0..0x3341a. Qualified lookup/definition scopes stop at
   the parameter marker; callers may request stopping at a class boundary. */
void __stdcall unwind_qualified_scopes(int stop_at_class)
{
    unsigned char kind;
    if (token_allocation_flags & 0x20000000U) return;
    if ((unsigned char)current_scope->flags < 0xfa) return;
    while ((kind = (unsigned char)current_scope->flags) != 0xfd) {
        if (stop_at_class && kind != 0xfe) {
            if (kind == 0xfc) {
                retire_scope_directory_entry();
                pop_symbol_scope();
            }
            return;
        }
        retire_scope_directory_entry();
        pop_symbol_scope();
        if ((unsigned char)current_scope->flags < 0xfa) return;
    }
}
