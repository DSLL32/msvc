/* Numeric tokens accepted by the inline assembler's MASM vocabulary. */
#include "asm.h"
#include "characters.h"
#include "source-input.h"
#include <recon.h>

/* Original0x4da9b..0x4dc64. Prefixes and radix suffixes share the scanner
   workspace. C L/U suffixes still select the lexer token's signedness/width. */
unsigned char __stdcall scan_assembler_number(unsigned char character)
{
    unsigned char token = TOK_INT_CONSTANT, following;
    unsigned char *cursor = (unsigned char *)token_text, *end;
    unsigned suffix;
    int radix;
    if (character == '0') {
        character = read_numeric_character();
        if ((character & 0xdf) == 'X') {
            character = read_numeric_character();
            radix = 16;
            while (character_flags[character] & CF_HEX) {
                *cursor++ = character;
                character = read_numeric_character();
            }
            end = cursor;
            if (end == (unsigned char *)token_text) report_syntax_error(153);
            goto width_suffix;
        }
        radix = 8;
        *cursor++ = '0';
    } else radix = 10;
    while (character_flags[character] & CF_HEX) {
        *cursor++ = character;
        character = read_numeric_character();
    }
    suffix = cursor[-1];
    end = cursor - 1;
    if (!(character_flags[suffix] & CF_RADIX_SUFFIX) || (character & 0xdf) == 'H') {
        suffix = character;
        end = cursor;
        if (character_flags[suffix] & CF_RADIX_SUFFIX) character = read_numeric_character();
    }
    if (character_flags[suffix] & CF_RADIX_SUFFIX) {
        switch (suffix & 0xdf) {
        case 'B': radix = 2; break;
        case 'D': radix = 10; break;
        case 'H': radix = 16; break;
        case 'O': case 'Q': radix = 8; break;
        }
    }
width_suffix:
    if ((character & 0xdf) == 'L') {
        following = read_numeric_character();
        if ((following & 0xdf) == 'U') {
            token = TOK_ULONG_CONSTANT;
            goto convert;
        }
        token = TOK_LONG_CONSTANT;
    } else if ((character & 0xdf) == 'U') {
        following = read_numeric_character();
        if ((following & 0xdf) == 'L') {
            token = TOK_ULONG_CONSTANT;
            goto convert;
        }
        token = TOK_UINT_CONSTANT;
    }
    --source_text_cursor;
convert:
    *end = 0;
    if (radix == 2) {
        if (end - (unsigned char *)token_text < 33)
            current_token_value.words.low = parse_asm_binary_digits(token_text);
        else report_syntax_error(177);
    } else current_token_value.words.low =
        convert_integer_digits(token_text,end - (unsigned char *)token_text,radix);
    return token;
}
