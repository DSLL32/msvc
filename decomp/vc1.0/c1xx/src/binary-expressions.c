/* Builtin C++ binary expressions, original 0x5e040..0x5eddb.
   The native 101-way switch table at 0x5eddc..0x5ef70 is represented by
   the source switch below, rather than copied addresses or instructions. */
#include "binary-expressions.h"
#include "lexer.h"
#include "return-statements.h"
#include "numeric-conversion.h"
#include "keywords.h"
#include "call-expressions.h"
#include <recon.h>

unsigned integer_constant_scratch; /* original 0x912e8: shared constant payload */
long maximum_signed_int_constant = 0x7fffffffL; /* original 0x9fc20 */
typedef char LvalueClassFlags2OffsetMustBe52[
    (offsetof(ClassInfo, flags2) == 52) ? 1 : -1];

#define DECAY_TYPED_ARRAY(expression_) do { \
    if ((expression_)->type && (expression_)->type->related && \
        ((expression_)->type->related->flags & 0x77) == 2) \
        (expression_)->type = parameter_reference_type(expression_); \
} while (0)

IrToken *__stdcall builtin_binary_expression(unsigned char operation, IrToken *left, IrToken *right)
{
    IrToken *lhs, *rhs, *expression, *result, *pointer, *folded;
    CType *type;
    TypeShape *shape;
    unsigned left_shape, right_shape, stride, shift, shape_parameter;
    unsigned selection[4];
    unsigned short shape_flags, attributes;
    unsigned char category;
    int swapped;
    for (;;) {
        lhs = left;
        if (!left->type) {
            category = token_categories[operation];
            if (category == 0x3d || category == 0x6b || category == 0x7e) {
                lhs = validate_value_expression(left);
                if (!(lhs->flags & 8))
                    report_syntax_error(659, token_spellings[operation]);
                return 0;
            }
            lhs = resolve_assignment_operand(right->type, left);
            if (!lhs) {
                if (!right->type) report_expression_error(operation, left);
                return 0;
            }
        }
        category = token_categories[operation];
        if (category != 0x3d && category != 0x6b && category != 0x7e)
            lhs = fold_symbol_constant(lhs);
        rhs = right;
        if (!right->type) {
            rhs = resolve_assignment_operand(lhs->type, right);
            if (!rhs) {
                report_expression_error(operation, right);
                return 0;
            }
        }
        switch (token_categories[operation]) {
        case 0x3b: case 0x3d: case 0x3f: case 0x40: case 0x58:
        case 0x5d: case 0x5e: case 0x5f: case 0x7d: case 0x7e:
        case 0x8e: case 0x9f:
            break;
        case 0x4a: case 0x6b:
            rhs = promote_arithmetic_expression(rhs);
            break;
        default:
            lhs = promote_arithmetic_expression(lhs);
            rhs = promote_arithmetic_expression(rhs);
            break;
        }
        expression = new_ir_expression(operation, lhs, rhs, 0);
        if (!check_expression_operands(operation, 2, lhs, rhs)) return 0;
        switch (operation) {
        case 0x3a: case 0x42: case 0x51: case 0x53: case 0x56: case 0x64:
            expression->type = common_binary_type(operation, lhs->type, rhs->type);
            if (!expression->type) return 0;
            expression = convert_binary_operands(expression);
            return fold_binary_constant(expression, lhs, rhs);
        case 0x3c: case 0x3d: case 0x43: case 0x4b: case 0x50: case 0x52:
        case 0x54: case 0x57: case 0x5b: case 0x60: case 0x65: case 0x7e:
            result = convert_binary_operands(expression);
            if (!result) return 0;
            result->type = lhs->type;
            lhs = modifiable_lvalue(lhs, operation != 0x7e);
            result->value.pointer = lhs;
            if (!lhs) {
                report_syntax_error(106, token_spellings[operation]);
                return result;
            }
            if ((token_allocation_flags & 0x800) && result->type->related &&
                (result->type->related->attributes & 0xc)) {
                if (operation == 0x7e || operation == 0x3d)
                    result->tail.right = convert_based_expression(result->type, result->tail.right);
                result = qualify_based_storage(result);
            }
            return result;
        case 0x3f:
            if (lhs->type->related && (lhs->type->related->flags & 0x77) == 2)
                lhs->type = parameter_reference_type(lhs);
            if (rhs->type->related && (rhs->type->related->flags & 0x77) == 2)
                rhs->type = parameter_reference_type(rhs);
            return convert_pointer_operands(expression, lhs, rhs);
        case 0x41: case 0x49: case 0x5d: case 0x5e:
            type = common_binary_type(operation, lhs->type, 0);
            expression->type = type;
            if (!type) return 0;
            shape = type->related;
            if (shape && reject_special_indirection(operation, (unsigned char)shape->flags))
                return 0;
            if (lhs->type->flags & 0x200) {
                shape = lhs->type->related;
                if (shape && !(shape->flags & 0x77)) shape = shape->related;
                if (!shape) report_syntax_error(126, token_spellings[operation]);
            }
            lhs = modifiable_lvalue(lhs, 1);
            if (!lhs) {
                report_syntax_error(105, token_spellings[operation]);
                return expression;
            }
            type = expression->type;
            expression->value.pointer = lhs;
            shape_flags = type->related ? type->related->flags : 0;
            if (shape_flags) {
                if ((shape_flags & 0x77) == 1) {
                    integer_constant_scratch = complete_type_size(type);
                    if ((int)integer_constant_scratch > maximum_signed_int_constant)
                        expression->tail.right = new_literal_constant(8, &integer_constant_scratch);
                    else
                        expression->tail.right = new_literal_constant(7, &integer_constant_scratch);
                } else if ((shape_flags & 0x77) == 3 || (shape_flags & 0x44) == 0x44) {
                    report_syntax_error(171, token_spellings[operation]);
                }
                if ((token_allocation_flags & 0x800) &&
                    (expression->type->related->attributes & 0xc))
                    return qualify_based_storage(expression);
                return expression;
            }
            expression->tail.right = convert_numeric_constant(operation, type, expression->tail.right);
            return expression;
        case 0x44: case 0x47: case 0x48: case 0x4d: case 0x4e: case 0x55:
            if ((lhs->flags & 7) == 5 || (rhs->flags & 7) == 5)
                compiler_warning(4, 130, token_spellings[operation]);
            /* The native comparisons share the logical-operator conversion. */
        case 0x3b: case 0x58:
            expression = convert_binary_operands(expression);
            if (!expression) return 0;
            expression->type = type_0064;
            return fold_binary_constant(expression, lhs, rhs);
        case 0x4a:
            DECAY_TYPED_ARRAY(lhs);
            DECAY_TYPED_ARRAY(rhs);
            expression->type = common_binary_type(operation, lhs->type, 0);
            if (!expression->type) return 0;
            left_shape = lhs->type->related ? lhs->type->related->flags : 0;
            right_shape = rhs->type->related ? rhs->type->related->flags : 0;
            pointer = lhs;
            if (!left_shape || (left_shape & 0x77) > 2) {
                if (!right_shape || (right_shape & 0x77) > 2) {
                    report_syntax_error(109);
                    return expression;
                }
                expression->value.pointer = rhs;
                expression->tail.right = lhs;
                pointer = expression->value.pointer;
                expression->type = pointer->type;
                rhs = lhs;
                right_shape = left_shape;
            }
            if ((pointer->type->related->attributes & 7) == 4) {
                report_syntax_error(214);
                return expression;
            }
            if (right_shape) {
                report_syntax_error(107);
                return expression;
            }
            if (!(rhs->type->flags & 0x240)) {
                report_syntax_error(108);
                return expression;
            }
            integer_constant_scratch = complete_type_size(pointer->type);
            result = new_literal_constant(8, &integer_constant_scratch);
            result = builtin_binary_expression(0x53, rhs, result);
            expression->tail.right = result;
            folded = fold_symbol_constant(result);
            if ((folded->flags & 7) == 3 && pointer->kind == 0x4a &&
                (pointer->tail.right->flags & 7) == 3) {
                expression->value.pointer = pointer->value.pointer;
                folded->value.integer += pointer->tail.right->value.integer;
            }
            attributes = pointer->type->related->attributes;
            expression->type = drop_first_type_shape(pointer->type);
            if ((pointer->type->related->flags & 0x77) == 2 && (attributes & 0x3000)) {
                attributes &= 0x3000;
                shape = expression->type->related;
                if (!shape) {
                    expression->type->attributes |= attributes;
                } else if ((shape->attributes & 0x3000) != attributes) {
                    shape_parameter = shape->data.word;
                    expression->type = replace_first_type_shape(expression->type, shape->flags,
                        (unsigned short)(shape->attributes | attributes), &shape_parameter);
                }
            }
            if (!expression->type->related || (expression->type->related->flags & 0x77) != 2)
                expression = indirect_storage_expression(expression);
            return expression;
        case 0x4c: case 0x61:
            expression->value.pointer = lhs;
            expression->type = lhs->type;
            if (!expression->type) return 0;
            expression = convert_binary_operands(expression);
            return fold_binary_constant(expression, lhs, rhs);
        case 0x4f:
            DECAY_TYPED_ARRAY(lhs);
            DECAY_TYPED_ARRAY(rhs);
            expression->type = common_binary_type(operation, lhs->type, rhs->type);
            if (!expression->type) return 0;
            if (reject_member_pointer_arithmetic(expression)) return 0;
            left_shape = lhs->type->related ? lhs->type->related->flags : 0;
            right_shape = rhs->type->related ? rhs->type->related->flags : 0;
            if (!left_shape || (left_shape & 0x77) > 2) {
                if ((left_shape & 0x77) == 3 || (left_shape & 0x44) == 0x44) {
                    report_invalid_operand(operation, 1);
                    return 0;
                }
                if (right_shape & 3) {
                    report_syntax_error(113);
                    return 0;
                }
                if (((lhs->type->flags & 0x80) && !left_shape) ||
                    ((rhs->type->flags & 0x80) && !right_shape))
                    report_syntax_error(186, token_spellings[operation]);
                return fold_binary_constant(expression, lhs, rhs);
            }
            stride = complete_type_size(lhs->type);
            if (!right_shape || (right_shape & 0x77) > 2) {
                if ((right_shape & 0x77) == 3 || (right_shape & 0x44) == 0x44) {
                    report_invalid_operand(operation, 2);
                    return 0;
                }
                if (!(rhs->type->flags & 0x240)) {
                    report_syntax_error(112);
                    return expression;
                }
                integer_constant_scratch = stride;
                result = new_literal_constant(8, &integer_constant_scratch);
                expression->tail.right = builtin_binary_expression(0x53, rhs, result);
                return expression;
            }
            if ((lhs->type->flags & 0x400) && (rhs->type->flags & 0x400)) {
                expression = convert_pointer_operands(expression, lhs, rhs);
                if (!expression) return 0;
                lhs = expression->value.pointer;
                rhs = expression->tail.right;
                stride = complete_type_size(lhs->type);
            }
            warn_pointer_conversion(0x4f, lhs, rhs);
            expression->type = type_0064;
            left = fold_binary_constant(expression, lhs, rhs);
            if (stride == 1) return left;
            if (!(stride & (stride - 1))) {
                shift = 1;
                for (stride >>= 2; stride; stride >>= 1) ++shift;
                right = make_integer_constant(shift);
                operation = 0x61;
            } else {
                right = make_integer_constant(stride);
                operation = 0x42;
            }
            break;
        case 0x5a:
            swapped = 0;
            DECAY_TYPED_ARRAY(lhs);
            DECAY_TYPED_ARRAY(rhs);
            expression->type = common_binary_type(operation, lhs->type, rhs->type);
            if (!expression->type) return 0;
            left_shape = lhs->type->related ? lhs->type->related->flags : 0;
            right_shape = rhs->type->related ? rhs->type->related->flags : 0;
            if (reject_member_pointer_arithmetic(expression)) return 0;
            pointer = lhs;
            if (!(left_shape & 3)) {
                if (!(right_shape & 3)) {
                    if (((lhs->type->flags & 0x80) && !left_shape) ||
                        ((rhs->type->flags & 0x80) && !right_shape))
                        report_syntax_error(186, token_spellings[operation]);
                    return fold_binary_constant(expression, lhs, rhs);
                }
                expression->value.pointer = rhs;
                expression->tail.right = lhs;
                pointer = expression->value.pointer;
                expression->type = pointer->type;
                stride = left_shape;
                left_shape = right_shape;
                right_shape = stride;
                swapped = 1;
                rhs = lhs;
            }
            if ((left_shape & 0x77) == 3 || (left_shape & 0x44) == 0x44) {
                report_invalid_operand(operation, swapped ? 2 : 1);
                return 0;
            }
            if (right_shape & 3) {
                report_syntax_error(110);
                return 0;
            }
            if (!(rhs->type->flags & 0x240)) {
                report_syntax_error(111);
                return 0;
            }
            integer_constant_scratch = complete_type_size(pointer->type);
            result = new_literal_constant(8, &integer_constant_scratch);
            expression->tail.right = builtin_binary_expression(0x53, rhs, result);
            return expression;
        case 0x5f:
            if (lhs) {
                if ((short)is_unqualified_class_type(lhs->type)) {
                    lhs = convert_class_condition(lhs, 0, selection);
                    expression->value.pointer = lhs;
                }
                if (lhs && (short)is_unqualified_class_type(lhs->type)) {
                    report_syntax_error(83, type_kind_name(lhs->type->flags));
                    return 0;
                }
            }
            folded = fold_symbol_constant(lhs);
            if (!(folded->type->flags & 0x340)) {
                shape_flags = folded->type->related ? folded->type->related->flags : 0;
                if (!(shape_flags & 3)) {
                    report_syntax_error(155);
                    return 0;
                }
            }
            if ((folded->flags & 7) == 3) {
                result = folded->value.integer ? rhs->value.pointer : rhs->tail.right;
                expression = convert_initializer(1, rhs->type, result);
            }
            /* The condition and comma forms share result propagation. */
        case 0x40:
            expression->type = rhs->type;
            if (rhs->flags & 0x80) expression->flags |= 0x80;
            DECAY_TYPED_ARRAY(expression);
            return expression;
        case 0x9d: case 0x9e:
            type = lhs->type;
            if ((type->flags & 0x400) && (operation != 0x9d || !type->related) &&
                (operation != 0x9e || (type->related &&
                    (type->related->flags & 0x77) == 1 && !type->related->related))) {
                shape = rhs->type->related;
                if (shape && (shape->flags & 0x30)) return apply_member_pointer(expression);
                report_invalid_operand(operation, 2);
                return 0;
            }
            report_invalid_operand(operation, 1);
            return 0;
        default:
            return expression;
        }
    }
}

