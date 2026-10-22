/* Entry to compound-statement scope processing. */
#include "statement-scopes.h"
#include "lexer.h"
#include "class-finalization.h"
#include "work-list.h"
#include <recon.h>

SymbolScope *closed_scopes; /* original 0x914d0 */

/* Original 0x555b0..0x555b5 is a tail transfer to the shared scope entry. */
void begin_compound_statement(void)
{
    enter_compound_scope();
}

/* Original 0x555c0..0x5574c. Enter an ordinary statement scope and, at
   function-body depth, begin the appropriate C++ special-member body. */
void enter_compound_scope(void)
{
    ArenaPool *pool;
    SymbolScope *scope;
    if (browser_filename) browser_begin_tag();
    ++declaration_mode;
    global_record_kind = declaration_mode;
    ++current_declaration_scope;
    if (!(token_allocation_flags & 0x40)) {
        pool = &compiler_arena.pools[1];
        if (pool->remaining < (int)sizeof(SymbolScope))
            refill_arena(&compiler_arena, pool);
        scope = (SymbolScope *)pool->next;
        pool->remaining -= sizeof(SymbolScope);
        pool->next += sizeof(SymbolScope);
        scope->bucket_mask = 15;
        scope->flags = (scope->flags & 0xff00) | declaration_mode;
        scope->parent = current_scope;
        push_symbol_scope(scope);
        record_scope_directory_entry(scope);
    }
    write_ir_record(0x53, 0);
    if (options.configuration.option_qc && normalized_Ol && declaration_mode == 2)
        begin_qc_control(0);
    if (declaration_mode >= 2) record_compound_scope();
    if (!options.prototypes_only && declaration_mode == 2 &&
        (!syntax_diagnostic_count || function_symbol)) {
        if (function_symbol->declaration_flags & 0x1000) {
            emit_constructor_initializers(qualified_definition_class, pending_constructor_initializers);
            pending_constructor_initializers = 0;
            return;
        }
        if (pending_constructor_initializers) {
            report_syntax_error(550, format_symbol_name(function_symbol));
            return;
        }
        if (function_symbol->declaration_flags & 0x2000) {
            begin_destructor_body(qualified_definition_class);
            return;
        }
        if (function_symbol->name == name_delete_dtor) {
            emit_deleting_destructor_body(qualified_definition_class);
            return;
        }
        if (function_symbol->name == name_vector_delete_dtor) {
            emit_vector_deleting_destructor_body(qualified_definition_class);
            return;
        }
        if (function_symbol->name == name_default_ctor_closure)
            emit_default_ctor_closure_body(qualified_definition_class);
    }
}

/* Original 0x55750..0x55755 is the separate native tail-entry wrapper. */
void end_compound_statement(void)
{
    leave_compound_scope();
}

/* Original 0x55760..0x558a6: close the lexical/IR scope before retiring
   symbols, preserving optional inline-analysis state and scope ordering. */
void leave_compound_scope(void)
{
    SymbolScope *scope;
    if (!declaration_mode) return;
    if (browser_filename) browser_end_tag();
    if (declaration_mode >= 2) end_compound_scope_record();
    if (declaration_mode == 2 && current_return_type && !(cpp_parser_flags & 0x8000))
        emit_return_statement(0, 0);
    if (declaration_mode == 2) {
        current_declaration_scope = 0;
        finish_function_records();
    }
    scope = current_scope;
    if (!(token_allocation_flags & 0x40)) {
        if ((unsigned char)scope->flags != declaration_mode) fatal_error(506);
        retire_scope_directory_entry();
        pop_symbol_scope();
        if (scope->buckets && options.configuration.option_qc && normalized_Ol &&
            declaration_mode > 2 && !scope->qc_context)
            end_qc_control();
        if (options.configuration.option_qc) scope->qc_context = 0;
        if (options.configuration.option_qc && normalized_Ol && declaration_mode == 2)
            end_qc_control();
        emit_scope_cleanup_references();
    }
    if (!options.prototypes_only) {
        if (!(token_allocation_flags & 0x40)) queue_closed_scope(scope);
        write_ir_record(0x54, make_integer_constant(declaration_mode));
    }
    --declaration_mode;
    global_record_kind = declaration_mode;
}

/* Original 0x53ee0..0x53f39. Cleanup entries are pairs: the scope-depth
   word is on top of the symbol to reference at that depth. */
void emit_scope_cleanup_references(void)
{
    if (options.prototypes_only) return;
    while (scope_cleanup_stack) {
        if (scope_cleanup_stack->value.byte != declaration_mode) return;
        pop_work_list(&scope_cleanup_stack);
        write_ir_record(0x2b, symbol_reference(scope_cleanup_stack->value.pointer));
        pop_work_list(&scope_cleanup_stack);
    }
}

/* Original 0x558b0..0x558fd. Retain depth ordering, then place a scope
   immediately before any remaining descendants of that scope. */
void __stdcall queue_closed_scope(SymbolScope *scope)
{
    SymbolScope *previous, *cursor;
    if (!scope) return;
    previous = 0;
    cursor = closed_scopes;
    while (cursor) {
        if ((unsigned char)scope->flags < (unsigned char)cursor->flags) break;
        previous = cursor;
        cursor = cursor->next;
    }
    while (cursor && cursor->parent != scope) {
        previous = cursor;
        cursor = cursor->next;
    }
    if (previous) previous->next = scope;
    else closed_scopes = scope;
    scope->next = cursor;
}
