/* Assignment and type compatibility diagnostics. */
#include "parser.h"
#include "expressions.h"
#include "initializer-records.h"
#include "declaration-text.h"
#include <recon.h>


/* Original 0x3e760..0x3ef74, void-pointer dispatch 0x3ef74..0x3efc0. */
int __stdcall check_type_conversion(unsigned char operation, CType *target, CType *source)
{
    TypeShape *left, *right, *other;
    unsigned short target_flags, source_flags, left_kind, right_kind;
    unsigned short left_attributes, right_attributes, left_inner, right_inner;
    int target_void, source_void, target_void_pointer, source_void_pointer;
    int first = 1, failed = 0, comparison, index, count, source_count;
    ParameterInfo *left_parameters, *right_parameters;
    /* The original leaves these cursors unspecified until it sees two
       parameter descriptors. Preserve that distinction for incomplete types. */
    CType **left_parameter, **right_parameter;
    if (target == source) return 1;
    target_flags = target->flags;
    source_flags = source->flags;
    left = target->related;
    if (left && !(left->flags & 7)) left = left->related;
    right = source->related;
    if (right && !(right->flags & 7)) right = right->related;
    target_void = target_flags & 0x80;
    source_void = source_flags & 0x80;
    target_void_pointer = target_void && left && (left->flags & 7) == 1 && !left->related;
    source_void_pointer = source_void && right && (right->flags & 7) == 1 && !right->related;
    if (target_void_pointer != source_void_pointer) {
        other = target_void_pointer ? right : left;
        left_kind = other ? other->flags & 7 : 0;
        if (left_kind == 1 || left_kind == 2 || left_kind == 3) {
            switch (operation) {
            case TOK_ASSIGN: case TOK_ARGUMENT_END: case TOK_INITIALIZING: case TOK_RETURN:
                if (left_kind == 3) warn_void_function_pointer();
                if (!check_conversion_attributes(operation, target->attributes, source->attributes, 1)) return 0;
                break;
            case TOK_EQUAL: case TOK_NOT_EQUAL: break;
            case TOK_GREATER: case TOK_GREATER_EQUAL: case TOK_LESS: case TOK_LESS_EQUAL: case TOK_MINUS:
                conversion_diagnostic(133, 3, operation, source, target);
                return 1;
            default: return 1;
            }
            if (scanner_flags & 0x800) check_based_pointer_conversion(operation, left, right);
            return 1;
        }
    }
    if (left == right) {
        if (left) {
            if (!check_conversion_attributes(operation, target->attributes, source->attributes, 1)) return 0;
            goto base_types;
        }
        if ((target_flags & 0x140) && (source_flags & 0x340)) return 1;
        if (target->field08 != source->field08 &&
            !(operation == TOK_COLON && (target_flags & 0x200) && (source_flags & 0x200))) {
            report_syntax_error(115, token_names[operation]);
            return 0;
        }
        return 1;
    }
    while (left && right) {
        left_kind = left->flags;
        right_kind = right->flags;
        left_attributes = left->attributes;
        right_attributes = right->attributes;
        left_inner = left->related ? left->related->attributes : target->attributes;
        right_inner = right->related ? right->related->attributes : source->attributes;
        if ((left_kind & 7) == 2) left_inner |= left_attributes & 0x3000;
        if ((right_kind & 7) == 2) right_inner |= right_attributes & 0x3000;
        if (first) {
            if ((left_kind & 7) == 2) left_kind = 1;
            if ((right_kind & 7) == 2) right_kind = 1;
        }
        if ((left_kind ^ right_kind) & 7) {
            report_conversion_mismatch(operation, target, source);
            return 0;
        }
        left_kind &= 7;
        if (left_kind == 3) {
            if (!compatible_shape_attributes(left_attributes, right_attributes)) {
                report_syntax_error(152, token_names[operation]);
                return 0;
            }
            if ((left_attributes ^ right_attributes) & 0xf00)
                conversion_diagnostic(50, 4, operation, target, source);
        }
        if (scanner_flags & 0x800) check_based_pointer_conversion(operation, left, right);
        comparison = check_conversion_attributes(operation, left_inner, right_inner, first);
        if (!comparison) return 0;
        if (comparison == 2 && left_kind != 3 &&
            !(left_kind == 2 && (right_kind & 7) == 2) &&
            !compatible_shape_attributes(left_attributes, right_attributes)) {
            conversion_diagnostic(47, 1, operation, target, source);
            return 0;
        }
        switch (left_kind) {
        case 2:
            if (left->field08 != right->field08) {
                conversion_diagnostic(48, 1, operation, target, source);
                return 0;
            }
            break;
        case 4:
            left_parameters = (ParameterInfo *)left->field08;
            right_parameters = (ParameterInfo *)right->field08;
            if (left_parameters == right_parameters) break;
            if (left_parameters && right_parameters) {
                count = (int)left_parameters->count;
                source_count = (int)right_parameters->count;
                if (count != source_count) {
                    if (!(left->flags & 0x8000) && !(right->flags & 0x8000)) {
                        if (count || !options.extensions_enabled) conversion_diagnostic(113, 1, operation, source, target);
                        goto next_shape;
                    }
                    if (count > source_count) count = source_count;
                }
                left_parameter = left_parameters->types;
                right_parameter = right_parameters->types;
                for (index = 0; index < count; ++index, ++left_parameter, ++right_parameter) {
                    if (*left_parameter == *right_parameter) continue;
                    if (!*left_parameter || !*right_parameter) break;
                    if (first) {
                        if (!check_type_conversion(TOK_ARGUMENT_END, *left_parameter, *right_parameter)) failed = 1;
                    } else {
                        comparison = compare_prototype_parameter_types(*left_parameter, *right_parameter);
                        if (comparison == 1 || (comparison == 2 && !options.extensions_enabled)) {
                            compiler_warning(1, 28, index + 1);
                            failed = 1;
                        }
                    }
                }
            } else if (!(left->flags & 0x4000) && !(right->flags & 0x4000)) break;
            if (!failed && options.extensions_enabled &&
                ((left_parameter && !*left_parameter) != (right_parameter && !*right_parameter)))
                compiler_warning(4, 220);
            break;
        }
next_shape:
        first = 0;
        left = left->related;
        right = right->related;
        if (left == right) left = right = 0;
    }
    if (left != right) {
        if (first && ((source_void && !right) || (target_void && !left))) {
            report_syntax_error(120);
            return 0;
        }
        if ((!target->related && !(target_flags & 0x40)) ||
            (!source->related && !(source_flags & 0x40)))
            report_syntax_error(115, token_names[operation]);
        else report_conversion_mismatch(operation, target, source);
        return 0;
    }
base_types:
    if (target_flags == source_flags && target->field08 == source->field08) return !failed;
    if (options.extensions_enabled && !((target_flags ^ source_flags) & 0x2fdf) && target->field08 == source->field08) {
        conversion_diagnostic(57, 4, operation, target, source);
        return !failed;
    }
    if ((target_flags & 0x2fdf) == 0x80) return 1;
    conversion_diagnostic(133, 1, operation, source, target);
    return !failed;
}