/* Original 0x5fdd0..0x60339, with the native 69-way switch at
   0x6033c..0x60450. Preserve each operator's distinct operand requirements. */
IrToken *__stdcall convert_binary_operands(IrToken *expression)
{
    IrToken *left, *right, *result;
    CType *left_type, *right_type;
    unsigned left_shape, right_shape;
    unsigned short left_flags, right_flags;
    unsigned char operation;
    int invalid, assignment, left_signed, right_signed, position;
    left = expression->value.pointer;
    right = expression->tail.right;
    invalid = 0;
    assignment = 1;
    if (!left->type) resolve_untyped_binary_operand(expression->kind, &left, right);
    if (!right->type) resolve_untyped_binary_operand(expression->kind, &right, left);
    left_type = left->type;
    if (!left_type || !right->type) return 0;
    left_shape = left_type->related ? left_type->related->flags : 0;
    right_shape = right->type->related ? right->type->related->flags : 0;
    operation = expression->kind;
    switch (operation) {
    case 0x3a: case 0x4c: case 0x51: case 0x56: case 0x61: case 0x64:
        assignment = 0;
        break;
    case 0x3b: case 0x58:
        if (!(left_type->flags & 0x340) && !(left_shape & 3)) {
            report_invalid_operand(operation, 1);
            invalid = 1;
        }
        if (!(right->type->flags & 0x340) && !(right_shape & 3)) {
            report_invalid_operand(operation, 2);
            invalid = 1;
        }
        return invalid ? 0 : expression;
    case 0x3c: case 0x4b: case 0x52: case 0x57: case 0x60: case 0x65:
        break;
    case 0x3d:
        expression = convert_pointer_operands(expression, left, right);
        if (!expression) return 0;
        left = expression->value.pointer;
        right = expression->tail.right;
        /* Simple assignment shares the initialization warning path. */
    case 0x7e:
        warn_pointer_conversion(operation, left, right);
        return expression;
    case 0x42: case 0x53:
        if (!(left_type->flags & 0x340) || left_shape) {
            report_invalid_operand(operation, 1);
            invalid = 1;
        }
        if (!(right->type->flags & 0x340) || right_shape) {
            report_invalid_operand(operation, 2);
            invalid = 1;
        }
        return invalid ? 0 : expression;
    case 0x43: case 0x54:
        if (!(left_type->flags & 0x140) || left_shape) {
            report_invalid_operand(operation, 1);
            invalid = 1;
        }
        if (!(right->type->flags & 0x340) || right_shape) {
            report_invalid_operand(operation, 2);
            invalid = 1;
        }
        if (invalid) return 0;
        warn_pointer_conversion(operation, left, right);
        return expression;
    case 0x47: case 0x48: case 0x4d: case 0x4e:
        if (reject_member_pointer_arithmetic(expression)) return 0;
        /* Ordered and equality comparisons share conversion and diagnostics. */
    case 0x44: case 0x55:
        if ((left->flags & 7) != 3 && (right->flags & 7) != 3) {
            left_type = left->type;
            left_flags = left_type->flags;
            if ((left_flags & 0x40) && !left_shape) {
                right_type = right->type;
                right_flags = right_type->flags;
                if ((right_flags & 0x40) && !right_shape) {
                    left_signed = !(left_flags & 0x4000) &&
                        (!options.configuration.option_J ||
                         ((left_flags & 0x2fdf) == 0x41 && (left_flags & 0x8000)));
                    right_signed = !(right_flags & 0x4000) &&
                        (!options.configuration.option_J ||
                         ((right_flags & 0x2fdf) == 0x41 && (right_flags & 0x8000)));
                    if (left_signed != right_signed &&
                        (left->kind != 0x6e || left_type != type_0064 ||
                         !(((IrToken *)left->value.pointer)->type->flags & 0x4000)) &&
                        (right->kind != 0x6e || right_type != type_0064 ||
                         !(((IrToken *)right->value.pointer)->type->flags & 0x4000)))
                        report_type_conversion(18, 3, operation, left_type, right_type);
                }
            }
        }
        result = convert_pointer_operands(expression, left, right);
        if (!result) return 0;
        left = result->value.pointer;
        right = result->tail.right;
        warn_pointer_conversion(operation, left, right);
        if (left->type->related && (left->type->related->flags & 0x30))
            return compare_member_pointer_expression(result);
        return result;
    case 0x50: case 0x5b:
        if (reject_member_pointer_arithmetic(expression)) return 0;
        if (!right->type->related && (right->type->flags & 0x340)) {
            right_flags = right->type->flags;
            left_type = left->type;
            if (!(left_type->flags & 0x200)) {
                if (!left_type->related && (left_type->flags & 0x140)) {
                    warn_pointer_conversion(operation, left, right);
                    return expression;
                }
                if ((left_shape & 0x77) == 1) {
                    if (!(right_flags & 0x240)) {
                        report_syntax_error(114);
                        return 0;
                    }
                    integer_constant_scratch = complete_type_size(left_type);
                    result = new_literal_constant(8, &integer_constant_scratch);
                    expression->tail.right = builtin_binary_expression(0x53, right, result);
                    return expression;
                }
                if ((left_shape & 0x77) != 3 && (left_shape & 0x44) != 0x44)
                    return expression;
            }
            position = 1;
        } else {
            position = 2;
        }
        report_invalid_operand(operation, position);
        return 0;
    default:
        return 0;
    }
    if ((assignment ? !(left_type->flags & 0x40) : !(left_type->flags & 0x240)) || left_shape) {
        report_invalid_operand(operation, 1);
        invalid = 1;
    }
    if (!(right->type->flags & 0x240) || right_shape) {
        report_invalid_operand(operation, 2);
        invalid = 1;
    }
    return invalid ? 0 : expression;
}

