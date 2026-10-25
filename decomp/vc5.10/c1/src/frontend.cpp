#include <stdio.h>
#include <string.h>
#include "frontend.h"
#pragma intrinsic(memcpy)
#include <recon.h>

extern "C" {
void __cdecl initialize_frontend(void);
unsigned __cdecl pack_option_flags(void);
void __cdecl prepare_translation_unit(void);
void __cdecl load_precompiled_header(const char *, int);
void __cdecl begin_automatic_pch(void);
void __cdecl begin_debug_info(int);
void __cdecl initialize_parser(void);
void __cdecl initialize_parser_tables(void);
void __cdecl initialize_semantic_context(void);
void __cdecl parse_translation_unit(void);
void __cdecl write_ir_record(int, const void *);
void __cdecl write_global_header(void);
void *__cdecl make_integer_constant(int);
CompilerBackend *__cdecl acquire_backend(void);
void __cdecl finish_symbols(void);
void __cdecl finish_browser_output(void);
int __cdecl close_preprocessed_stream(FILE *);
void __cdecl cleanup_precompiled_files(void);
void __cdecl finish_compiler(void);
void __cdecl print_compiler_statistics(void);
void __cdecl preprocess_translation_unit(void);
void __cdecl finish_parser(int);
void __cdecl finish_parser_tables(void);
void __cdecl compiler_warning(int, ...);
void __cdecl report_response_error(int, ...);
}

/* Each assignment is a known setting within the original 1028-byte copy.
 * Unknown settings are neither filled nor semantically consumed here. Future
 * consumers of the remaining 316 bytes must stop until their types are known. */
#define SAVE_OPTION(field) c1_bss.saved_options.field = c1_data.options.field
#define SAVE_KNOWN_PCH_SETTINGS() do { \
    SAVE_OPTION(preprocess_only); SAVE_OPTION(language_extensions); \
    SAVE_OPTION(option_zbe); \
    c1_bss.saved_options.internal_zb_value = c1_data.internal_zb_value; \
    SAVE_OPTION(fastcall_calls); SAVE_OPTION(japanese_identifiers); \
    SAVE_OPTION(unsigned_char); SAVE_OPTION(dll_runtime); \
    SAVE_OPTION(multithread_runtime); SAVE_OPTION(pascal_calls); \
    SAVE_OPTION(debug_info); SAVE_OPTION(pch_debug); \
    SAVE_OPTION(preprocess_no_lines); SAVE_OPTION(option_pf); \
    SAVE_OPTION(option_bu); SAVE_OPTION(reproducible); \
    SAVE_OPTION(intrinsics); SAVE_OPTION(stack_checks); \
    SAVE_OPTION(cdecl_calls); SAVE_OPTION(stdcall_calls); \
    SAVE_OPTION(pool_strings); SAVE_OPTION(readonly_strings); \
    SAVE_OPTION(code_based_strings); SAVE_OPTION(processor); \
    SAVE_OPTION(minimal_rebuild); SAVE_OPTION(inline_level); \
    c1_bss.saved_options.option_flags = c1_data.pch_option_flags; \
    SAVE_OPTION(dependency_output); \
    c1_bss.saved_options.dependency_local = c1_data.dependency_local; \
    SAVE_OPTION(optimize_size); SAVE_OPTION(assume_no_alias); \
    SAVE_OPTION(assume_no_global_alias); SAVE_OPTION(global_optimize); \
    SAVE_OPTION(precise_float); SAVE_OPTION(omit_frame_pointer); \
    SAVE_OPTION(line_records); \
    memcpy(c1_bss.saved_options.database_path, c1_data.options.database_path, 256); \
    memcpy(c1_bss.saved_options.pch_link_path, c1_data.options.pch_link_path, 256); \
    SAVE_OPTION(option_oq); SAVE_OPTION(option_ov); \
    c1_bss.saved_options.c9_il = c1_data.c9_il; \
    SAVE_OPTION(incremental_compile); SAVE_OPTION(debug_types); \
    SAVE_OPTION(function_records); SAVE_OPTION(split_pdbs); \
    SAVE_OPTION(no_eh_for_new); \
    c1_bss.saved_options.normalized_pool_strings = c1_data.normalized_pool_strings; \
    c1_bss.saved_options.normalized_readonly_strings = c1_data.normalized_readonly_strings; \
    c1_bss.saved_options.browser_enabled = c1_data.browser_enabled; \
    c1_bss.saved_options.browser_full = c1_data.browser_full; \
    c1_bss.saved_options.normalized_stack_checks = c1_data.normalized_stack_checks; \
    c1_bss.saved_options.normalized_oq = c1_data.normalized_oq; \
} while (0)

