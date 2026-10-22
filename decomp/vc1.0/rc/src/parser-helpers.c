#include "menu.h"
#include <recon.h>
BOOL argument_had_comma;

/* Original 0x0100c730..0x0100c775. Repeated commas are accepted. */
WORD __cdecl advance_argument(void)
{
    argument_had_comma = FALSE;
    next_token(1);
    while (current_token.kind == TOKEN_COMMA) {
        next_token(1);
        argument_had_comma = TRUE;
    }
    return current_token.kind;
}

/* Original 0x0100b500..0x0100b523. */
WORD __cdecl parse_word_expression(void)
{
    if (current_token.kind == TOKEN_NUMBER)
        return current_token.length;
    parser_error("Expected Numerical Dialog constant", "");
    return 0;
}

/* Original 0x0100b870..0x0100b8b5. Word additions and ORs are left-to-right. */
void __stdcall combine_word_operators(WORD *value)
{
    BYTE operator;
    for (;;) {
        operator = current_token.kind;
        if (operator != TOKEN_PLUS && operator != TOKEN_OR)
            break;
        advance_argument();
        if (operator == TOKEN_PLUS)
            *value += parse_word_expression();
        else
            *value |= parse_word_expression();
        advance_argument();
    }
}
