/* Dialog/control style expressions keep token handling separate from RC's
 * general numeric evaluator. NOT clears bits in the current style value. */
#include "tokens.h"
WORD __stdcall advance_argument_flags(WORD);
BOOL __cdecl is_style_token(void);

/* Original 0x0100b400..0x0100b4f9. */
DWORD __stdcall parse_style(DWORD initial)
{
    BOOL negate;
    int parentheses = 0;
    advance_argument_flags(TOKEN_LITERAL_ONLY);
    negate = current_token.kind == 104;
    while (is_style_token()) {
        if (current_token.kind == TOKEN_NUMBER) {
            if (negate) {
                initial &= ~current_token.value;
                negate = FALSE;
            } else
                initial |= current_token.value;
        }
        next_token(TOKEN_LITERAL_ONLY | 1);
        while (current_token.kind == TOKEN_OR || current_token.kind == TOKEN_LPAREN ||
               current_token.kind == TOKEN_RPAREN || current_token.kind == 104) {
            switch (current_token.kind) {
            case TOKEN_LPAREN: ++parentheses; break;
            case TOKEN_RPAREN: --parentheses; break;
            case 104: negate = !negate; break;
            }
            next_token(TOKEN_LITERAL_ONLY | 1);
            if (current_token.kind == 104) {
                negate = TRUE;
                next_token(TOKEN_LITERAL_ONLY | 1);
            }
        }
    }
    if (parentheses)
        parser_error("Unbalanced Parentheses", "");
    return initial;
}

/* Original 0x0100c780..0x0100c7c4. */
WORD __stdcall advance_argument_flags(WORD flags)
{
    flags |= 1;
    next_token(flags);
    while (current_token.kind == TOKEN_COMMA)
        next_token(flags);
    return current_token.kind;
}

/* Original 0x0100c100..0x0100c12d. */
BOOL __cdecl is_style_token(void)
{
    return current_token.kind == TOKEN_NUMBER || current_token.kind == TOKEN_LPAREN ||
           current_token.kind == TOKEN_RPAREN || current_token.kind == 104;
}