/* Original 0x60450..0x60487: a null pointer constant needs no further
   pointer-conversion warning; other nonnull operands share the checker. */
void __stdcall warn_pointer_conversion(unsigned char operation, IrToken *left, IrToken *right)
{
    TypeShape *shape;
    if (!left || !right) return;
    if (left->flags & 0x80) {
        shape = right->type->related;
        if (shape && (shape->flags & 3)) return;
    }
    check_conversion_warning(operation, left->type, right);
}

/* Original 0x60490..0x6080e. The result of the type-only checker is
   preserved while value-dependent void, sign and precision diagnostics run. */
int __stdcall check_conversion_warning(unsigned char operation, CType *target, IrToken *value)
{
    CType *source;
    unsigned short target_flags, source_flags;
    unsigned constant;
    int result, void_count, float_to_integer, precision_loss, promotion_loss;
    short source_unsigned;
    if ((value->flags & 0x80) && target->related && (target->related->flags & 3))
        return 1;
    source = value->type;
    result = check_type_conversion_warning(operation, target, source);
    if (target->related || source->related) return result;
    void_count = !!(source->flags & 0x80) + !!(target->flags & 0x80);
    if (operation == 0x3f) {
        if (void_count == 1) compiler_warning(4, 53);
    } else if (void_count == 1) {
        if (operation != 0x87) {
            report_syntax_error(120);
        } else if (!(function_symbol->storage[2] & 1)) {
            report_syntax_error(561, format_symbol_name(function_symbol));
        }
    } else if (void_count == 2) {
        report_syntax_error(120);
    }
    switch (operation) {
    case 0x3d: case 0x43: case 0x50: case 0x54: case 0x5b:
    case 0x7a: case 0x7e: case 0x87:
        break;
    default:
        return result;
    }
    float_to_integer = precision_loss = promotion_loss = 0;
    target_flags = target->flags;
    source_flags = source->flags;
    if (!(target_flags & 0x140) || !(source_flags & 0x140)) return result;
    if (!(source_flags & 0x100)) {
        if (target_flags & 0x100) {
            if ((target_flags & 0xf) <= (source_flags & 0xf)) float_to_integer = 1;
            goto diagnose_numeric_conversion;
        }
        if ((value->flags & 7) == 3) {
            constant = value->value.integer;
            if ((target_flags & 0x4000) && !(source_flags & 0x4000) &&
                (!options.configuration.option_J || (source_flags & 0x2fdf) != 0x41 ||
                 !(source_flags & 0x8000)) && (int)constant < 0)
                compiler_warning(3, 18, token_spellings[operation]);
            if (source_flags != target_flags) {
                source_unsigned = (short)((source_flags & 0x4000) != 0 ||
                    (options.configuration.option_J && (source_flags & 0x2fdf) == 0x41 &&
                     (source_flags & 0x8000)));
                if (integer_constant_overflows((unsigned short)(target_flags & 0xf),
                    (short)(target_flags & 0x4000), source_unsigned, constant))
                    compiler_warning(2, 309, token_spellings[operation]);
            }
            goto diagnose_numeric_conversion;
        }
        if ((target_flags & 0xf) < (source_flags & 0xf)) {
            if (!(source_flags & 0x20) || !(source_flags & 0x40) ||
                (type_0064->flags & 0xf) < (target_flags & 0xf)) {
                precision_loss = 1;
                goto diagnose_numeric_conversion;
            }
        } else if (((source_flags ^ target_flags) & 0xf) ||
                   ((source_flags & 0x20) && (source_flags & 0x40)) ||
                   !(target_flags & 0x20) || !(target_flags & 0x40)) {
            goto diagnose_numeric_conversion;
        }
    } else {
        if (target_flags & 0x40) {
            float_to_integer = 1;
            goto diagnose_numeric_conversion;
        }
        if ((target_flags & 0xf) < (source_flags & 0xf)) {
            precision_loss = 1;
            goto diagnose_numeric_conversion;
        }
        if (((source_flags ^ target_flags) & 0xf) || !(source_flags & 0x100) ||
            !(source_flags & 0x20) || ((target_flags & 0x100) && (target_flags & 0x20)))
            goto diagnose_numeric_conversion;
    }
    promotion_loss = 1;
diagnose_numeric_conversion:
    if (float_to_integer)
        compiler_warning(2, 51);
    else if (precision_loss || promotion_loss)
        compiler_warning(precision_loss ? 3 : 4, (target_flags & 0x100) ? 136 : 135);
    return result;
}

