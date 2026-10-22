/* Closing output and compiler context for a completed function definition. */
#include "function-finalization.h"
#include "parameter-scopes.h"
#include "lexer.h"
#include "statement-scopes.h"
#include "work-list.h"
#include "control-ir.h"

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
