/* Narrow projection regression for the newly encountered exception counters. */
#include <string.h>
#include "frontend.h"
#include <recon.h>

int c1xx_test_exception_counters(void)
{
    CompilerBuiltinNames empty_names = { NULL };
    unsigned class_row;
    char empty_database_path[256] = { 0 };
    char empty_numeric_text[128] = { 0 };
    CompilerPredefinedOperator operator_defaults[51];
    unsigned record_bytes[26], size_class[26];
    struct CompilerSymbolLink *empty_recycled[11] = { NULL };
    if (frontend_data.recursion_depth || frontend_bss.error_count)
        return 0;
    if (initialize_compiler_pass())
        return 0;
    memcpy(operator_defaults, frontend_data.predefined_operators, sizeof(operator_defaults));
    memcpy(record_bytes, frontend_data.symbol_record_bytes, sizeof(record_bytes));
    memcpy(size_class, frontend_data.symbol_size_class, sizeof(size_class));
    memset(frontend_data.predefined_time_text, 0xa9, sizeof(frontend_data.predefined_time_text));
    frontend_data.next_symbol_record_id = 0x1734a579;
    frontend_data.emit_global_locations = 23;
    frontend_bss.global_output_active = 0xa7;
    frontend_data.source_frames = (struct CompilerSourceFrame *)&frontend_data;
    frontend_data.root_source_frame = (struct CompilerSourceFrame *)&frontend_bss;
    frontend_data.free_source_frames = (struct CompilerSourceFrame *)&frontend_data.options;
    frontend_data.arrow_identifier = (CompilerNameEntry *)&frontend_data;
    frontend_data.call_identifier = (CompilerNameEntry *)&frontend_bss;
    frontend_data.zero_constant = (struct CompilerExpression *)&frontend_data.options;
    frontend_data.free_parameter_sets = (struct CompilerParameterSet *)&frontend_bss;
    frontend_data.parameter_sentinel = (struct CompilerSymbol *)&frontend_data;
    frontend_data.purecall_symbol = (struct CompilerSymbol *)&frontend_bss;
    frontend_data.declaration_scope_number = 0x91a7;
    frontend_data.debug_module_kind = 0x91f7;
    frontend_data.browser_alternate_position = 0x1729;
    frontend_data.active_parser = (struct CompilerParser *)&frontend_data;
    frontend_data.initial_source_setup_done = 0x71a9;
    frontend_data.diagnostic_depth = 0x1739;
    frontend_data.diagnostic_records = (struct CompilerDiagnosticRecord *)&frontend_bss;
    frontend_data.diagnostic_record_count = 71;
    frontend_data.next_diagnostic_context = 0x2159;
    frontend_data.token_text_cursor = (char *)&frontend_bss;
    frontend_bss.browser_context_position = 0x83b7;
    frontend_bss.pdb_signature = 0x174591a7;
    memset(frontend_bss.incremental_database_path, 0x59, sizeof(frontend_bss.incremental_database_path));
    memset(frontend_data.predefined_operators, 0x59, sizeof(frontend_data.predefined_operators));
    frontend_data.source_descriptor = 117;
    frontend_data.conditional_depth = 29;
    frontend_data.source_mode_flags = 0x17345a79;
    frontend_bss.source_cursor = (BYTE *)&frontend_bss.context_stack;
    frontend_bss.declaration_scope_mode = 0xa9;
    frontend_bss.external_declaration_mode = 0x83;
    frontend_bss.current_parameters = (struct CompilerParameterSet *)&frontend_data;
    frontend_bss.current_parameter_kind = 0x219;
    for (class_row = 0; class_row < 60; ++class_row) {
        frontend_bss.parser_qualification_scopes[class_row] = (struct CompilerSymbol *)&frontend_bss;
        frontend_bss.declaration_controls[class_row].flags = (BYTE)(class_row + 1);
        frontend_bss.declaration_controls[class_row].secondary_flags = (BYTE)(class_row + 0x41);
    }
    frontend_data.source_include_depth = 19;
    frontend_bss.current_macro_text = (char *)&frontend_data;
    frontend_data.macro_reset_value = 0x317;
    frontend_data.declaration_classification_depth = 0x1937;
    frontend_data.parser_classification_state = -713;
    frontend_data.active_declaration_parser = (struct CompilerDeclarationParser *)&frontend_bss;
    frontend_data.allocation_operator = 0x9d;
    frontend_data.source_exhausted = -19;
    frontend_data.control_option_kind = 0x1395;
    frontend_data.control_option_text = (const char *)&frontend_bss;
    frontend_data.global_options = (struct CompilerGlobalOption *)&frontend_bss;
    frontend_data.inline_depth = 0x1937;
    frontend_data.inline_recursion = 0x1938;
    frontend_data.line_range_warning_enabled = -38;
    frontend_bss.control_text_count = 0x93;
    frontend_bss.control_byte = 0x94;
    frontend_bss.control_short = -394;
    frontend_data.disable_inline_records = -39;
    frontend_data.auto_inline = 31;
    frontend_data.function_reference_counter = 0x198adc;
    frontend_data.function_location_cache[0] = 0x198add;
    frontend_data.function_location_cache[1] = 0x198ade;
    frontend_bss.function_body_state = 0x198ac;
    frontend_bss.stack_check_warning_emitted = -38;
    frontend_bss.function_entry_label = (struct CompilerExpression *)&frontend_data;
    frontend_bss.function_parameter_count = 0x1391;
    frontend_bss.saved_reference_count = 0x1392;
    frontend_bss.local_reference_count = 0x1393;
    frontend_bss.alternate_output_flush = -0x1394;
    frontend_bss.function_source_position = 0x198ad;
    frontend_bss.function_reset_state[0] = 0x1937;
    frontend_bss.function_reset_state[1] = 0x1938;
    frontend_bss.function_reset_state[2] = 0x1939;
    frontend_data.function_result_type = (CompilerType *)&frontend_bss;
    frontend_data.default_code_segment = (struct CompilerSymbol *)&frontend_data;
    frontend_bss.function_return_class = (CompilerType *)&frontend_data;
    frontend_bss.current_parser_expression = (struct CompilerExpression *)&frontend_data;
    memset(&frontend_bss.declaration_token_value, 0x97, sizeof(frontend_bss.declaration_token_value));
    frontend_bss.macro_token_cursor = (unsigned char *)&frontend_data;
    frontend_bss.macro_text_cursor = (unsigned char *)&frontend_bss;
    frontend_bss.saved_macro_text_cursor = (unsigned char *)&frontend_data + 3;
    frontend_data.recursion_depth = 17;
    frontend_bss.error_count = 23;
    frontend_bss.suppress_parser_codegen = 0x1739;
    frontend_bss.tag_novtordisp = 0x173ab59;
    memset(frontend_bss.declaration_specs, 0xa7, sizeof(frontend_bss.declaration_specs));
    frontend_bss.declaration_nesting = 0x173945;
    frontend_bss.pending_member_queue = (struct CompilerMemberQueueState *)&frontend_data;
    frontend_bss.pending_member_error_count = 0x31579;
    frontend_bss.active_parameters = (struct CompilerParameterSet *)&frontend_data;
    frontend_bss.active_parameter_kind = 0x719a;
    frontend_bss.processing_pending_parser_input = 0x193a;
    frontend_bss.pending_parser_work = 0x7319;
    frontend_bss.qualified_lookup_recursion = 0x1937;
    frontend_bss.quoted_skip_mode = 0x1937;
    frontend_bss.lexical_skip_mode = -0x315;
    frontend_data.secondary_translation_symbols = (CompilerVector *)&frontend_bss;
    frontend_data.deferred_input_depth = 0x197a;
    frontend_data.pending_declarations = &frontend_bss.options;
    frontend_data.parser_input_end_state = (struct ParserInputEndState *)&frontend_bss;
    frontend_data.expression_work_current = (struct CompilerExpressionWork *)&frontend_data.options;
    frontend_data.expression_work_head = (struct CompilerExpressionWorkNode *)&frontend_bss.options;
    frontend_data.declaration_cleanup_state[0] = 0x7193;
    frontend_data.declaration_cleanup_state[1] = 0x8159;
    frontend_data.declaration_cleanup_state[2] = 0xa713;
    frontend_bss.declaration_controls[0].flags = 0x91;
    frontend_bss.declaration_controls[0].secondary_flags = 0x7a;
    frontend_bss.tag_declaration_changed = 0x2184ab7;
    frontend_bss.type_scratch_index = 0x1739;
    frontend_bss.first_type_scratch_frame.flags = 0x9841;
    frontend_bss.first_type_scratch_frame.attributes = 0x2317;
    frontend_bss.first_type_scratch_frame.shape = (struct CompilerTypeShape *)&frontend_data;
    frontend_bss.first_type_scratch_frame.auxiliary = 0x7194;
    frontend_bss.first_type_scratch_frame.debug_cache = 0x1329947;
    memset(frontend_bss.numeric_literal_text, 0xa6, sizeof(frontend_bss.numeric_literal_text));
    frontend_bss.saved_token_value.literal.type = (CompilerType *)&frontend_data;
    frontend_bss.saved_token_value.literal.value = 0x173945a7ab8913ee;
    frontend_bss.token_value.literal.type = (CompilerType *)&frontend_bss;
    frontend_bss.token_value.literal.value = 0x3179a5b7c918def1;
    frontend_bss.sentinel_token.value.literal.type = (CompilerType *)&frontend_data.options;
    frontend_bss.sentinel_token.value.literal.value = 0x2134ab798a8713ee;
    frontend_bss.parser_lookahead = (struct ParserTokenView *)&frontend_data;
    frontend_bss.parser_production = 0x1735;
    frontend_data.pch_option_flags = 37;
    frontend_data.saved_pch_mode = 41;
    frontend_data.report_statistics = 43;
    frontend_data.backend = (struct CompilerBackend *)&frontend_data;
    frontend_data.translation_unit = (struct CompilerTranslationUnit *)&frontend_data;
    frontend_data.dependency = (struct CompilerDependency *)&frontend_data;
    frontend_bss.initial_option_flags = 47;
    frontend_bss.pch_input_ready = 53;
    frontend_bss.pch_input_file = (char *)&frontend_data;
    frontend_bss.global_expression_scope_active = 59;
    frontend_bss.forced_source_index = 61;
    frontend_bss.debug_module = (struct CompilerDebugModule *)&frontend_data;
    frontend_bss.input = (struct CompilerInput *)&frontend_data;
    frontend_bss.preprocessed_stream = (FILE *)&frontend_data;
    frontend_bss.saved_options.language_extensions = 67;
    frontend_bss.saved_options.database_path[3] = 'x';
    frontend_bss.saved_options.pch_link_path[5] = 'y';
    frontend_bss.saved_options.pch_option_flags = 71;
    frontend_data.normalized_stack_checks = 73;
    frontend_data.normalized_oq = 79;
    frontend_data.normalized_pool_strings = 83;
    frontend_data.normalized_readonly_strings = 89;
    frontend_data.browser_enabled = 97;
    frontend_data.browser_full = 101;
    frontend_data.pch_separate_debug = 103;
    frontend_data.current_vtordisp = 107;
    frontend_data.bool_identifier = (CompilerNameEntry *)&frontend_data;
    memset(&frontend_data.builtin_names, 0xa5, sizeof(frontend_data.builtin_names));
    frontend_data.builtin_types.type_0060 = (CompilerType *)&frontend_data;
    frontend_bss.text_input = (struct CompilerTextInput *)&frontend_data;
    frontend_bss.browser_filename = (char *)&frontend_data;
    frontend_bss.pch_replayed = 109;
    frontend_bss.pch_file_failed = 113;
    frontend_bss.output_kinds[3] = 127;
    frontend_bss.preprocessor_output = (FILE *)&frontend_data;
    frontend_bss.lexer_backend = (CompilerLexerBackend *)&frontend_data;
    frontend_bss.saved_options.normalized_stack_checks = 131;
    frontend_bss.processor_instruction_mask = 137;
    frontend_bss.declaration_context.class_flags = 0xd3;
    frontend_bss.declaration_context.context_flags = 0xa6;
    frontend_bss.declaration_context.member_flags = 0x59;
    frontend_bss.declaration_context.reference_flags = 0x37;
    frontend_bss.declaration_context.reference = (struct CompilerSymbol *)&frontend_data;
    frontend_bss.declaration_context.attachment = (struct CompilerDeclarationAttachment *)&frontend_bss;
    frontend_bss.declaration_context.metadata = (struct CompilerDeclarationMetadata *)&frontend_data.options;
    frontend_bss.declaration_context.function_context_active = 37;
    frontend_bss.declaration_context.function_context = (struct CompilerFunctionContext *)&frontend_bss.context_stack;
    frontend_bss.declaration_context.class_context = 41;
    frontend_bss.declaration_depth = 211;
    frontend_bss.parser_context_depth = 17;
    frontend_bss.parameter_contexts[0] = (struct CompilerParameterContext *)&frontend_data;
    frontend_bss.parameter_contexts[1] = (struct CompilerParameterContext *)&frontend_bss;
    frontend_data.compiler_mode_flags = 223;
    frontend_data.declaration_context_index = 5;
    frontend_data.declaration_mode_index = 7;
    frontend_data.empty_parameters = (struct CompilerParameterTypes *)&frontend_data;
    frontend_bss.source_control.declaration_mode = 19;
    memset(frontend_bss.macro_date_time, 0x97, sizeof(frontend_bss.macro_date_time));
    frontend_bss.macro_date = frontend_bss.macro_date_time + 3;
    frontend_bss.macro_time = frontend_bss.macro_date_time + 17;
    frontend_bss.forced_include_index = 0x71ac;
    frontend_bss.default_source_name = (const char *)0x1234;
    frontend_bss.pending_forced_includes = (CompilerVector *)&frontend_data;
    frontend_data.canonical_token_records[137] = (struct CompilerTokenRecord *)&frontend_data;
    frontend_data.keyword_table = (struct CompilerKeywordTable *)&frontend_data;
    frontend_data.current_identifier = (CompilerNameEntry *)&frontend_data;
    frontend_data.frontend_pointer_table = (struct CompilerPointerArray *)&frontend_data;
    frontend_data.empty_pointer_array = (struct CompilerPointerArray *)&frontend_data;
    frontend_data.pch_expression_file = 149;
    frontend_bss.expression_seek_result = 151;
    frontend_bss.output_buffers[3].filename[259] = 113;
    frontend_data.constant_arena_index = 7;
    frontend_data.tagged_type_arena_index = 3;
    frontend_data.unnamed_translation_symbol = (struct CompilerTranslationSymbol *)&frontend_data;
    frontend_data.local_size_symbol = (struct CompilerTranslationSymbol *)&frontend_bss;
    frontend_data.translation_symbols = (CompilerVector *)&frontend_data.builtin_types;
    frontend_data.force_type_serialization = 157;
    frontend_data.type_debug_mode = 163;
    frontend_data.reference_context = 167;
    frontend_data.last_ir_line_delta = 179;
    frontend_bss.ir_line_delta = 181;
    frontend_bss.symbol_arena_kind = 7;
    frontend_bss.saved_symbol_arenas = (struct CompilerSymbolArenaFrame *)&frontend_data;
    frontend_data.type_tables.types = (CompilerTypeHash *)&frontend_data;
    frontend_data.type_tables.mapped = (CompilerTypeMap *)&frontend_data;
    frontend_data.type_tables.composite = (CompilerCompositeTypeHash *)&frontend_data;
    frontend_data.type_tables.sequences = (CompilerTypeSequenceHash *)&frontend_data;
    memset(frontend_data.symbol_record_bytes, 0xa5, sizeof(record_bytes));
    memset(frontend_data.symbol_size_class, 0xa5, sizeof(size_class));
    memset(frontend_data.recycled_symbols, 0xa5, sizeof(empty_recycled));
    frontend_data.symbol_groups = (struct CompilerHashTable *)&frontend_data;
    frontend_data.class_completion_checks = 0x1739;
    frontend_data.class_definition_head = (struct ParserClassDefinitionLink **)&frontend_data;
    frontend_data.class_layout_reference = (struct CompilerSymbol *)&frontend_data.options;
    frontend_data.declaration_lookup_owner = (struct CompilerSymbol *)&frontend_bss.options;
    frontend_bss.saved_options.check_completed_class_option = 0x19a7;
    for (class_row = 0; class_row < 16; ++class_row) {
        DeclarationClassScope *row = &frontend_bss.class_scope_rows[class_row];
        row->symbol = (struct CompilerGlobalSymbolView *)&frontend_data;
        row->reset_04 = &frontend_bss; row->type = (CompilerType *)&frontend_data;
        row->reset_10 = 0x1729; row->reset_14 = 0x193a;
        row->reset_18 = 0x7a19; row->state = 0x198a;
        row->reset_1c = 0x91; row->access_flags = 0x73;
    }
    frontend_bss.ir_record_count = 173;
    frontend_bss.current_ir_symbol = (struct CompilerSymbol *)&frontend_data;
    frontend_bss.current_function = (struct CompilerSymbol *)&frontend_data;
    frontend_data.browser_mode = 139;
    frontend_data.named_browser = (struct CompilerBackend *)&frontend_data;
    frontend_data.initial_backend = (struct CompilerBackend *)&frontend_data;
    if (initialize_compiler_pass())
        return 0;
    for (class_row = 0; class_row < 16; ++class_row) {
        DeclarationClassScope *row = &frontend_bss.class_scope_rows[class_row];
        if (row->symbol || row->reset_04 || row->type || row->reset_10 ||
            row->reset_14 || row->reset_18 || row->state || row->reset_1c || row->access_flags)
            return 0;
    }
    if (frontend_data.control_option_kind || frontend_data.control_option_text || frontend_data.global_options ||
        frontend_data.inline_depth != 8 || frontend_data.inline_recursion || frontend_data.line_range_warning_enabled != 1 ||
        frontend_bss.control_text_count || frontend_bss.control_byte || frontend_bss.control_short) return 0;
    if (frontend_data.function_result_type || frontend_data.default_code_segment || frontend_bss.function_return_class || frontend_bss.current_parser_expression ||
        frontend_data.disable_inline_records || frontend_data.auto_inline != 1 || frontend_data.function_reference_counter != 0x3fffff ||
        frontend_data.function_location_cache[0] != 0xffffffff || frontend_data.function_location_cache[1] != 0xffffffff ||
        frontend_bss.function_body_state || frontend_bss.stack_check_warning_emitted || frontend_bss.function_source_position ||
        frontend_bss.function_entry_label || frontend_bss.function_parameter_count ||
        frontend_bss.saved_reference_count || frontend_bss.local_reference_count || frontend_bss.alternate_output_flush ||
        frontend_bss.function_reset_state[0] || frontend_bss.function_reset_state[1] || frontend_bss.function_reset_state[2]) return 0;
    if (frontend_data.class_completion_checks || frontend_data.class_definition_head ||
        frontend_bss.saved_options.check_completed_class_option) return 0;
    {
        const BYTE *bytes = (const BYTE *)frontend_bss.declaration_specs;
        unsigned i;
        for (i = 0; i < sizeof(frontend_bss.declaration_specs); ++i) if (bytes[i]) return 0;
    }
    if (frontend_bss.declaration_nesting || frontend_bss.pending_member_queue || frontend_bss.pending_member_error_count || frontend_data.class_layout_reference || frontend_data.declaration_lookup_owner) return 0;
    if (frontend_bss.quoted_skip_mode || frontend_bss.lexical_skip_mode || frontend_bss.qualified_lookup_recursion || frontend_bss.processing_pending_parser_input || frontend_bss.pending_parser_work ||
        frontend_data.active_declaration_parser ||
        frontend_data.secondary_translation_symbols || frontend_data.deferred_input_depth ||
        frontend_data.pending_declarations || frontend_data.parser_input_end_state ||
        frontend_data.expression_work_current || frontend_data.expression_work_head) return 0;
    {
        const BYTE *bytes = (const BYTE *)&frontend_bss.declaration_token_value;
        unsigned i;
        for (i = 0; i < sizeof(frontend_bss.declaration_token_value); ++i) if (bytes[i]) return 0;
    }
    if (frontend_bss.active_parameters || frontend_bss.active_parameter_kind ||
        frontend_data.declaration_cleanup_state[0] || frontend_data.declaration_cleanup_state[1] ||
        frontend_data.declaration_cleanup_state[2]) return 0;
    for (class_row = 0; class_row < 60; ++class_row) {
        if (frontend_bss.parser_qualification_scopes[class_row] ||
            frontend_bss.declaration_controls[class_row].flags ||
            frontend_bss.declaration_controls[class_row].secondary_flags) return 0;
    }
    if (frontend_bss.parser_lookahead || frontend_bss.parser_production)
        return 0;
    return memcmp(frontend_data.predefined_time_text, "......Unknown date......\0", 26) == 0 &&
        frontend_data.next_symbol_record_id == 16 && frontend_data.emit_global_locations == 0 &&
        frontend_bss.global_output_active == 0 && frontend_data.source_frames == NULL &&
        frontend_data.root_source_frame == NULL && frontend_data.free_source_frames == NULL &&
        frontend_data.arrow_identifier == NULL && frontend_data.call_identifier == NULL &&
        frontend_data.zero_constant == NULL &&
        frontend_data.free_parameter_sets == NULL && frontend_data.parameter_sentinel == NULL &&
        frontend_data.purecall_symbol == NULL && frontend_data.declaration_scope_number == 0 &&
        frontend_data.debug_module_kind == 2 && frontend_bss.pdb_signature == 0 &&
        frontend_data.browser_alternate_position == 0 && frontend_bss.browser_context_position == 0 &&
        frontend_data.active_parser == NULL && frontend_data.initial_source_setup_done == 0 && frontend_data.diagnostic_depth == 0 && frontend_data.diagnostic_records == NULL &&
        frontend_data.diagnostic_record_count == 0 && frontend_data.next_diagnostic_context == 100 && frontend_data.token_text_cursor == compiler_token_text &&
        memcmp(frontend_bss.incremental_database_path, empty_database_path, 256) == 0 &&
        memcmp(operator_defaults, frontend_data.predefined_operators, sizeof(operator_defaults)) == 0 &&
        frontend_data.source_descriptor == -1 && frontend_data.conditional_depth == -1 &&
        frontend_data.source_mode_flags == 0 && frontend_bss.source_cursor == NULL &&
        frontend_bss.declaration_scope_mode == 0 && frontend_bss.external_declaration_mode == 0 &&
        frontend_bss.current_parameters == NULL && frontend_bss.current_parameter_kind == 0 &&
        frontend_bss.parser_qualification_scopes[0] == NULL &&
        frontend_data.source_include_depth == -1 && frontend_bss.current_macro_text == NULL &&
        frontend_data.macro_reset_value == 0 && frontend_bss.macro_token_cursor == NULL &&
        frontend_data.declaration_classification_depth == 0 && frontend_data.parser_classification_state == 0 &&
        frontend_data.allocation_operator == 1 &&
        frontend_data.source_exhausted == 0 &&
        frontend_bss.macro_text_cursor == NULL && frontend_bss.saved_macro_text_cursor == NULL &&
        frontend_data.recursion_depth == 0 && frontend_bss.error_count == 0 && frontend_bss.suppress_parser_codegen == 0 &&
        frontend_bss.tag_novtordisp == 0 && frontend_bss.tag_declaration_changed == 0 &&
        frontend_bss.type_scratch_index == 0 &&
        frontend_bss.first_type_scratch_frame.flags == 0 &&
        frontend_bss.first_type_scratch_frame.attributes == 0 &&
        frontend_bss.first_type_scratch_frame.shape == NULL &&
        frontend_bss.first_type_scratch_frame.auxiliary == 0 &&
        frontend_bss.first_type_scratch_frame.debug_cache == 0 &&
        memcmp(frontend_bss.numeric_literal_text, empty_numeric_text, 128) == 0 &&
        frontend_bss.saved_token_value.literal.type == NULL && frontend_bss.saved_token_value.literal.value == 0 &&
        frontend_bss.token_value.literal.type == NULL && frontend_bss.token_value.literal.value == 0 &&
        frontend_bss.sentinel_token.value.literal.type == NULL && frontend_bss.sentinel_token.value.literal.value == 0 &&
        frontend_data.pch_option_flags == 0 && frontend_data.saved_pch_mode == 0 &&
        frontend_data.report_statistics == 0 && frontend_data.backend == NULL &&
        frontend_data.translation_unit == NULL && frontend_data.dependency == NULL &&
        frontend_bss.initial_option_flags == 0 && frontend_bss.pch_input_ready == 0 &&
        frontend_bss.pch_input_file == NULL && frontend_bss.global_expression_scope_active == 0 &&
        frontend_bss.forced_source_index == 0 && frontend_bss.debug_module == NULL &&
        frontend_bss.input == NULL && frontend_bss.preprocessed_stream == NULL &&
        frontend_bss.saved_options.language_extensions == 0 &&
        frontend_bss.saved_options.database_path[3] == 0 &&
        frontend_bss.saved_options.pch_link_path[5] == 0 &&
        frontend_bss.saved_options.pch_option_flags == 0 &&
        frontend_data.normalized_stack_checks == 1 && frontend_data.normalized_oq == 0 &&
        frontend_data.normalized_pool_strings == 0 && frontend_data.normalized_readonly_strings == 0 &&
        frontend_data.browser_enabled == 0 && frontend_data.browser_full == 0 &&
        frontend_data.pch_separate_debug == 0 && frontend_data.current_vtordisp == 0 &&
        frontend_data.bool_identifier == NULL &&
        memcmp(&frontend_data.builtin_names, &empty_names, sizeof(empty_names)) == 0 && frontend_data.builtin_types.type_0060 == NULL &&
        frontend_bss.text_input == NULL && frontend_bss.browser_filename == NULL &&
        frontend_bss.pch_replayed == 0 && frontend_bss.pch_file_failed == 0 &&
        frontend_bss.output_kinds[3] == 0 && frontend_bss.preprocessor_output == NULL &&
        frontend_bss.lexer_backend == NULL && frontend_bss.saved_options.normalized_stack_checks == 0 &&
        frontend_bss.processor_instruction_mask == 0 && frontend_data.browser_mode == 0 &&
        frontend_bss.declaration_context.class_flags == 0 &&
        frontend_bss.declaration_context.context_flags == 0 &&
        frontend_bss.declaration_context.member_flags == 0 &&
        frontend_bss.declaration_context.reference_flags == 0 &&
        frontend_bss.declaration_context.reference == NULL &&
        frontend_bss.declaration_context.attachment == NULL &&
        frontend_bss.declaration_context.metadata == NULL &&
        frontend_bss.declaration_context.function_context_active == 0 &&
        frontend_bss.declaration_context.function_context == NULL &&
        frontend_bss.declaration_context.class_context == 0 &&
        frontend_bss.declaration_depth == 0 && frontend_data.compiler_mode_flags == 0 &&
        frontend_bss.parser_context_depth == 0 && frontend_bss.parameter_contexts[0] == NULL &&
        frontend_bss.parameter_contexts[1] == NULL &&
        frontend_data.declaration_context_index == -1 &&
        frontend_data.declaration_mode_index == -1 && frontend_data.language_context_depth == -1 &&
        &frontend_data.declaration_mode_index == &frontend_data.language_context_depth &&
        frontend_data.empty_parameters == NULL && frontend_bss.source_control.declaration_mode == 0 &&
        frontend_bss.macro_date_time[0] == 0 && frontend_bss.macro_date_time[25] == 0 &&
        frontend_bss.macro_date == NULL && frontend_bss.macro_time == NULL &&
        frontend_bss.forced_include_index == 0 && frontend_bss.pending_forced_includes == NULL &&
        frontend_bss.default_source_name == NULL &&
        frontend_data.named_browser == NULL && frontend_data.initial_backend == NULL &&
        frontend_data.canonical_token_records[137] == NULL && frontend_data.keyword_table == NULL && frontend_data.current_identifier == NULL && frontend_data.frontend_pointer_table == NULL && frontend_data.empty_pointer_array == NULL && frontend_data.pch_expression_file == 0 &&
        frontend_bss.expression_seek_result == 0 &&
        frontend_bss.output_buffers[3].filename[259] == 0 &&
        frontend_data.constant_arena_index == 5 && frontend_data.tagged_type_arena_index == 5 &&
        frontend_data.unnamed_translation_symbol == NULL && frontend_data.local_size_symbol == NULL &&
        frontend_data.translation_symbols == NULL &&
        frontend_data.force_type_serialization == 0 && frontend_data.type_debug_mode == 1 &&
        frontend_data.reference_context == 1 && frontend_bss.ir_record_count == 0 &&
        frontend_data.last_ir_line_delta == 0 && frontend_bss.ir_line_delta == 0 &&
        frontend_bss.symbol_arena_kind == 0 && frontend_bss.saved_symbol_arenas == NULL &&
        frontend_data.type_tables.types == NULL && frontend_data.type_tables.mapped == NULL &&
        frontend_data.type_tables.composite == NULL && frontend_data.type_tables.sequences == NULL &&
        frontend_bss.current_ir_symbol == NULL && frontend_bss.current_function == NULL &&
        memcmp(frontend_data.symbol_record_bytes, record_bytes, sizeof(record_bytes)) == 0 &&
        memcmp(frontend_data.symbol_size_class, size_class, sizeof(size_class)) == 0 &&
        memcmp(frontend_data.recycled_symbols, empty_recycled, sizeof(empty_recycled)) == 0 &&
        frontend_data.symbol_groups == NULL;
}