/* Original 0x60830..0x60eb0, with its 19-entry operator switch at
   0x60eb0..0x60efc. Compare qualifiers and nested declarator shapes. */
int __stdcall check_type_conversion_warning(unsigned char operation, CType *target, CType *source)
{
    TypeShape *destination, *origin, *other;
    unsigned short target_flags, source_flags, destination_flags, origin_flags;
    unsigned short destination_attributes, origin_attributes;
    unsigned short target_qualifiers, source_qualifiers, destination_kind, other_kind;
    int target_void, source_void, target_void_pointer, source_void_pointer;
    int first_layer, qualifiers;
    first_layer = 1;
    if (source == target) return 1;
    target_flags = target->flags;
    source_flags = source->flags;
    destination = target->related;
    if (destination && !(destination->flags & 0x77)) destination = destination->related;
    origin = source->related;
    if (origin && !(origin->flags & 0x77)) origin = origin->related;
    target_void = target_flags & 0x80;
    source_void = source_flags & 0x80;
    target_void_pointer = target_void && destination &&
        (destination->flags & 0x77) == 1 && !destination->related;
    source_void_pointer = source_void && origin &&
        (origin->flags & 0x77) == 1 && !origin->related;
    if (target_void_pointer != source_void_pointer) {
        other = target_void_pointer ? origin : destination;
        other_kind = other ? other->flags & 0x77 : 0;
        if (other_kind == 1 || other_kind == 3 || other_kind == 2) {
            switch (operation) {
            case 0x3d: case 0x7a: case 0x7e: case 0x87:
                if (source_void_pointer) {
                    report_syntax_error(551, token_spellings[operation]);
                    return 0;
                }
                if (!check_conversion_qualifiers(operation, target->attributes, source->attributes, 1))
                    return 0;
                break;
            case 0x44: case 0x55:
                break;
            case 0x47: case 0x48: case 0x4d: case 0x4e: case 0x4f:
                report_type_conversion(133, 3, operation, source, target);
                return 1;
            default:
                return 1;
            }
            if (token_allocation_flags & 0x800)
                check_based_pointer_conversion(operation, destination, origin);
            return 1;
        }
    }
    if (origin == destination) {
        if (!destination) {
            if ((target_flags & 0x140) && (source_flags & 0x340)) return 1;
            if (source->field08 != target->field08 &&
                (operation != 0x3f || !(target_flags & 0x200) || !(source_flags & 0x200))) {
                report_syntax_error(115, token_spellings[operation]);
                return 0;
            }
            return 1;
        }
        if (!check_conversion_qualifiers(operation, target->attributes, source->attributes, 1))
            return 0;
    } else if (destination) {
        if (!origin) {
            if (destination->flags & 0x40) {
                report_syntax_error(115, token_spellings[operation]);
                return 0;
            }
        } else {
            while (destination && origin) {
                destination_flags = destination->flags;
                origin_flags = origin->flags;
                destination_attributes = destination->attributes;
                origin_attributes = origin->attributes;
                target_qualifiers = destination->related ? destination->related->attributes : target->attributes;
                source_qualifiers = origin->related ? origin->related->attributes : source->attributes;
                if ((destination_flags & 0x77) == 2)
                    target_qualifiers |= destination_attributes & 0x3000;
                if ((origin_flags & 0x77) == 2)
                    source_qualifiers |= origin_attributes & 0x3000;
                if (first_layer) {
                    if ((destination_flags & 0x77) == 2) destination_flags = 1;
                    if ((origin_flags & 0x77) == 2) origin_flags = 1;
                }
                if (((origin_flags ^ destination_flags) & 0x77) &&
                    !(first_layer && (destination_flags & 0x40) && (origin_flags & 0x77) == 1) &&
                    !((destination_flags & 0x44) == 0x44 && (origin_flags & 8))) {
                    report_incompatible_type_shapes(operation, target, source);
                    return 0;
                }
                destination_kind = destination_flags & 0x77;
                if (destination_kind == 3) {
                    if (!equivalent_shape_attributes(destination_attributes, origin_attributes)) {
                        report_syntax_error(152, token_spellings[operation]);
                        return 0;
                    }
                    if ((origin_attributes ^ destination_attributes) & 0xf00)
                        report_type_conversion(50, 4, operation, target, source);
                }
                if (token_allocation_flags & 0x800)
                    check_based_pointer_conversion(operation, destination, origin);
                qualifiers = check_conversion_qualifiers(operation, target_qualifiers, source_qualifiers, first_layer);
                if (!qualifiers) return 0;
                if (qualifiers == 2 && destination_kind != 3 &&
                    (destination_kind != 2 || (origin_flags & 0x77) != 2) &&
                    !equivalent_shape_attributes(destination_attributes, origin_attributes)) {
                    report_type_conversion(47, 1, operation, target, source);
                    return 0;
                }
                if (destination_kind == 2) {
                    if (origin->data.word != destination->data.word) {
                        report_type_conversion(48, 1, operation, target, source);
                        return 0;
                    }
                } else if (destination_kind == 4 &&
                    origin->data.parameters->declarations != destination->data.parameters->declarations) {
                    report_syntax_error(116);
                }
                first_layer = 0;
                destination = destination->related;
                origin = origin->related;
                if (origin == destination) origin = destination = 0;
            }
            if (origin != destination) {
                if (first_layer && ((source_void && !origin) || (target_void && !destination))) {
                    report_syntax_error(120);
                    return 0;
                }
                if ((target->related || (target_flags & 0x40)) &&
                    (source->related || (source_flags & 0x40)) &&
                    (!first_layer || (!(destination_flags & 0x40) && (origin_flags & 0x77) != 1))) {
                    report_incompatible_type_shapes(operation, target, source);
                    return 0;
                }
                report_syntax_error(115, token_spellings[operation]);
                return 0;
            }
        }
    }
    if (source_flags == target_flags && source->field08 == target->field08) return 1;
    report_syntax_error(230, token_spellings[operation]);
    return 0;
}