typedef struct IntegerBounds {
    unsigned unsigned_maximum;
    int signed_maximum, signed_minimum;
} IntegerBounds;

/* Bounds indexed by the target's byte width. Widths0,3,5 are reserved. */
IntegerBounds integer_bounds[6] = {
    { 0, 0, 0 }, { 255, 127, -128 }, { 65535, 32767, -32768 },
    { 0, 0, 0 }, { 0xffffffffUL, 0x7fffffff, (-2147483647 - 1) }, { 0, 0, 0 }
};

/* Original 0x4c2b0..0x4c345. The unsigned-source path uses the full
   width limit even for a signed destination, as the original does. */
int __stdcall integer_conversion_overflow(unsigned short bytes,
    unsigned short target_unsigned, unsigned short source_unsigned, unsigned value)
{
    IntegerBounds *bounds = &integer_bounds[bytes];
    if (target_unsigned) {
        if (!source_unsigned && (int)value < 0) value = 0 - value;
        return value > bounds->unsigned_maximum;
    }
    if (source_unsigned) return value > bounds->unsigned_maximum;
    return (int)value > bounds->signed_maximum || (int)value < bounds->signed_minimum;
}

/* Original 0x34e00..0x34e45. Permit bit 0x1000 to differ;
   the int/long spelling distinction 0x20 is accepted with a warning. */
int __stdcall compatible_base_type_flags(unsigned short left, unsigned short right)
{
    unsigned difference = (unsigned short)(left ^ right);
    if (!(difference & 0xefff)) return 1;
    if (difference & ~0x20UL) return 0;
    compiler_warning(1, 142);
    return 1;
}

/* Original 0x3f2e0..0x3f31f. Compare memory-model, interrupt and calling
   convention attributes; syntax-only mode accepts their differences. */
int __stdcall compatible_shape_attributes(unsigned short target, unsigned short source)
{
    unsigned short difference;
    if (options.prototypes_only) return 1;
    difference = target ^ source;
    if (difference & 0xf) return 0;
    if (difference & 0x100) return 0;
    return !(difference & 0x70);
}

