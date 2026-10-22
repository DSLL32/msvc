/* Dispatch between resource keywords, string literals and numeric expressions. */
#include "tokens.h"
#include <ctype.h>
#include <recon.h>

/* Original 0x010089a0..0x01008ab8. */
unsigned int __stdcall next_token(unsigned int flags)
{
    for (;;) {
        while ((unsigned char)source_character <= ' ')
            advance_character();
        current_token.line = source_line;
        current_token.position = lexer_status;
        current_token.kind = TOKEN_EOF;
        if (source_character == TOKEN_EOF)
            break;
        if (source_character == '"') {
            if (kanji_mode)
                scan_kanji_string();
            else
                scan_ansi_string();
        } else if (source_character == '-' || source_character == '~' ||
                   (source_character == '(' && !(flags & TOKEN_LITERAL_ONLY)) ||
                   isdigit(source_character)) {
            if (flags & TOKEN_LITERAL_ONLY)
                scan_number_token();
            else
                scan_expression();
        } else {
            if (!scan_keyword_or_symbol(flags))
                continue;
            if (current_token.kind == TOKEN_WIDE_PREFIX)
                scan_wide_string();
        }
        break;
    }
    return current_token.kind;
}

BOOL __stdcall scan_keyword_or_symbol(unsigned int flags)
{
    struct LexOperator *operator;
    char *next;
    WORD keyword;
    for (operator = operator_tokens; operator->spelling; ++operator) {
        if (operator->spelling == source_character) {
            current_token.kind = (BYTE)operator->token;
            current_token.length = 0;
            advance_character();
            return current_token.kind >= 11;
        }
    }
    current_token.spelling[0] = source_character;
    next = current_token.spelling + 1;
    while (!token_delimiters[advance_character()])
        *next++ = source_character;
    *next = 0;
    if (!current_token.spelling[1] &&
        (current_token.spelling[0] == 'L' || current_token.spelling[0] == 'l') &&
        source_character == '"') {
        current_token.kind = TOKEN_WIDE_PREFIX;
        return TRUE;
    }
    keyword = lookup_keyword(current_token.spelling);
    current_token.length = keyword;
    if (keyword != 0xffff) {
        current_token.kind = (BYTE)keyword;
        return TRUE;
    }
    if (flags) {
        lexical_error("undefined keyword or key name: %s", current_token.spelling);
        return FALSE;
    }
    current_token.kind = 0;
    return TRUE;
}
