/* Directive state follows the actual initialized-data and BSS reset spans. */
#include "frontend.h"
#include <string.h>
#include <recon.h>

int c1xx_test_directive_state_reset(void)
{
    if (frontend_data.pch_creation_name_seen || initialize_compiler_pass()) return 0;
    frontend_data.pch_creation_name_seen = 0x19a73;
    frontend_data.member_expression_context = (struct CompilerExpression *)0x7c8;
    frontend_bss.access_diagnostic_symbol = (struct CompilerSymbol *)0x191;
    frontend_bss.access_diagnostic_base_code = 0x27;
    frontend_bss.access_diagnostic_owner = (struct CompilerSymbol *)0x383;
    frontend_bss.access_diagnostic_base = (struct CompilerSymbol *)0x494;
    frontend_bss.access_diagnostic_ambiguous = 0x5a5;
    frontend_bss.initializer_access_mode = 0x8d9;
    frontend_bss.access_diagnostic_code = 0x6b;
    frontend_data.binding_arena_index = 8;
    frontend_data.saved_binding_arena_index = 7;
    frontend_data.constructor_aliases = (struct CompilerHashTable *)0x5a70;
    frontend_data.member_initializer_depth = 0x316;
    frontend_data.member_initializer_state = 0x427;
    frontend_bss.initializer_context.constructor_state_index = 0x538;
    frontend_bss.initializer_context.cleanup_pending_count = 0x649;
    frontend_bss.initializer_context.last_cleanup_state_index = 0x75a;
    frontend_data.enumeration_type = (struct CompilerType *)0x947;
    frontend_data.enumeration_value = 0x1928374655LL;
    frontend_bss.current_declaration = (struct CompilerSymbol *)0xa58;
    frontend_bss.last_extracted_type_payload = (void *)0x35a81;
    frontend_bss.options.preprocess_stdout = 0x7319;
    frontend_bss.macro_parameter_count = -7;
    frontend_data.conditional_token = 0x73;
    frontend_data.conditional_parentheses = 12;
    frontend_data.declaration_references.fallback = (struct CompilerSymbol *)0x1351;
    frontend_data.declaration_references.type_context = (struct CompilerSymbol *)0x2462;
    frontend_data.declaration_references.definition = (struct CompilerSymbol *)0x3573;
    frontend_data.initializer_reset_values[0] = 0x181;
    frontend_data.initializer_reset_values[1] = 0x292;
    frontend_data.initializer_reset_values[2] = 0x3a3;
    frontend_bss.initializer_cleanup_owner = (struct CompilerSymbol *)0x724;
    frontend_bss.initializer_pending_state = 0x114;
    frontend_bss.initializing_symbol = (struct CompilerSymbol *)0x225;
    frontend_bss.initializer_value_state = 0x336;
    frontend_bss.initializer_saved_value_state = 0x559;
    frontend_bss.initializer_end_label = (struct CompilerExpression *)0x66a;
    frontend_bss.initializer_zero_bytes = 0x102030405060708LL;
    frontend_data.initializer_context_count = 0x447;
    frontend_data.last_initializer_symbol = (struct CompilerSymbol *)0x893;
    frontend_bss.initializer_context.head = (CompilerWorkLink *)0x558;
    frontend_bss.initializer_context.symbol = (struct CompilerSymbol *)0x669;
    frontend_bss.initializer_context.source_state = 0x77a;
    frontend_bss.initializer_context.state_index = 0x88b;
    frontend_bss.conditional_suppression = 9;
    frontend_bss.macro_input_mode = 7;
    frontend_bss.macro_whitespace_pending = 3;
    frontend_bss.macro_source_position = 0x9523;
    frontend_bss.macro_expansion_frames[0].disabled = 0x17;
    frontend_bss.macro_expansion_frames[255].disabled = 0x39;
    frontend_bss.macro_timestamp[24] = 'x';
    memset(&frontend_bss.conditional_value, 0x39, sizeof(frontend_bss.conditional_value));
    frontend_bss.conditional_states[0] = 0x31897;
    frontend_bss.conditional_states[2047] = 0x573a9;
    frontend_data.first_invocation = 0;
    if (initialize_compiler_pass()) return 0;
    return frontend_bss.initializer_access_mode == 0 &&
        frontend_data.member_expression_context == NULL &&
        frontend_bss.access_diagnostic_symbol == NULL &&
        frontend_bss.access_diagnostic_base_code == 0 && frontend_bss.access_diagnostic_owner == NULL &&
        frontend_bss.access_diagnostic_base == NULL && frontend_bss.access_diagnostic_ambiguous == 0 &&
        frontend_bss.access_diagnostic_code == 0 && frontend_data.constructor_aliases == NULL && frontend_data.binding_arena_index == 0 && frontend_data.saved_binding_arena_index == 0xffffffffU && frontend_data.enumeration_type == NULL && frontend_data.enumeration_value == -1 &&
        frontend_data.member_initializer_depth == 0 && frontend_data.member_initializer_state == 0 &&
        frontend_bss.initializer_context.constructor_state_index == 0 &&
        frontend_bss.initializer_context.cleanup_pending_count == 0 &&
        frontend_bss.initializer_context.last_cleanup_state_index == 0 &&
        frontend_bss.last_extracted_type_payload == NULL &&
        frontend_bss.current_declaration == NULL && frontend_data.pch_creation_name_seen == 0 && frontend_bss.options.preprocess_stdout == 0 &&
        frontend_bss.macro_parameter_count == 0 &&
        frontend_data.conditional_token == 1 && frontend_data.conditional_parentheses == 0 &&
        frontend_data.declaration_references.fallback == NULL &&
        frontend_data.declaration_references.type_context == NULL &&
        frontend_data.declaration_references.definition == NULL &&
        frontend_data.initializer_reset_values[0] == 0 &&
        frontend_data.initializer_reset_values[1] == 0 &&
        frontend_data.initializer_reset_values[2] == 0 &&
        frontend_bss.initializer_cleanup_owner == NULL &&
        frontend_bss.initializer_pending_state == 0 &&
        frontend_bss.initializing_symbol == NULL && frontend_bss.initializer_value_state == 0 &&
        frontend_bss.initializer_saved_value_state == 0 && frontend_bss.initializer_zero_bytes == 0 &&
        frontend_bss.initializer_end_label == NULL &&
        frontend_data.initializer_context_count == 0 &&
        frontend_data.last_initializer_symbol == NULL &&
        frontend_bss.initializer_context.head == NULL && frontend_bss.initializer_context.symbol == NULL &&
        frontend_bss.initializer_context.source_state == 0 && frontend_bss.initializer_context.state_index == 0 &&
        frontend_bss.conditional_suppression == 0 &&
        frontend_bss.macro_input_mode == 0 && frontend_bss.macro_whitespace_pending == 0 &&
        frontend_bss.macro_source_position == 0 &&
        frontend_bss.macro_expansion_frames[0].disabled == 0 &&
        frontend_bss.macro_expansion_frames[255].disabled == 0 && frontend_bss.macro_timestamp[24] == 0 &&
        frontend_bss.conditional_value.type_code == 0 && frontend_bss.conditional_value.integer == 0 &&
        frontend_bss.conditional_states[0] == 0 && frontend_bss.conditional_states[2047] == 0;
}
