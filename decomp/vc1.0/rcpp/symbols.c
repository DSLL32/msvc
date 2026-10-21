/* Name handling and macro symbol storage. */
#include "lexer.h"
/* Original [0100be50,0100be6a). The eight-bit sum deliberately folds case
 * for hashing; later lookup still compares the identifier's actual spelling. */
unsigned char __stdcall rc_hash_name(const char *name)
{
    unsigned char sum = 0;
    while (*name) sum += *name++ & 0x5f;
    return sum;
}

RcMacro *rc_macros[256];
/* Original [0100a550,0100a726). Kind -2 is a command-line definition, whose
 * replacement receives the original internal 'D' terminator marker. */
void __stdcall rc_define_macro(char *body, unsigned length, int kind)
{
    RcMacro *macro;
    unsigned n;
    char *replacement = length ? body : NULL;
    for (n = 0; n < 8 && rc_macro_name[n] == "defined"[n]; ++n) ;
    if (n == 8) {
        rc_message_format = rc_message(4112);
        rc_format(rc_diagnostic_buffer, rc_message_format, rc_macro_name, "#define");
        rc_warning(4112);
        return;
    }
    macro = rc_find_macro();
    if (macro) {
        if (macro->arguments < -2) {
            rc_message_format = rc_message(4112);
            rc_format(rc_diagnostic_buffer, rc_message_format, rc_macro_name, "#define");
            rc_warning(4112);
            return;
        }
        if (!rc_compare_macro_text(replacement, macro->body, length)) return;
        rc_message_format = rc_message(4005);
        rc_format(rc_diagnostic_buffer, rc_message_format, rc_macro_name);
        rc_warning(4005);
    } else {
        unsigned char name_bytes = (unsigned char)rc_macro_name_length;
        macro = rc_malloc(sizeof(*macro));
        if (!macro) {
            rc_message_format = rc_message(1002);
            rc_format(rc_diagnostic_buffer, rc_message_format);
            rc_error(1002);
            return;
        }
        macro->name = rc_copy_bytes(rc_macro_name, name_bytes);
        macro->next = rc_macros[rc_macro_hash];
        macro->body = NULL;
        macro->flags = 0;
        rc_macros[rc_macro_hash] = macro;
    }
    if (length) {
        macro->body = rc_copy_bytes(replacement, length);
        if (kind == -2) macro->body[length-1] = 'D';
    }
    macro->arguments = kind == -2 ? 0 : (signed char)kind;
}

/* Original [0100a730,0100a76f). Name length includes its NUL terminator. */
RcMacro *rc_find_macro(void)
{
    RcMacro *macro;
    unsigned n;
    for (macro = rc_macros[rc_macro_hash]; macro; macro = macro->next) {
        for (n = 0; n < (unsigned)rc_macro_name_length; ++n)
            if (rc_macro_name[n] != macro->name[n]) break;
        if (n == (unsigned)rc_macro_name_length) return macro;
    }
    return NULL;
}

/* Original [01010940,010109a6). Preserve embedded NULs in macro replacements
 * and append another NUL beyond the requested byte range. */
char *__stdcall rc_copy_bytes(const char *text, unsigned count)
{
    char *copy = rc_malloc(count + 1);
    unsigned i;
    if (!copy) {
        rc_message_format = rc_message(1002);
        rc_format(rc_diagnostic_buffer, rc_message_format);
        rc_error(1002);
        return NULL;
    }
    for (i = 0; i < count; ++i) copy[i] = text[i];
    copy[count] = 0;
    return copy;
}

/* Original [010108f0,01010936). This helper deliberately has no allocation
 * failure check; its callers inherit the original runtime's fault behavior. */
char *__stdcall rc_copy_string(const char *text)
{
    const char *end;
    char *copy;
    unsigned count, i;
    for (end = text; *end; ++end) ;
    count = end - text + 1;
    copy = rc_malloc(count);
    for (i = 0; i < count; ++i) copy[i] = text[i];
    return copy;
}