/* Original 0x3c0f0..0x3c13b. Only small integer and enum scalar types
   require the default integer promotion; a declaration prefix is transparent. */
CType *__stdcall default_argument_promotion(CType *type)
{
    TypeShape *shape;
    if (!(type->flags & 0x240)) return 0;
    shape = type->related;
    if (shape && !(shape->flags & 7)) shape = shape->related;
    if (shape) return 0;
    if ((type->flags & 0x200) || (type->flags & 15) < 4) return type_0064;
    return 0;
}

/* Original 0x4c1a0..0x4c253. Compare against the target-width bound
   before adding, preserving the original zero-left signed-limit choice. */
int __stdcall addition_overflows(unsigned left, unsigned right,
    unsigned short is_unsigned, unsigned short bytes)
{
    IntegerBounds *bounds = &integer_bounds[bytes];
    unsigned threshold;
    if (is_unsigned) return bounds->unsigned_maximum - left < right;
    if (((int)left >= 0) != ((int)right >= 0)) return 0;
    if ((int)left <= 0) {
        threshold = (unsigned)bounds->signed_minimum - left;
        return (int)right < (int)threshold;
    }
    threshold = (unsigned)bounds->signed_maximum - left;
    return (int)right > (int)threshold;
}

/* Original 0x3e2e0..0x3e317. A previously diagnosed pointer-valued
   operand suppresses a second conversion diagnostic. */
void __stdcall check_binary_type_conversion(unsigned char operation, IrToken *left, IrToken *right)
{
    TypeShape *shape;
    if (!left || !right) return;
    if (left->flags & 0x80) {
        shape = right->type->related;
        if (shape && (shape->flags & 3)) return;
    }
    check_assignment_conversion(operation, left->type, right);
}

/* Original 0x3f140..0x3f23c. Assignment-like operations may add cv
   qualifiers, but warn and fail this comparison when they discard one. */
int __stdcall check_conversion_attributes(unsigned char operation,
    unsigned short target, unsigned short source, int first)
{
    int discarded = 0;
    switch (token_categories[operation]) {
    case TOK_ASSIGN: case TOK_ARGUMENT_END: case TOK_INITIALIZING: case TOK_RETURN:
        if (((target ^ source) & 0x3000) && (source & 0x3000)) {
            if (!(target & 0x2000) && (source & 0x2000)) {
                compiler_warning(1, 90, token_names[operation], token_names[TOK_VOLATILE]);
                discarded = 1;
            }
            if (!(target & 0x1000) && (source & 0x1000)) {
                compiler_warning(1, 90, token_names[operation], token_names[TOK_CONST]);
                discarded = 1;
            }
            if (discarded) return 0;
        }
        return first ? 1 : 2;
    case TOK_COLON: case TOK_EQUALITY_OPERATOR: case TOK_RELATIONAL_OPERATOR:
        return 1;
    default: return 2;
    }
}

/* Original 0x414e0..0x418c5. Validate an explicit cast, diagnosing
   incompatible aggregate classes and changes in pointer representation. */
