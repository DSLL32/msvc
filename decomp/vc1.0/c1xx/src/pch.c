/* Precompiled-header path selection and saved frontend state. */
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include "initialization.h"
#include "global-ir.h"
#include "pragma.h"
#include "browser.h"
#include "function-finalization.h"
#include <recon.h>

/* Original 0x23310..0x233dd. The default name retains the source drive and
   directory, replaces the extension, and becomes the current pragma name.
   The explicit /Fp path and pragma_text share their original pointer owners. */
void __stdcall prepare_pch_output(char *filename)
{
    char drive[4], directory[256], basename[256];
    unsigned length, part;
    if (!options.option_Fp) {
        _splitpath(filename, drive, directory, basename, 0);
        for (length = 0; drive[length]; ++length) ;
        for (part = 0; directory[part]; ++part) ;
        length += part;
        for (part = 0; basename[part]; ++part) ;
        length += part + 5;
        options.option_Fp = malloc(length);
        if (!options.option_Fp) fatal_error(60);
        _makepath(options.option_Fp, drive, directory, basename, ".pch");
    }
    pragma_text = options.option_Fp;
}

/* Original 0x24840..0x248aa. Filename equality folds case and accepts either
   path separator, but preserves exact termination rather than prefix matching. */
int compare_filenames(char *first, char *second)
{
    while (*first && *second) {
        if (toupper((unsigned char)*first) != toupper((unsigned char)*second) &&
            !((*first == '/' || *first == '\\') &&
              (*second == '/' || *second == '\\')))
            return 0;
        ++first;
        ++second;
    }
    return *first == *second;
}

void read_pch_file(void);

/* Original 0x231c0..0x23308. Consume forced inputs and physical source up to
   /Yu's named include or hdrstop, then restore the actual saved frontend. */
void load_precompiled_header(void)
{
    char *filename;
    token_allocation_flags |= 0x10000;
    final_newline_emitted = 1;
    if (options.pch_use_file) {
        while (forced_source_inputs->count > forced_source_index) {
            current_forced_source = forced_source_inputs->items[forced_source_index++];
            if (compare_filenames(options.pch_use_file, current_forced_source)) {
                prepare_pch_output(options.pch_use_file);
                final_newline_emitted = 0;
                break;
            }
        }
    } else {
        while (forced_source_inputs->count > forced_source_index)
            current_forced_source = forced_source_inputs->items[forced_source_index++];
    }
    while (final_newline_emitted) {
        if ((unsigned char)options.pch_use) {
            while (read_skipped_directive() != PP_PRAGMA) ;
            read_pragma_token();
            if (current_token_code == 2 && classify_pragma(current_token_value.name) == 29) {
                synchronize_pch_pragma();
                prepare_pch_output(options.source_file);
                final_newline_emitted = 0;
            } else skip_source_line();
        } else {
            while (read_skipped_directive() != PP_INCLUDE) ;
            filename = read_include_name();
            if (compare_filenames(options.pch_use_file, filename)) {
                prepare_pch_output(options.pch_use_file);
                final_newline_emitted = 0;
            }
        }
    }
    browser_include_begin();
    read_pch_file();
    browser_include_begin();
    token_allocation_flags &= ~0x10000U;
}

PchHeader pch_header = { "DTJPCH0", 0x80117312 };
extern FILE *pch_file;
extern int pch_file_incomplete;
void restore_pch_frontend(void);

/* Original0x6c070..0x6c4c4. Read the actual format/configuration, report each
   incompatible option separately, then delegate stream/arena restoration. */
