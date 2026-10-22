/* Builtin C++ binary expressions, original 0x5e040..0x5eddb.
   The native 101-way switch table at 0x5eddc..0x5ef70 is represented by
   the source switch below, rather than copied addresses or instructions. */
#include "binary-expressions.h"
#include "lexer.h"

unsigned integer_constant_scratch; /* original 0x912e8: shared constant payload */
long maximum_signed_int_constant = 0x7fffffffL; /* original 0x9fc20 */

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
