/* Microsoft based-pointer declarations and expression conversions. */
#include "lexer.h"
#include "parser.h"
#include "class-lookup.h"
#include <recon.h>

/* Original0x2a580..0x2a655, stdcall4. A named base must resolve to a
   segment value or an ordinary near/far pointer, not an instance member. */
CppSymbol *__stdcall lookup_based_symbol(char *name)
{
    CppSymbol *symbol;
    TypeShape *shape;
    unsigned attributes;
    int valid;
    symbol = find_outer_name(name);
    if (!symbol) {
        report_syntax_error(65, name);
        return 0;
    }
    if ((symbol->kind & 31) == 18)
        symbol = ((QualifiedDeclarator *)symbol)->resolution->binding->target;
    shape = symbol->type->related;
    if (shape && !(shape->flags & 0x77)) shape = shape->related;
    if (!shape) valid = (symbol->type->flags & 0x800) != 0;
    else {
        attributes = shape->attributes & 7;
        valid = (shape->flags & 1) && (attributes == 1 || attributes == 2);
    }
    if (valid) {
        if ((symbol->kind & 31) == 3 && !is_static_data_member(symbol))
            report_syntax_error(293, format_symbol_name(symbol));
        else symbol->references += (unsigned short)options.configuration.debug_info;
    } else report_syntax_error(213, name);
    return symbol;
}
