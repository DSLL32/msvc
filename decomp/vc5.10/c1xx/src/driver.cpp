#include <stdio.h>
#include <new.h>
#include <string.h>
#include "frontend.h"
#include "pdb-context.h"
#include "frontend-init.h"
#include "parser.h"
#pragma intrinsic(memcpy, strlen)
#include <recon.h>

/* C1XX imports MSVCRT's FILE array directly. The supplied /MD SDK instead
 * expands stdout through __p__iob; preserve the target's actual data import. */
#if defined(_MSC_VER) && !defined(RECON_NATIVE_TEST)
#undef _iob
#undef stdout
extern "C" __declspec(dllimport) FILE _iob[];
#define stdout (&_iob[1])
#endif

extern "C" {
void __cdecl initialize_frontend(void);
unsigned __cdecl pack_option_flags(void);
void __cdecl prepare_translation_unit(void);
void __cdecl load_precompiled_header(const char *, int);
void __cdecl begin_automatic_pch(void);
void __cdecl begin_debug_info(int);
void __cdecl initialize_parser(void);
void __cdecl initialize_compilation_output(void);
void __cdecl initialize_parser_tables(void);
void __cdecl initialize_semantic_context(void);
int __cdecl parse_translation_unit(void);
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
int __cdecl report_response_error(int, ...);
}

/* Project only known original settings; future consumers of unknown
 * original bytes must remain behind their named reconstruction frontiers. */
