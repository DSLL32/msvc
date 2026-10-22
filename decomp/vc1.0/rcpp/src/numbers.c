/* Integer token spelling, suffixes and value classification. */
#include "numbers.h"
#include <ctype.h>
#include <recon.h>
int rc_collecting_macro_text;
char *rc_expansion_output = rc_text_buffer + 1024;
/* Original [0100c5e0,0100c8a6). Output/collection paths keep spellings; the
 * expression path converts a value and attaches its compiler token/type node. */
unsigned char __stdcall rc_parse_number(unsigned char first)
{
    unsigned char ch = first, kind = TK_INT;
    char *start = rc_collecting_macro_text ? rc_expansion_output : rc_macro_name;
    char *out = start;
    int radix;
    unsigned long value;
    if (ch == '0') {
        ch = rc_get_logical_character();
        if (toupper(ch) == 'X') {
            radix = 16;
            if (rc_preprocessing_started) { *out++ = '0'; *out++ = 'x'; }
            ch = rc_get_logical_character();
            while (rc_character_flags[ch] & 8) {
                *out++ = ch;
                ch = rc_get_logical_character();
            }
            if (out == rc_macro_name && !rc_collecting_macro_text) {
                rc_message_format = rc_message(2153);
                rc_format(rc_diagnostic_buffer, rc_message_format);
                rc_error(2153);
            }
            goto suffix;
        }
        radix = 8;
        *out++ = '0';
    } else radix = 10;
    while (rc_character_flags[ch] & 4) {
        *out++ = ch;
        ch = rc_get_logical_character();
    }
    if (ch == '.' || toupper(ch) == 'E') {
        --rc_input_cursor;
        return rc_read_floating(out);
    }
suffix:
    if (toupper(ch) == 'L') {
        if (rc_preprocessing_started) *out++ = ch;
        ch = rc_get_logical_character();
        if (toupper(ch) == 'U') {
            if (rc_preprocessing_started) *out++ = ch;
            kind = TK_ULONG;
        } else {
            kind = TK_LONG;
            --rc_input_cursor;
        }
    } else if (toupper(ch) == 'U') {
        if (rc_preprocessing_started) *out++ = ch;
        ch = rc_get_logical_character();
        if (toupper(ch) == 'L') {
            if (rc_preprocessing_started) *out++ = ch;
            kind = TK_ULONG;
        } else {
            kind = TK_UINT;
            --rc_input_cursor;
        }
    } else --rc_input_cursor;
    *out = 0;
    if (start == rc_expansion_output) {
        rc_expansion_output = out;
        return TK_NEWLINE;
    }
    if (rc_preprocessing_started) {
        fwrite(rc_macro_name, out - rc_macro_name, 1, rc_output);
        return TK_NEWLINE;
    }
    value = rc_integer_value(rc_macro_name, radix);
    switch (kind) {
    case TK_INT:
        kind = radix == 10 ? rc_integer_decimal_kind(value) : rc_integer_nondecimal_kind(value);
        break;
    case TK_LONG: kind = rc_long_kind(value); break;
    case TK_UINT: kind = rc_unsigned_kind(value); break;
    }
    rc_token_value = rc_make_constant(kind, &value);
    return kind;
}

/* Original [0100ca60,0100cb3b). Decimal overflow uses signed monotonicity;
 * other radices reject remaining digits when their next shift would overflow.
 * Arithmetic itself wraps at 32 bits, as in the original machine. */