IrToken *__stdcall convert_cast_expression(CType *target, IrToken *operand)
{
    CType *source;
    TypeShape *target_shape, *source_shape;
    unsigned classification;
    unsigned short target_kind, source_kind, target_attributes, source_attributes;
    unsigned short target_flags, source_flags;
    int source_bytes, target_bytes;
    target_shape = target->related;
    if (target_shape && !(target_shape->flags & 7)) {
        target = drop_first_type_shape(target);
        target_shape = target->related;
    }
    target_kind = target_attributes = 0;
    if (target_shape) {
        target_kind = target_shape->flags;
        if ((target_kind & 7) == 4) {
            report_syntax_error(66);
            return 0;
        }
        if ((target_kind & 7) == 2) {
            report_syntax_error(67);
            return 0;
        }
        target_attributes = target_shape->attributes;
    }
    source = operand->type;
    classification = classify_expression_conversion(target, operand, 0, 0);
    if (classification < 0x400) return convert_scalar_expression(TOK_CAST, target, operand);
    if (classification & 0x1000) {
        report_type_syntax_error(585, target);
        return 0;
    }
    target_flags = target->flags;
    source_flags = source->flags;
    source_shape = source->related;
    source_kind = source_attributes = 0;
    if (source_shape) {
        source_kind = source_shape->flags;
        source_attributes = source_shape->attributes;
        if ((source_kind & 7) == 2) source_kind = (source_kind & ~6) | 1;
    }
    if (target == type_0080) goto finish;
    if ((source_flags & 0x80) && !source_shape) {
        report_syntax_error(69);
        goto finish;
    }
    if (source_kind) {
        if (!target_kind) {
            if (target_flags & 0x800)
                return make_typed_unary_expression(TOK_SEGMENT, target, operand);
            if (!(target_flags & 0x40))
                conversion_diagnostic(440, 0, TOK_CAST, source, target);
            if (pointer_type_size(source_attributes) > (target_flags & 15))
                conversion_diagnostic(305, 4, TOK_CAST, source, target);
            goto finish;
        }
        source_kind &= 7;
        target_kind &= 7;
        if (source_kind == 3) {
            if (target_kind == 3) goto compare_sizes;
            if (target_kind == 1) {
                conversion_diagnostic(54, options.extensions_enabled ? 4 : 1,
                    TOK_CAST, source, target);
                goto compare_sizes;
            }
        } else if (source_kind == 1) {
            if (target_kind == 3) {
                conversion_diagnostic(55, 4, TOK_CAST, source, target);
                goto compare_sizes;
            }
            if (target_kind == 1) {
                source_bytes = pointer_type_size(source_attributes);
                target_bytes = pointer_type_size(target_attributes);
                if (source_bytes > target_bytes)
                    conversion_diagnostic(305, 4, TOK_CAST, source, target);
                else if (source_bytes < target_bytes && (target_attributes & 7) == 5 && (operand->flags & 7) == 5)
                    compiler_warning(1, 120);
            }
        }
        goto finish;
    }
    if (target_kind) {
        if (source_flags & 0x40) {
            target_bytes = pointer_type_size(target_attributes);
            source_bytes = source_flags & 15;
            if (source_bytes > target_bytes)
                conversion_diagnostic(305, 4, TOK_CAST, source, target);
            else if (source_bytes < target_bytes)
                conversion_diagnostic(306, 4, TOK_CAST, source, target);
            goto finish;
        }
    } else if (!(target_flags & 0x400) && !(source_flags & 0x400)) goto finish;
    conversion_diagnostic(440, 0, TOK_CAST, source, target);
    return 0;
compare_sizes:
    source_bytes = pointer_type_size(source_attributes);
    target_bytes = pointer_type_size(target_attributes);
    if (source_bytes > target_bytes)
        conversion_diagnostic(305, 4, TOK_CAST, source, target);
finish:
    return finish_scalar_cast(TOK_CAST, target, operand);
}

/* Original 0x413d0..0x41485. Null constants convert directly to pointer
   targets; array operands decay before the common type comparison. */
unsigned __stdcall classify_expression_conversion(CType *target, IrToken *operand,
    unsigned mode, unsigned flags)
{
    CType *source;
    if (!operand) return 0x2000;
    if (!target) return 0x400;
    source = operand->type;
    if (!source) return 0x2000;
    if (is_record_type(source) || is_record_type(target)) return 0x2000;
    if ((operand->flags & 0x80) && target->related) return 1;
    if (target->related && (target->related->flags & 7) == 1 &&
        source->related && (source->related->flags & 7) == 2)
        source = decay_array_expression_type(operand);
    return classify_type_conversion(target, source, mode, flags);
}

/* Original 0x41ff0..0x420bf. Equal scalar representations reuse the
   operand, preserving qualifiers and clearing the null-constant marker. */
IrToken *__stdcall convert_scalar_expression(unsigned char operation, CType *target, IrToken *operand)
{
    CType *source;
    TypeDescription *description;
    unsigned short target_qualifiers, source_qualifiers;
    if (!operand) return 0;
    source = remove_type_prefix(operand->type);
    target = remove_type_prefix(target);
    if (!target->related && !source->related) {
        if (source == target) {
            operand->flags &= ~0x80;
            return operand;
        }
        if (!((source->flags ^ target->flags) & 0xefdf) && source->field08 == target->field08) {
            target_qualifiers = target->attributes & 0x3000;
            source_qualifiers = source->attributes & 0x3000;
            if ((source_qualifiers | target_qualifiers) != target_qualifiers) {
                description = copy_type_description(target);
                description->attributes |= source_qualifiers;
                target = intern_type(description);
                --type_description_depth;
            }
            operand->type = target;
            operand->flags &= ~0x80;
            return operand;
        }
        return finish_scalar_cast(operation, target, operand);
    }
    if (source == target) return operand;
    return convert_pointer_expression(target, operand);
}

/* Original 0x41eb0..0x41fed. Null pointers acquire the target's pointer
   type directly; other values retain the original representation helpers. */