#define SAVE_KNOWN_PCH_SETTINGS() do { \
    frontend_bss.saved_options.preprocess_only = frontend_data.options.preprocess_only; \
    frontend_bss.saved_options.language_extensions = frontend_data.options.language_extensions; \
    frontend_bss.saved_options.option_zbe = frontend_data.options.option_zbe; \
    frontend_bss.saved_options.option_zb = frontend_data.option_zb; \
    frontend_bss.saved_options.fastcall_calls = frontend_data.options.fastcall_calls; \
    frontend_bss.saved_options.japanese_identifiers = frontend_data.options.japanese_identifiers; \
    frontend_bss.saved_options.unsigned_char = frontend_data.options.unsigned_char; \
    frontend_bss.saved_options.member_pointer_best_case = frontend_data.options.member_pointer_best_case; \
    frontend_bss.saved_options.member_pointer_general = frontend_data.options.member_pointer_general; \
    frontend_bss.saved_options.member_pointer_single = frontend_data.options.member_pointer_single; \
    frontend_bss.saved_options.member_pointer_multiple = frontend_data.options.member_pointer_multiple; \
    frontend_bss.saved_options.member_pointer_virtual = frontend_data.options.member_pointer_virtual; \
    frontend_bss.saved_options.vtordisp = frontend_data.options.vtordisp; \
    frontend_bss.saved_options.dll_runtime = frontend_data.options.dll_runtime; \
    frontend_bss.saved_options.multithread_runtime = frontend_data.options.multithread_runtime; \
    frontend_bss.saved_options.pascal_calls = frontend_data.options.pascal_calls; \
    frontend_bss.saved_options.debug_info = frontend_data.options.debug_info; \
    frontend_bss.saved_options.pch_debug = frontend_data.options.pch_debug; \
    frontend_bss.saved_options.preprocess_no_lines = frontend_data.options.preprocess_no_lines; \
    frontend_bss.saved_options.option_pf = frontend_data.options.option_pf; \
    frontend_bss.saved_options.option_bu = frontend_data.options.option_bu; \
    frontend_bss.saved_options.reproducible = frontend_data.options.reproducible; \
    frontend_bss.saved_options.intrinsics = frontend_data.options.intrinsics; \
    frontend_bss.saved_options.stack_checks = frontend_data.options.stack_checks; \
    frontend_bss.saved_options.cdecl_calls = frontend_data.options.cdecl_calls; \
    frontend_bss.saved_options.stdcall_calls = frontend_data.options.stdcall_calls; \
    frontend_bss.saved_options.pool_strings = frontend_data.options.pool_strings; \
    frontend_bss.saved_options.readonly_strings = frontend_data.options.readonly_strings; \
    frontend_bss.saved_options.code_based_strings = frontend_data.options.code_based_strings; \
    frontend_bss.saved_options.processor = frontend_data.options.processor; \
    frontend_bss.saved_options.minimal_rebuild = frontend_data.options.minimal_rebuild; \
    frontend_bss.saved_options.inline_level = frontend_data.options.inline_level; \
    frontend_bss.saved_options.exception_handling = frontend_data.options.exception_handling; \
    frontend_bss.saved_options.runtime_type_info = frontend_data.options.runtime_type_info; \
    frontend_bss.saved_options.no_bool = frontend_data.options.no_bool; \
    frontend_bss.saved_options.check_completed_class_option = frontend_data.class_completion_checks; \
    frontend_bss.saved_options.dependency_output = frontend_data.options.dependency_output; \
    frontend_bss.saved_options.incremental_rebuild = frontend_data.options.incremental_rebuild; \
    frontend_bss.saved_options.optimize_speed = frontend_data.options.optimize_speed; \
    frontend_bss.saved_options.assume_no_alias = frontend_data.options.assume_no_alias; \
    frontend_bss.saved_options.assume_no_global_alias = frontend_data.options.assume_no_global_alias; \
    frontend_bss.saved_options.global_optimize = frontend_data.options.global_optimize; \
    frontend_bss.saved_options.precise_float = frontend_data.options.precise_float; \
    frontend_bss.saved_options.omit_frame_pointer = frontend_data.options.omit_frame_pointer; \
    frontend_bss.saved_options.line_records = frontend_data.options.line_records; \
    memcpy(frontend_bss.saved_options.database_path, frontend_data.options.database_path, 256); \
    memcpy(frontend_bss.saved_options.pch_link_path, frontend_data.options.pch_link_path, 256); \
    frontend_bss.saved_options.option_oq = frontend_data.options.option_oq; \
    frontend_bss.saved_options.option_ov = frontend_data.options.option_ov; \
    frontend_bss.saved_options.option_c9il = frontend_data.option_c9il; \
    frontend_bss.saved_options.incremental_compile = frontend_data.options.incremental_compile; \
    frontend_bss.saved_options.debug_types = frontend_data.options.debug_types; \
    frontend_bss.saved_options.function_records = frontend_data.options.function_records; \
    frontend_bss.saved_options.split_pdbs = frontend_data.options.split_pdbs; \
    frontend_bss.saved_options.no_eh_for_new = frontend_data.options.no_eh_for_new; \
    frontend_bss.saved_options.incremental_rebuild_forced = frontend_data.options.incremental_rebuild_forced; \
    frontend_bss.saved_options.extern_c_no_throw = frontend_data.options.extern_c_no_throw; \
    frontend_bss.saved_options.pch_option_flags = frontend_data.pch_option_flags; \
    frontend_bss.saved_options.normalized_stack_checks = frontend_data.normalized_stack_checks; \
    frontend_bss.saved_options.normalized_oq = frontend_data.normalized_oq; \
    frontend_bss.saved_options.normalized_pool_strings = frontend_data.normalized_pool_strings; \
    frontend_bss.saved_options.normalized_readonly_strings = frontend_data.normalized_readonly_strings; \
    frontend_bss.saved_options.browser_enabled = frontend_data.browser_enabled; \
    frontend_bss.saved_options.browser_full = frontend_data.browser_full; \
} while (0)

