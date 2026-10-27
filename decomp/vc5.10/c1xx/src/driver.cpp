#include <stdio.h>
#include <io.h>
#include <new.h>
#include <string.h>
#include "frontend.h"
#include "pdb-context.h"
#include "frontend-init.h"
#include "parser.h"
#include "global-output.h"
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
void RECON_THISCALL CompilerParser::Finish(void) {}
extern "C" CompilerBackend *__cdecl acquire_backend(void)
{
    return frontend_data.named_browser ? frontend_data.named_browser : frontend_data.initial_backend;
}
/* Browser owners and their inline destruction are defined in browser.cpp. */
extern "C" void __cdecl finish_browser_output(void) { __asm { int 3 } }
extern "C" int __cdecl close_preprocessed_stream(FILE *stream) { __asm { int 3 } return 0; }
/* The two stream cells are source-only until a real PCH file producer is
 * supported. They retain distinct original BSS identities, not FILE defaults
 * or native state claims. */
struct PchCleanupSourceStreams {
    FILE *preprocessed; /* c0af4 */
    FILE *header;       /* c4f60 */
};
#ifndef RECON_NATIVE_TEST
static PchCleanupSourceStreams pch_cleanup_streams;
#else
#define pch_cleanup_streams (*(PchCleanupSourceStreams *)(recon_unsupported("PCH cleanup: unsupported stream owners"), 0))
#endif
extern "C" void __cdecl cleanup_precompiled_files(void)
{
    if (frontend_data.pch_files_cleaned) return;
    if (frontend_data.saved_pch_mode &&
        frontend_bss.error_count + (frontend_bss.options.warnings_are_errors > 1) != 0) {
        /* A failed saved-PCH operation cleans its file even if all flags
         * were already cleared. The ordinary path needs an active flag. */
    } else if (!frontend_data.pch_preprocessed_file && !frontend_data.pch_header_file &&
               !frontend_data.pch_expression_file) return;
    frontend_data.pch_files_cleaned = 1;
    if (frontend_data.pch_preprocessed_file) {
        frontend_bss.expression_seek_result = fclose(pch_cleanup_streams.preprocessed);
        if (frontend_bss.expression_seek_result == -1)
            report_response_error(82, 344, (const char *)frontend_data.pch_stop_owner);
        pch_cleanup_streams.preprocessed = NULL;
    } else if (frontend_data.pch_header_file) {
        frontend_bss.expression_seek_result = fclose(pch_cleanup_streams.header);
        if (frontend_bss.expression_seek_result == -1)
            report_response_error(82, 344, (const char *)frontend_data.pch_stop_owner);
        pch_cleanup_streams.header = NULL;
    } else if (frontend_data.pch_expression_file) {
        frontend_bss.expression_seek_result = frontend_bss.output_buffers[3].CloseFile();
        if (frontend_bss.expression_seek_result == -1)
            report_response_error(82, 344, (const char *)frontend_data.pch_stop_owner);
    } else return;
    if (_unlink((const char *)frontend_data.pch_stop_owner)) report_response_error(83, 340, (const char *)frontend_data.pch_stop_owner);
    frontend_data.pch_expression_file = 0;
    frontend_data.pch_header_file = 0;
    frontend_data.pch_preprocessed_file = 0;
}
extern "C" void __cdecl free_diagnostic_records(void);
extern "C" void __cdecl release_compiler_arenas(void);
extern "C" void __cdecl finish_compiler(void)
{
    ::operator delete(frontend_bss.lexer_backend);
    free_diagnostic_records();
    release_compiler_arenas();
    VirtualFree(frontend_bss.work_buffers[0].data, 0x500000, MEM_DECOMMIT);
    VirtualFree(frontend_bss.work_buffers[0].data, 0, MEM_RELEASE);
    VirtualFree(frontend_bss.work_buffers[1].data, 0x500000, MEM_DECOMMIT);
    VirtualFree(frontend_bss.work_buffers[1].data, 0, MEM_RELEASE);
}
extern "C" void __cdecl print_compiler_statistics(void) { __asm { int 3 } }
extern "C" void __cdecl preprocess_translation_unit(void) { __asm { int 3 } }
extern "C" void __cdecl finish_parser(int result) { __asm { int 3 } }
extern "C" void __cdecl finish_parser_tables(void) { __asm { int 3 } }
extern "C" void __cdecl compiler_warning(int code, ...) { __asm { int 3 } }
struct CompilerGeneratedSpan {
    const BYTE *data;
    int bytes;
};
extern "C" {
void __cdecl emit_function_output_blocks(void); /*355ea*/
void __cdecl write_global_reference(CompilerSymbolLink *);
CompilerExpression *__cdecl symbol_expression(CompilerSymbol *, unsigned);
CompilerSymbol *__cdecl prepare_pending_initializer(CompilerSymbol *); /*1bbe2*/
void __cdecl initialize_symbol(CompilerSymbol *, CompilerExpression *);
void __cdecl query_generated_output(unsigned, CompilerGeneratedSpan *); /*93dae*/
void __cdecl write_global_ir_record(unsigned, CompilerSymbol *);
}
void RECON_THISCALL CompilerTranslationUnit::Finish(void)
{
    CompilerSymbolLink *link;
    CompilerGlobalSymbolView *symbol;
    CompilerExpression *expression;
    CompilerSymbol *target;
    CompilerGeneratedSpan span;
    OutputBuffer *buffer;
    unsigned needed;
    int available;
    frontend_bss.global_output_active = 0;
    if (frontend_bss.error_count) return;
    for (link = pending_symbols; link; link = link->next) write_global_reference(link);
    emit_function_output_blocks();
    root->Check();
    for (link = pending_symbols; link; link = link->next) {
        symbol = (CompilerGlobalSymbolView *)((BYTE *)link - 8);
        if (symbol->pending_initializer) {
            write_global_reference(&((CompilerNamedSymbol *)symbol->pending_initializer)->link);
            expression = symbol_expression((CompilerSymbol *)symbol, 0);
            target = prepare_pending_initializer(symbol->pending_initializer);
            initialize_symbol(target, expression);
        }
    }
    if (!frontend_data.option_c9il) {
        query_generated_output(1, &span);
        buffer = &frontend_bss.output_buffers[2];
        needed = (unsigned)span.bytes + 1;
        available = 0;
        if ((int)needed >= 0) {
            available = 1;
            if (buffer->cursor + needed > buffer->data + buffer->capacity)
                available = buffer->Grow((int)needed) != 0;
        }
        buffer->ok &= available;
        if (buffer->ok) {
            memcpy(buffer->cursor, span.data, (unsigned)span.bytes);
            buffer->cursor += span.bytes;
        }
    }
    write_global_ir_record(10, NULL);
}
extern "C" CompilerSymbol *__cdecl prepare_pending_initializer(CompilerSymbol *) { __asm { int 3 } return NULL; }
extern "C" void __cdecl query_generated_output(unsigned, CompilerGeneratedSpan *) { __asm { int 3 } }
int RECON_THISCALL CompilerDependency::Finish(int success) { __asm { int 3 } return 0; }
int RECON_THISCALL OutputBuffer::Close(void)
{
    int result = CloseFile();
    frontend_bss.expression_seek_result = result;
    if (result == -1) return report_response_error(82, 339, NULL);
    return result;
}
int RECON_THISCALL OutputBuffer::CloseFile(void)
{
    int result;
    if (!file)
        return frontend_bss.error_count + (frontend_bss.options.warnings_are_errors > 1) ? 0 : -1;
    Flush();
    result = fclose(file);
    file = NULL;
    return result;
}
/* Module .data CB008 is distinct from the invocation's saved datafe state.
 * Its provider-active branch remains outside the current native domain. */
