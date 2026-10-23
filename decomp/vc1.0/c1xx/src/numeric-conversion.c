/* Scalar and constant conversion across pointer models and x87 values. */
#include "numeric-conversion.h"
#include "lexer.h"
#include "constructor-initializers.h"
#include "unary-expressions.h"
#include "member-expressions.h"
#include <limits.h>
#include <recon.h>

/* Original 0x65dc0..0x6607a. Retain the native enum-to-int view, integral
   range conversion, far-pointer casts and precision-limited floating roundtrip. */
IrToken *__stdcall convert_numeric_constant(unsigned char operation, CType *type, IrToken *expression)
{
    CType *source;
    TypeShape *shape;
    TypeDescription *description;
    unsigned short target_flags, source_flags, target_shape, source_shape;
    unsigned short target_attributes, source_attributes, base;
    int status, unsigned_result;
    long converted_integer;
    float single;
    double real;
    Extended number;

    source = expression->type;
    if ((source->flags & 0x200) && !(type->flags & 0x200))
        expression->type = source = type_0064;
    target_flags = type->flags;
    source_flags = source->flags;
    shape = type->related;
    target_shape = shape ? shape->flags : 0;
    target_attributes = shape ? shape->attributes : 0;
    shape = source->related;
    source_shape = shape ? shape->flags : 0;
    source_attributes = shape ? shape->attributes : 0;

    if (!(source_flags & 0x40) && !source_shape) {
        if (source_flags & 0x100) {
            if (target_flags & 0x40) {
                if (extended_to_integer(expression->value.pointer, &converted_integer))
                    compiler_warning(2, 56);
                unsigned_result = (target_flags & 0x4000) ||
                    ((target_flags & 0x2fdf) == 0x41 && options.configuration.option_J &&
                     !(target_flags & 0x8000));
                base = target_flags & 0x2fdf;
                /* Native four-entry table: int/uint/long/ulong kinds 7/9/8/10. */
                expression->kind = (base == 0x44 ? 8 : 7) + (unsigned_result ? 2 : 0);
                expression->value.integer = (unsigned)converted_integer;
            } else {
                if (!(target_flags & 0x100)) goto converted;
                if (!target_shape) number = *(Extended *)expression->value.pointer;
            }
        }
    } else if (!(target_flags & 0x40) && !target_shape) {
        if (!(target_flags & 0x100)) goto converted;
        if (source_flags & 0x4000)
            status = unsigned_to_extended((unsigned long *)&expression->value.integer, &number);
        else status = integer_to_extended((long *)&expression->value.integer, &number);
        if (status) compiler_warning(2, 56);
    } else {
        if (source_shape && (target_attributes & 7) == 2 &&
            (source_attributes & 7) != 2 && !(expression->flags & 0x80))
            return new_cast_expression(type, expression);
        expression->value.integer = convert_integral_constant(operation, expression,
            (unsigned char)target_shape, target_attributes, target_flags);
        if ((type->flags & 0x4000) && !(expression->type->flags & 0x4000)) {
            description = copy_type_description(expression->type);
            expression->type = (CType *)description;
            description->flags |= 0x4000;
            expression->type = intern_type(description);
            --type_description_depth;
        }
    }
    if ((target_flags & 0x100) && !target_shape) {
        if (target_flags == 0x108) {
            if (normalized_Op) {
                extended_to_double(&number, &real);
                double_to_extended(&real, &number);
            }
            number.format = 8;
            expression->kind = 5;
        } else if (target_flags == 0x104) {
            if (normalized_Op) {
                extended_to_float(&number, &single);
                float_to_extended(&single, &number);
            }
            number.format = 4;
            expression->kind = 4;
        } else {
            number.format = 8;
            expression->kind = 6;
        }
        expression->value.pointer = save_extended_constant(&number);
    }
converted:
    expression->type = type;
    expression->flags &= ~0x80;
    return expression;
}

#define WARN_CONSTANT_TRUNCATION(operation_) do { \
    if ((operation_) == 0x6e) compiler_warning(4, 310); \
    else compiler_warning(2, 309, token_spellings[(operation_)]); \
} while (0)

/* Original 0x660a0..0x662dc. Widen from the source representation before
   checking the target range, then narrow/sign-extend the final target value.
   The shape-attributes argument is retained although this body does not use it. */
