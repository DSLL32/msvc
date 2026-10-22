/* Closing output and compiler context for a completed function definition. */
#include "function-finalization.h"
#include "parameter-scopes.h"
#include "lexer.h"
#include "statement-scopes.h"
#include "work-list.h"
#include "control-ir.h"
#include <recon.h>

/* Lazily allocated by the aggregate-data initializer at original 0x45f00;
   it is distinct from the local-static guard immediately following it. */
CppSymbol *initializer_data_symbol; /* original 0x91830 */

/* Original 0x34d10..0x34d87, ending in a separate symbol-table reset entry. */
void end_function_definition(void)
{
    finish_function_body(0);
    local_static_guard.guard = 0;
    local_static_guard.state = 0;
    local_static_guard.count = 0;
    unwind_qualified_scopes(0);
    token_allocation_flags &= ~(0x00080000U | 0x100U);
    parsed_parameter_symbols = 0;
    active_parameter_symbols = 0;
    function_declaration_kind = 0;
    active_function_declaration_kind = 0;
    function_symbol = 0;
    release_arena(1);
    reset_function_symbol_tables();
}

/* Original 0x34bc0..0x34d05. Close output at the shared exit label, retire
   the outer scope, then clear the per-function parser/output context. */
void __stdcall finish_function_body(int recovering)
{
    if (function_symbol) {
        if (!options.prototypes_only) {
            if (!recovering) record_line_information(1);
            emit_label(function_exit_label);
            if (function_symbol->declaration_flags & 0x2000)
                emit_member_and_base_destructors(qualified_definition_class);
            else if (function_symbol->name == name_vbase_dtor)
                emit_vbase_destructor_epilogue(qualified_definition_class);
            if (options.configuration.debug_info) record_line_information(0x12);
            write_ir_record(0x47, 0);
        }
        if (recovering) leave_compound_scope();
        else end_compound_statement();
        finish_function_output();
        if (!options.prototypes_only)
            write_ir_record(0x54, make_integer_constant(0));
        if ((function_symbol->storage[1] & 1) && (function_symbol->storage[0] & 7) != 3) {
            local_static_guard.guard = 0;
            local_static_guard.state = 0;
            local_static_guard.count = 0;
        }
    }
    closed_scopes = 0;
    current_return_type = 0;
    scope_cleanup_stack = 0;
    initializer_data_symbol = 0;
    global_expression_scope_active = 0;
    text_work_cursor = expression_storage;
    global_record_kind = 0;
    declaration_mode = 0;
    qualified_definition_class = 0;
    current_declaration_scope = 0;
    reset_materialized_value_index();
}

/* Original 0x55ac0..0x55b5e. Emit labels and retained lexical scopes before
   recording the function's intermediate-stream positions and attributes. */
void finish_function_output(void)
{
    /* The native empty-list branch reads an unspecified local at 0x55b06.
       Preserve that original source condition; normal definitions queue scopes. */
    SymbolScope *scope;
    if (options.prototypes_only) return;
    global_record_kind = 1;
    finish_label_symbols();
    if (!closed_scopes) {
        global_record_kind = 1;
        write_global_record(13, 0);
        global_record_kind = 2;
        write_global_record(13, 0);
    } else {
        for (scope = closed_scopes; scope; scope = scope->next) serialize_scope(scope);
    }
    if (global_scope != scope) {
        if (options.configuration.option_qc && normalized_Ol) finish_inline_function();
        write_global_record(6, 0);
    }
    record_function_definition(function_symbol);
}

typedef char SerializedScopeBucketsOffsetMustBe8[
    (offsetof(SymbolScope, buckets) == 8) ? 1 : -1];
typedef char SerializedScopeMaskOffsetMustBe12[
    (offsetof(SymbolScope, bucket_mask) == 12) ? 1 : -1];
typedef char SerializedScopeFlagsOffsetMustBe14[
    (offsetof(SymbolScope, flags) == 14) ? 1 : -1];

/* Original 0x55980..0x55ab6. Preserve bucket/member order and defer global
   declarations until their dependencies have been emitted. */