IrToken *__stdcall convert_pointer_expression(CType *target, IrToken *operand)
{
    CType *source;
    TypeDescription *description;
    TypeShape *target_shape, *source_shape;
    unsigned short target_flags, target_attributes, attributes;
    int same_base;
    target_flags = target->flags;
    target_shape = target->related;
    if (target_shape && (operand->flags & 0x80)) {
        if ((target_shape->attributes & 7) >= 5) {
            description = copy_type_description(target);
            attributes = normalize_type_attributes(1, 0);
            operand->type = replace_outer_type_shape(description, 1, attributes, 0);
            --type_description_depth;
        } else operand->type = target;
        source = operand->type;
        source_shape = source->related;
        if (!(source->flags & 0x80) || !source_shape ||
            (source_shape->flags & 7) != 1 || source_shape->related)
            operand->flags &= ~0x80;
        return operand;
    }
    target_attributes = target_shape ? target_shape->attributes : 0;
    source = operand->type;
    source_shape = source->related;
    same_base = !((source->flags ^ target->flags) & 0xefdf) &&
        source->field08 == target->field08;
    if (!source_shape) return finish_scalar_cast(1, target, operand);
    if ((target_flags & 0x800) && !target_shape)
        return make_typed_unary_expression(TOK_SEGMENT, target, operand);
    if (target_attributes & 12)
        return normalize_based_expression(convert_based_pointer(target, operand));
    if (!(target_flags & 0x80) && !same_base)
        return finish_scalar_cast(1, target, operand);
    return convert_pointer_representation(target, operand);
}

/* Original 0x41180..0x413c2. Classification values retain the original
   ordering: 1 equivalent, 0x10 promotion, 0x20 conversion, 0x2000 invalid. */
unsigned __stdcall classify_type_conversion(CType *target, CType *source,
    unsigned mode, unsigned flags)
{
    unsigned result;
    unsigned short target_flags, source_flags, source_kind;
    target = remove_type_prefix(target);
    source = remove_type_prefix(source);
    if (target == source) return 1;
    result = classify_pointer_conversion(target, source, mode, flags);
    if (result) return result;
    target_flags = target->flags;
    source_flags = source->flags;
    if ((!((target_flags & 0x140) || (target_flags & 0x880) || target->related)) ||
        (!(source_flags & 0x9c0) && !source->related)) {
        if ((target_flags & 0x200) && (source_flags & 0x200))
            return target->field08 == source->field08 ? 1 : 0x2000;
        if (!(target_flags & 0x200) && (source_flags & 0x200) && (target_flags & 0x140))
            return (target_flags & 0x20) && (target_flags & 0x40) ? 0x10 : 0x20;
        if (!((source_flags ^ target_flags) & 0xefdf) && source->field08 == target->field08)
            return 1;
        return 0x2000;
    }
    if (target_flags == source_flags) return 1;
    if (source_flags & 0x80) return 0x2000;
    if ((source_flags & 0x40) && (target_flags & 0x40)) {
        if (!((source_flags ^ target_flags) & 0xefff)) return 1;
        source_kind = source_flags & 0x2fdf;
        if ((source_kind == 0x41 || source_kind == 0x42) &&
            (target_flags & 0x20) && (target_flags & 0x40) && !(target_flags & 0x4000))
            return 0x10;
        return 0x20;
    }
    if ((source_flags & 0x100) && (target_flags & 0x100)) {
        if (!((source_flags ^ target_flags) & 15) &&
            !!(source_flags & 0x20) == !!(target_flags & 0x20)) return 1;
        if ((source_flags & 15) < (target_flags & 15) && (target_flags & 15) <= 8)
            return 0x10;
        return 0x20;
    }
    if (target_flags & 0x800) return source_flags & 0x800 ? 1 : 0x2000;
    return 0x20;
}

/* Original 0x40e60..0x41178. Compare pointer shape, qualification and
   address model, retaining separate function and array shape rules. */
