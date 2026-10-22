/* Directive dispatcher, original [0100d4f0,0100dc6b), switch data
 * [0100dc6c,0100dca4). Every directive arm remains in this one function. */
#include "numbers.h"
#include <string.h>
int rc_in_directive, rc_in_condition_expression, rc_header_mode;
int rc_condition_state[32];

void rc_parse_directive(void)
{
    unsigned char ch, directive;
    const char *word;
    char *text, *a, *b;
    int saved_output, condition, skipped, level;
    if (rc_expanding_macro) return;
    ch = rc_read_nonspace();
    if (rc_character_class[ch] == LC_NEWLINE) {
        --rc_input_cursor;
        return;
    }
    if (rc_character_class[ch] != LC_IDENTIFIER) {
        rc_message_format = rc_message(2019);
        rc_format(rc_diagnostic_buffer, rc_message_format, (int)ch);
        rc_error(2019);
        rc_skip_line();
        return;
    }
    rc_read_identifier(ch);
    word = rc_macro_name;
    rc_in_directive = 1;
redispatch:
    directive = rc_directive_code(word);
    switch (directive) {
    case PP_IF:
        saved_output = rc_preprocessing_started;
        rc_preprocessing_started = 0;
        rc_in_condition_expression = 1;
        condition = rc_evaluate_condition();
        rc_in_condition_expression = 0;
        rc_preprocessing_started = saved_output;
        rc_expect_end("#if/#elif");
        if (!condition) {
            skipped = rc_skip_inactive(5);
            if (skipped != 3) {
                if (rc_options.preprocess_lines) rc_emit_line();
                if (skipped == 2) {
                    word = "if";
                    goto redispatch;
                }
                break;
            }
        }
        level = rc_push_condition();
        rc_condition_state[level] = 1;
        if (rc_options.preprocess_lines && !condition) rc_emit_line();
        break;
    case PP_ELIF:
        level = rc_conditional_depth--;
        if (level < 0) {
            rc_message_format = rc_message(1018);
            rc_format(rc_diagnostic_buffer, rc_message_format);
            rc_fatal(1018);
        }
        do skipped = rc_skip_inactive(5); while (skipped != 4);
        if (rc_options.preprocess_lines) rc_emit_line();
        break;
    case PP_ELSE:
        if (rc_conditional_depth >= 0) {
            level = rc_conditional_depth--;
            condition = rc_condition_state[level] == 1;
        } else condition = 0;
        if (!condition) {
            rc_message_format = rc_message(1019);
            rc_format(rc_diagnostic_buffer, rc_message_format);
            rc_fatal(1019);
        }
        rc_expect_end("#else");
        rc_skip_inactive(6);
        if (rc_options.preprocess_lines) rc_emit_line();
        break;
    case PP_ENDIF:
        level = rc_conditional_depth--;
        if (level < 0) {
            rc_message_format = rc_message(1020);
            rc_format(rc_diagnostic_buffer, rc_message_format);
            rc_fatal(1020);
        }
        if (rc_options.preprocess_lines) rc_emit_line();
        rc_expect_end("#endif");
        break;
    case PP_IFDEF:
    case PP_IFNDEF:
        ch = rc_read_nonspace();
        if (rc_character_class[ch] != LC_IDENTIFIER) {
            rc_message_format = rc_message(1016);
            rc_format(rc_diagnostic_buffer, rc_message_format);
            rc_fatal(1016);
        }
        rc_read_identifier(ch);
        condition = rc_find_macro() != NULL;
        rc_expect_end(directive == PP_IFDEF ? "#ifdef" : "#ifndef");
        if (directive == PP_IFNDEF) condition = !condition;
        if (!condition) {
            skipped = rc_skip_inactive(5);
            if (skipped != 3) {
                if (skipped == 2) {
                    word = "if";
                    goto redispatch;
                }
                break;
            }
        }
        level = rc_push_condition();
        rc_condition_state[level] = 1;
        break;
    case PP_DEFINE:
        rc_parse_define();
        break;
    case PP_INCLUDE:
        saved_output = rc_preprocessing_started;
        rc_preprocessing_started = 0;
        rc_header_mode = 1;
        rc_next_token();
        rc_header_mode = 0;
        rc_preprocessing_started = saved_output;
        if (rc_token_kind == TK_LESS) {
            if (!rc_parse_angle_name()) {
                rc_skip_line();
                rc_in_directive = 0;
                return;
            }
            rc_token_value = rc_macro_name;
        } else if (rc_token_kind != TK_STRING) {
            rc_message_format = rc_message(2006);
            rc_format(rc_diagnostic_buffer, rc_message_format, rc_token_info[rc_token_kind].name);
            rc_error(2006);
            rc_skip_line();
            rc_in_directive = 0;
            return;
        }
        rc_expect_end("#include");
        text = rc_token_value;
        if (strchr(rc_options.path_characters, (signed char)text[0]) ||
            (strchr(rc_options.path_characters, ':') && text[1] == ':')) {
            if (!rc_open_include(text, 0)) {
                rc_message_format = rc_message(1015);
                rc_format(rc_diagnostic_buffer, rc_message_format, text);
                rc_fatal(1015);
            }
        } else if (rc_token_kind != TK_STRING || !rc_search_current_directory()) {
            rc_search_include_directories();
        }
        break;
    case PP_PRAGMA:
        rc_parse_pragma();
        break;
    case PP_UNDEF:
        ch = rc_read_nonspace();
        if (rc_character_class[ch] == LC_IDENTIFIER) {
            rc_read_identifier(ch);
            rc_remove_macro();
        } else {
            rc_message_format = rc_message(4006);
            rc_format(rc_diagnostic_buffer, rc_message_format);
            rc_warning(4006);
        }
        rc_expect_end("#undef");
        break;
    case PP_LINE:
        saved_output = rc_preprocessing_started;
        rc_preprocessing_started = 0;
        rc_next_token();
        if (rc_token_kind != TK_INT) {
            rc_message_format = rc_message(2005);
            rc_format(rc_diagnostic_buffer, rc_message_format, rc_token_info[rc_token_kind].name);
            rc_error(2005);
            rc_preprocessing_started = saved_output;
            rc_skip_line();
            rc_in_directive = 0;
            return;
        }
        rc_line_number = ((RcConstant *)rc_token_value)->value.integer - 1;
        rc_next_token();
        rc_preprocessing_started = saved_output;
        if (rc_token_kind == TK_STRING) {
            a = rc_source_name;
            b = rc_token_value;
            while (*a == *b && *a) { ++a; ++b; }
            if (*a != *b) strncpy(rc_source_name, rc_token_value, 257);
        } else if (rc_token_kind != TK_NEWLINE) {
            rc_message_format = rc_message(2130);
            rc_format(rc_diagnostic_buffer, rc_message_format, rc_token_info[rc_token_kind].name);
            rc_error(2130);
            rc_skip_line();
            rc_in_directive = 0;
            return;
        }
        rc_emit_line();
        rc_expect_end("#line");
        break;
    case PP_UNKNOWN:
        rc_message_format = rc_message(1021);
        rc_format(rc_diagnostic_buffer, rc_message_format, word);
        rc_fatal(1021);
        break;
    case PP_ERROR:
        text = rc_macro_name;
        for (;;) {
            ch = rc_get_logical_character();
            if (ch == 3) break;
            if (ch == '\n') { --rc_input_cursor; break; }
            *text++ = ch;
        }
        *text = 0;
        rc_message_format = rc_message(2188);
        rc_format(rc_diagnostic_buffer, rc_message_format, rc_macro_name);
        rc_error(2188);
        rc_expect_end("#error");
        break;
    case PP_IDENT:
        saved_output = rc_preprocessing_started;
        rc_preprocessing_started = 0;
        rc_next_token();
        rc_preprocessing_started = saved_output;
        if (rc_token_kind != TK_STRING) {
            rc_message_format = rc_message(4079);
            rc_format(rc_diagnostic_buffer, rc_message_format, rc_token_info[rc_token_kind].name);
            rc_warning(4079);
        }
        /* The original diagnostic label is #error here, even for #ident. */
        rc_expect_end("#error");
        break;
    }
    rc_in_directive = 0;
}