/* Original 0x61690..0x617df, switch data 0x617e0..0x61888.
   Return the actual storage expression, retaining extension lvalue forms. */
IrToken *__stdcall modifiable_lvalue(IrToken *expression, int diagnose)
{
    IrToken *inner;
    CppSymbol *class_symbol;
    CType *type;
    int converted, constant;
    converted = 0;
    if (!expression || (expression->flags & 0x20)) return 0;
    while (expression && expression->kind == 0x80)
        expression = expression->value.pointer;
    if (expression->kind == 0x6e) return 0;
    if (expression->kind == 0x67) expression = expression->value.pointer;
    if (expression->kind == 0x68) return 0;
    if (token_categories[expression->kind] != 0x46) {
        if (!expression->type) return 0;
        switch (expression->kind) {
        case 0x3c: case 0x3d: case 0x43: case 0x4b: case 0x50: case 0x52:
        case 0x54: case 0x57: case 0x5b: case 0x5d: case 0x5e: case 0x60: case 0x65:
            expression = make_assignment_lvalue(expression);
            break;
        case 0x40:
            expression = make_comma_lvalue(expression);
            break;
        case 0x5f:
            expression = make_conditional_lvalue(expression);
            break;
        default:
            if (!expression->type->related || !(expression->type->related->flags & 0x40))
                return 0;
            break;
        }
        if (!expression) return 0;
        converted = 1;
    }
    constant = (type_qualifiers(expression->type) & 0x1000) != 0;
    type = expression->type;
    if ((type->flags & 0x400) && !type->related) {
        class_symbol = cpp_context_stack->items[type->field08];
        constant |= (class_symbol->detail.class_info->flags2 & 0x40) >> 6;
    }
    if (!converted) expression = expression->value.pointer;
    if (expression->kind == 0x59) {
        inner = expression->value.pointer;
        if (inner->kind == 0x7a && !inner->type->related) return 0;
    }
    if (diagnose && constant) report_syntax_error(166);
    return expression;
}

