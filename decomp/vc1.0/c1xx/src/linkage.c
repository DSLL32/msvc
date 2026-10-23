/* Language-linkage nesting, original 0x3cd30..0x3cd62. */
#include "options.h"
#include "parser.h"
#include <recon.h>

int linkage_depth = -1;
LanguageLinkage linkage_stack[10];

void __stdcall push_linkage(LanguageLinkage linkage)
{
    ++linkage_depth;
    if (linkage_depth == 10)
        fatal_error(45);
    linkage_stack[linkage_depth] = linkage;
}

/* Original 0x3ca20..0x3caee. Recognize only C and C++ linkage
   strings, and abbreviate an unsupported language spelling for diagnostics. */
unsigned __stdcall parse_linkage(DeclarationType *type, IrToken *language)
{
    char *text = language->value.text;
    unsigned length;
    if (language->tail.string.auxiliary_flags & 1) report_syntax_error(2);
    if (declaration_mode) report_syntax_error(598);
    if ((type->storage[0] & 7) != 1 || !(type->storage[0] & 0x10))
        compiler_warning(1, 502);
    if (text[0] == 'C') {
        if (!text[1]) return 2;
        if (text[1] == '+' && text[2] == '+' && !text[3]) return 1;
    }
    for (length = 0; text[length]; ++length) {}
    if (length > 32) {
        if (length >= 35) length = 35;
        text[length] = 0;
        text[length - 1] = '.';
        text[length - 2] = '.';
        text[length - 3] = '.';
    }
    report_syntax_error(537, text);
    return 0;
}
