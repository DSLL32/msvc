/* Frontend symbol scopes. */
#include "builtins.h"
#include "parser.h"
#include "global-ir.h"

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

/* Original 0x36490..0x364b6. */
void deactivate_symbol_scope(void)
{
    if (current_scope->buckets) {
        scope_directory[scope_depth] = 0;
        --scope_depth;
    }
}

/* Original 0x36430..0x3643d. */
void pop_symbol_scope(void)
{
    current_scope = current_scope->next;
}

/* Original 0x34650..0x3468c. The third argument is retained by the ABI;
   this implementation uses interned-name identity for every lookup. */
FrontendSymbol *__stdcall find_scope_declaration(SymbolScope *scope, char *name, int case_fold)
{
    unsigned index;
    if (!scope || !scope->buckets) return 0;
    index = (unsigned)name & scope->bucket_mask;
    return lookup_symbol_chain(scope->buckets[index], name);
}

/* Original 0x35810..0x3583f. Tag names occupy a separate C namespace. */
FrontendSymbol *__stdcall lookup_symbol_chain(FrontendSymbol *head, char *name)
{
    while (head) {
        if (head->name == name && (head->kind & 0x1f) != 4) return head;
        head = head->next;
    }
    return 0;
}

/* Original 0x355a0..0x355b3. */
void __stdcall insert_current_scope_symbol(FrontendSymbol *symbol)
{
    insert_scope_symbol(current_scope, symbol);
}

/* Original 0x35e70..0x35e75: public scope-entry wrapper. */
void begin_compound_statement(void)
{
    enter_compound_scope();
}

/* Original 0x35e80..0x35f2a. Nested declaration scopes use pool one and
   carry their statement depth in the low byte of the scope flags. */
void enter_compound_scope(void)
{
    ArenaPool *pool;
    SymbolScope *scope;
    if (browser_filename) browser_begin_function_scope();
    ++parameter_nesting;
    global_record_kind = parameter_nesting;
    ++function_scope_level;
    if (!(scanner_flags & 0x40)) {
        pool = &compiler_arena.pools[1];
        if (pool->remaining < (int)sizeof(SymbolScope)) refill_arena(&compiler_arena, pool);
        scope = (SymbolScope *)pool->next;
        pool->remaining -= sizeof(SymbolScope);
        pool->next += sizeof(SymbolScope);
        scope->bucket_mask = 15;
        scope->flags = (scope->flags & 0xff00) | parameter_nesting;
        scope->parent = current_scope;
        push_symbol_scope(scope);
        activate_symbol_scope(scope);
    }
    write_ir_record(0x53, 0);
    if (options.option_qc && normalized_Ol && parameter_nesting == 2) begin_inline_region(0);
}
