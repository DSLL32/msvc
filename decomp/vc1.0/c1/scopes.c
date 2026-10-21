/* Frontend symbol scopes. */
#include "builtins.h"

SymbolScope *global_scope, *current_scope, *lookup_scope;
SymbolScope **scope_directory;

/* Original 0x33690..0x336e6. */
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

unsigned scope_serial; /* original 0x6bff0 */

/* Original 0x356a0..0x356f8. */
void __stdcall insert_scope_symbol(SymbolScope *scope, FrontendSymbol *symbol)
{
    unsigned index;
    if (options.option_qc)
        scope->field10 = scope_serial;
    if (!scope->buckets)
        create_scope_buckets(scope, symbol);
    else {
        index = scope->bucket_mask & (unsigned)symbol->name;
        symbol->next = scope->buckets[index];
    }
    index = scope->bucket_mask & (unsigned)symbol->name;
    scope->buckets[index] = symbol;
}

FrontendSymbol *translation_sentinel, *translation_placeholder; /* 0x6b634/638 */
char *anonymous_symbol_name; /* original 0x6b694 */

/* Original 0x35990..0x35a21. */
void initialize_translation_state(void)
{
    translation_sentinel = allocate_permanent(sizeof(FrontendSymbol));
    translation_sentinel->kind = 8;
    translation_sentinel->detail.intrinsic.operation = 0;
    translation_sentinel->detail.intrinsic.flags = 0;
    translation_sentinel->type = type_0000;
    translation_sentinel->attributes |= 0x20;
    translation_sentinel->flags |= 1;
    translation_sentinel->index = 1;
    translation_placeholder = allocate_permanent(sizeof(FrontendSymbol));
    translation_placeholder->kind = 8;
    translation_placeholder->name = anonymous_symbol_name;
    translation_placeholder->type = type_0000;
    translation_placeholder->index = 10;
    translation_placeholder->attributes |= 0x20;
    translation_placeholder->flags |= 1;
}

/* Original 0x36410..0x36424. */
void __stdcall push_symbol_scope(SymbolScope *scope)
{
    scope->next = current_scope;
    current_scope = scope;
}

unsigned char scope_depth; /* original 0x6ce6c */

/* Original 0x36440..0x3648a. Empty scopes stay on the declaration chain,
   but need no directory entry for symbol lookup. */
void __stdcall activate_symbol_scope(SymbolScope *scope)
{
    if (scope_directory[scope_depth] != scope && scope->buckets) {
        if (++scope_depth >= 128) fatal_error(61);
        scope_directory[scope_depth] = scope;
    }
}
