/* Microsoft based-pointer declarations and expression conversions. */
#include "lexer.h"
#include "parser.h"
#include "class-lookup.h"
#include "unary-expressions.h"
#include "function-definitions.h"
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

IrToken *__stdcall based_type_expression(CType *type);
int __stdcall equal_based_expressions(IrToken *first, IrToken *second);

/* Original0x2aae0..0x2ab40, stdcall4. Attach the type's base exactly once
   when an expression carries one of the based storage attributes. */
IrToken *__stdcall qualify_based_storage(IrToken *expression)
{
    TypeShape *shape;
    IrToken *base;
    if (!expression) return 0;
    shape = expression->type->related;
    if (!shape || !(shape->attributes & 0xc)) return expression;
    base = based_type_expression(expression->type);
    if (expression->kind == 0x67 && equal_based_expressions(base, expression->tail.right))
        return expression;
    return new_left_typed_expression(0x67, expression, base);
}
