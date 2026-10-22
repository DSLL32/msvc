/* PCH validation, state restoration and current-command reconciliation. */
#include "pch-relocate.h"
#include "source-input.h"
#include "pragma.h"
#include "lexer.h"
#include "control-ir.h"
#include "debug-info.h"
#include <recon.h>

/* Original 0x45e90..0x462e4. Diagnose incompatible command settings
   before replacing permanent state with the saved compiler graph. */
void read_pch_file(void)
{
    PchHeader header;
    unsigned i, length, out;
    char description[260];
    pch_file = fopen(options.option_Fp, "r+b");
    if (!pch_file) {
        report_fatal(83, message_text(344), options.option_Fp);
        return;
    }
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
    fread(&header, 1, sizeof(header), pch_file);
    pch_symbol_bytes = header.symbol_bytes;
    pch_global_bytes = header.global_bytes;
    pch_expression_bytes = header.expression_bytes;
    pch_line_bytes = header.line_bytes;
    pch_debug_bytes = header.debug_bytes;
    pch_browser_bytes = header.browser_bytes;
    pch_event_bytes = header.event_bytes;
    pch_event_offset = header.event_offset;
    browser_saved_position = header.browser_offset;
    permanent_page_count = header.page_count;
    pch_header_reserved = header.reserved;
    for (i = 0; i < 8; ++i) {
        if (header.signature[i] != "DTJPCH0"[i]) {
            fatal_error(852, options.option_Fp);
            return;
        }
    }
    if (header.format != 0x80117112) {
        fatal_error(853, options.option_Fp);
        return;
    }
    fread(&saved_pch_settings, 1, sizeof(saved_pch_settings), pch_file);
    if (options.option_Gn != saved_pch_settings.option_Gn)
        compiler_warning(1, 652, "Native caller(/Gn)");
    if (options.option_PLMN != saved_pch_settings.option_PLMN)
        compiler_warning(1, 652, "Pascal calls");
    if (options.option_PLMF != saved_pch_settings.option_PLMF)
        compiler_warning(1, 652, "Pascal calls");
    if (options.option_Gr != saved_pch_settings.option_Gr)
        compiler_warning(1, 652, "FastCall(/Gr)");
    if (options.japanese_identifiers != saved_pch_settings.japanese_identifiers ||
        options.option_J != saved_pch_settings.option_J)
        compiler_warning(1, 652, "Unsigned char(/J)");
    if (pch_member_model[0] != saved_pch_settings.member_model[0])
        compiler_warning(1, 652, "PM representation(/vmb)");
    if (pch_member_model[1] != saved_pch_settings.member_model[1])
        compiler_warning(1, 652, "PM representation(/vmg)");
    if (pch_member_model[2] != saved_pch_settings.member_model[2] ||
        pch_member_model[3] != saved_pch_settings.member_model[3] ||
        pch_member_model[4] != saved_pch_settings.member_model[4])
        compiler_warning(1, 652, "general PM representation(/vm[smv])");
    if (pch_vtordisp != saved_pch_settings.vtordisp)
        compiler_warning(1, 652, "vtordisp(/vd[01])");
    if (options.option_MD != saved_pch_settings.option_MD)
        compiler_warning(1, 652, "DLL (/MD)");
    if (options.option_Oy != saved_pch_settings.option_Oy ||
        options.option_Oa != saved_pch_settings.option_Oa ||
        options.option_Ow != saved_pch_settings.option_Ow ||
        options.option_Oz != saved_pch_settings.option_Oz ||
        options.option_Op != saved_pch_settings.option_Op)
        compiler_warning(1, 652, "Optimizations (one or more of /Oyawp)");
    if (options.option_M != saved_pch_settings.option_M) {
        report_syntax_error(855, "Memory Model(/A)");
        options.option_M = saved_pch_settings.option_M;
        return;
    }
    if (options.option_NoComdat != saved_pch_settings.option_NoComdat)
        compiler_warning(1, 652, "Use Comdats");
    if (options.debug_info && !saved_pch_settings.debug_info)
        compiler_warning(1, 650);
    if ((browser_enabled && !saved_pch_settings.browser_enabled) ||
        browser_full != saved_pch_settings.browser_full) {
        compiler_warning(1, 652, "Generate Browser Info");
        browser_filename = 0;
        browser_enabled = 0;
    }
    if (options.line_records &&
        !compare_filenames(database_path, saved_pch_settings.database_path)) {
        for (out = 0; (description[out] = "program database name (/Fd"[out]) != 0; ++out) ;
        for (i = 0; (description[out + i] = database_path[i]) != 0; ++i) ;
        out += i;
        description[out++] = ')';
        description[out] = 0;
        report_syntax_error(855, description);
    }
    if (pch_object_name[0]) {
        for (length = 0; pch_object_name[length] == saved_pch_settings.pch_object_name[length] &&
                         pch_object_name[length]; ++length) ;
        if (pch_object_name[length] != saved_pch_settings.pch_object_name[length])
            compiler_warning(1, 652, "Library PCT Identifier (/Yl)");
    }
    restore_pch_state();
}

