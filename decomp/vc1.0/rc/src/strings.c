/* RC's byte-string scanner and its period escape rules. */
#include "tokens.h"
#include <ctype.h>
#include <string.h>
#include <recon.h>
#pragma intrinsic(memcpy)

/* Original 0x01008f60..0x010091c5. Unlike C, \a maps to backspace;
 * unknown escapes disappear. Octal and hexadecimal are limited to 3/2 digits. */
void __cdecl scan_ansi_string(void)
{
    char converted[4098], *next = current_token.spelling;
    char upper;
    short length = 0;
    unsigned int value, digits;
    current_token.kind = TOKEN_STRING;
    current_token.length = 0;
    if (read_source_character() != TOKEN_EOF) {
        for (;;) {
            if (source_character == '"' && advance_character() != '"')
                break;
            if (current_token.length++ == 4097)
                lexical_error("string literal too long", "");
            else
                *next++ = source_character;
            if (read_source_character() == TOKEN_EOF)
                break;
        }
    }
    if (source_character == TOKEN_EOF)
        lexical_error("unexpected end of file in string literal", "");
    *next = 0;
    for (next = current_token.spelling; *next; ++next) {
        if (*next != '\\') {
            converted[length++] = *next;
            continue;
        }
        ++next;
        switch (*next) {
        case '\\': converted[length++] = '\\'; break;
        case 'T': case 't': converted[length++] = '\t'; break;
        case 'A': case 'a': converted[length++] = '\b'; break;
        case 'n': converted[length++] = '\n'; break;
        case 'r': converted[length++] = '\r'; break;
        case '"': converted[length++] = '"'; break;
        case 'X': case 'x':
            value = 0;
            ++next;
            for (digits = 0; digits != 2; ++digits) {
                upper = (char)rc_toupper(*next);
                if (!isxdigit((unsigned char)upper))
                    break;
                value = value * 16 + (upper >= 'A' ? upper - 'A' + 10 : upper - '0');
                ++next;
            }
            converted[length++] = (char)value;
            --next;
            break;
        default:
            if (*next >= '0' && *next <= '7') {
                value = 0;
                for (digits = 0; digits != 3 && *next >= '0' && *next <= '7'; ++digits)
                    value = value * 8 + *next++ - '0';
                converted[length++] = (char)value;
                --next;
            }
        }
    }
    converted[length] = 0;
    memcpy(current_token.spelling, converted, length + 1);
    current_token.length = length;
}
