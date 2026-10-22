/* Literal WORD/DWORD, byte-string and wide-string RCDATA contents. */
#include "tokens.h"
#include "resource-data.h"

/* Original 0x0100adb0..0x0100af6e. */
void __stdcall parse_raw_data(struct ResourceInfo *resource)
{
    BYTE *bytes;
    WCHAR *words;
    if (current_token.kind != TOKEN_BEGIN) {
        do {
            switch (current_token.kind) {
            case 130:
                next_token(0);
                if (current_token.kind != TOKEN_NUMBER)
                    parser_error("CHARACTERISTICS not a number", "");
                resource->characteristics = current_token.value;
                break;
            case 131:
                resource->language = parse_language();
                break;
            case 132:
                next_token(0);
                if (current_token.kind != TOKEN_NUMBER)
                    parser_error("VERSION not a number", "");
                resource->version = current_token.value;
                break;
            default:
                parser_error("BEGIN expected in RCDATA", "");
            }
            next_token(0);
        } while (current_token.kind != TOKEN_BEGIN);
        if (current_token.kind != TOKEN_BEGIN)
            parser_error("BEGIN expected in RCData", "");
    }
    advance_argument();
    while (current_token.kind != TOKEN_END) {
        switch (current_token.kind) {
        case TOKEN_NUMBER:
            if (current_token.is_long)
                put_dword(current_token.value);
            else
                put_word(current_token.length);
            break;
        case TOKEN_STRING:
            bytes = (BYTE *)current_token.spelling;
            while (current_token.length--)
                put_byte(*bytes++);
            break;
        case TOKEN_WIDE_STRING:
            words = current_token.text;
            while (current_token.length--)
                put_word(*words++);
            break;
        default:
            parser_error("Unexpected value in RCData", "");
            return;
        }
        advance_argument();
    }
}
