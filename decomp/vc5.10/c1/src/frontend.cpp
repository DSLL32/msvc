#include <new.h>
#include <stdio.h>
#include <io.h>
#include <string.h>
#include "frontend.h"
#include "pragma.h"
#pragma intrinsic(memcpy, strlen)
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
int __cdecl parse_translation_unit(void);
void __cdecl write_ir_record(int, const void *);
void __cdecl write_global_header(void);
CompilerIntegerConstant *__cdecl make_integer_constant(int);
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

extern "C" unsigned __cdecl pack_option_flags(void)
{
    /* Sparse positions are the established IL option-bit contract, not fields
     * synthesized for the unused entries of the original pointer table. */
    static const int *values[21] = {
        &c1_data.normalized_stack_checks, NULL, NULL, NULL, NULL, NULL,
        &c1_data.options.assume_no_alias, &c1_data.options.assume_no_global_alias,
        &c1_data.options.optimize_size, NULL, &c1_data.options.global_optimize,
        NULL, NULL, &c1_data.options.precise_float, NULL, NULL, NULL,
        &c1_data.normalized_oq, &c1_data.options.option_ov, NULL,
        &c1_data.options.omit_frame_pointer
    };
    unsigned result = 0, bit = 0x80000000U, index;
    for (index = 0; index < 21; ++index, bit >>= 1)
        if (values[index] && *values[index])
            result |= bit;
    return result;
}
/* Shared lexer scratch: the original append bound at40f61 is base55120
 * through55920. Current native -V use is held, so this source-only owner has
 * no modeled byte defaults, reset or equality claim yet. */
char c1_lexer_token_text[2048];
extern "C" void __cdecl initialize_string_literal_type(CompilerParserNode *);
extern "C" void __cdecl record_version_literal(CompilerParserNode *);
extern "C" void __cdecl prepare_translation_unit(void)
{
    if (c1_bss.options.version_string) {
#ifdef RECON_NATIVE_TEST
        recon_unsupported("version record: lexer scratch and later record owners are unmodeled");
#endif
        CompilerArena *arena = &c1_bss.arenas[c1_data.expression_arena_kind];
        CompilerParserNode *record = (CompilerParserNode *)arena->cursor;
        arena->cursor += 24;
        if (arena->cursor > arena->committed_limit)
            arena->CommitExtent();
        record->kind = 7;
        record->flags = 5;
        record->string_length = (WORD)(strlen(c1_bss.options.version_string) + 1);
        record->text = c1_lexer_token_text;
        memcpy(c1_lexer_token_text, c1_bss.options.version_string, record->string_length);
        initialize_string_literal_type(record);
        record_version_literal(record);
        c1_bss.global_expression_scope_active = 1;
    }
}
extern "C" const signed char c1_primitive_widths[16];
extern "C" CompilerTypeShape *__cdecl make_type_shape(unsigned, unsigned, unsigned *);
extern "C" CompilerTypeShape *__cdecl intern_type_shape(CompilerTypeShape *);
extern "C" CompilerType *__cdecl intern_type(CompilerType *);
extern "C" void __cdecl initialize_string_literal_type(CompilerParserNode *literal)
{
    CompilerType *description;
    CompilerTypeShape *shape;
    unsigned count, unit;
    WORD flags;
    if (literal->string_flags & 1) flags = 0x4042;
    else flags = c1_data.options.language_extensions && c1_data.options.unsigned_char ? 0x4061 : 0x61;
    if (c1_bss.type_description_depth != 0) {
#ifdef __linux__
        recon_unsupported("string literal type scratch beyond encountered slot1");
#else
        __asm { int 3 }
#endif
    }
    description = &c1_bss.type_descriptions[++c1_bss.type_description_depth];
    /* Unlike the copied scratch descriptors, this constructor really clears
     * all sixteen bytes, including the separate debug-index cell. */
    description->flags = flags;
    description->attributes = 0;
    description->shape = NULL;
    description->auxiliary = 0;
    description->debug_index = 0;
    unit = literal->string_flags & 1 ? 2 : (unsigned)(int)c1_primitive_widths[1];
    count = literal->string_length / unit;
    shape = make_type_shape(2, 0, &count);
    description->shape = intern_type_shape(shape);
    literal->type = intern_type(description);
    --c1_bss.type_description_depth;
}
extern "C" void __cdecl record_version_literal(CompilerParserNode *) { __asm { int 3 } }
extern "C" void __cdecl load_precompiled_header(const char *path, int required) { __asm { int 3 } }
extern "C" void __cdecl begin_automatic_pch(void) { __asm { int 3 } }
extern "C" void __cdecl begin_debug_info(int mode) { __asm { int 3 } }
extern "C" void __cdecl initialize_parser(void) { __asm { int 3 } }
extern "C" void __cdecl initialize_parser_tables(void) { __asm { int 3 } }
extern "C" void __cdecl initialize_semantic_context(void) { __asm { int 3 } }
extern "C" int __cdecl parse_translation_unit(void)
{
    CompilerParser *previous = c1_data.active_parser;
    /* Original saves/restores all24 bytes. Only the encountered firstDWORD
     * is transferred by this typed projection;20 bytes remain UNMODELED. */
    CompilerParserValue saved_value;
#ifdef RECON_NATIVE_TEST
    saved_value.declaration_flags = c1_bss.parser_value.declaration_flags;
#else
    saved_value = c1_bss.parser_value;
#endif
    void *memory = ::operator new(8864);
    c1_data.active_parser = memory ? ((CompilerParser *)memory)->Initialize() : NULL;
    int result = c1_data.active_parser->Run();
    if (c1_data.active_parser) {
        c1_data.active_parser->Finish();
        ::operator delete(c1_data.active_parser);
    }
    c1_data.active_parser = previous;
#ifdef RECON_NATIVE_TEST
    c1_bss.parser_value.declaration_flags = saved_value.declaration_flags;
#else
    c1_bss.parser_value = saved_value;
#endif
    return result;
}
CompilerParser *RECON_THISCALL CompilerParser::Initialize(void)
{
    Reset();
    return this;
}
void RECON_THISCALL CompilerParser::Reset(void)
{
    value_top = (CompilerParserStackValue *)this;
    lookahead = NULL;
    state = 0;
    recovery_shifts = 0;
    syntax_errors = 0;
    /* First Run increment reaches states[0]; the saved position lies within
     * the owning allocation, two bytes before that array. */
    state_top = (short *)((BYTE *)this + offsetof(CompilerParser, states) - 2);
}
void RECON_THISCALL CompilerParser::Finish(void) {}
extern "C" CompilerBackend *__cdecl acquire_backend(void)
{
    return c1_data.named_browser ? c1_data.named_browser : c1_data.initial_backend;
}
extern "C" void __cdecl finish_browser_output(void) { __asm { int 3 } }
extern "C" int __cdecl close_preprocessed_stream(FILE *stream) { __asm { int 3 } return 0; }
/* These two streams belong to PCH domains not executed by this case. Their
 * storage and initial contents remain unmodeled; the guard precedes reads. */
