/* Function-macro actual arguments, original [0100b170,0100b4c0),
 * switch table [0100b4c0,0100b564). */
#include "macros.h"
#include <string.h>
void __stdcall rc_select_macro_argument(unsigned char);
int rc_argument_is_pasted(void);
void __stdcall rc_append_argument(const char *);

void __stdcall rc_read_macro_arguments(RcMacro *macro, int expected)
{
    int nesting = 0, mode;
    unsigned status;
    unsigned char ch;
    char *argument, *saved_end, *saved_work, *original_work = rc_expansion_work;
    *rc_expansion_output++ = rc_input_cursor[-1];
    for (;;) {
        if (rc_expansion_output >= rc_text_buffer + 2043) rc_macro_fatal(10056);
        for (;;) {
            if (!rc_skip_macro_whitespace()) return;
            if (rc_character_class[*rc_input_cursor] != LC_SLASH) {
                argument = rc_expansion_output;
                break;
            }
            ++rc_input_cursor;
            if (rc_skip_comment()) continue;
            argument = rc_expansion_output;
            *rc_expansion_output++ = '/';
            break;
        }
        mode = 0;
        for (;;) {
            ch = *rc_input_cursor++;
            switch (rc_character_class[ch]) {
            case LC_SPACE: case LC_CR: case LC_NEWLINE:
                --rc_input_cursor;
                if (!rc_skip_macro_whitespace()) return;
                continue;
            case LC_SLASH:
                if (rc_skip_comment()) {
                    if (rc_expansion_output[-1] == ' ') continue;
                    ch = ' ';
                }
                break;
            case LC_END:
                saved_end = rc_expansion_output;
                saved_work = rc_expansion_work;
                status = rc_resume_input();
                if (status & 10) return;
                rc_expansion_work = saved_work;
                rc_expansion_output = saved_end;
                if (status != 16) continue;
                *rc_expansion_output++ = ch;
                ch = rc_get_logical_character();
                break;
            case LC_CHARACTER: case LC_STRING:
                rc_expansion_output = rc_copy_macro_quote(rc_expansion_output, ch);
                continue;
            case LC_COMMA:
                if (!nesting) goto argument_end;
                break;
            case LC_RPAREN:
                if (--nesting < 0) goto argument_end;
                break;
            case LC_LPAREN:
                ++nesting;
                break;
            case LC_INTERNAL_2:
                rc_stringize_argument('"', rc_macro_argument());
                continue;
            case LC_INTERNAL_3:
                rc_stringize_argument('\'', rc_macro_argument());
                continue;
            case LC_IDENTIFIER:
                *rc_expansion_output++ = ch;
                ch = *rc_input_cursor++;
                while (rc_character_flags[ch] & 16) {
                    if (rc_expansion_output >= rc_text_buffer + 2043) rc_macro_fatal(10056);
                    *rc_expansion_output++ = ch;
                    ch = *rc_input_cursor++;
                }
                if (rc_character_class[ch] != LC_INTERNAL_1) {
                    --rc_input_cursor;
                    continue;
                }
                mode = 1;
                /* The suffix parameter continues the same pasted spelling. */
            case LC_INTERNAL_1:
                rc_append_argument(rc_substituted_argument(&mode));
                continue;
            }
            *rc_expansion_output++ = ch;
        }
    argument_end:
        if (rc_character_class[(unsigned char)rc_expansion_output[-1]] == LC_SPACE)
            --rc_expansion_output;
        if (argument < rc_expansion_output) {
            ++rc_argument_count;
            rc_compact_macro_output(argument, rc_expansion_output);
        }
        *rc_expansion_output++ = ch;
        if (nesting >= 0) continue;
        rc_argument_work = original_work;
        if ((int)rc_argument_count > expected) {
            rc_message_format = rc_message(4002);
            rc_format(rc_diagnostic_buffer, rc_message_format, rc_macro_name);
            rc_warning(4002);
        } else if ((int)rc_argument_count < expected) {
            rc_message_format = rc_message(4003);
            rc_format(rc_diagnostic_buffer, rc_message_format, rc_macro_name);
            rc_warning(4003);
        }
        if (macro->body) { rc_push_macro(macro); rc_start_macro_argument(); }
        else {
            rc_expansion_work = rc_argument_work;
            rc_expansion_output = rc_macro_output_start;
        }
        return;
    }
}

/* Original [0100b600,0100b670). Arguments occupy aligned records with a
 * signed 16-bit size, text, and internal NUL/P/A transition markers. */