extern "C" int __cdecl run_compiler(void)
{
    CompilerOutputBuffer *buffer;
    int bytes, good;
    unsigned requested;
    char error_text[260];

    initialize_frontend();
    c1_bss.initial_option_flags = pack_option_flags();
    c1_data.options.pch_debug &= !c1_data.options.line_records;
    c1_data.compiler_phase = 3;
    if (c1_bss.options.preprocess_stdout) {
        preprocess_translation_unit();
        if (c1_data.options.dependency_output) {
            c1_data.dependency->Finish(
                c1_bss.error_count + (c1_bss.options.warnings_are_errors > 1) == 0);
            c1_current_pdb_context->Finish();
        }
        c1_data.compiler_phase = 5;
        goto finish_streams;
    }
    if (c1_data.c9_il)
        prepare_translation_unit();
    if (c1_data.options.pch_use) {
        if (c1_bss.pch_input_ready)
            load_precompiled_header(c1_bss.pch_input_file, 0);
        load_precompiled_header(c1_data.options.pch_use_file, 1);
    } else {
        c1_data.pch_option_flags = pack_option_flags();
        if (c1_data.options.pch_auto) {
            c1_data.options.pch_debug = !c1_data.options.line_records;
            begin_automatic_pch();
        } else {
            c1_bss.forced_source_index = 0;
            if (c1_data.c9_il && !c1_bss.options.preprocess_stdout &&
                !c1_bss.options.prototypes_only && c1_data.options.debug_info)
                begin_debug_info(0);
        }
        SAVE_KNOWN_PCH_SETTINGS();
    }
    if (!c1_data.c9_il && !c1_data.options.pch_auto) {
        if (c1_data.saved_pch_mode)
            initialize_parser();
        else {
            initialize_parser_tables();
            initialize_semantic_context();
        }
    }
    parse_translation_unit();
    if (!c1_bss.options.prototypes_only) {
        c1_data.translation_unit->Finish();
        if (c1_data.c9_il) {
            if (c1_bss.global_expression_scope_active) {
                write_ir_record(0x53, NULL);
                write_ir_record(0x54, make_integer_constant(0));
            }
            write_ir_record(0x4d, NULL);
        }
        write_global_header();
    }
    c1_data.compiler_phase = 5;
    if (c1_bss.error_count + (c1_bss.options.warnings_are_errors > 1))
        c1_pass_mode = 1;
    c1_data.backend = acquire_backend();
    c1_data.backend->Finish();
    finish_symbols();
    if (c1_bss.error_count + (c1_bss.options.warnings_are_errors > 1) == 0 &&
        !c1_bss.debug_module->Finish())
        compiler_warning(471, c1_current_pdb_context->database->QueryLastError(error_text));
    if (!c1_bss.debug_module->Close())
        compiler_error(73, ".\\main.c", 1449);
    c1_bss.debug_module = NULL;
    if (!c1_bss.options.prototypes_only) {
        if (c1_data.dependency_local || c1_data.options.dependency_output)
            c1_data.dependency->Finish(
                c1_bss.error_count + (c1_bss.options.warnings_are_errors > 1) == 0);
        if ((!c1_bss.options.preprocess_stdout && c1_data.options.line_records) ||
            c1_data.options.debug_info)
            finish_browser_output();
        if (c1_data.c9_il) {
            c1_bss.output_buffers[3].Close();
            c1_bss.output_buffers[1].Close();
            c1_bss.output_buffers[2].Close();
            bytes = c1_bss.output_buffers[5].cursor - c1_bss.output_buffers[5].data;
            if (bytes) {
                buffer = &c1_bss.output_buffers[4];
                requested = (unsigned)bytes + 1;
                if ((int)requested < 0)
                    good = 0;
                else if (buffer->cursor + requested > buffer->data + buffer->capacity)
                    good = buffer->Grow((int)requested) != 0;
                else
                    good = 1;
                buffer->good &= good;
                if (buffer->good) {
                    memcpy(buffer->cursor, c1_bss.output_buffers[5].data, bytes);
                    buffer->cursor += bytes;
                }
                c1_bss.output_buffers[5].cursor = c1_bss.output_buffers[5].data;
            }
            c1_bss.output_buffers[4].Close();
            if (c1_data.options.debug_info)
                c1_bss.output_buffers[0].Close();
        } else {
            finish_parser(0);
            finish_parser_tables();
        }
        c1_current_pdb_context->Finish();
    }
    c1_bss.input->Close();
finish_streams:
    if (c1_bss.options.preprocessed_file &&
        close_preprocessed_stream(c1_bss.preprocessed_stream))
        report_response_error(85, 340, c1_bss.options.preprocessed_file);
    if (fflush(stdout))
        report_response_error(85, 340, NULL);
    cleanup_precompiled_files();
    finish_compiler();
    c1_data.compiler_phase = 8;
    if (c1_data.report_statistics)
        print_compiler_statistics();
    return c1_bss.error_count + (c1_bss.options.warnings_are_errors > 1);
}