extern "C" int __cdecl run_compiler(void)
{
    OutputBuffer *buffer;
    int bytes, good;
    unsigned requested;
    char error_text[260];

    initialize_frontend();
    frontend_bss.initial_option_flags = pack_option_flags();
    frontend_data.options.pch_debug &= !frontend_data.options.line_records;
    frontend_data.compiler_phase = 3;
    if (frontend_bss.options.preprocess_stdout) {
        preprocess_translation_unit();
        if (frontend_data.options.dependency_output) {
            frontend_data.dependency->Finish(
                frontend_bss.error_count + (frontend_bss.options.warnings_are_errors > 1) == 0);
            current_pdb_context->Finish();
        }
        frontend_data.compiler_phase = 5;
        goto finish_streams;
    }
    if (frontend_data.option_c9il)
        prepare_translation_unit();
    if (frontend_data.options.pch_use) {
        if (frontend_bss.pch_input_ready)
            load_precompiled_header(frontend_bss.pch_input_file, 0);
        load_precompiled_header(frontend_data.options.pch_use_file, 1);
    } else {
        frontend_data.pch_option_flags = pack_option_flags();
        if (frontend_data.options.pch_auto) {
            frontend_data.options.pch_debug = !frontend_data.options.line_records;
            begin_automatic_pch();
        } else {
            frontend_bss.forced_source_index = 0;
            if (frontend_data.option_c9il && !frontend_bss.options.preprocess_stdout &&
                !frontend_bss.options.prototypes_only && frontend_data.options.debug_info)
                begin_debug_info(0);
        }
        SAVE_KNOWN_PCH_SETTINGS();
    }
    if (!frontend_data.option_c9il && !frontend_data.options.pch_auto) {
        if (frontend_data.saved_pch_mode)
            initialize_parser();
        else {
            initialize_parser_tables();
            initialize_semantic_context();
        }
    }
    parse_translation_unit();
    if (!frontend_bss.options.prototypes_only) {
        initialize_compilation_output();
        frontend_data.translation_unit->Finish();
        if (frontend_data.option_c9il) {
            if (frontend_bss.global_expression_scope_active) {
                write_ir_record(0x53, NULL);
                write_ir_record(0x54, make_integer_constant(0));
            }
            write_ir_record(0x4d, NULL);
        }
        write_global_header();
    }
    frontend_data.compiler_phase = 5;
    if (frontend_bss.error_count + (frontend_bss.options.warnings_are_errors > 1))
        compiler_mode = 1;
    frontend_data.backend = acquire_backend();
    frontend_data.backend->Finish();
    finish_symbols();
    if (frontend_bss.error_count + (frontend_bss.options.warnings_are_errors > 1) == 0 &&
        !frontend_bss.debug_module->Finish())
        compiler_warning(471, current_pdb_context->database->QueryLastError(error_text));
    if (!frontend_bss.debug_module->Close())
        report_compiler_diagnostic(73, ".\\main.c", 1449);
    frontend_bss.debug_module = NULL;
    if (!frontend_bss.options.prototypes_only) {
        if (frontend_data.options.incremental_rebuild || frontend_data.options.dependency_output)
            frontend_data.dependency->Finish(
                frontend_bss.error_count + (frontend_bss.options.warnings_are_errors > 1) == 0);
        if ((!frontend_bss.options.preprocess_stdout && frontend_data.options.line_records) ||
            frontend_data.options.debug_info)
            finish_browser_output();
        if (frontend_data.option_c9il) {
            frontend_bss.output_buffers[3].Close();
            frontend_bss.output_buffers[1].Close();
            frontend_bss.output_buffers[2].Close();
            bytes = frontend_bss.output_buffers[5].cursor - frontend_bss.output_buffers[5].data;
            if (bytes) {
                buffer = &frontend_bss.output_buffers[4];
                requested = (unsigned)bytes + 1;
                if ((int)requested < 0)
                    good = 0;
                else if (buffer->cursor + requested > buffer->data + buffer->capacity)
                    good = buffer->Grow((int)requested) != 0;
                else
                    good = 1;
                buffer->ok &= good;
                if (buffer->ok) {
                    memcpy(buffer->cursor, frontend_bss.output_buffers[5].data, bytes);
                    buffer->cursor += bytes;
                }
                frontend_bss.output_buffers[5].cursor = frontend_bss.output_buffers[5].data;
            }
            frontend_bss.output_buffers[4].Close();
            if (frontend_data.options.debug_info)
                frontend_bss.output_buffers[0].Close();
        } else {
            finish_parser(0);
            finish_parser_tables();
        }
        current_pdb_context->Finish();
    }
    frontend_bss.input->Close();
finish_streams:
    if (frontend_bss.options.preprocessed_file &&
        close_preprocessed_stream(frontend_bss.preprocessed_stream))
        report_response_error(85, 340, frontend_bss.options.preprocessed_file);
    if (fflush(stdout))
        report_response_error(85, 340, NULL);
    cleanup_precompiled_files();
    finish_compiler();
    frontend_data.compiler_phase = 8;
    if (frontend_data.report_statistics)
        print_compiler_statistics();
    return frontend_bss.error_count + (frontend_bss.options.warnings_are_errors > 1);
}