/* Original 0x5de30..0x5e031. Class overload selection precedes the builtin
   operator path, including assignment-model and short-circuit bookkeeping. */
IrToken *__stdcall binary_expression(unsigned char operation, IrToken *left, IrToken *right)
{
    IrToken *result;
    CppSymbol *tag;
    unsigned failure, record, rule, classification;
    int left_class;
    if (!left || !right) {
        if (operation == OP_LOGICAL_AND || operation == OP_LOGICAL_OR)
            increment_expression_record_depth();
        return 0;
    }
    if (left->kind == 0xa3) {
        report_syntax_error(275, format_symbol_name(left->value.pointer));
        return 0;
    }
    if (right->kind == 0xa3) {
        report_syntax_error(275, format_symbol_name(right->value.pointer));
        return 0;
    }
    left_class = is_overload_operand(left);
    right = fold_symbol_constant(right);
    if (left_class || is_overload_operand(right)) {
        failure = 0;
        result = resolve_operator_expression(operation, left, right, left_class, &failure);
        if (result) {
            if (operation == OP_LOGICAL_AND || operation == OP_LOGICAL_OR) {
                /* Consult the same encountered IR vocabulary as the writer,
                   retaining this original inline table lookup in its caller. */
                record = 0x9e;
                for (rule = 0; rule < declaration_code_count; ++rule)
                    if (declaration_codes[rule].token == result->kind) {
                        record = declaration_codes[rule].value; break;
                    }
                for (rule = 0; rule < expression_record_rule_count; ++rule)
                    if (expression_record_rules[rule].operation == result->kind) {
                        record = expression_record_rules[rule].record; break;
                    }
                if (record < 0x1b || record > 0x1c) increment_expression_record_depth();
            }
            return result;
        }
        if (operation == OP_ASSIGN && left_class && !(left->type->flags & 0x200)) {
            tag = cpp_context_stack->items[left->type->field08];
            if (!has_assignment_member_model(left->type))
                report_syntax_error(582, tag->name);
            else report_conversion_rank_failure(operation, failure, left->type, right->type);
            return 0;
        }
        if (failure) {
            classification = left ? (unsigned short)is_unqualified_class_type(left->type) : 0;
            if (!operator_has_builtin_fallback(operation, 2, classification)) {
                report_operator_resolution_failure(operation, 2, failure, left->type, right->type);
                return 0;
            }
        }
    }
    if (operation == OP_LOGICAL_AND || operation == OP_LOGICAL_OR)
        right = prepare_expression(right, 3);
    return builtin_binary_expression(operation, left, right);
}

/* Original 0x5d800..0x5d8ab. The operation parameter is retained by the
   native ABI but is unused: shapes, scalar category and width choose a type. */
CType *__stdcall common_binary_type(unsigned char operation, CType *left, CType *right)
{
    unsigned short left_flags, right_flags;
    if (!left) return right;
    if (!right) return left;
    if (left->related) {
        if (right->related && (right->flags & 0x80)) return right;
        return left;
    }
    if (right->related) return right;
    left_flags = left->flags;
    right_flags = right->flags;
    if (left_flags & 0x100) {
        if (!(right_flags & 0x100) || (right_flags & 15) <= (left_flags & 15)) return left;
        return right;
    }
    if (right_flags & 0x100) return right;
    if (!(left_flags & 0x240) || !(right_flags & 0x240)) return left;
    if ((left_flags ^ right_flags) & 15)
        return (right_flags & 15) < (left_flags & 15) ? left : right;
    return (left_flags & 0x4000) ? left : right;
}

/* Original 0x5d330..0x5d709. Choose a permitted direction of conversion
   for comparison/assignment operands, or a common arithmetic type. */