void __stdcall rc_compact_macro_output(char *start, char *end)
{
    unsigned length = end - start;
    char *after;
    if (rc_expansion_work + length >= rc_text_buffer + 1022) rc_macro_fatal(10056);
    strncpy(rc_expansion_work + 2, start, length);
    after = rc_expansion_work + length + 2;
    *after = 0;
    if ((unsigned)after & 1) { ++after; *after = 'P'; }
    *++after = 'A';
    ++after;
    *(short *)rc_expansion_work = (short)(after - rc_expansion_work);
    rc_expansion_work = after;
}

/* Original [0100b880,0100b8ab). */
void __stdcall rc_select_macro_argument(unsigned char index)
{
    char *record = rc_macro_frames[rc_expanding_macro].argument_work;
    while (--index) record += *(short *)record;
    rc_input_cursor = (unsigned char *)record + 2;
}

/* Original [0100b8b0,0100b8dd). Its final edge is a tail call in the target. */
void rc_start_macro_argument(void)
{
    RcMacroFrame *frame = rc_macro_frames + rc_expanding_macro;
    if (frame->argument_count > frame->argument_index) {
        ++frame->argument_index;
        rc_select_macro_argument(frame->argument_index);
    } else rc_start_macro_body();
}

/* Original [0100bbf0,0100bc2f): select the unexpanded actual argument. */
char *rc_macro_argument(void)
{
    RcMacroFrame *frame = rc_macro_frames + rc_expanding_macro;
    unsigned char index = *rc_input_cursor++;
    char *record = frame->argument_work;
    if (index > frame->argument_count) return NULL;
    if (index) while (--index) record += *(short *)record;
    return record + 2;
}

/* Original [0100b8e0,0100b91c). A following parameter or identifier
 * continuation prevents argument pre-expansion during token pasting. */
int rc_argument_is_pasted(void)
{
    unsigned char ch = *rc_input_cursor;
    return rc_character_class[ch] == LC_INTERNAL_1 || (rc_character_flags[ch] & 16) != 0;
}

/* Original [0100b920,0100b991): choose raw or pre-expanded argument text
 * according to the neighboring pieces of a pasted identifier. */
char *__stdcall rc_substituted_argument(int *mode)
{
    RcMacroFrame *frame = rc_macro_frames + rc_expanding_macro;
    unsigned char index = *rc_input_cursor++;
    char *record = frame->argument_work;
    int pasted;
    if (index > frame->argument_count) return NULL;
    pasted = rc_argument_is_pasted();
    if (!*mode && !pasted)
        index += rc_macro_frames[rc_expanding_macro].argument_count;
    *mode = pasted;
    if (index) while (--index) record += *(short *)record;
    return record + 2;
}

/* Original [0100b780,0100b7c0). */
void __stdcall rc_append_argument(const char *argument)
{
    if (argument) while (*argument) {
        if (rc_expansion_output >= rc_text_buffer + 2043) rc_macro_fatal(10056);
        *rc_expansion_output++ = *argument++;
    }
}

/* Original [0100bce0,0100bd12): report expansion errors at its opening line. */
void __stdcall rc_macro_fatal(unsigned code)
{
    rc_line_number = rc_expansion_line;
    rc_message_format = rc_message(code);
    rc_format(rc_diagnostic_buffer, rc_message_format);
    rc_fatal(code);
}

/* Original [0100b670,0100b777). Stringizing quotes nested string literals
 * and their escapes; characterizing retains the target's distinct handling
 * of apostrophes and the same byte/DBCS copy behavior. */
void __stdcall rc_stringize_argument(unsigned char quote, const char *argument)
{
    unsigned char ch, category, quoted_category;
    int in_quote = 0;
    if (!argument) return;
    *rc_expansion_output++ = quote;
    for (;;) {
        if (rc_expansion_output >= rc_text_buffer + 2043) rc_macro_fatal(10056);
        ch = (unsigned char)*argument;
        category = rc_character_class[ch];
        switch (category) {
        case LC_END:
            if (!ch) { *rc_expansion_output++ = quote; return; }
            if (in_quote) *rc_expansion_output++ = '\\';
            break;
        case LC_STRING:
            if (rc_character_class[quote] == LC_STRING) *rc_expansion_output++ = '\\';
            /* String and character literals share quotation state. */
        case LC_CHARACTER:
            if (rc_character_class[quote] == LC_CHARACTER) break;
            if (!in_quote) { in_quote = 1; quoted_category = category; }
            else if (quoted_category == category) in_quote = 0;
            break;
        case LC_DBCS_LEAD:
            *rc_expansion_output++ = ch;
            ++argument;
            break;
        }
        *rc_expansion_output++ = *argument++;
    }
}
