/* Entry to compound-statement scope processing. */
#include "statement-scopes.h"
#include "lexer.h"

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