unsigned __stdcall convert_integral_constant(unsigned char operation, IrToken *expression,
    unsigned char shape_flags, unsigned short shape_attributes, unsigned short target_flags)
{
    unsigned value = expression->value.integer, original;
    unsigned target_width, source_width;
    /* Native stack slots +0x10/+0x14 are unwritten for unsupported scalar
       categories. Valid integer/enum widths select the defined branches. */
    unsigned unspecified_target_width, unspecified_source_width;
    unsigned short source_flags;
    short source_unsigned;
    if (shape_flags & 1) return value;
    if (target_flags & 0x40) target_width = target_flags & 15;
    else target_width = (target_flags & 0x200) ? 4 : unspecified_target_width;
    source_flags = expression->type->flags;
    if (!expression->type->related) {
        if (source_flags & 0x40) source_width = source_flags & 15;
        else source_width = (source_flags & 0x200) ? 4 : unspecified_source_width;
        if ((int)source_width < (int)target_width) {
            original = value;
            if (source_width == 2) {
                if (source_flags & 0x4000) value &= 0xffff;
                else value = (unsigned)(short)value;
                if (value != original) WARN_CONSTANT_TRUNCATION(operation);
            } else if (source_width != 4) {
                if ((source_flags & 0x4000) ||
                    (options.configuration.option_J && !(source_flags & 0x8000))) value &= 0xff;
                else value = (unsigned)(signed char)value;
                if (value != original) WARN_CONSTANT_TRUNCATION(operation);
            }
        }
    }
    source_unsigned = (source_flags & 0x4000) ||
        (!options.configuration.extensions_enabled && options.configuration.option_J &&
         (source_flags & 0x2fdf) == 0x41 && !(source_flags & 0x8000));
    if (integer_constant_overflows(target_flags & 15, target_flags & 0x4000,
        source_unsigned, value)) WARN_CONSTANT_TRUNCATION(operation);
    if (target_width == 2) {
        if (target_flags & 0x4000) return value & 0xffff;
        return (unsigned)(short)value;
    }
    if (target_width == 4) return value;
    if ((target_flags & 0x4000) ||
        (options.configuration.option_J && !(target_flags & 0x8000))) return value & 0xff;
    return (unsigned)(signed char)value;
}

/* Native 0xa3860/0xa3878/0xa3890: range by byte width. The unused
   width-zero/three/five slots are zero, as in the encountered six-slot tables. */
const unsigned long unsigned_integer_limits[6] = { 0, UCHAR_MAX, USHRT_MAX, 0, ULONG_MAX, 0 };
const long signed_integer_maxima[6] = { 0, SCHAR_MAX, SHRT_MAX, 0, LONG_MAX, 0 };
const long signed_integer_minima[6] = { 0, SCHAR_MIN, SHRT_MIN, 0, LONG_MIN, 0 };

/* Original 0x74a60..0x74af5. Preserve the native unsigned-range branch:
   a negative signed source is tested by its magnitude when the target is
   unsigned, and unsigned sources use the unsigned limit even for signed targets. */
int __stdcall integer_constant_overflows(unsigned short width, short target_unsigned,
    short source_unsigned, unsigned value)
{
    if (target_unsigned) {
        if (!source_unsigned && (int)value < 0) value = 0U - value;
        return unsigned_integer_limits[width] < value;
    }
    if (source_unsigned) return unsigned_integer_limits[width] < value;
    if (signed_integer_maxima[width] < (long)value) return 1;
    if ((long)value < signed_integer_minima[width]) return 1;
    return 0;
}

IrToken *__stdcall try_user_defined_conversion(CType *type, IrToken *expression, int convert_result);
IrToken *__stdcall convert_member_pointer_initializer(CType *type, IrToken *expression);
IrToken *__stdcall try_class_pointer_initializer(CType *type, IrToken *expression);

/* Original 0x65ce0..0x65db0. Prefer user-defined and class/member-pointer
   conversions before based-address handling, numeric folding or a cast node.
   Keep the expression's original type across the first conversion attempt. */