unsigned __stdcall classify_pointer_conversion(CType *target, CType *source,
    unsigned mode, unsigned flags)
{
    TypeShape *left = target->related, *right = source->related, *nested;
    unsigned result = 1;
    unsigned short left_kind, right_kind, source_qualifiers, difference;
    if (!left && !right) return 0;
    if (!left) return 0x2000;
    left_kind = left->flags;
    if ((target->flags & 0x80) && (left_kind & 7) != 4)
        result = classify_void_pointer_conversion(result, left, right);
    if (!right) return 0x2000;
    right_kind = right->flags;
    if (result != 0x100 && right->related != left->related) return 0x2000;
    if (left->related) {
        if ((target->attributes ^ source->attributes) & 0x3000) return 0x2000;
    } else {
        source_qualifiers = source->attributes;
        if (result == 0x100 && source->related && source->related->related) {
            nested = source->related->related;
            source_qualifiers = nested->attributes;
        }
        result = classify_pointer_qualifiers(result, target->attributes, source_qualifiers);
        if (result & 0x2000) {
            if (!(parser_flags & 2) || (mode & 1)) return 0x2000;
            result &= 0x1fff;
            if (result != 5 && result != 6) return 0x2000;
            result = 1;
        }
    }
    result = classify_pointer_model(result, left->attributes, right->attributes, right);
    if (result & 0x2000) return 0x2000;
    if ((left_kind ^ right_kind) & 7) {
        left_kind &= 7;
        right_kind &= 7;
        if ((left_kind == 1 && right_kind == 2) ||
            (left_kind == 2 && right_kind == 1))
            result = result == 1 ? 2 : result | 2;
        else return 0x2000;
    } else {
        left_kind &= 7;
        if (left_kind == 2 && left->field08 && left->field08 != right->field08)
            return 0x2000;
    }
    if ((left_kind == 3 || left_kind == 4) &&
        ((right_kind & 7) == 3 || (right_kind & 7) == 4)) {
        if (left_kind == 3) left = left->related;
        if ((right_kind & 7) == 3) right = right->related;
        if (left->field08 != right->field08) return 0x2000;
        difference = left->attributes ^ right->attributes;
        if ((difference & 0xf0ff) || (difference & 0x100)) return 0x2000;
    }
    if ((!((source->flags ^ target->flags) & 0xefdf) && source->field08 == target->field08) ||
        (result & 0x100)) return result;
    return 0x2000;
}

/* Original 0x41910..0x41996. Fold constant operands when the target
   permits it, otherwise retain conversion checks and representation nodes. */
IrToken *__stdcall finish_scalar_cast(unsigned char operation, CType *target, IrToken *operand)
{
    if (!operand) return 0;
    if ((scanner_flags & 0x800) && target->related && (target->related->attributes & 12))
        return normalize_based_expression(convert_based_pointer(target, operand));
    if ((operand->flags & 7) == 3 && (!(target->flags & 0x800) || target->related))
        return convert_expression(operation, target, operand);
    if (operation != TOK_CAST && operation != 1)
        check_assignment_conversion(operation, target, operand);
    return convert_pointer_representation(target, operand);
}

/* Original 0x419a0..0x41c66. Convert constant payloads in place, retaining
   the compiler's twelve-byte extended representation until final rounding. */
IrToken *__stdcall convert_expression(unsigned char operation, CType *target, IrToken *operand)
{
    CType *source;
    TypeShape *shape;
    TypeDescription *description;
    unsigned short target_flags, source_flags, target_shape, source_shape;
    unsigned short target_attributes, source_attributes;
    Extended number;
    double double_value;
    float float_value;
    int integer_value, failed, is_unsigned;
    source = operand->type;
    if ((source->flags & 0x200) && !(target->flags & 0x200)) {
        operand->type = type_0064;
        source = operand->type;
    }
    target_flags = target->flags;
    source_flags = source->flags;
    shape = target->related;
    target_shape = shape ? shape->flags : 0;
    target_attributes = shape ? shape->attributes : 0;
    shape = source->related;
    source_shape = shape ? shape->flags : 0;
    source_attributes = shape ? shape->attributes : 0;
    if ((source_flags & 0x40) || source_shape) {
        if ((target_flags & 0x40) || target_shape) {
            if (source_shape && (target_attributes & 7) == 2 &&
                (source_attributes & 7) != 2 && !(operand->flags & 0x80))
                return convert_pointer_representation(target, operand);
            operand->value.integer = convert_integer_constant(operation, operand,
                target_shape, target_attributes, target_flags);
            if ((target->flags & 0x4000) && !(operand->type->flags & 0x4000)) {
                description = copy_type_description(operand->type);
                operand->type = description;
                description->flags |= 0x4000;
                operand->type = intern_type(description);
                --type_description_depth;
            }
        } else if (target_flags & 0x100) {
            if (source_flags & 0x4000)
                failed = unsigned_to_extended(&operand->value.integer, &number);
            else failed = integer_to_extended((int *)&operand->value.integer, &number);
            if (failed) compiler_warning(2, 56);
        } else goto finish;
    } else if (source_flags & 0x100) {
        if (target_flags & 0x40) {
            if (extended_to_integer(operand->value.pointer, &integer_value)) compiler_warning(2, 56);
            is_unsigned = (target_flags & 0x4000) ||
                (!options.extensions_enabled && (target_flags & 0x2fdf) == 0x41 &&
                 options.option_J && !(target_flags & 0x8000));
            if ((target_flags & 0x2fdf) == 0x44)
                operand->kind = is_unsigned ? TOK_ULONG_CONSTANT : TOK_LONG_CONSTANT;
            else operand->kind = is_unsigned ? TOK_UINT_CONSTANT : TOK_INT_CONSTANT;
            operand->value.integer = integer_value;
        } else if (target_flags & 0x100) {
            if (!target_shape) number = *(Extended *)operand->value.pointer;
        } else goto finish;
    }
    if ((target_flags & 0x100) && !target_shape) {
        if (target_flags == 0x108) {
            if (normalized_Op) {
                extended_to_double(&number, &double_value);
                double_to_extended(&double_value, &number);
            }
            number.format = 8;
            operand->kind = TOK_DOUBLE_CONSTANT;
        } else if (target_flags == 0x104) {
            if (normalized_Op) {
                extended_to_float(&number, &float_value);
                float_to_extended(&float_value, &number);
            }
            number.format = 4;
            operand->kind = TOK_FLOAT_CONSTANT;
        } else {
            number.format = 8;
            operand->kind = TOK_LONG_DOUBLE_CONSTANT;
        }
        operand->value.pointer = save_extended_constant(&number);
    }
finish:
    operand->type = target;
    return operand;
}

