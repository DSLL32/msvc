/* Assignment and type compatibility diagnostics. */
#include "parser.h"


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
            if (!compatible_function_attributes(left_attributes, right_attributes)) {
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
            !compatible_function_attributes(left_attributes, right_attributes)) {
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
                        if (count || !options.z_a) conversion_diagnostic(113, 1, operation, source, target);
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
                        if (comparison == 1 || (comparison == 2 && !options.z_a)) {
                            compiler_warning(1, 28, index + 1);
                            failed = 1;
                        }
                    }
                }
            } else if (!(left->flags & 0x4000) && !(right->flags & 0x4000)) break;
            if (!failed && options.z_a &&
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
    if (options.z_a && !((target_flags ^ source_flags) & 0x2fdf) && target->field08 == source->field08) {
        conversion_diagnostic(57, 4, operation, target, source);
        return !failed;
    }
    if ((target_flags & 0x2fdf) == 0x80) return 1;
    conversion_diagnostic(133, 1, operation, source, target);
    return !failed;
}