/* Original 0x45b40..0x45e81. Load the saved graph and reconcile macro
   definitions with this invocation, retaining the original string mutation. */
void restore_pch_state(void)
{
    char object_name[256], *current, *saved;
    unsigned i, length;
    short current_index, saved_index;
    Vector *definitions;
    extract_pch_intermediates(saved_pch_settings.option_Yd && options.debug_info);
    if (pch_mode == 2) {
        intermediate_status = fseek(pch_file, pch_event_bytes + pch_event_offset, 0);
        if (intermediate_status)
            report_error(86, message_text(344), options.option_Fp, pch_diagnostic_name, 738);
    }
    fread(&saved_pch_settings, 1, sizeof(saved_pch_settings), pch_file);
    unpack_option_flags(saved_pch_settings.optimization_flags);
    if (pch_mode == 2)
        RESTORE_PCH_SETTINGS(saved_pch_settings, options, database_path, pch_object_name, i);
    fread(&pch_compiler_image, 1, sizeof(pch_compiler_image), pch_file);
    for (i = 0; i < 105; ++i)
        if (pch_root_cells[i]) *pch_root_cells[i] = pch_compiler_image.roots[i];
    pch_reserved_state.after_roots = pch_compiler_image.reserved_after_roots;
    pch_relocation_count = pch_compiler_image.relocation_count;
    permanent_available = pch_compiler_image.available;
    pch_reserved_state.string_count = pch_compiler_image.reserved_string_count;
    function_record_index = pch_compiler_image.function_record_index;
    pch_reserved_state.record_count = pch_compiler_image.reserved_record_count;
    active_plmf_pragma = pch_compiler_image.active_plmf;
    active_plmn_pragma = pch_compiler_image.active_plmn;
    next_symbol_index = pch_compiler_image.next_symbol_index;
    browser_symbol_limit = pch_compiler_image.browser_symbol_limit;
    pch_reserved_state.one = pch_compiler_image.reserved_one;
    pch_reserved_state.zero = pch_compiler_image.reserved_zero;
    pch_symbol_offset_adjustment = pch_compiler_image.symbol_offset;
    next_debug_type_index = pch_compiler_image.next_debug_index;
    pch_reserved_state.debug_padding = pch_compiler_image.reserved_debug;
    pragma_record_number = pch_compiler_image.pragma_record_number;
    debug_type_bytes = pch_compiler_image.debug_type_bytes;
    pch_reserved_state.debug_bytes = pch_compiler_image.reserved_debug_bytes;
    for (i = 0; i < 800; ++i) warning_levels[i] = pch_compiler_image.warning_levels[i];
    lowest_warning_700 = pch_compiler_image.lowest_warning;
    highest_warning_700 = pch_compiler_image.highest_warning;
    source_file_counter = pch_compiler_image.source_file_counter;
    scanner_flags = pch_compiler_image.scanner_flags;
    parser_flags = pch_compiler_image.parser_flags;
    if (saved_pch_settings.browser_name_bytes) {
        fread(object_name, 1, saved_pch_settings.browser_name_bytes, pch_file);
        if (browser_filename && !pch_browser_bytes) restore_pch_browser_name(object_name);
    }
    if (saved_pch_settings.object_name_bytes)
        fread(object_name, 1, saved_pch_settings.object_name_bytes, pch_file);
    read_pch_pages();
    restore_pch_pointers();
    if (options.debug_info) {
        if (!saved_pch_settings.option_Yd) {
            if (saved_pch_settings.debug_info && options.option_Fo &&
                compare_filenames(options.option_Fo, object_name)) {
                if (!options.line_records) compiler_warning(3, 622, options.option_Fo);
                if (pch_object_name[0]) declare_pch_debug_library();
            }
            restore_pch_debug_state(object_name);
            if (!options.pch_auto && saved_pch_settings.pch_object_name[0])
                reference_pch_debug_library(saved_pch_settings.pch_object_name);
        } else restore_pch_separate_debug();
    }
    definitions = options.defines;
    for (current_index = 0; current_index < definitions->count; ++current_index) {
        current = definitions->items[current_index];
        for (saved_index = 0; saved_index < pch_saved_defines->count; ++saved_index) {
            saved = pch_saved_defines->items[saved_index];
            for (length = 0; current[length] && current[length] == saved[length]; ++length) ;
            if (current[length] == saved[length]) {
                *saved = 0;
                break;
            }
        }
        if (saved_index >= pch_saved_defines->count) define_command_line_macro(current);
    }
    for (saved_index = 0; saved_index < pch_saved_defines->count; ++saved_index) {
        saved = pch_saved_defines->items[saved_index];
        if (*saved) compiler_warning(1, 651, saved);
    }
    if (!pch_write_enabled) {
        intermediate_status = fclose(pch_file);
        if (intermediate_status == -1)
            report_error(82, message_text(344), options.option_Fp, pch_diagnostic_name, 886);
    }
    options.option_Fp = 0;
}