static int pdb_names_close_mode; /* original CB008, initially0 */
void RECON_THISCALL CompilerPdbContext::Finish(void)
{
    char error_text[260];
    int good;
    if (!database) return;
#ifdef RECON_NATIVE_TEST
    recon_unsupported("PDB finish: active provider domain is unsupported");
#endif
    if (!pdb_names_close_mode) {
        if (!names->Commit()) goto failed;
        good = 1;
        if (stream_state) {
            good = stream_state->Close() & 1;
            stream_state = NULL;
        }
        if (frontend_bss.error_count + (frontend_bss.options.warnings_are_errors > 1) == 0 &&
            !read_mode) {
            if (!database->Commit()) good = 0;
        }
        if (!good) goto failed;
        return;
    }
    if (!names->Close())
        compiler_warning(471, database->QueryLastError(error_text));
    names = NULL;
    if (minimal_rebuild || types) return;
    database->QueryLastError(error_text);
    good = 1;
    if (stream_state) {
        good = stream_state->Close() & 1;
        stream_state = NULL;
    }
    if (frontend_bss.error_count + (frontend_bss.options.warnings_are_errors > 1) == 0 &&
        !read_mode && !database->Commit()) good = 0;
    else if (!database->Close()) good = 0;
    database = NULL;
    if (!good) compiler_warning(471, error_text);
    return;
failed:
    compiler_warning(471, database->QueryLastError(error_text));
}
int RECON_THISCALL PdbStreamState::Close(void) { __asm { int 3 } return 0; }

extern "C" {
void __cdecl finish_compilation_table_a(void); /*353c4*/
void __cdecl finish_compilation_table_b(void); /*3542d*/
void __cdecl finish_compilation_table_c(void); /*35441*/
void __cdecl complete_rtti_records(void); /*b4e3b*/
void __cdecl finish_completion_item(void *); /*a0137*/
void __cdecl visit_vector_items(CompilerVector *, void (__cdecl *)(void *));
CompilerBackend *__cdecl acquire_initial_backend(void);
}
extern "C" void __cdecl initialize_compilation_output(void)
{
    CompilerLexerBackend *state;
    finish_compilation_table_a();
    finish_compilation_table_b();
    finish_compilation_table_c();
    if (frontend_data.options.runtime_type_info) complete_rtti_records();
    state = frontend_bss.lexer_backend;
    state->backend = frontend_data.backend;
    frontend_data.backend = acquire_initial_backend();
    state->enabled = 0;
    visit_vector_items(frontend_data.compilation_completion_items, finish_completion_item);
    frontend_data.compilation_completion_items = NULL;
    state = frontend_bss.lexer_backend;
    frontend_data.backend = state->backend;
    state->enabled = 1;
}
/* The cursor exposes a record/context pair. Its pending initializer/advance
 * own the first12 bytes. The record view interprets only the actual +c flag,
 * without establishing an allocation extent or any other record fields. */
