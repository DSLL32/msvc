/* Precompiled-header event playback and include ownership. */
#include <stdlib.h>
#include <ctype.h>
#undef toupper
#include <string.h>
#include "pch.h"
#include "global-ir.h"
#include "source-input.h"
#include "pragma.h"
#include "control-ir.h"
#include "lexer.h"
#include "preprocessor.h"
#include "literal-names.h"
#include <recon.h>

PchRecord *pch_record_cursor; /* original 0x5b568 */
short pch_include_count = -1; /* original 0x6de1c */
unsigned short pch_include_page_count; /* original 0x6de20 */
int pch_macro_changed; /* original 0x6de24 */
int pch_rebuild_on_macro_change; /* original 0x6de28 */

/* Original 0x45380..0x4549a. During PCH playback, includes and their
   macro-presence dependencies are consumed from the recorded event stream. */
int __stdcall pch_skip_include(char *filename)
{
    short previous_count;
    switch (pch_mode) {
    case 0:
        return 0;
    case 1:
        previous_count = pch_include_count--;
        if (!previous_count) finish_pch_capture();
        else pch_record_string(2, permanent_page_count, filename);
        if (options.pch_auto_file && compare_filenames(options.pch_auto_file, filename))
            pch_include_count = 0;
        return 0;
    case 2:
        if (pch_record_cursor->kind != 2 ||
            !compare_filenames((char *)(pch_record_cursor + 1), filename))
            return pch_replay_mismatch();
        ++pch_include_count;
        pch_include_page_count = (unsigned short)pch_record_cursor->value;
        for (;;) {
            pch_record_cursor = (PchRecord *)((char *)(pch_record_cursor + 1) +
                                            pch_record_cursor->payload_bytes);
            if (pch_record_cursor->kind != 5) return 1;
            if (pch_macro_defined((char *)(pch_record_cursor + 1)) != pch_record_cursor->value) {
                pch_macro_changed = 1;
                if (pch_rebuild_on_macro_change) return pch_replay_mismatch();
            }
        }
    }
    return pch_replay_mismatch();
}

/* Original 0x1fef0..0x1ffbd. Choose the explicit PCH pathname or replace
   the header's extension with .pch, retaining its drive and directory. */
void __stdcall prepare_pch_output(char *filename)
{
    char drive[4], directory[256], basename[256];
    unsigned drive_bytes, directory_bytes, basename_bytes;
    if (!options.option_Fp) {
        _splitpath(filename, drive, directory, basename, 0);
        for (drive_bytes = 0; drive[drive_bytes]; ++drive_bytes) ;
        for (directory_bytes = 0; directory[directory_bytes]; ++directory_bytes) ;
        for (basename_bytes = 0; basename[basename_bytes]; ++basename_bytes) ;
        options.option_Fp = (char *)malloc(drive_bytes + directory_bytes + basename_bytes + 5);
        if (!options.option_Fp) fatal_error(60);
        _makepath(options.option_Fp, drive, directory, basename, ".pch");
    }
    pragma_text = options.option_Fp;
}

/* Original 0x213f0..0x2145a. Case-insensitive DOS path comparison treats
   slash and backslash as the same separator without canonicalizing paths. */
int compare_filenames(char *first, char *second)
{
    int left, right;
    while (*first && *second) {
        left = toupper((unsigned char)*first);
        right = toupper((unsigned char)*second);
        if (left != right &&
            !((*first == '/' || *first == '\\') &&
              (*second == '/' || *second == '\\'))) return 0;
        ++first;
        ++second;
    }
    return *first == *second;
}

/* Separate wire images hold serialized views; canonical runtime state remains
   in its owning modules and is gathered before writing. */
Vector *pch_saved_defines, *pch_saved_include_directories; /* 0x6b664/668 */
/* Native compatibility fields are compared before a PCH is accepted.
   They have no executable setters outside configuration restoration. */
int pch_reserved_calling_convention; /* original 0x6bb5c */
int pch_member_model[5]; /* original 0x6bb68: /vm representation flags */
int pch_vtordisp = 1; /* original 0x6bb7c: /vd compatibility mode */
int pch_optimization_reserved; /* original 0x6bbf8 */
int pch_optimization_sentinel = -1; /* original 0x6bbfc */
PchSettingsImage saved_pch_settings;
PchCompilerImage pch_compiler_image;
unsigned short pch_object_name_bytes, pch_browser_name_bytes; /* 0x6bbbc/e */
unsigned pch_global_bytes, pch_line_bytes, pch_debug_bytes, pch_browser_bytes;
unsigned pch_event_bytes, pch_event_offset, pch_header_reserved;
WorkingStack *pch_event_stack; /* original 0x6de14 */

