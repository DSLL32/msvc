/* Frontend symbol scopes. */
#include "builtins.h"
#include "parser.h"
#include "global-ir.h"
#include <recon.h>

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

/* Original 0x35f30..0x35f35: public scope-exit wrapper. */
void finish_compound_statement(void)
{
    leave_compound_scope();
}

/* Original 0x35f40..0x3604e. Close the declaration scope, retain it for
   later symbol serialization, and write the matching expression boundary. */
void leave_compound_scope(void)
{
    SymbolScope *scope; /* Original unspecified in raw scanner mode. */
    if (!parameter_nesting) return;
    if (browser_filename) browser_finish_scope();
    if (parameter_nesting == 2) function_scope_level = 0;
    if (!(scanner_flags & 0x40)) {
        scope = current_scope;
        if ((scope->flags & 0xff) != parameter_nesting) fatal_error(506);
        deactivate_symbol_scope();
        pop_symbol_scope();
        if (scope->buckets && options.option_qc && normalized_Ol &&
            parameter_nesting > 2 && !scope->field10) finish_inline_region();
        if (options.option_qc) {
            scope->field10 = 0;
            if (normalized_Ol && parameter_nesting == 2) finish_inline_region();
        }
        emit_scope_cleanup_records();
    }
    if (!options.prototypes_only) {
        if (!(scanner_flags & 0x40)) queue_completed_scope(scope);
        write_ir_record(0x54, make_integer_constant(parameter_nesting));
    }
    --parameter_nesting;
    global_record_kind = parameter_nesting;
}

/* Cleanup registration uses two consecutive eight-byte work-list entries:
   statement depth followed by the symbol whose cleanup record is emitted. */
ScopeCleanupEntry *scope_cleanup_stack; /* original 0x60078 */

/* Original 0x34c80..0x34cd9. */
void emit_scope_cleanup_records(void)
{
    if (options.prototypes_only) return;
    while (scope_cleanup_stack && scope_cleanup_stack->value.depth == parameter_nesting) {
        pop_work_list((void **)&scope_cleanup_stack);
        write_ir_record(0x2b, make_symbol_reference(scope_cleanup_stack->value.symbol));
        pop_work_list((void **)&scope_cleanup_stack);
    }
}

SymbolScope *completed_scopes; /* original 0x60080 */

/* Original 0x36050..0x3609d. Keep completed parent scopes ahead of their
   children while retaining the original ordering among unrelated scopes. */
void __stdcall queue_completed_scope(SymbolScope *scope)
{
    SymbolScope *previous = 0, *next = completed_scopes;
    if (!scope) return;
    while (next && (next->flags & 0xff) <= (scope->flags & 0xff)) {
        previous = next;
        next = next->next;
    }
    while (next && next->parent != scope) {
        previous = next;
        next = next->next;
    }
    if (previous) previous->next = scope;
    else completed_scopes = scope;
    scope->next = next;
}

/* Original 0x36220..0x362be. Serialize labels and completed lexical scopes
   before recording the function's stream positions and final attributes. */
void finish_function_records(void)
{
    SymbolScope *scope; /* Original unspecified when no scope was queued. */
    if (options.prototypes_only) return;
    global_record_kind = 1;
    finish_label_symbols();
    if (!completed_scopes) {
        global_record_kind = 1;
        write_global_record(13, 0);
        global_record_kind = 2;
        write_global_record(13, 0);
    } else {
        for (scope = completed_scopes; scope; scope = scope->next) serialize_scope(scope);
    }
    if (global_scope != scope) {
        if (options.option_qc && normalized_Ol) finish_inline_function();
        write_global_record(6, 0);
    }
    record_function_definition(function_symbol);
}

/* Original 0x36110..0x3621f. Emit a scope in bucket order, postponing
   dependent global declarations until their prerequisites have been emitted. */
int __stdcall serialize_scope(SymbolScope *scope)
{
    FrontendSymbol *pending = 0, *symbol, *next, **link;
    int changed = 1, bucket, count;
    if (error_count) return 0;
    global_record_kind = (unsigned char)scope->flags;
    if (global_scope != scope) write_global_record(13, 0);
    if (scope->buckets) {
        count = scope->bucket_mask + 1;
        for (bucket = 0; bucket < count; ++bucket) {
            symbol = scope->buckets[scope->bucket_mask & bucket];
            while (symbol) {
                if (!global_record_kind && defer_global_symbol(symbol)) {
                    next = symbol->next;
                    symbol->next = pending;
                    pending = symbol;
                    symbol = next;
                } else {
                    emit_symbol_declaration(symbol);
                    symbol = symbol->next;
                }
            }
        }
    }
    while (pending && changed) {
        changed = 0;
        link = &pending;
        while ((symbol = *link) != 0) {
            if (!defer_global_symbol(symbol)) {
                changed = 1;
                emit_symbol_declaration(symbol);
                *link = symbol->next;
            } else link = &symbol->next;
        }
    }
    return 0;
}

