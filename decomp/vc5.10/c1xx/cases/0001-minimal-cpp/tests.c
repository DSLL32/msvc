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
        frontend_bss.saved_options.pch_option_flags == 0;
}
