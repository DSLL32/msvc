/* Original 0x0100d260..0x0100d2a6. The record parameter is unused. */
#include "tokens.h"
#include "resource-data.h"

void __stdcall parse_dialog_include(struct ResourceInfo *resource)
{
    if (current_token.kind != TOKEN_STRING) {
        parser_error("String not found in DLGINCLUDE statement", "");
        return;
    }
    ansi_to_wide(current_token.text, current_token.spelling, current_token.length + 1);
    put_wide_string(current_token.text);
}