/* Original 0x40cd0..0x40d07. A single ordinary pointer to void permits
   conversion across base types without requiring identical nested shapes. */
unsigned __stdcall classify_void_pointer_conversion(unsigned result, TypeShape *target, TypeShape *source)
{
    if (source && !target->related && (target->flags & 7) == 1) return 0x100;
    return result;
}

/* Original 0x41c70..0x41c84. Retain the separate representation-cast
   entry and its ordinary typed unary expression node. */
IrToken *__stdcall convert_pointer_representation(CType *target, IrToken *operand)
{
    return make_typed_unary_expression(TOK_CAST, target, operand);
}

/* Original 0x40e00..0x40e5b. Dropping const or volatile is diagnosed;
   adding a qualifier preserves the conversion's compatible-kind marker. */
unsigned __stdcall classify_pointer_qualifiers(unsigned result, unsigned short target, unsigned short source)
{
    if (target == source) return result;
    if (target & 0x1000) {
        if (!(source & 0x1000)) result |= 8;
    } else if (source & 0x1000) return 0x2005;
    if (target & 0x2000) {
        if (!(source & 0x2000)) result |= 8;
    } else if (source & 0x2000) return 0x2006;
    return result;
}

/* Original 0x40d10..0x40df9. Classify address-model changes separately
   from the pointed-to type, including the type of a based pointer's base. */
unsigned __stdcall classify_pointer_model(unsigned result, unsigned short target,
    unsigned short source, TypeShape *shape)
{
    TypeShape *base;
    if (!!(target & 12) != !!(source & 12)) {
        if ((target & 4) || (source & 7) == 4) return 0x2000;
        if ((source & 12) && is_pointer_based_shape(shape)) {
            base = ((FrontendSymbol *)shape->field08)->type->related;
            if (base && !(base->flags & 7)) base = base->related;
            if ((base->attributes & 7) == 1)
                return result | ((target & 7) == 1 ? 0x10 : 0x80);
        }
        return result | ((target & 7) == 1 ? 0x80 : 0x10);
    }
    if ((source ^ target) & 7)
        return result | ((target & 7) == 1 ? 0x80 : 0x10);
    return result;
}

/* Original 0x4c080..0x4c191. Divide the width's range by one operand
   before testing the other, so the multiplication itself cannot wrap. */
int __stdcall multiplication_overflows(unsigned left, unsigned right,
    unsigned short is_unsigned, unsigned short bytes)
{
    IntegerBounds *bounds = &integer_bounds[bytes];
    unsigned magnitude_left, magnitude_right, limit;
    if (is_unsigned) {
        if (!left) return 0;
        return bounds->unsigned_maximum / left < right;
    }
    magnitude_left = (int)left < 0 ? 0U - left : left;
    magnitude_right = (int)right < 0 ? 0U - right : right;
    if (((int)left > 0) != ((int)right > 0)) {
        /* The original's negative-limit cases are defined for the three
           integer widths; other width values leave this local unspecified. */
        switch (bytes) {
        case 1: limit = (unsigned char)(0U - (unsigned)bounds->signed_minimum); break;
        case 2: limit = (unsigned short)(0U - (unsigned)bounds->signed_minimum); break;
        case 4: limit = 0U - (unsigned)bounds->signed_minimum; break;
        }
    } else limit = bounds->signed_maximum;
    if (!magnitude_left) return 0;
    return limit / magnitude_left < magnitude_right;
}

