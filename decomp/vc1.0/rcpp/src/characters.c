/* Character-expression constants and escape decoding. */
#include "numbers.h"

/* Original [0100c1e0,0100c384). Plain bytes and escape results deliberately
 * differ in signedness; /J masks an escape result before making its constant. */
unsigned char rc_parse_character(void)
{
    unsigned char ch, kind = rc_options.unsigned_char ? TK_UINT : TK_INT;
    unsigned long value;
    for (;;) {
        ch = *rc_input_cursor++;
        switch (rc_character_class[ch]) {
        case LC_END:
            if (rc_resume_input() != 16) continue;
            value = rc_escape_value(rc_get_logical_character());
            if (kind == TK_UINT) value &= 255;
            break;
        case LC_NEWLINE:
            rc_message_format = rc_message(2001);
            rc_format(rc_diagnostic_buffer, rc_message_format);
            rc_error(2001);
            --rc_input_cursor;
            value = ch;
            break;
        case LC_CHARACTER:
            rc_message_format = rc_message(2137);
            rc_format(rc_diagnostic_buffer, rc_message_format);
            rc_error(2137);
            value = 0;
            --rc_input_cursor;
            break;
        case LC_ESCAPE:
            /* No external byte initially has this internal category. */
            break;
        default:
            value = ch;
            break;
        }
        break;
    }
    ch = rc_get_logical_character();
    if (ch != '\'') {
        rc_message_format = rc_message(2015);
        rc_format(rc_diagnostic_buffer, rc_message_format);
        rc_error(2015);
        while (ch != '\n' && ch != '\'') ch = rc_get_logical_character();
        if (ch == '\n') {
            rc_message_format = rc_message(2016);
            rc_format(rc_diagnostic_buffer, rc_message_format);
            rc_error(2016);
        }
    }
    rc_token_value = rc_make_constant(kind, &value);
    return kind;
}

/* Original [0100cc00,0100cd96), switch data [0100cd98,0100cdf8).
 * Octal and hexadecimal escapes consume at most three digits. Their result
 * is the sign-extended low byte; an unknown escape simply returns its letter. */
unsigned __stdcall rc_escape_value(int ch)
{
    int value, count;
    unsigned char next;
    for (;;) {
        if (rc_character_flags[(unsigned char)ch] & 2) {
            value = rc_digit_value(ch);
            count = 1;
            next = rc_get_logical_character();
            while (count < 3 && (rc_character_flags[next] & 2)) {
                value = value * 8 + rc_digit_value(next);
                ++count;
                next = rc_get_logical_character();
            }
            if (!rc_preprocessing_started && value > 255) {
                rc_message_format = rc_message(2022);
                rc_format(rc_diagnostic_buffer, rc_message_format, value);
                rc_error(2022);
            }
            --rc_input_cursor;
            return (signed char)value;
        }
        switch (ch) {
        case 'a': return 7;
        case 'b': return 8;
        case 'f': return 12;
        case 'n': return 10;
        case 'r': return 13;
        case 't': return 9;
        case 'v': return 11;
        case 'x':
            value = count = 0;
            next = rc_get_logical_character();
            while (count < 3 && (rc_character_flags[next] & 8)) {
                value = value * 16 + rc_digit_value(next);
                ++count;
                next = rc_get_logical_character();
            }
            if (!count) {
                rc_message_format = rc_message(2153);
                rc_format(rc_diagnostic_buffer, rc_message_format);
                rc_error(2153);
            }
            --rc_input_cursor;
            return (signed char)value;
        case '\\':
            if (!rc_splice_line()) return ch;
            ch = rc_get_logical_character();
            continue;
        default:
            return ch;
        }
    }
}
