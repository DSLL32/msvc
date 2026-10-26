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

/* Original0x61000..0x6109d, stdcall12. The operation formal is unused;
   diagnose differing concrete pointer modes or differing named bases. */
void __stdcall check_based_pointer_conversion(unsigned char operation,
    TypeShape *target, TypeShape *source)
{
    unsigned short target_mode, source_mode;
    CppSymbol *target_base, *source_base;
    if ((source->flags & 0x77) == 2 && (source->attributes & 0xc)) return;
    target_mode = target->attributes & 15;
    source_mode = source->attributes & 15;
    if (!(target_mode & 4) && !(source_mode & 4)) return;
    if (!(target_mode & 12) || !(source_mode & 12) ||
        source_mode == 4 || target_mode == 4) return;
    if (source_mode != target_mode) {
        compiler_warning(1, 123);
        return;
    }
    target_base = (target_mode & 7) >= 5 ? target->data.based_symbol : 0;
    source_base = (source_mode & 7) >= 5 ? source->data.based_symbol : 0;
    if (source_base != target_base)
        compiler_warning(1, 119, target_base->name, source_base->name);
}

CType *__stdcall make_expression_qualified_type(CType *base, unsigned short kind,
    unsigned short attributes, void *parameter);

/* Original0x62810..0x62858, stdcall8. Attach an arena-owned based
   modifier while keeping a function's declared pointer-model attributes. */
void __stdcall apply_symbol_based_type(CppSymbol *symbol, CppSymbol *base)
{
    unsigned attributes;
    symbol->attributes |= 2;
    attributes = (symbol->kind & 31) == 7 ? symbol->type->attributes : 5;
    symbol->type = make_expression_qualified_type(symbol->type, 0,
        (unsigned short)attributes, &base);
}

/* Original0x2a660..0x2a73c, stdcall8. A typed __based value must
   cast to a segment and name a scalar base or an ordinary pointer base. */
unsigned short __stdcall validate_based_pointer_cast(unsigned short flags, char *name)
{
    CppSymbol *symbol;
    TypeShape *shape;
    unsigned kind;
    if (!(flags & 0x800)) {
        report_syntax_error(209);
        return 2;
    }
    symbol = find_outer_name(name);
    if (!symbol) {
        report_syntax_error(65, name);
        return 2;
    }
    kind = symbol->kind & 31;
    if (kind != 16 && kind != 17) {
        shape = symbol->type->related;
        if (symbol->attributes & 2) shape = shape->related;
        if (!shape) {
            symbol->references += (unsigned short)options.configuration.debug_info;
            based_symbol = symbol;
            return 5;
        }
        if ((shape->flags & 0x77) == 1 && !(shape->attributes & 4)) {
            symbol->references += (unsigned short)options.configuration.debug_info;
            based_symbol = symbol;
            return 7;
        }
    }
    report_syntax_error(210, format_symbol_name(symbol));
    return 2;
}

/* Original0x2a740..0x2a7d6, stdcall8. Address-form casts retain
   an existing based symbol when present, otherwise the named object's address. */
unsigned short __stdcall validate_based_address_cast(unsigned short flags, char *name)
{
    CppSymbol *symbol;
    unsigned kind;
    unsigned short mode;
    if (!(flags & 0x800)) {
        report_syntax_error(209);
        return 2;
    }
    symbol = find_outer_name(name);
    if (!symbol) {
        report_syntax_error(65, name);
        return 2;
    }
    kind = symbol->kind & 31;
    if (kind == 6 || kind == 7) {
        report_syntax_error(211, name);
        return 2;
    }
    if (symbol->attributes & 2) {
        symbol = symbol->type->related->data.based_symbol;
        mode = 5;
    } else mode = 6;
    symbol->references += (unsigned short)options.configuration.debug_info;
    based_symbol = symbol;
    return mode;
}

/* Original 0x2a7e0..0x2a807, stdcall4/WORD result. Grammar rule161
   is __based((TYPE_NAME)__self), not an ordinary C++ bit field. A segment
   type selects self-relative mode8; invalid casts diagnose and use mode2. */
unsigned short __stdcall validate_based_self_cast(unsigned short flags)
{
    if (flags & 0x800) return 8;
    report_syntax_error(209);
    return 2;
}

/* Original 0x2a960..0x2aa03, stdcall4. Strip transparent wrappers to
   reject constants, then inspect the actual storage or pointer shape. The
   original requires a non-NULL, nonempty expression chain and dereferences
   an exhausted chain; retain that defect rather than inventing a guard. */
void __stdcall validate_based_self_source(IrToken *expression)
{
    IrToken *source = expression;
    TypeShape *shape;
    unsigned storage;
    while (source && (source->kind == 0x80 || source->kind == 0x6e || source->kind == 0x67))
        source = source->value.pointer;
    if ((source->flags & 7) == 3) {
        report_syntax_error(101, token_spellings[137]);
        return;
    }
    source = modifiable_lvalue(expression, 0);
    if (source && (source->flags & 7) == 4) {
        storage = ((CppSymbol *)source->value.pointer)->storage[0] & 7;
        if (storage >= 5 && storage <= 6)
            report_syntax_error(103, token_spellings[137]);
    } else {
        shape = expression->type->related;
        if (!shape || !(shape->flags & 3))
            report_syntax_error(102, token_spellings[137]);
    }
}

/* Original 0x2ab40..0x2acc5, stdcall8. Compare only the segment/base
   expression forms constructed by based_type_expression. This is deliberately
   not a general recursive AST or type-equality operation. */
int __stdcall equal_based_expressions(IrToken *first, IrToken *second)
{
    if (first == second) return 1;
    if (!first || !second) return 0;
    if (first->flags != second->flags || first->kind != second->kind) return 0;
    switch (first->kind) {
    case 0x89:
        return 1;
    case 0x1b:
        return first->value.pointer == second->value.pointer;
    case 0x1a:
        first = first->value.pointer;
        second = second->value.pointer;
        if (!first || !second || first->kind != second->kind) return 0;
        if (first->kind == 2)
            return first->value.pointer == second->value.pointer;
        if (first->kind != 0x46) return 0;
        first = first->value.pointer;
        second = second->value.pointer;
        if (!first || !second || first->kind != second->kind || first->kind != 2) return 0;
        return first->value.pointer == second->value.pointer;
    case 0x46:
        first = first->value.pointer;
        second = second->value.pointer;
        if (!first || !second || (first->flags & 7) != 4 || first->kind != 2 ||
            (second->flags & 7) != 4 || second->kind != 2) return 0;
        return first->value.pointer == second->value.pointer;
    }
    return 0;
}