/* Original [0100de50,0100de9e). Leave the terminating newline for the line
 * scanner; unexpected suffix text warns and is consumed to the same boundary. */
void __stdcall rc_expect_end(const char *directive)
{
    if (rc_read_nonspace() == '\n') --rc_input_cursor;
    else {
        rc_message_format = rc_message(4067);
        rc_format(rc_diagnostic_buffer, rc_message_format, directive);
        rc_warning(4067);
        rc_skip_line();
    }
}

/* Original [0100d3a0,0100d3e2): the conditional stack has 32 entries. */
int rc_push_condition(void)
{
    ++rc_conditional_depth;
    if (rc_conditional_depth >= 32) {
        rc_message_format = rc_message(1052);
        rc_format(rc_diagnostic_buffer, rc_message_format);
        rc_fatal(1052);
    }
    return rc_conditional_depth;
}

unsigned char rc_find_inactive_directive(void);
void __stdcall rc_skip_inactive_quote(unsigned char);

/* Original [0100dcb0,0100dd55), six-entry switch [0100dd58,0100dd70). */
int __stdcall rc_skip_inactive(int mode)
{
    int nesting = 0;
    unsigned char directive;
    for (;;) {
        directive = rc_find_inactive_directive();
        switch (directive) {
        case PP_IF: case PP_IFDEF: case PP_IFNDEF:
            ++nesting;
            break;
        case PP_ELIF: case PP_ELSE:
            if (nesting) break;
            if (mode == 6) {
                rc_message_format = rc_message(1022);
                rc_format(rc_diagnostic_buffer, rc_message_format);
                rc_fatal(1022);
                break;
            }
            if (directive == PP_ELSE) {
                rc_expect_end("#else");
                return 3;
            }
            return 2;
        case PP_ENDIF:
            if (nesting) { --nesting; break; }
            rc_expect_end("#endif");
            return 4;
        }
    }
}