struct CompletionPairRecord {
    BYTE unmodeled_00_0b[12];
    int emitted;
};
struct CompletionPairCursor {
    BYTE unmodeled_00_0b[12];
    CompletionPairRecord *record;
    void *context;
};
extern "C" {
void __cdecl initialize_completion_pair_cursor(CompletionPairCursor *, void *); /*445aa*/
int __cdecl next_completion_pair(CompletionPairCursor *); /*445bd*/
void __cdecl emit_first_completion_pair(CompletionPairRecord *, void *); /*5075b*/
void __cdecl emit_second_completion_pair(CompletionPairRecord *, void *); /*b3956*/
}
extern "C" void __cdecl finish_compilation_table_a(void)
{
    CompletionPairCursor cursor;
    if (!frontend_data.compilation_pair_tables[0]) return;
    initialize_completion_pair_cursor(&cursor, frontend_data.compilation_pair_tables[0]);
    while (next_completion_pair(&cursor)) {
        if (!cursor.record->emitted) {
            cursor.record->emitted = 1;
            emit_first_completion_pair(cursor.record, cursor.context);
        }
    }
}
extern "C" void __cdecl finish_compilation_table_b(void)
{
    CompletionPairCursor cursor;
    if (!frontend_data.compilation_pair_tables[1]) return;
    initialize_completion_pair_cursor(&cursor, frontend_data.compilation_pair_tables[1]);
    while (next_completion_pair(&cursor)) {
        if (!cursor.record->emitted) {
            cursor.record->emitted = 1;
            emit_second_completion_pair(cursor.record, cursor.context);
        }
    }
}
extern "C" void __cdecl initialize_completion_pair_cursor(CompletionPairCursor *, void *) { __asm { int 3 } }
extern "C" int __cdecl next_completion_pair(CompletionPairCursor *) { __asm { int 3 } return 0; }
extern "C" void __cdecl emit_first_completion_pair(CompletionPairRecord *, void *) { __asm { int 3 } }
extern "C" void __cdecl emit_second_completion_pair(CompletionPairRecord *, void *) { __asm { int 3 } }
/* These partial views describe only the real pointer/flag consumers in the
 * completion walk. Their sizeof values do not establish allocation extents. */
struct CompletionMember {
    BYTE unmodeled_00_37[56];
    unsigned emitted;
};
struct CompletionNestedGroup {
    BYTE unmodeled_00_13[20];
    CompilerVector *members[2];
};
typedef char CompletionMember_emitted_offset[offsetof(CompletionMember, emitted) == 56 ? 1 : -1];
extern "C" {
void __cdecl initialize_pending_declaration_cursor(PendingDeclarationCursor *, void *);
int __cdecl next_pending_declaration(PendingDeclarationCursor *);
void __cdecl complete_declaration_member(CompletionMember *, CompilerGlobalSymbolView *); /*40085*/
}
extern "C" void __cdecl finish_compilation_table_c(void)
{
    PendingDeclarationCursor cursor;
    CompilerGlobalSymbolView *declaration;
    CompilerGlobalClassDetail *set;
    CompilerVector *members, *children;
    CompletionMember *member;
    short member_index, child_index;
    unsigned group;
    if (!frontend_data.completion_declarations) return;
    initialize_pending_declaration_cursor(&cursor, frontend_data.completion_declarations);
    while (next_pending_declaration(&cursor)) {
        declaration = (CompilerGlobalSymbolView *)cursor.current;
        for (group = 0; group != 2; ++group) {
            set = declaration->class_detail;
            members = set->member_groups[group];
            children = set->additional_members;
            member_index = child_index = 0;
            for (;;) {
                while (members && member_index < members->count) {
                    member = (CompletionMember *)members->items[member_index];
                    ++member_index;
                    if (!member->emitted) complete_declaration_member(member, declaration);
                }
                if (!children || child_index >= children->count) break;
                members = ((CompletionNestedGroup *)children->items[child_index])->members[group];
                ++child_index;
                member_index = 0;
            }
        }
    }
}
extern "C" void __cdecl complete_declaration_member(CompletionMember *, CompilerGlobalSymbolView *) { __asm { int 3 } }
extern "C" void __cdecl complete_rtti_records(void) { __asm { int 3 } }
extern "C" void __cdecl finish_completion_item(void *) { __asm { int 3 } }