extern "C" unsigned __cdecl pack_option_flags(void)
{
    /* The21 logical bit positions and their nine live option aliases come
     * from the encountered C1XX option map at CDB78. NULL slots stay clear. */
    static const int *const options[21] = {
        &frontend_data.normalized_stack_checks,
        NULL, NULL, NULL, NULL, NULL,
        &frontend_data.options.assume_no_alias,
        &frontend_data.options.assume_no_global_alias,
        &frontend_data.options.optimize_speed,
        NULL,
        &frontend_data.options.global_optimize,
        NULL, NULL,
        &frontend_data.options.precise_float,
        NULL, NULL, NULL,
        &frontend_data.normalized_oq,
        &frontend_data.options.option_ov,
        NULL,
        &frontend_data.options.omit_frame_pointer
    };
    unsigned packed = 0, bit = 0x80000000U, i;
    for (i = 0; i < 21; ++i, bit >>= 1)
        if (options[i] && *options[i]) packed |= bit;
    return packed;
}
/* The actual byte appender at89672 bounds the shared token buffer by its
 * C0BC8..C13C8 interval. This later lexer owner is source-only here: no native
 * default/snapshot claim precedes the first supported writer. */
char compiler_token_text[2048];
struct CompilerVersionRecord {
    BYTE kind, storage_kind;
    BYTE unmodeled_02[14];
    char *text;
    WORD length;
};
extern "C" void __cdecl process_version_literal(CompilerVersionRecord *);
extern "C" void __cdecl record_version_literal(CompilerVersionRecord *);
extern "C" void __cdecl prepare_translation_unit(void)
{
    if (frontend_bss.options.version_string) {
#ifdef RECON_NATIVE_TEST
        recon_unsupported("version literal: unmodeled record and shared text state");
#endif
        CompilerArena *arena = &frontend_bss.arenas[frontend_data.constant_arena_index];
        CompilerVersionRecord *record = (CompilerVersionRecord *)arena->cursor;
        arena->cursor += 24;
        if (arena->cursor > arena->committed_limit) arena->CommitExtent();
        record->storage_kind = 5;
        record->kind = 7;
        record->length = (WORD)(strlen(frontend_bss.options.version_string) + 1);
        record->text = compiler_token_text;
        memcpy(record->text, frontend_bss.options.version_string, record->length);
        process_version_literal(record);
        record_version_literal(record);
        frontend_bss.global_expression_scope_active = 1;
    }
}
extern "C" void __cdecl process_version_literal(CompilerVersionRecord *) { __asm { int 3 } }
extern "C" void __cdecl record_version_literal(CompilerVersionRecord *) { __asm { int 3 } }
extern "C" void __cdecl load_precompiled_header(const char *path, int required) { __asm { int 3 } }
extern "C" void __cdecl begin_automatic_pch(void) { __asm { int 3 } }
extern "C" void __cdecl begin_debug_info(int mode) { __asm { int 3 } }
extern "C" void __cdecl initialize_parser(void) { __asm { int 3 } }
extern "C" void __cdecl initialize_parser_tables(void) { __asm { int 3 } }
extern "C" void __cdecl initialize_semantic_context(void) { __asm { int 3 } }
extern "C" int __cdecl parse_translation_unit(void)
{
    CompilerParser *previous = frontend_data.active_parser;
    TokenValue saved_value;
    /* Save/restore the real40-byte storage, retaining opaque bytes without
     * interpreting other semantic arms or asserting their defaults. */
    memcpy(&saved_value, &frontend_bss.token_value, sizeof(saved_value));
    void *memory = ::operator new(sizeof(CompilerParser));
    frontend_data.active_parser = memory ? ((CompilerParser *)memory)->Initialize() : NULL;
    int result = frontend_data.active_parser->Run();
    if (frontend_data.active_parser) {
        frontend_data.active_parser->Finish();
        ::operator delete(frontend_data.active_parser);
    }
    frontend_data.active_parser = previous;
    memcpy(&frontend_bss.token_value, &saved_value, sizeof(saved_value));
    return result;
}
CompilerParser *RECON_THISCALL CompilerParser::Initialize(void)
{
    Reset();
    return this;
}
void RECON_THISCALL CompilerParser::Reset(void)
{
    reset_value = 0;
    recovery_shifts = 0;
    syntax_errors = 0;
}
void RECON_THISCALL CompilerParser::Finish(void) { __asm { int 3 } }
extern "C" CompilerBackend *__cdecl acquire_backend(void)
{
    return frontend_data.named_browser ? frontend_data.named_browser : frontend_data.initial_backend;
}
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
int RECON_THISCALL CompilerDependency::Finish(int success) { __asm { int 3 } return 0; }
int RECON_THISCALL OutputBuffer::Close(void) { __asm { int 3 } return 0; }
void RECON_THISCALL CompilerPdbContext::Finish(void) { __asm { int 3 } }

extern "C" void __cdecl initialize_compilation_output(void) { __asm { int 3 } }
