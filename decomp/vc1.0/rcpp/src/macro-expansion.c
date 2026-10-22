/* Macro rescanning, original [0100ad30,0100b0be), switch data
 * [0100b0c0,0100b164). Definitions keep their internal D/A/R markers. */
#include "macros.h"
#include <recon.h>
RcMacroStorage rc_macro_storage;
char *rc_expansion_work = rc_text_buffer;
char *rc_macro_output_start, *rc_argument_work;
unsigned rc_argument_count;
int rc_expansion_line;

int __stdcall rc_expand_macro(RcMacro *macro)
{
    int expanded = 0, arguments, saved_output;
    unsigned i, status;
    unsigned char ch, look;
    char *saved_end;
    rc_collecting_macro_text = 0;
    rc_macro_output_start = rc_expansion_output;
    rc_expansion_line = rc_line_number;
process_macro:
    rc_argument_work = rc_expansion_work;
    rc_argument_count = 0;
    arguments = macro->arguments;
    if (arguments < -2) {
        rc_push_macro(macro);
        ++rc_macro_frames[rc_expanding_macro].macro->flags;
        if (rc_finish_macro_body()) return 1;
    } else if (!arguments) {
        expanded = 1;
        if (macro->body) { rc_push_macro(macro); rc_start_macro_body(); }
        else rc_expansion_output = rc_macro_output_start;
    } else {
        if (arguments == -1) arguments = 0;
        for (;;) {
            if (!rc_skip_macro_whitespace()) goto rescan;
            if (rc_character_class[*rc_input_cursor] != LC_SLASH) break;
            ++rc_input_cursor;
            if (rc_skip_comment()) continue;
            --rc_input_cursor;
            break;
        }
        if (rc_character_class[*rc_input_cursor] == LC_LPAREN) {
            ++rc_input_cursor;
            expanded = 1;
            rc_read_macro_arguments(macro, arguments);
        } else {
            saved_end = rc_expansion_output;
            rc_push_macro(macro);
            ++rc_macro_frames[rc_expanding_macro].macro->flags;
            rc_expansion_output = saved_end;
            if (rc_finish_macro_body()) return 0;
        }
    }
rescan:
    while (rc_collecting_macro_text) {
        if (rc_expansion_output >= rc_text_buffer + 2043) rc_macro_fatal(10056);
        ch = *rc_input_cursor++;
        switch (rc_character_class[ch]) {
        case LC_SPACE:
            while (rc_character_flags[*rc_input_cursor++] & 64) ;
            --rc_input_cursor;
            *rc_expansion_output++ = ' ';
            break;
        case LC_END:
            status = rc_resume_input();
            if (status == 16) {
                *rc_expansion_output++ = ch;
                *rc_expansion_output++ = *rc_input_cursor++;
            }
            break;
        case LC_CHARACTER: case LC_STRING:
            rc_expansion_output = rc_copy_macro_quote(rc_expansion_output, ch);
            break;
        case LC_DOT:
            *rc_expansion_output++ = '.';
            for (;;) {
                look = *rc_input_cursor++;
                if (rc_character_class[look] == LC_END) {
                    if (rc_resume_input() == 16) { --rc_input_cursor; break; }
                    if (rc_collecting_macro_text > 0) continue;
                    break;
                }
                if (rc_character_class[look] == LC_DOT) {
                    *rc_expansion_output++ = '.';
                    if (!rc_match_character('.')) { --rc_input_cursor; break; }
                    *rc_expansion_output++ = '.';
                } else if (rc_character_class[look] == LC_DIGIT) {
                    *rc_expansion_output++ = look;
                    rc_read_floating(rc_expansion_output);
                } else --rc_input_cursor;
                break;
            }
            break;
        case LC_DIGIT:
            saved_output = rc_preprocessing_started;
            rc_preprocessing_started = 1;
            rc_parse_number(ch);
            rc_preprocessing_started = saved_output;
            break;
        case LC_INTERNAL_1: case LC_IDENTIFIER:
            rc_macro_output_start = rc_expansion_output;
            if (rc_collect_macro_identifier(ch)) {
                macro = rc_find_macro();
                if (macro) {
                    if (!macro->flags) goto process_macro;
                    *rc_expansion_output++ = 4;
                    *rc_expansion_output++ = (unsigned char)(rc_macro_name_length - 1);
                }
            }
            if (rc_in_condition_expression) {
                for (i = 0; i < 8 && rc_macro_name[i] == "defined"[i]; ++i) ;
                if (i == 8) rc_identifier_value(rc_macro_name);
            }
            break;
        case LC_INTERNAL_2:
            rc_stringize_argument('"', rc_macro_argument());
            break;
        case LC_INTERNAL_3:
            rc_stringize_argument('\'', rc_macro_argument());
            break;
        default:
            *rc_expansion_output++ = ch;
            break;
        }
    }
    return expanded;
}