/* Original 0x3efc0..0x3f138, RET8. Nested function parameter types
   distinguish exact compatibility, mismatch and the void-pointer extension. */
int __stdcall compare_prototype_parameter_types(CType *target, CType *source)
{
    TypeShape *left = target->related, *right = source->related, *other;
    int target_void, source_void, comparison;
    unsigned kind;
    if (target == source) return 0;
    if (right == left && !((source->flags ^ target->flags) & 0x2fdf) &&
        source->field08 == target->field08) comparison = 0;
    else {
        target_void = (target->flags & 0x80) && left &&
                      (left->flags & 7) == 1 && !left->related;
        source_void = (source->flags & 0x80) && right &&
                      (right->flags & 7) == 1 && !right->related;
        if (!target_void && !source_void) return 1;
        if (target_void && source_void) comparison = 0;
        else {
            other = target_void ? right : left;
            if (!other) return 1;
            comparison = 2;
            kind = other->flags & 7;
            if (kind == 3) warn_void_function_pointer();
            else if (kind != 1 && kind != 2) return 1;
        }
        if ((right->attributes ^ left->attributes) & 15) return 1;
    }
    if (!check_conversion_attributes(TOK_ARGUMENT_END, source->attributes,
                                     target->attributes, 1)) return 1;
    if (left && right && (scanner_flags & 0x800))
        check_based_pointer_conversion(TOK_ARGUMENT_END, left, right);
    return comparison;
}

/* Original 0x3e730..0x3e759. Function/data pointer conversion is a
   level-four extension warning, or a level-one warning in strict mode. */
void warn_void_function_pointer(void)
{
    if (options.extensions_enabled) compiler_warning(4, 152);
    else compiler_warning(1, 153);
}

/* Original 0x3c380..0x3c414, RET20. Keep both rendered declarators
   alive until the diagnostic has consumed them, then release the text arena. */
void __stdcall conversion_diagnostic(unsigned code, unsigned level,
    unsigned char operation, CType *target, CType *source)
{
    char *target_name, *source_name;
    begin_declaration_text();
    target_name = target ? declaration_text_value(build_declaration_text(0, target))
                         : message_text(345);
    source_name = source ? declaration_text_value(build_declaration_text(0, source))
                         : message_text(345);
    if (level)
        compiler_warning(level, code, token_names[operation], target_name, source_name);
    else
        report_syntax_error(code, token_names[operation], target_name, source_name);
    end_declaration_text();
}

/* Original 0x3c1b0..0x3c1e2, RET12. Render the operand type even
   when a particular warning format does not consume its extra argument. */
void __stdcall type_warning(unsigned level, unsigned code, CType *type)
{
    char *name;
    begin_declaration_text();
    name = declaration_text_value(build_declaration_text(0, type));
    compiler_warning(level, code, name);
    end_declaration_text();
}

/* Original 0x3c1f0..0x3c281, RET20. Render both conflicting types
   before formatting either diagnostic form; a supplied symbol contributes
   its complete declarator, while the other form uses the caller's name. */
void __stdcall report_type_conflict(unsigned code, FrontendSymbol *symbol,
    char *name, CType *actual, CType *expected)
{
    short actual_handle, expected_handle, symbol_handle;
    char *actual_name, *expected_name, *symbol_name;
    begin_declaration_text();
    actual_handle = build_declaration_text(0, actual);
    expected_handle = build_declaration_text(0, expected);
    if (symbol) {
        symbol_handle = build_declaration_text(symbol, 0);
        expected_name = declaration_text_value(expected_handle);
        actual_name = declaration_text_value(actual_handle);
        symbol_name = declaration_text_value(symbol_handle);
        report_syntax_error(code, symbol_name, actual_name, expected_name);
    } else {
        expected_name = declaration_text_value(expected_handle);
        actual_name = declaration_text_value(actual_handle);
        report_syntax_error(code, name, actual_name, expected_name);
    }
    end_declaration_text();
}

/* Original 0x3c180..0x3c1ad, RET8. The formatted type survives
   through the syntax diagnostic, then the nested text scope is released. */
void __stdcall report_type_syntax_error(unsigned code, CType *type)
{
    char *name;
    begin_declaration_text();
    name = declaration_text_value(build_declaration_text(0, type));
    report_syntax_error(code, name);
    end_declaration_text();
}

/* Original0x3e710..0x3e72b, RET12. Preserve the one-byte operation
   and the two owned type descriptors for the common conversion formatter. */
void __stdcall report_conversion_mismatch(unsigned char operation, CType *target, CType *source)
{
    conversion_diagnostic(47,1,operation,target,source);
}
