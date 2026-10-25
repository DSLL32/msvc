/* Microsoft based-pointer declarations and expression conversions. */
#include "lexer.h"
#include "parser.h"
#include "class-lookup.h"
#include "unary-expressions.h"
#include "function-definitions.h"
#include "expression-preparation.h"
#include "binary-expressions.h"
#include "numeric-conversion.h"
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

void __stdcall validate_based_self_source(IrToken *expression);

/* Original0x2aa10..0x2aad6, stdcall8. Convert nulls without a base,
   subtract an indirect target base, or retain/cast the ordinary offset value. */
IrToken *__stdcall convert_based_expression(CType *type, IrToken *expression)
{
    TypeDescription *description;
    CType *converted;
    TypeShape *shape;
    IrToken *base;
    unsigned short attributes;
    if (expression->flags & 0x80) {
        description = copy_type_description(type);
        attributes = normalize_type_attributes(1, 0);
        converted = replace_first_type_shape((CType *)description, 1, attributes, 0);
        --type_description_depth;
        expression->type = converted;
        return expression;
    }
    if (based_pointer_has_indirect_base(type->related)) {
        shape = expression->type->related;
        if (shape && (shape->flags & 3)) {
            base = based_type_expression(type);
            return new_ir_expression(0x69, expression, base, type);
        }
    }
    if ((type->related->attributes & 8) && expression->type != type)
        validate_based_self_source(expression);
    shape = expression->type->related;
    if (shape && (shape->attributes & 0xc)) expression->type = type;
    else expression = new_cast_expression(type, expression);
    if ((type->related->attributes & 7) == 4) expression = qualify_based_storage(expression);
    return expression;
}

/* Original0x2a530..0x2a576, stdcall4. A symbol-based pointer is indirect
   when its base symbol itself retains a pointer/array declarator layer. */
int __stdcall based_pointer_has_indirect_base(TypeShape *shape)
{
    CppSymbol *symbol;
    TypeShape *base;
    if ((shape->flags & 0x77) != 1 || (shape->attributes & 7) != 5) return 0;
    symbol = shape->data.based_symbol;
    base = symbol->type->related;
    if (base && !(base->flags & 0x77)) base = base->related;
    return base && (base->flags & 3);
}
