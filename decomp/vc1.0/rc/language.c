/* Original 0x0100a520..0x0100a5e0. LANGUAGE's two numeric fields. */
#include "tokens.h"

WORD __cdecl parse_language(void)
{
    WORD primary;
    next_token(0);
    if (current_token.kind != TOKEN_NUMBER) {
        parser_error("PRIMARY LANGUAGE ID not a number", "");
        return 0;
    }
    if (current_token.is_long) {
        parser_error("PRIMARY LANGUAGE ID too large", "");
        return 0;
    }
    primary = current_token.length;
    next_token(0);
    if (current_token.kind != TOKEN_COMMA) {
        parser_error("Missing COMMA in LANGUAGE Statement", "");
        return 0;
    }
    next_token(0);
    if (current_token.kind != TOKEN_NUMBER) {
        parser_error("SUBLANGUAGE ID not a number", "");
        return 0;
    }
    if (current_token.is_long) {
        parser_error("SUBLANGUAGE ID too large", "");
        return 0;
    }
    return (WORD)((current_token.length << 10) | primary);
}