void read_pch_file(void)
{
    char warning[260];
    unsigned i, length;
    char *left, *right;
    pch_file = fopen(options.option_Fp, "r+b");
    if (!pch_file) {
        report_fatal(83, message_text(344), options.option_Fp);
        return;
    }
    fread(&pch_header, 1, sizeof(pch_header), pch_file);
    for (i = 0; i < 8 && pch_header.signature[i] == "DTJPCH0"[i]; ++i) ;
    if (i != 8) {
        fatal_error(852, options.option_Fp);
        return;
    }
    if (pch_header.format != 0x80117312) {
        fatal_error(853, options.option_Fp);
        return;
    }
    fread(&saved_configuration, 1, sizeof(saved_configuration), pch_file);
    if (saved_configuration.option_Gn != options.configuration.option_Gn)
        compiler_warning(1, 652, "Native caller(/Gn)");
    if (saved_configuration.option_PLMN != options.configuration.option_PLMN)
        compiler_warning(1, 652, "Pascal calls");
    if (saved_configuration.option_PLMF != options.configuration.option_PLMF)
        compiler_warning(1, 652, "Pascal calls");
    if (saved_configuration.option_Gr != options.configuration.option_Gr)
        compiler_warning(1, 652, "FastCall(/Gr)");
    if (saved_configuration.japanese_identifiers != options.configuration.japanese_identifiers ||
        saved_configuration.option_J != options.configuration.option_J)
        compiler_warning(1, 652, "Unsigned char(/J)");
    if (saved_configuration.option_vmb != options.configuration.option_vmb)
        compiler_warning(1, 652, "PM representation(/vmb)");
    if (saved_configuration.option_vmg != options.configuration.option_vmg)
        compiler_warning(1, 652, "PM representation(/vmg)");
    if (saved_configuration.option_vms != options.configuration.option_vms ||
        saved_configuration.option_vmm != options.configuration.option_vmm ||
        saved_configuration.option_vmv != options.configuration.option_vmv)
        compiler_warning(1, 652, "general PM representation(/vm[smv])");
    if (saved_configuration.option_vd != options.configuration.option_vd)
        compiler_warning(1, 652, "vtordisp(/vd[01])");
    if (saved_configuration.option_MD != options.configuration.option_MD)
        compiler_warning(1, 652, "DLL (/MD)");
    if (saved_configuration.option_Oy != options.configuration.option_Oy ||
        saved_configuration.option_Oa != options.configuration.option_Oa ||
        saved_configuration.option_Ow != options.configuration.option_Ow ||
        saved_configuration.option_Oz != options.configuration.option_Oz ||
        saved_configuration.option_Op != options.configuration.option_Op)
        compiler_warning(1, 652, "Optimizations (one or more of /Oyawp)");
    if (saved_configuration.option_M != options.configuration.option_M) {
        report_syntax_error(855, "Memory Model(/A)");
        options.configuration.option_M = saved_configuration.option_M;
        return;
    }
    if (saved_configuration.option_NoComdat != options.configuration.option_NoComdat)
        compiler_warning(1, 652, "Use Comdats");
    if (options.configuration.debug_info && !saved_configuration.debug_info)
        compiler_warning(1, 650);
    if ((options.configuration.browser_enabled && !saved_configuration.browser_enabled) ||
        saved_configuration.browser_full != options.configuration.browser_full) {
        compiler_warning(1, 652, "Generate Browser Info");
        browser_filename = 0;
        options.configuration.browser_enabled = 0;
    }
    if (options.configuration.line_records &&
        !compare_filenames(options.configuration.database_path, saved_configuration.database_path)) {
        for (i = 0; (warning[i] = "program database name (/Fd"[i]) != 0; ++i) ;
        for (length = 0; options.configuration.database_path[length]; ++length)
            warning[i++] = options.configuration.database_path[length];
        warning[i++] = ')';
        warning[i] = 0;
        report_syntax_error(855, warning);
    }
    if (options.configuration.pch_object_name[0]) {
        left = options.configuration.pch_object_name;
        right = saved_configuration.pch_object_name;
        while (*left == *right && *left) { ++left; ++right; }
        if (*left != *right) compiler_warning(1, 652, "Library PCT Identifier (/Yl)");
    }
    restore_pch_frontend();
}

