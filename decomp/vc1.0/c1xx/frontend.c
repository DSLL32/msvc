/* Translation-unit orchestration, original 0x69730..0x69a97. */
#include <stdlib.h>
#include "frontend.h"

unsigned initial_option_flags, token_allocation_flags;
unsigned cpp_parser_flags; /* original 0x9f794 */
CompilerConfiguration saved_configuration;
int version_directive_emitted, error_count;
CppSymbol *current_function;
char version_string[256];
Vector *forced_source_inputs;
short forced_source_index;
char *current_forced_source;
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
    if (options.option_V) {
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
        for (length = 0; options.option_V[length]; ++length)
            ;
        token->length = (unsigned short)(length + 1);
        token->value.text = version_string;
        for (i = 0; i < token->length; ++i)
            version_string[i] = options.option_V[i];
        record_version_token(token);
        emit_version_token(token);
        version_directive_emitted = 1;
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
            current_function = 0;
            if (!options.option_E && !options.syntax_only && options.configuration.debug_info)
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
    if (!options.syntax_only) {
        finish_declarations();
        finish_types();
        finish_inline_functions();
        finish_expression_output();
        if (version_directive_emitted) {
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
    if (!options.syntax_only) {
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