IrToken *__stdcall convert_scalar_initializer(unsigned char operation, CType *type,
    IrToken *expression)
{
    CType *source;
    IrToken *result;
    if (!expression) return 0;
    source = expression->type;
    result = try_user_defined_conversion(type, expression, 1);
    if (result) return result;
    if (type->related && (type->related->flags & 0x30) &&
        source->related && (source->related->flags & 0x30))
        return convert_member_pointer_initializer(type, expression);
    result = try_class_pointer_initializer(type, expression);
    if (result) return result;
    if ((token_allocation_flags & 0x800) && type->related &&
        (type->related->attributes & 12))
        return qualify_based_storage(convert_based_expression(type, expression));
    if ((expression->flags & 7) == 3 && (!(type->flags & 0x800) || type->related))
        return convert_numeric_constant(operation, type, expression);
    if (operation != 0x6e && operation != 1)
        check_conversion_warning(operation, type, expression);
    return new_cast_expression(type, expression);
}

void __stdcall check_conversion_access(CppSymbol *conversion, CType *object_type);
IrToken *__stdcall invoke_user_conversion(CppSymbol *conversion, IrToken *expression, CType *type);
int __stdcall can_bind_conversion_reference(CType *reference, CType *value);
int __stdcall find_constructor_result_reference(IrToken *call, IrToken **reference);
void __stdcall cancel_temporary_cleanup(CppSymbol *symbol);

/* Original 0x65af0..0x65c40. Try constructors or conversion functions
   only for class/reference operands. Retain reference binding exceptions and
   cancel a result temporary's cleanup when probing without a final cast. */
IrToken *__stdcall try_user_defined_conversion(CType *type, IrToken *expression,
    int convert_result)
{
    CType *source = expression->type, *referent;
    CppSymbol *conversion;
    IrToken *result, *temporary;
    unsigned rank;
    if (!is_class_or_reference_type(source) && !is_class_or_reference_type(type))
        return 0;
    conversion = find_user_conversion(type, source, &rank);
    if (rank & 0x2000) return 0;
    check_conversion_access(conversion,
        (conversion->declaration_flags & 0x1000) ? type : source);
    result = invoke_user_conversion(conversion, expression, type);
    if (!result) return 0;
    if (result->type == type) return result;
    if (type->related && (type->related->flags & 0x40)) {
        if (result->flags & 0x10) return result;
        referent = dereference_pointer_type(type);
        if (is_unqualified_class_type(referent) && (type_qualifiers(referent) & 0x1000) &&
            can_bind_conversion_reference(type, result->type)) return result;
    }
    if (convert_result) return convert_operand_if_needed(1, type, result);
    if ((result->flags & 7) == 1 && result->value.pointer &&
        find_constructor_result_reference(result->value.pointer, &temporary) &&
        temporary && (temporary->flags & 7) == 4)
        cancel_temporary_cleanup((CppSymbol *)temporary->value.pointer);
    return 0;
}

IrToken *__stdcall convert_class_pointer(IrToken *expression, CType *type,
    CType *source, QualifiedDeclarator *path);

/* Original 0x66a80..0x66b47. Class pointer/reference conversions require
   a single outer indirection. A class value can first become an address when
   binding a reference; pass that address's resulting type to the converter. */
IrToken *__stdcall try_class_pointer_initializer(CType *type, IrToken *expression)
{
    CType *source = expression->type;
    TypeShape *target_shape, *source_shape;
    unsigned short flags;
    if (!(type->flags & 0x400)) return 0;
    target_shape = type->related;
    if (!target_shape || target_shape->related) return 0;
    flags = target_shape->flags;
    if ((flags & 0x77) != 1 && !(flags & 0x40)) return 0;
    if (!(source->flags & 0x400)) return 0;
    source_shape = source->related;
    if (!source_shape) {
        if (!(flags & 0x40)) return 0;
        expression = builtin_unary_expression(0x3a, expression);
        return convert_class_pointer(expression, type, expression->type, 0);
    }
    if (source_shape->related) return 0;
    if ((source_shape->flags & 0x77) != 1 && !(source_shape->flags & 0x40)) return 0;
    return convert_class_pointer(expression, type, source, 0);
}

IrToken *__stdcall make_null_member_pointer(CType *type);
IrToken *__stdcall convert_reference_initializer(CType *type, IrToken *expression);

/* Original 0x662e0..0x66488. Declarator-shaped initialization retains
   its null-pointer and reference paths before selecting based/scalar/cast
   conversion. Save the original target flag/attribute words across helpers. */