/* Original 0x46a70..0x46e27. Append saved state and relocated arena pages,
   rewrite the leading header, then restore the running compiler's pointers. */
void finish_pch_output(void)
{
    unsigned i, length;
    int separate_debug, stream_error;
    PchSettingsImage settings;
    PchHeader header;
    if (!pch_mode && conditional_depth >= 0) report_syntax_error(856);
    if (error_count) return;
    if (options.debug_info && !options.option_Yd && pch_object_name[0])
        declare_pch_debug_library();
    separate_debug = pch_separate_debug;
    if (separate_debug) finish_pch_debug_symbols();
    pch_saved_include_directories = copy_vector(options.include_directories, 0,
                                      (void *(__stdcall *)(void *, int))copy_string);
    pch_saved_defines = copy_vector(options.defines, 0,
                                      (void *(__stdcall *)(void *, int))copy_string);
    stage_pch_intermediates();
    if (options.option_Fo) {
        for (length = 0; options.option_Fo[length]; ++length) ;
        pch_object_name_bytes = (unsigned short)(length + 1);
    }
    if (browser_filename) {
        for (length = 0; options.option_Fr[length]; ++length) ;
        pch_browser_name_bytes = (unsigned short)(length + 1);
        if (!pch_mode) begin_browser_pch_record();
    }
    relocate_pch_state();
    if (pch_mode == 1) {
        intermediate_status = fflush(pch_file);
        if (intermediate_status == -1)
            report_error(88, message_text(344), options.option_Fp, pch_diagnostic_name, 1342);
        intermediate_status = ftell(pch_file);
        if (intermediate_status == -1)
            report_error(87, message_text(344), options.option_Fp, pch_diagnostic_name, 1343);
        pch_event_offset = intermediate_status;
        pch_event_bytes = write_pch_event_stack(pch_event_stack, pch_file, 0) & 0xffff;
    }
    precompiled_option_flags = pack_option_flags();
    CAPTURE_PCH_SETTINGS(settings, options, database_path, pch_object_name, i);
    intermediate_status = fwrite(&settings, 1, sizeof(settings), pch_file);
    if (intermediate_status == -1)
        report_error(85, message_text(344), options.option_Fp, pch_diagnostic_name, 1350);
    fwrite(&pch_compiler_image, 1, sizeof(pch_compiler_image), pch_file);
    fwrite(browser_filename, 1, pch_browser_name_bytes, pch_file);
    fwrite(options.option_Fo, 1, pch_object_name_bytes, pch_file);
    write_pch_pages();
    stream_error = (pch_file->_flag & 0x20) != 0;
    rewind(pch_file);
    for (i = 0; i < 8; ++i) header.signature[i] = "DTJPCH0"[i];
    header.format = 0x80117112;
    header.symbol_bytes = pch_symbol_bytes;
    header.global_bytes = pch_global_bytes;
    header.expression_bytes = pch_expression_bytes;
    header.line_bytes = pch_line_bytes;
    header.debug_bytes = pch_debug_bytes;
    header.browser_bytes = pch_browser_bytes;
    header.event_bytes = pch_event_bytes;
    header.event_offset = pch_event_offset;
    header.browser_offset = browser_saved_position;
    header.page_count = permanent_page_count;
    header.reserved = pch_header_reserved;
    intermediate_status = fwrite(&header, sizeof(header), 1, pch_file);
    if (intermediate_status == -1)
        report_error(85, message_text(344), options.option_Fp, pch_diagnostic_name, 1394);
    fwrite(&saved_pch_settings, 1, sizeof(saved_pch_settings), pch_file);
    restore_pch_pointers();
    pch_output_written = 1;
    intermediate_status = fclose(pch_file);
    if (intermediate_status == -1)
        report_error(82, message_text(344), options.option_Fp, pch_diagnostic_name, 1415);
    pch_file_incomplete = 0;
    if (error_count || stream_error) {
        if (options.option_Fp && options.option_Fp[0]) remove(options.option_Fp);
        intermediate_status = stream_error;
        if (stream_error)
            report_error(85, message_text(344), options.option_Fp, pch_diagnostic_name, 1423);
    }
    if (separate_debug && !options.line_records) {
        pending_pragma_kind = 7;
        pending_pragma_text = (unsigned char *)"\003\000\240\007";
        record_line_information(11);
    }
}

