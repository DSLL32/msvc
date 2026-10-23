/* String literal scanning and its shared lexical value. */
#include "literals-internal.h"
#include "characters.h"
#include "source-input.h"
#include <stdlib.h>
#include <recon.h>

/* Original 0x24ea0..0x250cb, stdcall8. Retain raw capture, escaped
   delimiters, DBCS pairs, wide code units, Pascal prefixes and newline recovery.
   The segment encoder remains a separate original graph node. */
void __stdcall scan_string_literal(int wide, char delimiter)
{
    unsigned char *cursor = text_work_cursor, *segment, *checked, *previous;
    unsigned character, recovery, value;
    unsigned char classification;
    int odd;
    current_token_value.string.text = (char *)text_work_cursor;
    current_token_value.string.flags =
        (current_token_value.string.flags & ~1U) | (wide & 1);
    current_token_value.string.flags &= ~2U;
    for (;;) {
        segment = cursor;
        character = (unsigned char)*source_text_cursor++;
        classification = character_class[character];
        if (classification == CC_ESCAPE_OR_END) {
            recovery = recover_special_character();
            if (recovery & ~0x50U) continue;
            if ((recovery & 0x10) && !(token_allocation_flags & 0x4000) &&
                !(token_allocation_flags & 0x20000000UL)) {
                character = next_source_character();
                if (options.option_m && (character == 'p' || character == 'P') &&
                    cursor == text_work_cursor) {
                    current_token_value.string.flags |= 2;
                } else {
                    value = decode_literal_escape((unsigned char)character, wide) & 0xffff;
                    character = value;
                    if (wide && (value & 0xff) != value) {
                        cursor = check_literal_cursor(cursor);
                        *cursor++ = (unsigned char)value;
                        *cursor++ = (unsigned char)(value >> 8);
                        continue;
                    }
                }
            }
        } else if (classification == CC_NEWLINE) {
            --source_text_cursor;
            report_syntax_error(1);
            if (character_class[cursor[-1]] == CC_CARRIAGE_RETURN) --cursor;
            character = (unsigned short)(short)delimiter;
            goto possible_delimiter;
        } else if (classification == CC_APOSTROPHE || classification == CC_QUOTE) {
            goto possible_delimiter;
        } else if (classification == CC_LEAD_BYTE) {
            cursor = check_literal_cursor(cursor);
            *cursor++ = (unsigned char)character;
            character = next_source_character();
        }
store_character:
        checked = check_literal_cursor(cursor);
        *checked = (unsigned char)character;
        checked[1] = 0;
        cursor = encode_literal_segment(segment, wide, checked + 1 - segment);
        continue;
possible_delimiter:
        if ((int)delimiter != (unsigned short)character) goto store_character;
        if (!(token_allocation_flags & 0x20000000UL) || cursor[-1] != '\\')
            goto literal_end;
        previous = cursor - 1;
        odd = 0;
        if (previous >= (unsigned char *)current_token_value.string.text) {
            do {
                if (*previous != '\\') break;
                odd = !odd;
                --previous;
            } while (previous >= (unsigned char *)current_token_value.string.text);
            if (previous >= (unsigned char *)current_token_value.string.text &&
                character_class[*previous] == CC_LEAD_BYTE)
                odd = !odd;
        }
        if (odd) goto store_character;
literal_end:
        checked = check_literal_cursor(cursor);
        *checked = 0;
        cursor = encode_literal_segment(segment, wide, checked + 1 - segment);
        current_token_value.string.length = (unsigned short)
            (cursor - (unsigned char *)current_token_value.string.text);
        if (current_token_value.string.flags & 2)
            finish_pascal_literal(&current_token_value);
        text_work_cursor = cursor;
        return;
    }
}

/* Original 0x24e00..0x24e1c. Diagnose a full 2048-byte text workspace
   and restart output at its first byte, retaining this distinct check node. */
unsigned char *__stdcall check_literal_cursor(unsigned char *cursor)
{
    if (cursor >= expression_storage + 2048) {
        report_syntax_error(26);
        cursor = expression_storage;
    }
    return cursor;
}

/* Original 0x24e20..0x24e9f, stdcall12. Capture mode retains the source
   bytes. Otherwise wide segments convert one multibyte character through a
   separate aligned 16-bit temporary; even a conversion error retains the
   original two-byte copy from that temporary. */
unsigned char *__stdcall encode_literal_segment(unsigned char *segment,
                                                int wide, unsigned bytes)
{
    unsigned char *temporary;
    if ((token_allocation_flags & 0x20000000UL) || !wide)
        return segment + bytes;
    temporary = check_literal_cursor((unsigned char *)
        (((unsigned long)(segment + bytes + 1) + 3) & ~3UL));
    /* void * preserves the target CRT's 16-bit wchar destination without
       imposing a host wchar_t representation on reconstructed source. */
    if (mbtowc((void *)temporary, (char *)segment, bytes) < 0)
        report_syntax_error(2);
    *segment++ = temporary[0];
    *segment++ = temporary[1];
    return segment;
}