extern "C" unsigned __cdecl pack_option_flags(void) { __asm { int 3 } return 0; }
extern "C" void __cdecl prepare_translation_unit(void) { __asm { int 3 } }
extern "C" void __cdecl load_precompiled_header(const char *path, int required) { __asm { int 3 } }
extern "C" void __cdecl begin_automatic_pch(void) { __asm { int 3 } }
extern "C" void __cdecl begin_debug_info(int mode) { __asm { int 3 } }
extern "C" void __cdecl initialize_parser(void) { __asm { int 3 } }
extern "C" void __cdecl initialize_parser_tables(void) { __asm { int 3 } }
extern "C" void __cdecl initialize_semantic_context(void) { __asm { int 3 } }
extern "C" void __cdecl parse_translation_unit(void) { __asm { int 3 } }
extern "C" void __cdecl write_ir_record(int kind, const void *value) { __asm { int 3 } }
extern "C" void __cdecl write_global_header(void) { __asm { int 3 } }
extern "C" void *__cdecl make_integer_constant(int value) { __asm { int 3 } return NULL; }
extern "C" CompilerBackend *__cdecl acquire_backend(void) { __asm { int 3 } return NULL; }
extern "C" void __cdecl finish_symbols(void) { __asm { int 3 } }
extern "C" void __cdecl finish_browser_output(void) { __asm { int 3 } }
extern "C" int __cdecl close_preprocessed_stream(FILE *stream) { __asm { int 3 } return 0; }
extern "C" void __cdecl cleanup_precompiled_files(void) { __asm { int 3 } }
extern "C" void __cdecl finish_compiler(void) { __asm { int 3 } }
extern "C" void __cdecl print_compiler_statistics(void) { __asm { int 3 } }
extern "C" void __cdecl preprocess_translation_unit(void) { __asm { int 3 } }
extern "C" void __cdecl finish_parser(int result) { __asm { int 3 } }
extern "C" void __cdecl finish_parser_tables(void) { __asm { int 3 } }
extern "C" void __cdecl compiler_warning(int code, ...) { __asm { int 3 } }
void RECON_THISCALL CompilerTranslationUnit::Finish(void) { __asm { int 3 } }
void RECON_THISCALL CompilerDependency::Finish(int success) { __asm { int 3 } }
int RECON_THISCALL CompilerOutputBuffer::Close(void) { __asm { int 3 } return 0; }
int RECON_THISCALL CompilerOutputBuffer::Grow(int bytes) { __asm { int 3 } return 0; }
void RECON_THISCALL CompilerPdbContext::Finish(void) { __asm { int 3 } }