/* Original [0100df40,0100dff0). Directive names are only recognized after
 * a physical/logical newline. Quotes protect hashes in skipped text. */
unsigned char rc_find_inactive_directive(void)
{
    unsigned char ch;
    for (;;) {
        ch = rc_read_nonspace();
    inspect:
        switch (rc_character_class[ch]) {
        case LC_NEWLINE:
            ++rc_line_number;
            if (rc_preprocessing_started) fputc('\n', rc_output);
            ch = rc_read_nonspace();
            if (ch != '#') goto inspect;
            ch = rc_read_nonspace();
            if (rc_character_class[ch] != LC_IDENTIFIER) goto inspect;
            rc_read_identifier(ch);
            return rc_directive_code(rc_macro_name);
        case LC_END:
            if (rc_resume_input() == 16) ++rc_input_cursor;
            break;
        case LC_CHARACTER: case LC_STRING:
            rc_skip_inactive_quote(ch);
            break;
        }
    }
}

/* Original [0100dea0,0100df38). Skipped quotes retain the old permissive
 * escape handling; reaching newline reports a warning and leaves it unread. */
void __stdcall rc_skip_inactive_quote(unsigned char quote)
{
    unsigned char ch;
    for (;;) {
        ch = *rc_input_cursor++;
        switch (rc_character_class[ch]) {
        case LC_END:
            if (rc_resume_input() == 16) ++rc_input_cursor;
            break;
        case LC_NEWLINE:
            rc_message_format = rc_message(4093);
            rc_format(rc_diagnostic_buffer, rc_message_format);
            rc_warning(4093);
            --rc_input_cursor;
            return;
        case LC_CHARACTER: case LC_STRING:
            if (ch == quote) return;
            break;
        case LC_DBCS_LEAD:
            rc_get_logical_character();
            break;
        }
    }
}

/* Original [0100e380,0100e3cc). Pragmas pass through preprocessing output
 * verbatim after logical-character splicing; newline stays with the caller. */
void rc_parse_pragma(void)
{
    unsigned char ch;
    if (!rc_preprocessing_started) return;
    fwrite("#pragma", 7, 1, rc_output);
    while ((ch = rc_get_logical_character()) != '\n') fputc(ch, rc_output);
    --rc_input_cursor;
}