Vector *pch_saved_include_directories; /* original0x9f200 */
Vector *pch_saved_defines; /* original0x9f1fc */
WorkingStack *pch_event_stack; /* original0xa2354 */
unsigned pch_operator_count; /* original0x9f420 */
PchCompilerImage pch_compiler_image; /* serialized view, filled by relocation */
void declare_pch_debug_library(void);
void finish_pch_debug_symbols(void);
void stage_pch_intermediates(void);
void begin_browser_pch_record(void);
void relocate_pch_state(void);
unsigned short __stdcall write_pch_events(WorkingStack *, FILE *, int);
void write_pch_pages(void);
void restore_pch_pointers(void);

/* Original0x6cc50..0x6d038. Save stream prefixes and canonical root state,
   write the format records/pages, then restore the running frontend pointers. */
void finish_pch_output(void)
{
    unsigned length;
    int separate_debug, stream_error;
    if (!pch_mode && conditional_depth >= 0) report_syntax_error(856);
    if (error_count) return;
    if (token_allocation_flags & 0x40) flush_current_function(1);
    if (options.configuration.debug_info && !options.configuration.option_Yd &&
        options.configuration.pch_object_name[0]) declare_pch_debug_library();
    separate_debug = pch_separate_debug;
    if (separate_debug) finish_pch_debug_symbols();
    pch_saved_include_directories = copy_vector_elements(options.include_directories, 0,
        (VectorCopyElement)copy_string);
    pch_saved_defines = copy_vector_elements(options.defines, 0,
        (VectorCopyElement)copy_string);
    stage_pch_intermediates();
    if (options.option_Fo) {
        for (length = 0; options.option_Fo[length]; ++length) ;
        options.configuration.field6c = (options.configuration.field6c & 0xffff0000U) |
            (unsigned short)(length + 1);
    }
    if (browser_filename) {
        for (length = 0; options.option_Fr[length]; ++length) ;
        options.configuration.field6c = (options.configuration.field6c & 0xffffU) |
            ((unsigned)(unsigned short)(length + 1) << 16);
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
        pch_header.event_offset = intermediate_status;
        pch_header.event_bytes = write_pch_events(pch_event_stack, pch_file, 0);
    }
    options.configuration.precompiled_option_flags = pack_option_flags();
    intermediate_status = fwrite(&options.configuration, 1, sizeof(options.configuration), pch_file);
    if (intermediate_status == -1)
        report_error(85, message_text(344), options.option_Fp, pch_diagnostic_name, 1350);
    fwrite(&pch_compiler_image, 1, sizeof(pch_compiler_image), pch_file);
    fwrite(browser_filename, 1, (unsigned)options.configuration.field6c >> 16, pch_file);
    fwrite(options.option_Fo, 1, (unsigned short)options.configuration.field6c, pch_file);
    fwrite(builtin_operators, 1, pch_operator_count * sizeof(BuiltinOperator), pch_file);
    write_pch_pages();
    stream_error = (pch_file->_flag & 0x20) != 0;
    rewind(pch_file);
    intermediate_status = fwrite(&pch_header, sizeof(pch_header), 1, pch_file);
    if (intermediate_status == -1)
        report_error(85, message_text(344), options.option_Fp, pch_diagnostic_name, 1394);
    fwrite(&saved_configuration, 1, sizeof(saved_configuration), pch_file);
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
    if (separate_debug && !options.configuration.line_records) {
        pending_pragma_kind = 7;
        pending_pragma_text = (unsigned char *)"\003\000\240\007";
        record_line_information(11);
    }
}

void finish_pch_debug_types(void);

/* Original0x55be0..0x55bfa. Finalize the current global scope before the
   PCH-specific type terminator; diagnostics suppress both operations. */
void finish_pch_debug_symbols(void)
{
    if (!error_count) {
        serialize_scope(global_scope);
        finish_pch_debug_types();
    }
}
