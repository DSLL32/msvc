/* Closing output and compiler context for a completed function definition. */
#include "function-finalization.h"
#include "parameter-scopes.h"
#include "lexer.h"
#include "statement-scopes.h"
#include "work-list.h"
#include "control-ir.h"
#include "object-initializers.h"
#include "call-expressions.h"
#include "return-statements.h"
#include "symbol-output.h"
#include "cpp-context.h"
#include "special-members.h"
#include "constructor-initializers.h"
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
#ifdef RECON_NATIVE_TEST
        /* Original 0x55b06 reads the unwritten entryESP-4 local here.
           Keep the two real records above, then reject this native path. */
        __asm { int 3 }
#endif
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

/* Original 0x34280..0x3434b. Finish deferred file-scope cleanup and build
   the initializer function that calls the translation unit's queued symbols. */
void finish_translation_symbols(void)
{
    Vector *functions;
    short index;
    IrToken *call;
    if (error_count) return;
    if (token_allocation_flags & 0x40) flush_current_function(1);
    if (pending_function_cleanup(0) && !declaration_mode) {
        emit_pending_function_cleanup();
        if (token_allocation_flags & 0x40) chain_initializer_function();
    }
    if (token_allocation_flags & 0x40) flush_current_function(1);
    if (vector_count(translation_symbols) > 0) {
        begin_initializer_function(0);
        functions = translation_symbols;
        index = 0;
        while (index < functions->count) {
            call = make_resolved_call(functions->items[index++], 0, 0, 0, 0);
            emit_expression_statement(call);
        }
        register_initializer_function(function_symbol);
        flush_current_function(0);
    }
}

/* Original BSS 0x914d4..0x9154f, ending before declaration_kind at
   0x91550. The producer indexes these work-list heads by declaration_mode;
   slot zero holds file-scope cleanup. This is contextual list state, not a
   copied data image. The other slots are populated when their scopes queue
   deferred objects. */
WorkList *pending_cleanup_by_scope[31];
typedef char PendingCleanupScopeStorageMustBe124[
    (sizeof(pending_cleanup_by_scope) == 124) ? 1 : -1];

/* Original 0x4f4d0..0x4f4df. */
WorkList *__stdcall pending_function_cleanup(unsigned char kind)
{
    return pending_cleanup_by_scope[kind];
}

HashTable *declaration_thunks; /* original 0x9f338 */

/* Original 0x4dd60..0x4ddaa. Emit each retained declaration thunk using
   the description key and generated function symbol carried by its map entry. */
void finish_declarations(void)
{
    HashIterator iterator;
    if (declaration_thunks) {
        begin_hash_iteration(&iterator, declaration_thunks);
        while (next_hash_entry(&iterator))
            emit_declaration_thunk(iterator.key, iterator.value);
    }
}

HashTable *deferred_type_tables; /* original 0x9f33c */

/* Original 0x4d820..0x4d86a. Materialize retained type-related tables
   after parsing, preserving their map iteration and generated-symbol pairing. */
void finish_types(void)
{
    HashIterator iterator;
    if (deferred_type_tables) {
        begin_hash_iteration(&iterator, deferred_type_tables);
        while (next_hash_entry(&iterator))
            emit_deferred_type_table(iterator.key, iterator.value);
    }
}

CppSymbol *segment_symbols; /* original 0x9f254: linked segment declarations */

/* Original 0x55b60..0x55bdb. Emit the segment declarations, function
   directory and global scope, then initialize requested segment aliases. */
void finish_expression_output(void)
{
    CppSymbol *segment, *alias;
    IrToken *reference;
    global_record_kind = 0;
    if (error_count) return;
    for (segment = segment_symbols; segment; segment = segment->next)
        emit_symbol_declaration(segment);
    write_function_directory();
    serialize_scope(global_scope);
    for (segment = segment_symbols; segment; segment = segment->next) {
        alias = (CppSymbol *)segment->field28;
        if (alias) {
            emit_symbol_declaration(alias);
            reference = symbol_reference(segment);
            alias = prepare_initializer_symbol((CppSymbol *)segment->field28);
            initialize_object(alias, reference);
        }
    }
    write_global_record(10, 0);
}