static FILE *pch_primary_stream; /* RVA550a0 */
static FILE *pch_secondary_stream; /* RVA57a28 */
extern "C" void __cdecl cleanup_precompiled_files(void)
{
    int status;
    if (c1_data.pch_cleanup_done) return;
    if (c1_data.pch_primary_stream_open || c1_data.pch_secondary_stream_open ||
        c1_data.pch_expression_stream_open) {
#ifdef __linux__
        recon_unsupported("PCH cleanup: open-stream ownership is not yet modeled");
#else
        __asm { int 3 }
#endif
    }
    if ((!c1_data.saved_pch_mode ||
        c1_bss.error_count + (c1_bss.options.warnings_are_errors > 1) == 0) &&
        !c1_data.pch_primary_stream_open && !c1_data.pch_secondary_stream_open &&
        !c1_data.pch_expression_stream_open) return;
    c1_data.pch_cleanup_done = 1;
    if (c1_data.pch_primary_stream_open) {
        status = fclose(pch_primary_stream);
        c1_bss.expression_seek_status = status;
        if (status == -1) report_response_error(82, 344, c1_pch_stop_owner);
        pch_primary_stream = NULL;
    } else if (c1_data.pch_secondary_stream_open) {
        status = fclose(pch_secondary_stream);
        c1_bss.expression_seek_status = status;
        if (status == -1) report_response_error(82, 344, c1_pch_stop_owner);
        pch_secondary_stream = NULL;
    } else if (c1_data.pch_expression_stream_open) {
        status = c1_bss.output_buffers[3].CloseStream();
        c1_bss.expression_seek_status = status;
        if (status == -1) report_response_error(82, 344, c1_pch_stop_owner);
    } else return;
    if (_unlink((const char *)c1_pch_stop_owner))
        report_response_error(83, 340, c1_pch_stop_owner);
    c1_data.pch_expression_stream_open = 0;
    c1_data.pch_secondary_stream_open = 0;
    c1_data.pch_primary_stream_open = 0;
}
extern "C" void __cdecl cleanup_saved_contexts(void);
extern "C" void __cdecl cleanup_arenas(void);
extern "C" void __cdecl cleanup_work_buffers(void);
extern "C" void __cdecl finish_compiler(void)
{
    ::operator delete(c1_bss.lexer_backend);
    cleanup_saved_contexts();
    cleanup_arenas();
    cleanup_work_buffers();
}
extern "C" void __cdecl free_argument_allocation(void *);
extern "C" void __cdecl cleanup_saved_contexts(void)
{
    int index;
    for (index = 0; index < c1_data.saved_context_count; ++index)
        if (c1_data.saved_context_records[index].allocation)
            free_argument_allocation(c1_data.saved_context_records[index].allocation);
    free_argument_allocation(c1_data.saved_context_records);
}
extern "C" void __cdecl cleanup_arenas(void)
{
    unsigned index;
    c1_bss.pch_state.Release();
    for (index = 0; index < 6; ++index) c1_bss.arenas[index].Release();
}
void RECON_THISCALL CompilerPchState::Release(void)
{
    static const BYTE close_order[6] = { 0, 1, 3, 5, 2, 4 };
    int index;
    if (!images) return;
    if (image_count > 0) {
#ifdef __linux__
        recon_unsupported("PCH release: nonempty image table is not yet modeled");
#else
        __asm { int 3 }
#endif
    }
    for (index = 0; index < image_count; ++index)
        if (images[index] && images[index]->kind == 6) images[index]->Release();
    for (index = 0; index < 6; ++index) {
        unsigned slot = close_order[index];
        if (mapping_handles[slot]) {
            CloseHandle(mapping_handles[slot]);
            mapping_handles[slot] = NULL;
        }
    }
    if (file) { CloseHandle(file); file = NULL; }
}

