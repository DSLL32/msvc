/* Constant conversion across scalar widths, pointer models and x87 values. */
#include "numeric-conversion.h"
#include "lexer.h"
#include <limits.h>

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
