/* String constants used in expressions, #line and include directives. */
#include "directives.h"
unsigned short rc_peek_wide_character(void);
/* Original [0100c390,0100c4b3). Include-name mode preserves backslashes;
 * ordinary constants translate escapes. The target warns once and discards
 * excess characters after the shared string buffer's effective limit. */
void rc_parse_string(void)
{
    char *out = rc_text_buffer;
    unsigned char ch;
    unsigned status;
    int warn = 1;
    rc_token_value = rc_text_buffer;
    for (;;) {
        ch = *rc_input_cursor++;
        switch (rc_character_class[ch]) {
        case LC_END:
            status = rc_resume_input();
            if (status != 16) continue;
            if (!rc_header_mode) ch = (unsigned char)rc_escape_value(rc_get_logical_character());
            break;
        case LC_NEWLINE:
            --rc_input_cursor;
            rc_message_format = rc_message(2001);
            rc_format(rc_diagnostic_buffer, rc_message_format);
            rc_error(2001);
            goto finished;
        case LC_STRING:
            goto finished;
        case LC_DBCS_LEAD:
            *out++ = ch;
            ch = rc_get_logical_character();
            break;
        }
        if (out - rc_text_buffer > 2043) {
            if (warn) {
                rc_message_format = rc_message(4009);
                rc_format(rc_diagnostic_buffer, rc_message_format);
                rc_warning(4009);
                warn = 0;
            }
        } else *out++ = ch;
    }
finished:
    *out++ = 0;
    rc_token_string_length = (unsigned short)(out - (char *)rc_token_value);
}

/* Original [0100bfe0,0100c1bf), wide dispatch [0100c1c0,0100c1e0).
 * Output preprocessing preserves quotes/escapes. Non-ASCII wide characters
 * become the original six-digit octal spelling. Buffer flushes are internal
 * to this function and do not introduce another native graph vertex. */
void __stdcall rc_copy_quoted_text(unsigned char quote)
{
    char *out = rc_macro_name + 1;
    unsigned char ch;
    unsigned short wide;
    rc_macro_name[0] = quote;
    if (rc_input_encoding == 1) {
        for (;;) {
            ch = *rc_input_cursor++;
            switch (rc_character_class[ch]) {
            case LC_CR:
                continue;
            case LC_END:
                if (ch != '\\') { rc_resume_input(); continue; }
                *out++ = ch;
                ch = rc_get_logical_character();
                break;
            case LC_ESCAPE: case LC_DBCS_LEAD:
                *out++ = ch;
                ch = rc_get_logical_character();
                break;
            case LC_NEWLINE:
                --rc_input_cursor;
                goto finish;
            case LC_CHARACTER: case LC_STRING:
                if (ch == quote) { *out++ = ch; goto finish; }
                break;
            }
            *out++ = ch;
            if (out == rc_macro_name + 255) {
                *out = 0;
                fwrite(rc_macro_name, out - rc_macro_name, 1, rc_output);
                out = rc_macro_name;
            }
        }
    }
    for (;;) {
        wide = rc_peek_wide_character();
        ch = *rc_input_cursor++;
        if (wide < 0x7f) {
            switch (rc_character_class[ch]) {
            case LC_CR:
                continue;
            case LC_END:
                rc_resume_input();
                continue;
            case LC_NEWLINE:
                --rc_input_cursor;
                goto finish;
            case LC_ESCAPE:
                *out++ = ch;
                break;
            case LC_CHARACTER: case LC_STRING:
                if (ch == quote) { *out++ = ch; goto finish; }
                break;
            }
            *out++ = ch;
        } else {
            *out++ = '\\';
            *out++ = (char)('0' + ((wide >> 15) & 7));
            *out++ = (char)('0' + ((wide >> 12) & 7));
            *out++ = (char)('0' + ((wide >> 9) & 7));
            *out++ = (char)('0' + ((wide >> 6) & 7));
            *out++ = (char)('0' + ((wide >> 3) & 7));
            *out++ = (char)('0' + (wide & 7));
        }
        if (out > rc_macro_name + 246) {
            *out = 0;
            fwrite(rc_macro_name, out - rc_macro_name, 1, rc_output);
            out = rc_macro_name;
        }
    }
finish:
    *out = 0;
    fwrite(rc_macro_name, out - rc_macro_name, 1, rc_output);
}

/* Original [01009c20,01009c42). The parallel word buffer shares the byte
 * scanner's logical offset, including a lookahead into the current character. */
unsigned short rc_peek_wide_character(void)
{
    RcInput *input = rc_inputs + rc_input_depth;
    return input->wide[rc_input_cursor - input->bytes];
}