IrToken *__stdcall convert_pointer_operands(IrToken *expression, IrToken *left, IrToken *right)
{
    CType *left_type = left->type, *right_type = right->type;
    CType *common, *left_element, *right_element;
    /* The original leaves this stack slot unwritten on unsupported fallthrough
       paths. Valid scalar pairs below select a type before using it. */
    CType *unselected_type;
    IrToken *converted_left, *converted_right;
    unsigned to_right, to_left;
    unsigned short left_flags, right_flags;
    unsigned char operation = expression->kind;
    int one_way = operation == 0x3d || operation == 0x7a ||
        operation == 0x7e || operation == 0x87;

    if (left_type == right_type) expression->type = left_type;
    to_right = one_way ? 0x2000 : classify_expression_conversion(right_type, left, 0, 0);
    to_left = classify_expression_conversion(left_type, right, 0, 0);
    if ((int)to_right >= 0x400) {
        if ((int)to_left >= 0x400) {
            if ((to_right & 0x2000) && operation != 0x3d && left_type && right_type &&
                left_type->related && (left_type->related->flags & 0x77) == 2 &&
                right_type->related && (right_type->related->flags & 0x77) == 2) {
                right_element = drop_first_type_shape(right_type);
                left_element = drop_first_type_shape(left_type);
                if (classify_type_conversion(left_element, right_element, 0, 0) == 1) {
                    expression->type = left_type;
                    return expression;
                }
            }
            if (!one_way) report_type_conversion(0x1be, 0, operation, right_type, left_type);
            else report_conversion_rank_failure(operation, to_left, left_type, right_type);
            return 0;
        }
        goto convert_right;
    }
    if ((int)to_left >= 0x400) goto convert_left;
    if ((int)to_right < (int)to_left) {
        if ((int)to_right >= 0x400 || (int)to_right < 0x20) goto convert_left;
    } else if ((int)to_left < (int)to_right) {
        if ((int)to_left >= 0x400 || (int)to_left < 0x20) goto convert_right;
    } else {
        if (right->flags & 0x80) {
            expression->tail.right = convert_initializer(operation, left_type, right);
            expression->type = left_type;
            return expression;
        }
        if (left->flags & 0x80) {
            expression->value.pointer = convert_initializer(operation, right_type, left);
            expression->type = right_type;
            return expression;
        }
        if ((int)to_right >= 0x400 || (int)to_right < 0x20 || to_right == 0x100) {
            expression->type = right_type;
            return expression;
        }
    }
    if (!left_type->related && !right_type->related) {
        left_flags = left_type->flags;
        right_flags = right_type->flags;
        if ((left_flags & 0x340) && (right_flags & 0x340)) {
            if (left_flags & 0x100) {
                common = left_type;
                if ((right_flags & 0x100) && (left_flags & 15) < (right_flags & 15))
                    common = right_type;
            } else if (right_flags & 0x100) common = right_type;
            else if ((left_flags & 0x240) && (right_flags & 0x240))
                common = common_binary_type(operation, left_type, right_type);
            else common = unselected_type;
        } else if (to_left == to_right) {
            report_type_conversion(0x252, 0, operation, right_type, left_type);
            return 0;
        } else common = unselected_type;
    } else {
        if ((int)to_right < (int)to_left) goto convert_left;
        if ((int)to_left < (int)to_right) goto convert_right;
        common = unselected_type;
    }
    converted_left = convert_initializer(operation, common, left);
    if (!converted_left) return 0;
    converted_right = convert_initializer(operation, common, right);
    if (!converted_right) return 0;
    expression->value.pointer = converted_left;
    expression->tail.right = converted_right;
    expression->type = common;
    return expression;

convert_left:
    expression->value.pointer = convert_operand_if_needed(operation, right_type, left);
    expression->type = right_type;
    return expression;
convert_right:
    expression->tail.right = convert_operand_if_needed(operation, left_type, right);
    expression->type = left_type;
    return expression;
}

/* Original 0x5dad0..0x5de28. Convert the alternative values, detach their
   cleanup expressions, and conditionally select those cleanups through a
   saved condition value before constructing the final conditional expression. */
IrToken *__stdcall conditional_expression(IrToken *condition, IrToken *if_true, IrToken *if_false)
{
    unsigned to_false = 0, to_true = 0, selection[4], index;
    CType *condition_type;
    IrToken *true_cleanup, *false_cleanup, *temporary, *cleanup, *pair, *result;
    IrToken *true_value, *false_value, *zero;
    if (!condition || !if_true || !if_false) return 0;
    if (condition->kind == 0xa3) {
        report_syntax_error(0x113, format_symbol_name(condition->value.pointer));
        return 0;
    }
    if_true = prepare_value_operand(0x3f, if_true);
    if_false = prepare_value_operand(0x3f, if_false);
    if (if_true->type->related && (if_true->type->related->flags & 0x77) == 2)
        if_true->type = parameter_reference_type(if_true);
    if (if_false->type->related && (if_false->type->related->flags & 0x77) == 2)
        if_false->type = parameter_reference_type(if_false);
    if ((if_true && is_unqualified_class_type(if_true->type)) ||
        (if_false && is_unqualified_class_type(if_false->type))) {
        to_false = classify_expression_conversion(if_false->type, if_true, 0, 0);
        to_true = classify_expression_conversion(if_true->type, if_false, 0, 0);
        if ((int)to_false >= 0x400 && (int)to_true >= 0x400) to_false = to_true = 0;
    }
    if ((int)to_true < (int)to_false)
        if_false = convert_operand_if_needed(0x3f, if_true->type, if_false);
    else if (!to_false && !to_true && (if_false->flags & 0x80))
        if_false = convert_initializer(0x3f, if_true->type, if_false);
    false_cleanup = prepare_expression(0, 2);
    if ((int)to_false < (int)to_true)
        if_true = convert_operand_if_needed(0x3f, if_false->type, if_true);
    else if (!to_false && !to_true && (if_true->flags & 0x80))
        if_true = convert_initializer(0x3f, if_false->type, if_true);
    true_cleanup = prepare_expression(0, 2);
    if (condition) {
        if (is_unqualified_class_type(condition->type))
            condition = convert_class_condition(condition, 0, selection);
        if (condition && is_unqualified_class_type(condition->type)) {
            if (selection[0] & 0x2000)
                report_syntax_error(0x1c3,
                    ((CppSymbol *)cpp_context_stack->items[condition->type->field08])->name);
            zero = make_integer_constant(0);
            condition = new_right_typed_expression(0x40, condition, zero);
            goto assemble_result;
        }
    }
    if (true_cleanup || false_cleanup) {
        condition_type = condition->type;
        if (next_symbol_index >= 0x7fffffffUL) fatal_error(0x37);
        if (next_symbol_index >= browser_symbol_limit && browser_filename) browser_symbol_rollover();
        index = next_symbol_index++;
        temporary = new_temporary_reference(index, condition->type);
        condition = new_left_typed_expression(0x3d, temporary, condition);
        if (!true_cleanup) true_cleanup = make_integer_constant(0);
        else if (!false_cleanup) false_cleanup = make_integer_constant(0);
        temporary = new_temporary_reference(index, condition_type);
        temporary = indirect_storage_expression(temporary);
        cleanup = binary_expression(0x3f, true_cleanup, false_cleanup);
        cleanup = binary_expression(0x5f, temporary, cleanup);
        record_conditional_cleanup(cleanup);
    }
assemble_result:
    pair = binary_expression(0x3f, if_true, if_false);
    if (!pair) return 0;
    true_value = pair->value.pointer;
    false_value = pair->tail.right;
    result = binary_expression(0x5f, condition, pair);
    if (result && (true_value->type->flags & 0x2fdf) == 0x41 &&
        false_value->type->flags == true_value->type->flags &&
        !true_value->type->related && !false_value->type->related)
        result = new_cast_expression(true_value->type, result);
    return result;
}

