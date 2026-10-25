/* Microsoft based-pointer declarations and expression conversions. */
#include "lexer.h"
#include "parser.h"
#include "class-lookup.h"
#include "unary-expressions.h"
#include "function-definitions.h"
#include "expression-preparation.h"
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

/* Original0x2a810..0x2a956, stdcall4. Construct the segment/base value
   selected by the actual pointer shape, retaining each original reference use. */
IrToken *__stdcall based_type_expression(CType *type)
{
    TypeShape *shape = type->related;
    CppSymbol *symbol;
    IrToken *expression;
    ArenaPool *pool;
    switch (shape->attributes & 7) {
    case 4:
        return 0;
    case 5:
        symbol = shape->data.based_symbol;
        symbol->references += (unsigned short)options.configuration.debug_info;
        if ((symbol->kind & 31) == 13) return new_unary_ir_expression(0x1b, symbol);
        expression = symbol_reference(symbol);
        return indirect_storage_expression(expression);
    case 6:
        symbol = shape->data.based_symbol;
        symbol->references += (unsigned short)options.configuration.debug_info;
        expression = symbol_reference(symbol);
        return new_typed_unary_expression(0x1a, symbol->type, expression);
    case 7:
        symbol = shape->data.based_symbol;
        symbol->references += (unsigned short)options.configuration.debug_info;
        expression = symbol_reference(symbol);
        expression = indirect_storage_expression(expression);
        return new_typed_unary_expression(0x1a, symbol->type, expression);
    }
    if (shape->attributes & 8) {
        if (token_allocation_flags & 0x20000020) expression = allocate_permanent(sizeof(IrToken));
        else {
            pool = &compiler_arena.pools[2];
            if (pool->remaining < (int)sizeof(IrToken)) refill_arena(&compiler_arena, pool);
            expression = (IrToken *)pool->next;
            pool->remaining -= sizeof(IrToken);
            pool->next += sizeof(IrToken);
        }
        expression->flags = 0;
        expression->kind = 0x89;
        expression->type = type_0802;
        return expression;
    }
    /* Original defect:2a94b returns the unwritten entryESP-4 local for
       other modes. Preserve that Win32 behavior; native cannot reproduce it. */
#ifdef RECON_NATIVE_TEST
    __asm { int 3 }
#endif
    return expression;
}
