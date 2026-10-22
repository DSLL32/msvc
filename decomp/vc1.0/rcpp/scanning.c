/* Low-level preprocessing characters and identifier transport. */
#include "numbers.h"
/* Original [0100d020,0100d098). Consume horizontal whitespace, comments and
 * continued lines, while leaving the first significant character consumed. */
unsigned char rc_read_nonspace(void)
{
    unsigned char ch;
    unsigned category;
    for (;;) {
        ch = *rc_input_cursor++;
        if (ch == '/') {
            if (rc_skip_comment()) continue;
            return '/';
        }
        if (ch <= ' ') {
            category = rc_character_class[ch];
            if (category <= LC_CR) continue;
            if (category == LC_END) { rc_resume_input(); continue; }
            return ch;
        }
        if (ch == '\\' && rc_splice_line()) continue;
        return ch;
    }
}

/* Original [0100be70,0100bfd9). Identifier hashes and stored lengths are
 * eight-bit quantities even though the shared spelling buffer holds 512 bytes.
 * In expression/compilation mode names beyond 31 bytes are truncated with the
 * original warning. Output mode retains the spelling up to the hard limit. */
unsigned __stdcall rc_read_identifier(unsigned char first)
{
    char *out = rc_macro_name + 1;
    unsigned hash = first & 0x5f;
    unsigned length;
    rc_macro_name[0] = first;
    for (;;) {
        *out = *rc_input_cursor++;
        while (rc_character_flags[(unsigned char)*out] & 16) {
            hash += (unsigned char)*out & 0x5f;
            ++out;
            *out = *rc_input_cursor++;
        }
        if (!*out) {
            if (rc_refill_input()) {
                rc_message_format = rc_message(1004);
                rc_format(rc_diagnostic_buffer, rc_message_format);
                rc_fatal(1004);
            }
            continue;
        }
        if (*out == '\\' && rc_splice_line()) continue;
        break;
    }
    --rc_input_cursor;
    if (out >= rc_macro_name + sizeof(rc_macro_name)) {
        rc_message_format = rc_message(1067);
        rc_format(rc_diagnostic_buffer, rc_message_format);
        rc_fatal(1067);
    }
    if (out - rc_macro_name > 31 && !rc_preprocessing_started) {
        rc_macro_name[31] = 0;
        out = rc_macro_name + 32;
        hash = rc_hash_name(rc_macro_name);
        rc_message_format = rc_message(4011);
        rc_format(rc_diagnostic_buffer, rc_message_format, rc_macro_name);
        rc_warning(4011);
    } else {
        *out++ = 0;
    }
    rc_macro_hash = (unsigned char)hash;
    length = (unsigned char)(out - rc_macro_name);
    rc_macro_name_length = length;
    return length;
}

/* Original [0100d120,0100d16e). A NUL is returned only while collecting an
 * internal macro spelling; otherwise it asks the input machinery to resume. */
unsigned char rc_get_logical_character(void)
{
    unsigned char ch;
    for (;;) {
        ch = *rc_input_cursor++;
        if (ch == '\r') continue;
        if (!ch && rc_collecting_macro_text <= 0) { rc_resume_input(); continue; }
        if (ch == '\\' && rc_splice_line()) continue;
        return ch;
    }
}
