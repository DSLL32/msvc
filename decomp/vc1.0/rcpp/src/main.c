/* Top-level RCPP driver, original [01007030,010071aa). */
#include "lexer.h"
#include "tokens.h"
#include <string.h>
#include <recon.h>

static int argument_count;
static char **argument_vector;
char rc_diagnostic_buffer[256];
char *rc_message_format;
int rc_error_count, rc_preprocessing_started;
char rc_source_name[260];

int rc_main(int argc, char **argv, char **envp)
{
    static RcSuboption z_options[] = {
        { 'a', 5, &rc_options.extensions },
        { 'e', 1, &rc_options.extensions },
        { 'E', 1, &rc_options.zE },
        { 'i', 1, &rc_options.debug_information },
        { 'g', 1, &rc_options.prototype_output },
        { 'p', 1, &rc_options.packing },
        { 'I', 1, &rc_options.zI },
        { 'c', 1, &rc_options.zc },
        { 0, 0, NULL }
    };
    static RcOption options[] = {
        { "-pc#", &rc_options.path_characters, 1, 0x22 },
        { "-pf", &rc_options.pf, 1, 1 },
        { "-C", &rc_options.keep_comments, 1, 1 },
        { "-CP#", &rc_options.codepage, 1, 0x22 },
        { "-D#", &rc_definitions, 1, 0x26 },
        { "-E", &rc_options.preprocess_lines, 1, 1 },
        { "-I#", &rc_include_directories, 1, 0x26 },
        { "-P", &rc_options.preprocess_plain, 1, 1 },
        { "-f", &rc_options.input_name, 1, 0x22 },
        { "-g", &rc_options.output_name, 1, 0x22 },
        { "-J", &rc_options.unsigned_char, 1, 1 },
        { "-Zp", &rc_options.packing, 1, 1 },
        { "-Zp#", &rc_options.packing, 1, 0x24 },
        { "-Z*", z_options, 1, 0x23 },
        { "-Oi", &rc_options.intrinsics, 1, 1 },
        { "-Ol", &rc_options.loop_optimization, 1, 1 },
        { "-db#", &rc_options.debug_file, 1, 0x22 },
        { "-ef#", &rc_options.error_file, 1, 0x22 },
        { "-il#", &rc_options.il_file, 1, 0x22 },
        { "-xc", &rc_options.xc, 1, 1 },
        { "-H", &rc_options.H, 1, 1 },
        { "-V#", &rc_options.version, 1, 0x22 },
        { "-Gs", &rc_options.stack_check, 1, 5 },
        { "-Gc", &rc_options.pascal_calls, 1, 1 },
        { "-char#", &rc_options.char_type, 1, 0x24 },
        { "-A#", &rc_options.A, 1, 0x22 },
        { "-Q#", &rc_options.Q, 1, 0x22 },
        { "-Fs", &rc_options.Fs, 1, 1 },
        { "-R", &rc_options.R, 1, 1 },
        { "*", &rc_options.unmatched_input, 0, 0x22 },
        { NULL, NULL, 0, 0 }
    };
    argument_count = argc;
    argument_vector = argv;
    while (rc_parse_option(options, rc_next_argument(), rc_next_argument, 0)) ;
    if (rc_options.unmatched_input) {
        rc_message_format = rc_message(1007);
        rc_format(rc_diagnostic_buffer, rc_message_format,
                  rc_options.unmatched_input, "c1");
        rc_fatal(1007);
    }
    if (!rc_options.input_name) {
        rc_message_format = rc_message(1008);
        rc_format(rc_diagnostic_buffer, rc_message_format);
        rc_fatal(1008);
    }
    if (!rc_options.output_name) {
        rc_message_format = rc_message(1011);
        rc_format(rc_diagnostic_buffer, rc_message_format);
        rc_fatal(1011);
    }
    rc_preprocessing_started = 1;
    if (!rc_options.preprocess_lines && !rc_options.preprocess_plain)
        rc_options.preprocess_lines = 1;
    strncpy(rc_source_name, rc_options.input_name, 128);
    SetConsoleCtrlHandler(rc_control_handler, TRUE);
    rc_initialize_preprocessor(rc_options.input_name, rc_options.output_name,
                               &rc_definitions);
    rc_emit_tokens();
    SetConsoleCtrlHandler(rc_control_handler, FALSE);
    if (rc_conditional_depth >= 0) {
        rc_message_format = rc_message(1022);
        rc_format(rc_diagnostic_buffer, rc_message_format);
        rc_fatal(1022);
    }
    rc_exit(rc_error_count);
    return 0;
}

/* Original [010071b0,010071c7). The event value is ignored; every handled
 * console notification closes user streams and terminates with status one. */
BOOL WINAPI rc_control_handler(DWORD event)
{
    (void)event;
    _fcloseall();
    rc_exit(1);
    return TRUE;
}

/* Original [01007000,01007023). Its zero-return tail is also a byte-match for
 * unrelated CRT leaf routines; it is part of this function's conditional path. */
char *rc_next_argument(void)
{
    if (--argument_count > 0) return *++argument_vector;
    return NULL;
}

/* Original [010071d0,01007252). The lexer writes identifiers, whitespace,
 * numbers and quoted text itself; this loop emits punctuation spellings. */
void rc_emit_tokens(void)
{
    unsigned category, length;
    const char *text;
    for (;;) {
        category = rc_next_token();
        if (!category) return;
        if (category == 1) continue;
        if (!rc_token_kind) {
            rc_message_format = rc_message(1068);
            rc_format(rc_diagnostic_buffer, rc_message_format);
            rc_fatal(1068);
        }
        text = rc_token_info[rc_token_kind].name;
        for (length = 0; text[length]; ++length) ;
        fwrite(text, length, 1, rc_output);
    }
}