/* Original [0100b7c0,0100b85e). Frames are one-based; frame zero supplies
 * the target's unused leading record and the original limit is retained. */
void __stdcall rc_push_macro(RcMacro *macro)
{
    RcMacroFrame *frame;
    if (++rc_expanding_macro > 64) {
        rc_message_format = rc_message(1009);
        rc_format(rc_diagnostic_buffer, rc_message_format, rc_macro_name);
        rc_fatal(1009);
    }
    ++rc_collecting_macro_text;
    frame = rc_macro_frames + rc_expanding_macro;
    frame->macro = macro;
    frame->argument_work = rc_argument_work;
    frame->argument_count = (unsigned char)rc_argument_count;
    frame->argument_index = 0;
    frame->saved_cursor = rc_input_cursor;
    rc_expansion_output = rc_macro_output_start;
    frame->output_base = rc_expansion_output;
}

/* Original [0100b860,0100b87c). */
void rc_start_macro_body(void)
{
    RcMacro *macro = rc_macro_frames[rc_expanding_macro].macro;
    rc_input_cursor = (unsigned char *)macro->body;
    ++macro->flags;
}

/* Original [0100b570,0100b5f7). Outermost expansion is rescanned through
 * its encoded R terminator; nested collection restores its parent's state. */
int rc_finish_macro_body(void)
{
    RcMacroFrame *frame;
    if (!--rc_collecting_macro_text) {
        if (rc_expansion_output >= rc_text_buffer + 2043) rc_macro_fatal(10056);
        *rc_expansion_output++ = 0;
        *rc_expansion_output++ = 'R';
        rc_input_cursor = (unsigned char *)rc_macro_frames[rc_expanding_macro].output_base;
        return 1;
    }
    frame = rc_macro_frames + rc_expanding_macro;
    rc_input_cursor = frame->saved_cursor;
    rc_expansion_work = frame->argument_work;
    --frame->macro->flags;
    --rc_expanding_macro;
    return 0;
}

/* Original [0100bd20,0100be40). Decode refill/splice and macro transition
 * markers. Status bits let callers distinguish an argument, restored parent,
 * completed expansion, and an ordinary backslash. */
unsigned rc_resume_input(void)
{
    RcMacroFrame *frame;
    unsigned marker;
    if (rc_input_cursor[-1] == '\\') return rc_splice_line() ? 1 : 16;
    if (!rc_expanding_macro) {
        if (rc_refill_input()) {
            rc_message_format = rc_message(1004);
            rc_format(rc_diagnostic_buffer, rc_message_format);
            rc_fatal(1004);
        }
        return 1;
    }
    for (;;) {
        marker = *rc_input_cursor++;
        frame = rc_macro_frames + rc_expanding_macro;
        switch (marker) {
        case 'P':
            continue;
        case 'A':
            rc_compact_macro_output(frame->output_base, rc_expansion_output);
            rc_expansion_output = rc_macro_frames[rc_expanding_macro].output_base;
            rc_start_macro_argument();
            return 2;
        case 'D':
            return rc_finish_macro_body() ? 8 : 4;
        case 'R':
            rc_input_cursor = frame->saved_cursor;
            --frame->macro->flags;
            if (rc_expanding_macro == 1) {
                rc_expansion_work = rc_text_buffer;
                rc_expansion_output = rc_text_buffer + 1024;
            }
            --rc_expanding_macro;
            return 4;
        default:
            return marker;
        }
    }
}
