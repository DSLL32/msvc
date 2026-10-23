/* Translation-unit orchestration, original 0x69730..0x69a97. */
#include <stdlib.h>
#include "frontend.h"
#include <recon.h>

unsigned initial_option_flags, token_allocation_flags;
unsigned cpp_parser_flags; /* original 0x9f794 */
CompilerConfiguration saved_configuration;
int global_expression_scope_active, error_count;
int pch_mode;
char version_string[256];
Vector *forced_source_inputs;
FILE *expression_file, *symbol_file, *line_file, *global_file;
FILE *debug_file, *preprocessor_file;

int compile_translation_unit(void)
{
    ArenaPool *pool;
    IrToken *token;
    unsigned length, i;

    initialize_frontend();
    initial_option_flags = pack_option_flags();
    options.configuration.option_Yd &= !options.configuration.line_records;
    if (options.option_E) {
        preprocess_translation_unit();
        goto finish;
    }
    if (version_directive_text) {
        if (token_allocation_flags & 0x20000020)
            token = allocate_permanent(sizeof(IrToken));
        else {
            pool = &compiler_arena.pools[2];
            if (pool->remaining < (int)sizeof(IrToken))
                refill_arena(&compiler_arena, pool);
            token = (IrToken *)pool->next;
            pool->remaining -= sizeof(IrToken);
            pool->next += sizeof(IrToken);
        }
        token->flags = 5;
        token->kind = 3;
        for (length = 0; version_directive_text[length]; ++length)
            ;
        token->tail.string.length = (unsigned short)(length + 1);
        token->value.text = version_string;
        for (i = 0; i < token->tail.string.length; ++i)
            version_string[i] = version_directive_text[i];
        record_string_literal(token);
        materialize_string_literal(token);
        global_expression_scope_active = 1;
    }
    if (options.pch_use ||
        (options.pch_use_file && (!options.pch_create_file ||
         !compare_filenames(options.pch_create_file, options.pch_use_file))))
        load_precompiled_header();
    else {
        options.configuration.precompiled_option_flags = pack_option_flags();
        if (options.pch_auto) {
            options.configuration.option_Yd = !options.configuration.line_records;
            begin_automatic_pch();
        } else {
            pch_mode = 0;
            if (!options.option_E && !options.prototypes_only && options.configuration.debug_info)
                begin_debug_info();
        }
        saved_configuration = options.configuration;
    }
    while (forced_source_inputs->count > forced_source_index) {
        current_forced_source = forced_source_inputs->items[forced_source_index++];
        if (open_forced_include(current_forced_source, 0))
            break;
    }
    parse_translation_unit();
    release_arena(5);
    if (!options.prototypes_only) {
        finish_declarations();
        finish_types();
        finish_inline_functions();
        finish_expression_output();
        if (global_expression_scope_active) {
            write_ir_record(0x53, 0);
            write_ir_record(0x54, make_integer_constant(0));
        }
        write_ir_record(0x4d, 0);
        if (!options.option_E && options.configuration.debug_info)
            finish_debug_info();
        write_global_header();
    }
    if (options.configuration.option_Ol && options.configuration.option_qc)
        generate_inline_queries();
    if (!options.prototypes_only) {
        close_intermediate(expression_file);
        close_intermediate(symbol_file);
        close_intermediate(global_file);
        close_intermediate(line_file);
        if (options.configuration.debug_info)
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