extern "C" void __cdecl print_compiler_statistics(void) { __asm { int 3 } }
extern "C" void __cdecl preprocess_translation_unit(void)
{
    CompilerParserToken *token;
    const char *text;
    for (;;) {
        token = c1_bss.current_context.ReadToken();
        switch (token->kind) {
        case 0: case 109:
            return;
        case 1: case 136: case 240: case 242:
            continue;
        case 7: case 101:
            /* Preprocessing retains the source spelling, including its NUL
             * in the WORD length. Only that spelling is written to stdout. */
            fwrite(token->value.string.text, (WORD)token->value.string.length - 1,
                1, stdout);
            break;
        default:
            /* Only encountered punctuation relations from the original
             * token-name column at RVA638a0 are available at this boundary. */
            if (!C1_HAS_TOKEN_SPELLING(token->kind)) {
#ifdef __linux__
            recon_unsupported("unmodeled preprocessor token spelling");
#else
            __asm { int 3 }
#endif
            }
            text = c1_token_spellings.indexed[token->kind];
            fwrite(text, strlen(text), 1, stdout);
            break;
        }
    }
}
extern "C" void __cdecl finish_parser(int result) { __asm { int 3 } }
extern "C" void __cdecl finish_parser_tables(void) { __asm { int 3 } }
extern "C" void __cdecl compiler_warning(int code, ...) { __asm { int 3 } }
void RECON_THISCALL CompilerDependency::Finish(int success) { __asm { int 3 } }
int RECON_THISCALL CompilerOutputBuffer::Close(void)
{
    int result = CloseStream();
    c1_bss.expression_seek_status = result;
    if (result == -1) report_response_error(82, 339, NULL);
    return result;
}
int RECON_THISCALL CompilerOutputBuffer::CloseStream(void)
{
    int result;
    if (!stream)
        return c1_bss.error_count + (c1_bss.options.warnings_are_errors > 1) ? 0 : -1;
    Flush();
    result = fclose(stream);
    stream = NULL;
    return result;
}
void RECON_THISCALL CompilerPdbContext::Finish(void)
{
    char error_text[260];
    int rebuild_ok = 1, database_ok;
    if (!database) return;
    if (!c1_pass_mode) {
        if (names->Slot18()) {
            if (rebuild_state) {
                rebuild_ok = rebuild_state->Finish() & 1;
                rebuild_state = NULL;
            }
            database_ok = c1_bss.error_count + (c1_bss.options.warnings_are_errors > 1) ||
                read_mode || database->Commit();
            if (database_ok & rebuild_ok) return;
        }
        compiler_warning(471, database->QueryLastError(error_text));
        return;
    }
    if (!names->Slot00())
        compiler_warning(471, database->QueryLastError(error_text));
    names = NULL;
    if (retain_database || types) return;
    database->QueryLastError(error_text);
    if (rebuild_state) {
        rebuild_ok = rebuild_state->Finish() & 1;
        rebuild_state = NULL;
    }
    database_ok = c1_bss.error_count + (c1_bss.options.warnings_are_errors > 1) ||
        read_mode || database->Commit();
    if (!database->Close()) database_ok = 0;
    database = NULL;
    if (!(database_ok & rebuild_ok)) compiler_warning(471, error_text);
}
int RECON_THISCALL CompilerMreState::Finish(void) { __asm { int 3 } return 0; }
