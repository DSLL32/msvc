/* Narrow projection regression for the newly encountered exception counters. */
#include "frontend.h"
#include <recon.h>

int c1xx_test_exception_counters(void)
{
    if (frontend_data.recursion_depth || frontend_bss.error_count)
        return 0;
    if (initialize_compiler_pass())
        return 0;
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
    frontend_data.current_scope = (struct CompilerScope *)&frontend_data;
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
    frontend_data.canonical_token_records[137] = (struct CompilerTokenRecord *)&frontend_data;
    frontend_data.keyword_table = (struct CompilerKeywordTable *)&frontend_data;
    frontend_data.current_identifier = (struct CompilerIdentifier *)&frontend_data;
    frontend_data.frontend_pointer_table = (struct CompilerPointerArray *)&frontend_data;
    frontend_data.empty_pointer_array = (struct CompilerPointerArray *)&frontend_data;
    frontend_data.pch_expression_file = 149;
    frontend_bss.expression_seek_result = 151;
    frontend_bss.output_buffers[3].filename[259] = 113;
    frontend_data.constant_arena_index = 7;
    frontend_data.force_type_serialization = 157;
    frontend_data.type_debug_mode = 163;
    frontend_data.reference_context = 167;
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
        frontend_data.current_scope == NULL && frontend_data.builtin_types.type_0060 == NULL &&
        frontend_bss.text_input == NULL && frontend_bss.browser_filename == NULL &&
        frontend_bss.pch_replayed == 0 && frontend_bss.pch_file_failed == 0 &&
        frontend_bss.output_kinds[3] == 0 && frontend_bss.preprocessor_output == NULL &&
        frontend_bss.lexer_backend == NULL && frontend_bss.saved_options.normalized_stack_checks == 0 &&
        frontend_bss.processor_instruction_mask == 0 && frontend_data.browser_mode == 0 &&
        frontend_data.named_browser == NULL && frontend_data.initial_backend == NULL &&
        frontend_data.canonical_token_records[137] == NULL && frontend_data.keyword_table == NULL && frontend_data.current_identifier == NULL && frontend_data.frontend_pointer_table == NULL && frontend_data.empty_pointer_array == NULL && frontend_data.pch_expression_file == 0 &&
        frontend_bss.expression_seek_result == 0 &&
        frontend_bss.output_buffers[3].filename[259] == 0 &&
        frontend_data.constant_arena_index == 5 &&
        frontend_data.force_type_serialization == 0 && frontend_data.type_debug_mode == 1 &&
        frontend_data.reference_context == 1 && frontend_bss.ir_record_count == 0 &&
        frontend_bss.current_ir_symbol == NULL && frontend_bss.current_function == NULL;
}
