/* Wide literals widen source bytes, then interpret wide numeric escapes. */
#include "tokens.h"
#include <ctype.h>
#include <string.h>
#include <recon.h>
#pragma intrinsic(memcpy)

/* Original 0x010086b0..0x01008991. */
void __cdecl scan_wide_string(void)
{
    WCHAR converted[4098], *next = current_token.text, upper;
    short length = 0;
    unsigned int value, digits;
    current_token.kind = TOKEN_WIDE_STRING;
    current_token.length = 0;
    if (read_source_character() != TOKEN_EOF) {
        for (;;) {
            if (source_character == '"' && advance_character() != '"')
                goto convert;
            if (current_token.length++ == 4097)
                lexical_error("string literal too long", "");
            else
                *next++ = (BYTE)source_character;
            if (read_source_character() == TOKEN_EOF)
                break;
        }
    }
    if (source_character == TOKEN_EOF)
        lexical_error("unexpected end of file in string literal", "");
    if (current_token.length >= 256)
        fprintf(diagnostics, "\n%s (%d) : warning RW4005: String Literal longer than 256 - stored anyway\n",
                source_filename, current_token.line);
convert:
    *next = 0;
    for (next = current_token.text; *next; ++next) {
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
            for (digits = 0; digits != 4; ++digits) {
                upper = (WORD)rc_toupper((signed char)(BYTE)*next);
                if (!isxdigit(upper))
                    break;
                value = value * 16 + (upper >= 'A' ? upper - 'A' + 10 : upper - '0');
                ++next;
            }
            converted[length++] = (WORD)value;
            --next;
            break;
        default:
            if (*next >= '0' && *next <= '7') {
                value = 0;
                for (digits = 0; digits != 7 && *next >= '0' && *next <= '7'; ++digits)
                    value = value * 8 + *next++ - '0';
                converted[length++] = (WORD)value;
                --next;
            }
        }
    }
    converted[length] = 0;
    current_token.length = length;
    memcpy(current_token.text, converted, length * 2 + 2);
}
