/* Identifier assembly during replacement collection and its final rescan. */
#include "macros.h"
#include <recon.h>

/* Original [0100b9a0,0100bbe9). Parameter text can supply only a fragment
 * of a pasted identifier. Keep nonidentifiers in output without looking them
 * up as macros; an internal 4 marker suppresses a recursive name. */
int __stdcall rc_collect_macro_identifier(unsigned char ch)
{
    char *start = rc_expansion_output, *argument;
    int mode = 0, valid = 1;
    unsigned suppressed = 0, i, length;
    for (;;) {
        if (rc_character_class[ch] == LC_INTERNAL_1) {
            argument = rc_substituted_argument(&mode);
            if (argument) while (*argument) {
                if (!(rc_character_flags[(unsigned char)*argument] & 16)) valid = 0;
                if (rc_expansion_output >= rc_text_buffer + 2043) rc_macro_fatal(10056);
                *rc_expansion_output++ = *argument++;
            }
        } else *rc_expansion_output++ = ch;
    scan_fragment:
        ch = *rc_input_cursor++;
        while (rc_character_flags[ch] & 16) {
            if (rc_expansion_output >= rc_text_buffer + 2043) rc_macro_fatal(10056);
            *rc_expansion_output++ = ch;
            ch = *rc_input_cursor++;
        }
        if (rc_character_class[ch] == LC_INTERNAL_4) {
            suppressed = *rc_input_cursor++;
            goto scan_fragment;
        }
        if (rc_character_class[ch] != LC_INTERNAL_1) break;
        mode = 1;
    }
    --rc_input_cursor;
    if (!valid || rc_expansion_output - suppressed == start) return 0;
    length = rc_expansion_output - start;
    if (length > 31 && !rc_preprocessing_started) {
        rc_expansion_output = start + 31;
        *rc_expansion_output = 0;
        rc_message_format = rc_message(4011);
        rc_format(rc_diagnostic_buffer, rc_message_format, start);
        rc_warning(4011);
    } else *rc_expansion_output = 0;
    if (rc_expansion_output - start > 512) {
        rc_expansion_output = start + 31;
        *rc_expansion_output = 0;
        rc_message_format = rc_message(4011);
        rc_format(rc_diagnostic_buffer, rc_message_format, start);
        rc_warning(4011);
    }
    length = rc_expansion_output - start;
    for (i = 0; i <= length; ++i) rc_macro_name[i] = start[i];
    rc_macro_hash = rc_hash_name(rc_macro_name);
    rc_macro_name_length = (unsigned char)(length + 1);
    return valid;
}

/* Original [01010790,010108d1). Expanded text is contiguous; a trailing
 * suppression marker is stripped before the normal symbol lookup. */
int __stdcall rc_read_expanded_identifier(unsigned char first)
{
    char *out = rc_macro_name + 1;
    unsigned char ch, hash = first & 0x5f;
    unsigned suppressed = 0;
    rc_macro_name[0] = first;
    for (;;) {
        ch = *rc_input_cursor++;
        *out = ch;
        while (rc_character_flags[ch] & 16) {
            hash += ch & 0x5f;
            ++out;
            ch = *rc_input_cursor++;
            *out = ch;
        }
        if (rc_character_class[ch] != LC_INTERNAL_4) break;
        suppressed = *rc_input_cursor++;
    }
    --rc_input_cursor;
    if (out >= rc_macro_name + 512) {
        rc_message_format = rc_message(1067);
        rc_format(rc_diagnostic_buffer, rc_message_format);
        rc_fatal(1067);
    }
    if (out - rc_macro_name > 31 && !rc_preprocessing_started) {
        out = rc_macro_name + 31;
        *out = 0;
        hash = rc_hash_name(rc_macro_name);
        rc_message_format = rc_message(4011);
        rc_format(rc_diagnostic_buffer, rc_message_format, rc_macro_name);
        rc_warning(4011);
    } else *out = 0;
    rc_macro_hash = hash;
    rc_macro_name_length = (unsigned char)(out - rc_macro_name + 1);
    return (unsigned)(out - rc_macro_name - suppressed) >= 1;
}