IrToken *__stdcall convert_derived_initializer(CType *type, IrToken *expression)
{
    unsigned short flags = type->flags, shape_flags, attributes;
    TypeShape *shape = type->related, *source_shape;
    CType *source;
    TypeDescription *description;
    unsigned short pointer_attributes;
    int same_base;
    if (shape) {
        if ((expression->flags & 0x80) && (shape->flags & 0x30) &&
            (!expression->type || !expression->type->related ||
             !(expression->type->related->flags & 0x30)))
            return make_null_member_pointer(type);
        shape_flags = shape->flags;
        if (!(shape_flags & 0x40) && (expression->flags & 0x80)) {
            if ((shape->attributes & 7) >= 5) {
                description = copy_type_description(type);
                pointer_attributes = normalize_type_attributes(1, 0);
                expression->type = replace_first_type_shape((CType *)description, 1,
                    pointer_attributes, 0);
                --type_description_depth;
            } else expression->type = type;
            expression->flags &= ~0x80;
            return expression;
        }
        attributes = shape->attributes;
    } else {
        shape_flags = 0;
        attributes = 0;
    }
    source = expression->type;
    source_shape = source->related;
    same_base = !((source->flags ^ type->flags) & 0xefff) &&
        source->field08 == type->field08;
    if ((shape_flags & 0x77) == 0x40)
        return convert_reference_initializer(type, expression);
    if (!source_shape) return convert_scalar_initializer(1, type, expression);
    if ((flags & 0x800) && !shape)
        return new_typed_unary_expression(0x1a, type, expression);
    if (attributes & 12)
        return qualify_based_storage(convert_based_expression(type, expression));
    if (!(shape_flags & 0x30) && !(source_shape->flags & 0x30) &&
        ((flags & 0x80) || same_base))
        return new_cast_expression(type, expression);
    return convert_scalar_initializer(1, type, expression);
}

int __stdcall qualified_path_has_virtual_base(QualifiedDeclarator *path);

/* Original 0x66b50..0x66d29. Equal class-name identities may share a
   representation, subject to pointer-model and cv-cast rules. Other classes
   search derived-to-base first, then base-to-derived; reverse conversion through
   a virtual base is diagnosed before the qualified object adjustment. */
IrToken *__stdcall convert_class_pointer(IrToken *expression, CType *type,
    CType *source, QualifiedDeclarator *path)
{
    int derived_to_base;
    if (expression->flags & 0x80) return expression;
    if (((CppSymbol *)cpp_context_stack->items[type->field08])->name ==
        ((CppSymbol *)cpp_context_stack->items[source->field08])->name) {
        if ((source->related->attributes ^ type->related->attributes) & 7)
            return new_cast_expression(type, expression);
        if (((expression->type->attributes ^ type->attributes) & 0x3000) &&
            (cpp_parser_flags & 2)) return new_cast_expression(type, expression);
        expression->type = type;
        return expression;
    }
    derived_to_base = 0;
    if (!path) {
        path = lookup_class_name(cpp_context_stack->items[source->field08],
            ((CppSymbol *)cpp_context_stack->items[type->field08])->name, 1, 386);
        if (path) derived_to_base = 1;
        else {
            path = lookup_class_name(cpp_context_stack->items[type->field08],
                ((CppSymbol *)cpp_context_stack->items[source->field08])->name, 1, 386);
            if (!path) {
                report_syntax_error(519,
                    ((CppSymbol *)cpp_context_stack->items[source->field08])->name,
                    ((CppSymbol *)cpp_context_stack->items[type->field08])->name);
                return expression;
            }
        }
    }
    if (!derived_to_base && qualified_path_has_virtual_base(path)) {
        report_syntax_error(635,
            ((CppSymbol *)cpp_context_stack->items[source->field08])->name,
            ((CppSymbol *)cpp_context_stack->items[type->field08])->name);
        return expression;
    }
    return adjust_qualified_member_object(expression, path, 1, derived_to_base, type);
}

/* Original 0x66080..0x66093. Keep this cast wrapper's own stdcall
   entry; allocation and node initialization remain in the typed unary helper. */
IrToken *__stdcall new_cast_expression(CType *type, IrToken *expression)
{
    return new_typed_unary_expression(0x6e, type, expression);
}
