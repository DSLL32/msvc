/* KANJI keeps configured lead/trail pairs intact, including trail bytes that
 * happen to equal backslash. It has fewer escapes than the ANSI scanner and
 * records the final NUL in the token's WORD length. */
#include "tokens.h"
#include <string.h>
#include <stddef.h>
#include <recon.h>
typedef char KanjiRangeSize[(sizeof(struct KanjiRange) == 8) ? 1 : -1];
typedef char KanjiUpperOffset[(offsetof(struct KanjiRange, last) == 4) ? 1 : -1];
#pragma intrinsic(strcpy)
BOOL __stdcall is_kanji_lead(BYTE);

/* Original 0x01009210..0x01009390. */
void __cdecl scan_kanji_string(void)
{
    char converted[4098], *next = current_token.spelling, *input;
    unsigned int length = 0;
    current_token.kind = TOKEN_STRING;
    current_token.length = 0;
    while (read_source_character() != TOKEN_EOF) {
        if (is_kanji_lead((BYTE)source_character)) {
            *next++ = source_character;
            *next++ = (char)read_source_character();
            current_token.length += 2;
        } else if (source_character == '"') {
            /* This scanner drops doubled quotes rather than copying one. */
            if (advance_character() != '"')
                break;
        } else {
            ++current_token.length;
            *next++ = source_character;
        }
        if (current_token.length >= 4097) {
            lexical_error("string literal too long", "");
            while (advance_character() != TOKEN_EOF && source_character != '"')
                ;
            break;
        }
    }
    if (source_character == TOKEN_EOF)
        lexical_error("unexpected end of file in string literal", "");
    *next = 0;
    input = current_token.spelling;
    while (*input) {
        if (is_kanji_lead((BYTE)*input)) {
            converted[length++] = *input++;
            converted[length++] = *input++;
        } else if (*input == '\\') {
            ++input;
            switch (*input) {
            case '\\': converted[length++] = '\\'; break;
            case 'T': case 't': converted[length++] = '\t'; break;
            case 'A': case 'a': converted[length++] = '\b'; break;
            }
            ++input;
        } else
            converted[length++] = *input++;
    }
    converted[length] = 0;
    strcpy(current_token.spelling, converted);
    current_token.length = (WORD)(length + 1);
}

/* Original 0x010091d0..0x0100920f. Stored bounds are signed WORDs. */
BOOL __stdcall is_kanji_lead(BYTE character)
{
    return (kanji_ranges[0].first <= character && character <= kanji_ranges[0].last) ||
           (kanji_ranges[1].first <= character && character <= kanji_ranges[1].last);
}
