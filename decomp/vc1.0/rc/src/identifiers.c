/* Resource names and numeric ordinals use a deliberately simpler scanner than
 * expressions: read a whitespace-delimited word, uppercase it, then recognize
 * a decimal number or at most four hexadecimal digits. */
#include "resource.h"
#include <ctype.h>
#include <stdlib.h>
#include <recon.h>

/* Original 0x01009540..0x01009646. */
BOOL __cdecl parse_resource_identifier(void)
{
    char *next;
    unsigned int value = 0;
    int digits;
    if (!read_identifier())
        return FALSE;
    _strupr(current_token.spelling);
    if (isdigit(current_token.spelling[0])) {
        if (current_token.spelling[1] == 'X') {
            next = current_token.spelling + 2;
            for (digits = 4; digits && isxdigit(*next); --digits, ++next)
                value = value * 16 + (*next >= 'A' ? *next - 'A' + 10 : *next - '0');
            current_token.length = (WORD)value;
        } else
            current_token.length = (WORD)atoi(current_token.spelling);
    } else
        current_token.length = 0;
    return TRUE;
}

/* Original 0x01009650..0x010096da. Whitespace terminates a resource name;
 * quotes remain in the spelling, as observed in the original .res oracle. */
char * __cdecl read_identifier(void)
{
    char *next;
    while ((unsigned char)source_character <= ' ')
        advance_character();
    if (source_character == TOKEN_EOF) {
        current_token.kind = TOKEN_EOF;
        return 0;
    }
    current_token.spelling[0] = source_character;
    next = current_token.spelling + 1;
    while (read_source_character() != TOKEN_EOF && (unsigned char)source_character > ' ')
        *next++ = source_character;
    *next = 0;
    advance_character();
    if (source_character == TOKEN_EOF)
        current_token.kind = TOKEN_EOF;
    return current_token.spelling;
}
