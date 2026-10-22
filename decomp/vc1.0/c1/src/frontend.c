/* Frontend orchestration, original compile_translation_unit 0x43950. */
#include <stdlib.h>
#include "frontend.h"
#include "pch.h"
#include "pragma.h"
#include "global-ir.h"

unsigned initial_option_flags, precompiled_option_flags;
int global_expression_scope_active; /* original 0x5b25c */
int error_count; /* original 0x5b228, shared by diagnostics, recovery and exit */
char version_string[256];
Vector *forced_source_inputs;
short forced_source_index;
char *current_forced_source;
FILE *expression_file, *symbol_file, *line_file, *global_file;
FILE *debug_file, *preprocessor_file;

/* Original 0x43950..0x43cb9. */
int compile_translation_unit(void)
{
    ArenaPool *pool;
    IrToken *token;
    unsigned length, i;
    unsigned char first;
    initialize_frontend();
    initial_option_flags = pack_option_flags();
    options.option_Yd &= !options.line_records;
    if (options.option_E) {
        preprocess_translation_unit();
        goto finish;
    }
    if (options.option_V) {
        pool = &compiler_arena.pools[2];
        if (pool->remaining < (int)sizeof(IrToken))
            refill_arena(&compiler_arena, pool);
        token = (IrToken *)pool->next;
        pool->remaining -= sizeof(IrToken);
        pool->next += sizeof(IrToken);
        token->flags = 5;
        token->kind = 3;
        for (length = 0; options.option_V[length]; ++length)
            ;
        token->length = (unsigned short)(length + 1);
        token->value.text = version_string;
        for (i = 0; i <= length; ++i)
            version_string[i] = options.option_V[i];
        assign_string_type(token);
        materialize_string_literal(token);
        global_expression_scope_active = 1;
    }
    if (options.pch_use ||
        (options.pch_use_file && (!options.pch_create_file ||
         !compare_filenames(options.pch_create_file, options.pch_use_file))))
        load_precompiled_header();
    else {
        precompiled_option_flags = pack_option_flags();
        if (options.pch_auto) {
            options.option_Yd &= !options.line_records;
            begin_automatic_pch();
        } else {
            error_count = 0;
            if (!options.option_E && !options.prototypes_only && options.debug_info)
                begin_debug_info();
        }
        CAPTURE_PCH_SETTINGS(saved_pch_settings, options, database_path, pch_object_name, i);

    }
    while (forced_source_inputs->count > forced_source_index) {
        current_forced_source = forced_source_inputs->items[forced_source_index++];
        if (open_include_file(current_forced_source, 0))
            break;
    }
    first = scan_token();
    unget_token(first);
    if (!first) {
        if (options.extensions_enabled)
            compiler_warning(4, 206);
        else
            report_syntax_error(183);
    }
    parse_translation_unit();
    if (!options.prototypes_only) {
        finish_declarations();
        if (global_expression_scope_active) {
            write_ir_record(0x53, 0);
            write_ir_record(0x54, make_integer_constant(0));
        }
        write_ir_record(0x4d, 0);
        if (!options.option_E && options.debug_info)
            finish_debug_info();
        write_global_header();
    }
    if (options.option_Ol && options.option_qc)
        generate_inline_queries();
    if (!options.prototypes_only) {
        close_intermediate(expression_file);
        close_intermediate(symbol_file);
        close_intermediate(global_file);
        close_intermediate(line_file);
        if (options.debug_info)
            close_intermediate(debug_file);
    }
finish:
    if (options.option_Fr)
        finish_browser_output();
    if (options.option_Fi && fclose(preprocessor_file))
        report_error(85, message_text(340), options.option_Fi, "main.c", 797);
    if (fflush(stdout))
        report_error(85, message_text(340), 0, "main.c", 807);
    cleanup_precompiled_files();
    exit(error_count + (options.option_WX > 1));
    return error_count + (options.option_WX > 1);
}