/* Original 0x5da70..0x5daca. A value operand may be a constant symbol or
   an unresolved overload, but a type designator cannot supply a value. */
IrToken *__stdcall prepare_value_operand(unsigned char operation, IrToken *expression)
{
    IrToken *resolved;
    expression = fold_symbol_constant(expression);
    if (expression->kind == 0xa3) {
        report_syntax_error(0x113, format_symbol_name(expression->value.pointer));
        return 0;
    }
    if (!expression->type) {
        resolved = resolve_assignment_operand(0, expression);
        if (!resolved) {
            report_expression_error(operation, expression);
            return 0;
        }
        return resolved;
    }
    return expression;
}

/* Original 0x5fd80..0x5fdcc. Diagnose each member-pointer operand
   independently, even when the other operand has already been rejected. */
int __stdcall reject_member_pointer_arithmetic(IrToken *expression)
{
    IrToken *left = expression->value.pointer;
    TypeShape *shape = left->type->related;
    int rejected = 0;
    if (shape && (shape->flags & 0x30)) {
        report_invalid_operand(expression->kind, 1);
        rejected = 1;
    }
    shape = expression->tail.right->type->related;
    if (shape && (shape->flags & 0x30)) {
        report_invalid_operand(expression->kind, 2);
        rejected = 1;
    }
    return rejected;
}

/* Original 0x66560..0x6659b. Preserve an already matching class operand;
   other operands, including null, go through the ordinary initializer path. */
IrToken *__stdcall convert_operand_if_needed(unsigned char operation, CType *type, IrToken *expression)
{
    if (expression && (type == expression->type ||
        same_unqualified_class_type(type, expression->type)))
        return expression;
    return convert_initializer(operation, type, expression);
}

/* Original 0x60f00..0x60ffd, stdcall16. Conversion contexts
   differ in whether dropping const/volatile is an error or a deeper-layer check. */
int __stdcall check_conversion_qualifiers(unsigned char operation,
    unsigned short target, unsigned short source, int first_layer)
{
    int removed = 0;
    switch (token_categories[operation]) {
    case 0x3f: case 0x78: case 0x86:
        return 1;
    case 0x3d: case 0x7a: case 0x7e: case 0x87:
        if (((target ^ source) & 0x3000) && (source & 0x3000)) {
            if (!(target & 0x2000) && (source & 0x2000)) {
                report_syntax_error(268, token_spellings[operation], token_spellings[TOK_VOLATILE]);
                removed = 1;
            }
            if (!(target & 0x1000) && (source & 0x1000)) {
                report_syntax_error(268, token_spellings[operation], token_spellings[TOK_CONST]);
                removed = 1;
            }
            if (removed) return 0;
        }
        return first_layer ? 1 : 2;
    default:
        return 2;
    }
}

/* Original0x619b0..0x61a7d, stdcall4. Yield the assigned lvalue
   after evaluation. Nonrepeatable addresses are captured in a temporary so
   the complete assignment evaluates the original address only once. */
IrToken *__stdcall make_assignment_lvalue(IrToken *expression)
{
    IrToken *left, *right, *temporary, *copy, *assignment, *value;
    CType *type;
    unsigned index;
    if (!expression || !(left = expression->value.pointer)) return 0;
    right = expression->tail.right;
    if (expression_can_be_repeated(left)) {
        type = left->type;
        copy = clone_expression(left, 0);
        return new_ir_expression(0x40, expression, copy, type);
    }
    if (next_symbol_index >= 0x7fffffffUL) fatal_error(55);
    if (next_symbol_index >= browser_symbol_limit && browser_filename)
        browser_symbol_rollover();
    index = next_symbol_index++;
    temporary = new_temporary_reference(index, left->type);
    assignment = new_right_typed_expression(OP_ASSIGN, temporary, left);
    assignment = new_right_typed_expression(OP_ASSIGN, assignment, right);
    copy = clone_expression(temporary, 0);
    value = new_typed_unary_expression(OP_DEREFERENCE, left->type, copy);
    return new_ir_expression(0x40, assignment, value, left->type);
}

/* Original 0x63780..0x637e8. A builtin fallback may still accept an
   overload-search failure; already diagnosed ranks retain their marker. */
int __stdcall report_operator_resolution_failure(unsigned char operation,
    unsigned char arity, unsigned failure, CType *left, CType *right)
{
    if (failure < 0x400)
        return 0;
    if (failure & 0x4000)
        return 1;
    if (operator_has_builtin_fallback(operation, arity,
            is_unqualified_class_type(left)))
        return 0;
    report_conversion_rank_failure(operation, failure, left, right);
    return 1;
}

/* Original 0x61ac0..0x61b33. Both branches must yield modifiable
   lvalues before the copied condition is evaluated through one temporary. */
IrToken *__stdcall make_conditional_lvalue(IrToken *expression)
{
    IrToken *if_true, *if_false;
    if_true = modifiable_lvalue(expression->tail.right->value.pointer, 0);
    if (!if_true)
        return 0;
    if_false = modifiable_lvalue(expression->tail.right->tail.right, 0);
    if (!if_false)
        return 0;
    expression = clone_expression(expression, 0);
    expression->tail.right->value.pointer = if_true;
    expression->tail.right->tail.right = if_false;
    expression->tail.right->type = make_pointer_to_type(0, expression->type);
    expression->type = expression->tail.right->type;
    return materialize_call_result(expression);
}

/* Original 0x61a80..0x61ab6. A comma expression inherits the final
   operand's lvalue and type without mutating the caller's original tree. */
IrToken *__stdcall make_comma_lvalue(IrToken *expression)
{
    IrToken *right = modifiable_lvalue(expression->tail.right, 0);
    if (!right)
        return 0;
    expression = clone_expression(expression, 0);
    expression->tail.right = right;
    expression->type = right->type;
    return expression;
}