/* Original 0x5ae60..0x5b0a5. Restore each function's recorded output
   state, decide whether an out-of-line body is required, and write its final
   directory record. The head is consumed; the optimization setting is restored. */
void write_function_directory(void)
{
    int saved_Oq = normalized_Oq;
    unsigned count, index, flags;
    CppSymbol *function, *tag;
    FunctionDirectoryEntry *entry;
    if (function_directory) {
        do {
            count = function_directory == function_directory_tail ? function_directory_index + 1 : 20;
            index = 0;
            do {
                entry = &function_directory->entries[index];
                function = entry->symbol;
                function_ir.expression_offset = entry->expression_offset;
                function_ir.symbol_offset = entry->symbol_offset;
                flags = entry->flags;
                hidden_frame_enabled = (flags & 0x10) >> 4;
                function_ir.complexity = entry->complexity;
                function_ir.parameter_count = entry->parameter_count;
                inline_candidate = -(int)(flags & 1);
                function_call_seen = (flags & 2) || ((function->type->related->attributes & 7) == 1);
                function_not_inlinable = -(int)((flags >> 2) & 1);
                /* The original deliberately reads this slot from the final
                   block even when consuming an earlier directory block. */
                normalized_Oq = -(int)((function_directory_tail->entries[index].flags >> 3) & 1);
                function_ir.out_of_line_required = 0;
                if (function->name == name_main || function->name == name_winmain || function->name == name_dllmain) {
                    if ((function->storage[0] & 7) == 3) {
                        compiler_warning(3, 8, function->name, token_spellings[TOK_STATIC]);
                        function->storage[0] = (function->storage[0] & 0xf9) | 1;
                    }
                    if (function->storage[1] & 1) {
                        compiler_warning(3, 8, function->name, token_spellings[TOK_INLINE]);
                        function->storage[1] &= ~1;
                    }
                    function->attributes |= 0x8000;
                    function_ir.out_of_line_required = 1;
                } else if ((function->storage[0] & 7) != 3 || (function->declaration_flags & 0x400)) {
                    if (!(function->storage[1] & 1)) function_ir.out_of_line_required = 1;
                    else {
                        function->storage[0] |= 0x40;
                        if (((function->storage[1] & 0x10) || ((function->storage[2] & 12) == 12)) &&
                            (!(function->storage[2] & 4) || (function->storage[2] & 8)))
                            function_ir.out_of_line_required = 1;
                    }
                }
                if (function->storage[1] & 0x80) function_ir.out_of_line_required = 1;
                if (!function_ir.out_of_line_required && !function->references)
                    compiler_warning(4, 0x1f9, format_symbol_name(function));
                else {
                    if (options.configuration.debug_info) {
                        if (!(function->declaration_flags & 0x400)) reference_debug_type(function->type);
                        else {
                            tag = cpp_context_stack->items[function->auxiliary.declaring_class];
                            tag->type->debug_index = debug_tag_type(tag);
                        }
                        flush_debug_types();
                    }
                    write_global_record(14, function);
                }
                function->flags |= 8;
                ++index;
            } while (--count);
            function_directory = function_directory->next;
        } while (function_directory);
    }
    normalized_Oq = saved_Oq;
}

/* Original 0x55900..0x5597b. PCH-resident declarations need no further
   ordering. Other tagged declarations defer their dependent storage entry. */
int defer_global_symbol(CppSymbol *symbol)
{
    CppSymbol *dependency;
    unsigned kind;
    if (pch_separate_debug) return 0;
    kind = symbol->kind & 0x1f;
    if (kind == 4 || kind == 5)
        return (symbol->flags & 8) && !(symbol->declaration_flags & 1);
    if ((symbol->storage[0] & 7) == 7) {
        dependency = cpp_context_stack->items[symbol->type->field08];
        if (dependency && !(symbol->declaration_flags & 1) && defer_global_symbol(dependency))
            return 1;
    }
    return 0;
}

/* Original 0x55ed0..0x55efd, stdcall8. Local static storage keeps
   its declaration-to-global alias map in the current function's arena. */
void __stdcall record_function_symbol_alias(CppSymbol *symbol, CppSymbol *alias)
{
    HashTable *table = function_symbol_aliases;
    if (!table) table = new_hash_table(31, 1, 2, 1);
    function_symbol_aliases = table;
    set_hash_value(table, symbol, alias);
}