unsigned long __stdcall rc_integer_value(const char *text, int radix)
{
    unsigned long value = 0, previous = 0;
    int digit;
    while (*text) {
        value *= radix;
        digit = rc_digit_value((unsigned char)*text);
        if (digit >= radix && !rc_preprocessing_started) {
            rc_message_format = rc_message(2020);
            rc_format(rc_diagnostic_buffer, rc_message_format, (int)(unsigned char)*text, radix);
            rc_error(2020);
        }
        value += digit;
        ++text;
        if (radix == 10) {
            if ((long)previous > (long)value) { --text; break; }
        } else if (*text) {
            if (radix == 8 ? (value & 0xe0000000UL) : (value & 0xf0000000UL)) break;
        }
        previous = value;
    }
    if (*text) {
        rc_message_format = rc_message(2177);
        rc_format(rc_diagnostic_buffer, rc_message_format);
        rc_error(2177);
        value = 0;
    }
    return value;
}

/* Original [0100cbc0,0100cbf7). The caller validates the digit's radix. */
int __stdcall rc_digit_value(int ch)
{
    if (rc_character_flags[(unsigned char)ch] & 4) return ch - '0';
    return toupper(ch) - toupper('A') + 10;
}

/* The four original classifiers retain separate function graph vertices.
 * Their nominal promotion tests cannot succeed on this 32-bit target: the
 * first/third compare a signed value with LONG_MAX, the second compares its
 * wrapped signed absolute value with LONG_MAX, and the fourth compares that
 * absolute value unsigned with ULONG_MAX-1. Preserve their actual outcomes. */
/* [0100cb40,0100cb51) */
unsigned char __stdcall rc_integer_nondecimal_kind(unsigned long value)
{
    return TK_INT;
}
/* [0100cb60,0100cb77) */
unsigned char __stdcall rc_integer_decimal_kind(unsigned long value)
{
    return TK_INT;
}
/* [0100cb80,0100cb91) */
unsigned char __stdcall rc_long_kind(unsigned long value)
{
    return TK_LONG;
}
/* [0100cba0,0100cbb5) */
unsigned char __stdcall rc_unsigned_kind(unsigned long value)
{
    return TK_UINT;
}

/* Original [0100c8b0,0100ca5d). This resource-preprocessor scanner preserves
 * floating spelling and reports its token kind; it does not convert to an
 * in-memory floating value. The caller has already stored any leading digits. */
unsigned char __stdcall rc_read_floating(char *out)
{
    unsigned char ch = rc_get_logical_character(), kind;
    if (rc_options.xc && !rc_collecting_macro_text) {
        rc_message_format = rc_message(4012);
        rc_format(rc_diagnostic_buffer, rc_message_format);
        rc_warning(4012);
        rc_options.xc = 0;
    }
    if ((rc_character_flags[ch] & 4) || ch == '.') {
        do {
            *out++ = ch;
            ch = rc_get_logical_character();
        } while (rc_character_flags[ch] & 4);
    }
    if (toupper(ch) == 'E') {
        *out++ = ch;
        ch = rc_get_logical_character();
        if (ch == '+' || ch == '-') {
            *out++ = ch;
            ch = rc_get_logical_character();
        }
        if (rc_character_flags[ch] & 4) {
            do {
                *out++ = ch;
                ch = rc_get_logical_character();
            } while (rc_character_flags[ch] & 4);
        } else if (!rc_options.R) {
            if (!rc_collecting_macro_text) {
                rc_message_format = rc_message(2021);
                rc_format(rc_diagnostic_buffer, rc_message_format, (int)ch);
                rc_error(2021);
            }
            *out++ = '0';
        }
    }
    if (toupper(ch) == 'F') {
        kind = TK_FLOAT;
        if (rc_preprocessing_started) *out++ = ch;
    } else if (toupper(ch) == 'L') {
        kind = TK_LONG_DOUBLE;
        if (rc_preprocessing_started) *out++ = ch;
    } else {
        --rc_input_cursor;
        kind = TK_DOUBLE;
    }
    *out = 0;
    if (rc_collecting_macro_text > 0) {
        rc_expansion_output = out;
        return TK_NEWLINE;
    }
    if (rc_preprocessing_started) {
        fwrite(rc_macro_name, out - rc_macro_name, 1, rc_output);
        return TK_NEWLINE;
    }
    return kind;
}