int __stdcall serialize_scope(SymbolScope *scope)
{
    CppSymbol *pending = 0, *symbol, *next, **link;
    unsigned bucket, count;
    int changed = 1;
    if (error_count) return 0;
    global_record_kind = (unsigned char)scope->flags;
    if (scope != global_scope) write_global_record(13, 0);
    if (scope->buckets) {
        count = scope->bucket_mask + 1;
        for (bucket = 0; bucket < count; ++bucket) {
            if ((unsigned char)scope->flags < 0xfa)
                symbol = scope->buckets[scope->bucket_mask & bucket];
            else {
                /* Qualified class scopes retain a pointer to their tag in
                   the bucket slot; its auxiliary word owns the member chain. */
                symbol = (*scope->buckets)->auxiliary.scope_members;
            }
            while (symbol) {
                if ((symbol->kind & 0x1f) == 16 || (symbol->kind & 0x1f) == 17)
                    emit_function_symbols(symbol);
                else if (!global_record_kind && defer_global_symbol(symbol)) {
                    next = symbol->next;
                    symbol->next = pending;
                    pending = symbol;
                    symbol = next;
                    continue;
                } else emit_symbol_declaration(symbol);
                symbol = symbol->next;
            }
        }
    }
    while (pending && changed) {
        link = &pending;
        changed = 0;
        for (symbol = pending; symbol; symbol = symbol->next) {
            if (!defer_global_symbol(symbol)) {
                changed = 1;
                emit_symbol_declaration(symbol);
                *link = symbol->next;
            } else link = &symbol->next;
        }
    }
    return 0;
}

/* Linked directory blocks are retained until final translation-unit output. */
FunctionDirectoryBlock *function_directory; /* original 0x9f1cc */
FunctionDirectoryBlock *function_directory_tail; /* original 0x9f1d0 */
unsigned function_directory_index = 19; /* original 0x9f424 */

/* These FILE operations are inlined in the original caller. */
#define DIRECTORY_WORD(value_) do { \
    unsigned short word_ = (unsigned short)(value_); \
    if ((line_file->_cnt -= 2) >= 0) { \
        *(unsigned short *)line_file->_ptr = word_; line_file->_ptr += 2; \
    } else { \
        line_file->_cnt += 2; fwrite(&word_, 2, 1, line_file); \
    } \
} while (0)

/* Original 0x5aae0..0x5ae58. Save positions before resetting complexity,
   then write the compact function marker to the line intermediate stream. */
void __stdcall record_function_definition(CppSymbol *function)
{
    FunctionDirectoryBlock *block;
    FunctionDirectoryEntry *entry;
    unsigned index;
    if (!function) return;
    function_directory_index = (function_directory_index + 1) % 20;
    if (!function_directory_index) {
        block = allocate_permanent(sizeof(FunctionDirectoryBlock));
        if (!function_directory) function_directory = function_directory_tail = block;
        else {
            function_directory_tail->next = block;
            function_directory_tail = block;
        }
    }
    entry = &function_directory_tail->entries[function_directory_index];
    entry->symbol = function;
    if (pch_write_enabled && !pch_output_written) {
        entry->flags |= 0x10;
        if (pch_read_enabled) {
            function_ir.expression_offset += pch_expression_bytes + 0x334;
            function_ir.symbol_offset += pch_symbol_bytes;
        }
    } else entry->flags &= ~0x10;
    entry->expression_offset = function_ir.expression_offset;
    entry->symbol_offset = function_ir.symbol_offset;
    entry->complexity = function_ir.complexity;
    entry->flags = (entry->flags & ~15U) | (inline_candidate & 1) |
        (((unsigned)function_call_seen << 1) & 2) |
        (((unsigned)function_not_inlinable << 2) & 4) |
        (((unsigned)normalized_Oq << 3) & 8);
    entry->parameter_count = function_ir.parameter_count;
    function_ir.complexity = 0;
    if (cpp_parser_flags & 0x20000000U) return;
    putc(9, line_file);
    if (function_symbol) {
        index = function_symbol->index;
        if (index > 0x7fff) {
            DIRECTORY_WORD(index | 0x8000);
            index >>= 15;
        }
        DIRECTORY_WORD(index);
    } else {
        putc(0, line_file);
        putc(0, line_file);
    }
}

/* Original 0x5d9c0..0x5d9cb. A lazily assigned temporary symbol index,
   consumed by the materialized-value expression helper at 0x5d920. */
unsigned materialized_value_index; /* original 0xa1e20 */
void reset_materialized_value_index(void)
{
    materialized_value_index = 0;
}

/* Original 0xa0d24/0xa0d28. Native 0x55ed0 and 0x56000 lazily create
   these per-function alias and binding hash tables. */
HashTable *function_symbol_aliases, *function_symbol_bindings;

/* Original 0x55eb0..0x55ec5, also the final tail entry of 0x34d10. */
void reset_function_symbol_tables(void)
{
    function_symbol_aliases = 0;
    function_symbol_bindings = 0;
}
