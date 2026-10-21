/* Stable #line output emitted when entering, leaving or renaming an input. */
#include "lexer.h"
/* Original [01009ba0,01009c14). The name formatter returns an end pointer
 * rather than a terminated string; the following caller supplies newlines. */
void rc_emit_line(void)
{
    char prefix[16], *end;
    unsigned length;
    rc_format(prefix, "#line %d ", rc_line_number + 1);
    for (length = 0; prefix[length]; ++length) ;
    fwrite(prefix, length, 1, rc_output);
    end = rc_quote_filename(rc_macro_name, rc_source_name);
    fwrite(rc_macro_name, end - rc_macro_name, 1, rc_output);
}

/* Original [01009b50,01009b9e). Backslashes are doubled according to their
 * lexer category; multibyte trail characters are copied without escaping. */
char *__stdcall rc_quote_filename(char *output, const char *name)
{
    unsigned category;
    *output++ = '"';
    *output = *name;
    while (*output) {
        category = rc_character_class[(unsigned char)*name];
        if (category == LC_END) *++output = '\\';
        else if (category == LC_DBCS_LEAD) *++output = *++name;
        ++name;
        *++output = *name;
    }
    *output++ = '"';
    return output;
}
