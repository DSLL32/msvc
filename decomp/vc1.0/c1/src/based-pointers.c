/* Microsoft named-base pointer declarations and expression adjustment. */
#include "parser.h"
#include "expressions.h"
#include <recon.h>

/* Original 0x25e30..0x25ec5. A named base must be a segment scalar or
   an ordinary near/far data pointer. Invalid bases still return their symbol
   after the diagnostic; missing names alone return null. */
FrontendSymbol *__stdcall resolve_based_pointer_symbol(char *name)
{
    FrontendSymbol *symbol = find_identifier_symbol(name);
    TypeShape *shape;
    unsigned model;
    int valid;
    if (!symbol) {
        report_syntax_error(65, name);
        return 0;
    }
    shape = symbol->type->related;
    if (shape && !(shape->flags & 7)) shape = shape->related;
    if (shape) {
        model = shape->attributes & 7;
        valid = (shape->flags & 1) && (model == 1 || model == 2);
    } else valid = symbol->type->flags & 0x800;
    if (valid) symbol->references += (unsigned short)options.debug_info;
    else report_syntax_error(213, name);
    return symbol;
}

/* Original 0x262e0..0x26340. Attach the base operand only to based
   pointer expressions which do not already carry the same normalized base. */
IrToken *__stdcall normalize_based_expression(IrToken *expression)
{
    TypeShape *shape;
    IrToken *base;
    if (!expression) return 0;
    shape = expression->type->related;
    if (!shape || !(shape->attributes & 12)) return expression;
    base = make_based_pointer_base(expression->type);
    if (expression->kind == TOK_BASED_REFERENCE &&
        same_based_pointer_base(base, ((ExpressionNode *)expression)->value.children.right))
        return expression;
    return make_ir_expression(TOK_BASED_REFERENCE, expression, base);
}

/* Original 0x26060..0x2617d. Build the stored base representation:
   a loaded base pointer, explicit segment expression, or implicit self base. */
IrToken *__stdcall make_based_pointer_base(CType *type)
{
    TypeShape *shape = type->related;
    unsigned mode = shape->attributes & 7;
    FrontendSymbol *symbol;
    IrToken *base;
    ArenaPool *pool;
    if (mode == 4) return 0;
    if (mode == 5) {
        symbol = (FrontendSymbol *)shape->field08;
        symbol->references += (unsigned short)options.debug_info;
        if ((symbol->kind & 31) == 13) return make_symbol_ir_leaf(TOK_SEGNAME, symbol);
        return make_indirection(make_symbol_reference(symbol));
    }
    if (mode == 6) {
        symbol = (FrontendSymbol *)shape->field08;
        symbol->references += (unsigned short)options.debug_info;
        return make_typed_unary_expression(TOK_SEGMENT, symbol->type,
                                          make_symbol_reference(symbol));
    }
    if (mode == 7) {
        symbol = (FrontendSymbol *)shape->field08;
        symbol->references += (unsigned short)options.debug_info;
        base = make_indirection(make_symbol_reference(symbol));
        return make_typed_unary_expression(TOK_SEGMENT, symbol->type, base);
    }
    if (shape->attributes & 8) {
        pool = &compiler_arena.pools[2];
        if (pool->remaining < (int)sizeof(IrToken)) refill_arena(&compiler_arena,pool);
        base = (IrToken *)pool->next;
        pool->remaining -= sizeof(IrToken);
        pool->next += sizeof(IrToken);
        base->flags = 0;
        base->kind = 129;
        base->type = type_0802;
    }
    /* The original leaves this local unwritten for a non-based input.
       Callers provide one of the based representations above. */
    return base;
}

/* Original 0x26210..0x262d6. Constants retain an ordinary pointer type;
   other sources either subtract a base or retain their based representation. */
IrToken *__stdcall convert_based_pointer(CType *target, IrToken *value)
{
    CType *description;
    TypeShape *source;
    if (value->flags & 0x80) {
        description = copy_type_description(target);
        value->type = replace_outer_type_shape(description, 1,
                             normalize_type_attributes(1, 0), 0);
        --type_description_depth;
        return value;
    }
    if (is_pointer_based_shape(target->related)) {
        source = value->type->related;
        if (source && (source->flags & 3))
            return make_typed_ir_expression(TOK_DESEG, value,
                                           make_based_pointer_base(target), target);
    }
    if ((target->related->attributes & 8) && value->type != target)
        validate_self_based_source(value);
    source = value->type->related;
    if (!source || !(source->attributes & 12))
        value = convert_pointer_representation(target, value);
    else value->type = target;
    if ((target->related->attributes & 7) == 4)
        value = normalize_based_expression(value);
    return value;
}

/* Original 0x25de0..0x25e26. Distinguish a named pointer base from
   segment/self bases before an ordinary address is converted into an offset. */
int __stdcall is_pointer_based_shape(TypeShape *shape)
{
    FrontendSymbol *symbol;
    TypeShape *base;
    if ((shape->flags & 7) != 1 || (shape->attributes & 7) != 5) return 0;
    symbol = (FrontendSymbol *)shape->field08;
    base = symbol->type->related;
    if (base && !(base->flags & 7)) base = base->related;
    return base && (base->flags & 3) ? 1 : 0;
}

/* Original 0x3f240..0x3f2dd. Based arrays bypass this comparison;
   explicit pointer bases warn when their models or named base symbols differ. */
void __stdcall check_based_pointer_conversion(unsigned char operation,
                                             TypeShape *target, TypeShape *source)
{
    unsigned short target_attributes, source_attributes;
    unsigned target_model, source_model;
    unsigned target_base = 0, source_base = 0;
    if ((source->flags & 7) == 2 && (source->attributes & 12)) return;
    source_attributes = source->attributes;
    target_attributes = target->attributes;
    target_model = target_attributes & 15;
    source_model = source_attributes & 15;
    if (!((target_attributes | source_attributes) & 4) ||
        !(target_attributes & 12) || !(source_attributes & 12) ||
        target_model == 4 || source_model == 4) return;
    if (target_model != source_model) {
        compiler_warning(1, 123);
        return;
    }
    if ((target_model & 7) > 4) target_base = target->field08;
    if ((source_model & 7) > 4) source_base = source->field08;
    if (target_base != source_base) compiler_warning(1, 119);
}