/* Original 0x55f00..0x55f12, stdcall4. The recorded function-local
   declaration is the key; the value supplies its serialized global identity. */
CppSymbol *__stdcall symbol_alias(CppSymbol *symbol)
{
    return find_hash_value(function_symbol_aliases, symbol);
}

CppSymbol *atexit_function; /* original 0x97be4: compiler-created declaration */

/* Original 0x341b0..0x34241, stdcall4. Emit a call registering the
   generated cleanup function; the compiler itself never calls atexit here. */
void __stdcall emit_atexit_registration(CppSymbol *function)
{
    IrToken *argument, *arguments, *callee, *call;
    if (!atexit_function) {
        atexit_function = new_symbol(name_atexit, 0);
        atexit_function->kind = 7;
        atexit_function->type = make_function_type(0x11, type_0064);
        atexit_function->cpp.parts.token = 1;
        atexit_function->declaration_flags =
            (atexit_function->declaration_flags & ~0x100) | 0x200;
    }
    argument = builtin_unary_expression(0x3a, symbol_reference(function));
    arguments = append_expression(0, argument, 2);
    callee = symbol_reference(atexit_function);
    call = new_left_typed_expression(0x7a, callee, arguments);
    call->type = type_0064;
    emit_expression_statement(call);
}

/* Original 0x4f370..0x4f3b6, stdcall4. Retain the definition in its
   declaration-mode list and ensure any synthesized destructor is emitted. */
void __stdcall defer_object_definition(CppSymbol *symbol)
{
    WorkList *item = push_work_list(&pending_cleanup_by_scope[declaration_mode],
        declaration_mode != 0);
    item->value.pointer = symbol;
    if (is_unqualified_class_type(symbol->type))
        defer_generated_member(lookup_complete_object_destructor(symbol->type));
}

/* Original 0x4f3c0..0x4f4c5, cdecl0. Drain file-scope work,
   switching generated initializer functions around queued object cleanup. */
void emit_pending_function_cleanup(void)
{
    WorkList *item;
    CppSymbol *symbol, *tag;
    CType *saved_type;
    IrToken *cleanup;
    int emitted_function = 0;
    while ((item = pending_cleanup_by_scope[0]) != 0) {
        pop_work_list(&pending_cleanup_by_scope[0]);
        symbol = item->value.pointer;
        if ((symbol->kind & 0x1f) == 7) {
            if (token_allocation_flags & 0x40) chain_initializer_function();
            begin_initializer_function(symbol);
            emitted_function = 1;
            continue;
        }
        tag = cpp_context_stack->items[symbol->type->field08];
        if (!(tag->detail.class_info->trivial_special_members & 8) &&
            has_destructor_member_model(symbol->type)) {
            saved_type = symbol->type;
            if (!(token_allocation_flags & 0x40)) begin_initializer_function(0);
            symbol->type = remove_type_qualifiers(saved_type, 0x3000);
            if (classify_incomplete_type(symbol->type) & 2)
                cleanup = make_array_destructor_call(resolve_symbol_expression(symbol));
            else {
                cleanup = builtin_unary_expression(OP_BIT_AND, resolve_symbol_expression(symbol));
                tag = cpp_context_stack->items[symbol->type->field08];
                cleanup = make_destructor_call(tag, cleanup, 3, 1, 0);
            }
            emit_expression_statement(cleanup);
            symbol->type = saved_type;
        }
        if (emitted_function) {
            flush_current_function(0);
            emitted_function = 0;
        }
    }
}

/* Original 0x48510..0x4855e, stdcall4. Synthetic initializer
   functions enter the ordinary body and scope machinery with capture enabled. */
void __stdcall begin_initializer_function(CppSymbol *symbol)
{
    token_allocation_flags |= 0x40;
    function_symbol = prepare_function_definition(symbol);
    token_allocation_flags |= 0x80000;
    begin_function_body();
    enter_compound_scope();
    function_symbol->flags |= 4;
    declaration_mode = 0;
    function_symbol->declaration_flags =
        (function_symbol->declaration_flags & ~0x100) | 0x200;
}