/* Original 0x1fda0..0x1fee8. Skip physical source until /Yu's named
   include, or the bare /Yu header-stop pragma, then restore frontend state. */
void load_precompiled_header(void)
{
    char *filename;
    unsigned char directive;
    scanner_flags |= 0x10000;
    final_newline_emitted = 1;
    if (!options.pch_use_file) {
        while (forced_source_index < forced_source_inputs->count)
            current_forced_source = forced_source_inputs->items[forced_source_index++];
    } else {
        while (forced_source_index < forced_source_inputs->count) {
            current_forced_source = forced_source_inputs->items[forced_source_index++];
            if (compare_filenames(options.pch_use_file, current_forced_source)) {
                prepare_pch_output(options.pch_use_file);
                final_newline_emitted = 0;
                break;
            }
        }
    }
    for (;;) {
        if (!final_newline_emitted) {
            browser_include_begin();
            read_pch_file();
            browser_include_begin();
            scanner_flags &= ~0x10000;
            return;
        }
        if (options.pch_use) {
            do { directive = find_conditional_directive(); } while (directive != 8);
            read_pragma_token();
            if (current_token_code == TOK_IDENTIFIER &&
                classify_pragma(current_token_value.identifier) == 29) {
                synchronize_pch_pragma();
                prepare_pch_output(options.source_file);
                final_newline_emitted = 0;
            } else skip_source_line();
        } else {
            do { directive = find_conditional_directive(); } while (directive != 7);
            filename = read_include_filename();
            if (compare_filenames(options.pch_use_file, filename)) {
                prepare_pch_output(options.pch_use_file);
                final_newline_emitted = 0;
            }
        }
    }
}

/* Original0x44f90..0x45146. Record main-file macro definitions while
   creating a PCH, or consume an identical pending definition during replay.
   Name/replacement comparisons were compiler-inlined in the original. */
int __stdcall pch_record_macro(char *name, char *replacement, int parameter_count)
{
    PchRecord *record;
    unsigned name_bytes, replacement_bytes;
    unsigned short count = (unsigned short)parameter_count;
    unsigned char *left, *right;
    char *saved_name;
    if (!pch_mode) return 0;
    if (pch_mode == 1) {
        name_bytes = 1;
        left = (unsigned char *)name;
        while (*left++) ++name_bytes;
        replacement_bytes = 1;
        left = (unsigned char *)replacement;
        while (*left++) ++replacement_bytes;
        name_bytes = (unsigned short)name_bytes;
        replacement_bytes = (unsigned short)replacement_bytes;
        write_pch_record_header(1,count,name_bytes + replacement_bytes);
        write_initializer_bytes(name,name_bytes,1,pch_event_stack);
        write_initializer_bytes(replacement,replacement_bytes,1,pch_event_stack);
        return 0;
    }
    if (pch_mode != 2) return 0;
    record = pch_record_cursor;
    for (;;) {
        if (record->kind) {
            if (record->kind != 1) {
                saved_name = copy_string(interned_name,2);
                if (!pch_replay_mismatch()) {
                    intern_identifier(saved_name);
                    return 0;
                }
                return 1;
            }
            if ((unsigned short)record->value == count) {
                left = (unsigned char *)(record + 1);
                right = (unsigned char *)name;
                while (*left == *right && *left) { ++left; ++right; }
                if (*left == *right) {
                    left = (unsigned char *)(record + 1);
                    while (*left++) ;
                    right = (unsigned char *)replacement;
                    while (*left == *right && *left) { ++left; ++right; }
                    if (*left == *right) {
                        record->kind = 0;
                        while (!pch_record_cursor->kind)
                            pch_record_cursor = (PchRecord *)((char *)(pch_record_cursor + 1) +
                                pch_record_cursor->payload_bytes);
                        return 1;
                    }
                }
            }
        }
        record = (PchRecord *)((char *)(record + 1) + record->payload_bytes);
    }
}
