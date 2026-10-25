/* Narrow projection regression for the newly encountered exception counters. */
#include <string.h>
#include "frontend.h"
#include <recon.h>

int c1xx_test_exception_counters(void)
{
    CompilerBuiltinNames empty_names = { NULL };
    unsigned record_bytes[26], size_class[26];
    struct CompilerSymbolLink *empty_recycled[11] = { NULL };
    if (frontend_data.recursion_depth || frontend_bss.error_count)
        return 0;
    if (initialize_compiler_pass())
        return 0;
    memcpy(record_bytes, frontend_data.symbol_record_bytes, sizeof(record_bytes));
    memcpy(size_class, frontend_data.symbol_size_class, sizeof(size_class));
    frontend_data.recursion_depth = 17;
    frontend_bss.error_count = 23;
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
    frontend_bss.declaration_depth = 211;
    frontend_bss.parser_context_depth = 17;
    frontend_bss.parameter_contexts[0] = (struct CompilerParameterContext *)&frontend_data;
    frontend_data.compiler_mode_flags = 223;
    frontend_data.declaration_context_index = 5;
    frontend_data.declaration_mode_index = 7;
    frontend_data.empty_parameters = (struct CompilerParameterTypes *)&frontend_data;
    frontend_bss.default_declaration_mode = 19;
    memset(frontend_bss.macro_date_time, 0x97, sizeof(frontend_bss.macro_date_time));
    frontend_bss.macro_date = frontend_bss.macro_date_time + 3;
    frontend_bss.macro_time = frontend_bss.macro_date_time + 17;
    frontend_bss.forced_include_index = 0x71ac;
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
    frontend_bss.ir_record_count = 173;
    frontend_bss.current_ir_symbol = (struct CompilerSymbol *)&frontend_data;
    frontend_bss.current_function = (struct CompilerSymbol *)&frontend_data;
    frontend_data.browser_mode = 139;
    frontend_data.named_browser = (struct CompilerBackend *)&frontend_data;
    frontend_data.initial_backend = (struct CompilerBackend *)&frontend_data;
    if (initialize_compiler_pass())
        return 0;
    return frontend_data.recursion_depth == 0 && frontend_bss.error_count == 0 &&
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
        frontend_bss.declaration_depth == 0 && frontend_data.compiler_mode_flags == 0 &&
        frontend_bss.parser_context_depth == 0 && frontend_bss.parameter_contexts[0] == NULL &&
        frontend_data.declaration_context_index == -1 &&
        frontend_data.declaration_mode_index == -1 && frontend_data.empty_parameters == NULL && frontend_bss.default_declaration_mode == 0 &&
        frontend_bss.macro_date_time[0] == 0 && frontend_bss.macro_date_time[25] == 0 &&
        frontend_bss.macro_date == NULL && frontend_bss.macro_time == NULL &&
        frontend_bss.forced_include_index == 0 && frontend_bss.pending_forced_includes == NULL &&
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