/* Per-function alias map, original 0x6ce74. Its entries are allocated in
   the function arena, so finishing a function only discards the root. */
static struct SymbolAliasTable *symbol_aliases;

/* Original 0x36360..0x3636b. */
void clear_symbol_aliases(void)
{
    symbol_aliases = 0;
}

/* Original 0x360a0..0x36107. A global type must wait for its
   referenced declaration unless that declaration belongs to the PCH. */
int defer_global_symbol(FrontendSymbol *symbol)
{
    unsigned kind;
    FrontendSymbol *dependency;
    if (pch_separate_debug) return 0;
    kind = symbol->kind & 31;
    if (kind == 4 || kind == 5)
        return (symbol->flags & 2) && !(symbol->flags & 0x40);
    if ((symbol->storage[0] & 7) == 7) {
        dependency = (FrontendSymbol *)symbol->type->field08;
        if (dependency && !(symbol->flags & 0x40) && defer_global_symbol(dependency))
            return 1;
    }
    return 0;
}

unsigned char found_scope_depth; /* original 0x5b138 */

/* Original 0x285c0..0x28650. Search active scopes from innermost to
   outermost, retaining the lookup depth for subsequent declaration checks. */
FrontendSymbol *__stdcall lookup_tag(char *name)
{
    unsigned char depth = scope_depth;
    SymbolScope *scope;
    FrontendSymbol *head, *tag;
    if (name == name_unnamed) return 0;
    for (;;) {
        scope = scope_directory[depth];
        head = scope->buckets ? scope->buckets[(unsigned)name & scope->bucket_mask] : 0;
        if (head) {
            tag = lookup_tag_chain(name, head);
            if (tag) {
                found_scope_depth = depth;
                tag_in_current_scope = depth == scope_depth;
                return tag;
            }
        }
        if (!depth--) break;
    }
    tag_in_current_scope = 0;
    return 0;
}

SymbolScope *found_symbol_scope; /* original 0x6007c */

/* Original 0x35700..0x35774. Ordinary identifiers share scope hash buckets
   with tags but occupy the other C namespace. Preserve the scope of a hit. */
FrontendSymbol *__stdcall find_identifier_symbol(char *name)
{
    unsigned char depth = scope_depth;
    SymbolScope *scope;
    FrontendSymbol *symbol;
    if (!name) return 0;
    for (;;) {
        scope = scope_directory[depth];
        symbol = scope->buckets[(unsigned)name & scope->bucket_mask];
        while (symbol) {
            if (symbol->name == name && (symbol->kind & 0x1f) != 4) {
                found_scope_depth = (unsigned char)scope->flags;
                found_symbol_scope = scope;
                return symbol;
            }
            symbol = symbol->next;
        }
        if (!depth--) break;
    }
    return 0;
}

/* Original 0x28590..0x285b3. Callers supply a nonempty bucket chain. */
FrontendSymbol *__stdcall lookup_tag_chain(char *name, FrontendSymbol *head)
{
    do {
        if (head->name == name && (head->kind & 0x1f) == 4) return head;
        head = head->next;
    } while (head);
    return 0;
}

/* Original 0x355f0..0x3569a. Delay the sixteen-bucket allocation until
   a scope first needs symbols, then make that scope visible to lookup. */
void __stdcall create_scope_buckets(SymbolScope *scope, FrontendSymbol *symbol)
{
    ArenaPool *pool;
    unsigned kind;
    if (options.option_qc && !symbol) scope->field10 = scope_serial;
    if (scope->buckets) return;
    if (options.option_qc && normalized_Ol && parameter_nesting > 2) {
        kind = symbol ? symbol->kind & 0x1f : 0;
        if (kind != 10 && kind != 11 && !scope->field10) begin_inline_region(8);
    }
    pool = &compiler_arena.pools[1];
    if (pool->remaining < 64) refill_arena(&compiler_arena, pool);
    scope->buckets = (FrontendSymbol **)pool->next;
    pool->remaining -= 64;
    pool->next += 64;
    activate_symbol_scope(scope);
}
