#include "optimizer.h"
#include "small-nodes.h"
#include "expression-order.h"
#include <recon.h>

extern int expression_rewrite_changed, c2_float_precision;
extern C2Record *expression_statement_context;
int __fastcall expression_can_flatten(C2Expression *expression);
void __fastcall flatten_expression_inputs(C2Expression *owner, C2Expression *child);
C2Expression *__fastcall negate_rewritten_operand(C2Expression *expression, C2Expression *context);
C2Expression *__fastcall fold_expression_constants(C2Expression *expression, C2Expression *context);
void __fastcall prune_detached_operands(C2Expression *expression);

/* Genuine pending application boundaries; their bodies retain named INT3s. */
C2Expression *__fastcall operation_identity(unsigned opcode, unsigned short type);
C2Expression *__fastcall operation_absorber(unsigned opcode, unsigned short type);
int c2_real_compare(int condition, C2Real *left, C2Real *right);
C2Expression *__fastcall finalize_expression_tree(C2Expression *expression);
int __fastcall clear_expression_values(C2Expression *expression)
{
    C2Expression *operand, *other, *definition, *pruned;
    unsigned mask = 1u << (expression->kind & 31);
    if (!(mask & 0x7fe)) {
        if (input_has_side_effects(expression)) {
            pruned = prune_input_tree(expression, 0);
            if (pruned) finalize_expression_tree(pruned);
            return 0;
        }
        for (operand = expression->left; operand; operand = operand->next)
            clear_expression_values(operand);
        return 1;
    }
    if (mask & 0x60) {
        operand = expression->aggregate_value;
        if ((!operand || ((1u << (operand->kind & 31)) & 6) &&
                         operand->value.reg->value)) {
            other = expression->aggregate_second;
            if ((!other || ((1u << (other->kind & 31)) & 6) &&
                           other->value.reg->value) && operand != other)
                expression = operand;
        }
    }
    /* The original assumes a usable first address operand when only the
     * second is present; retain its genuine NULL-read defect in that case. */
    if ((1u << (expression->kind & 31)) & 6) {
        definition = expression->value.reg->value;
        if (definition && clear_expression_values(definition))
            release_small_node(definition);
    }
    return 1;
}
void __fastcall remove_input_operand(C2Expression *expression, C2Expression *operand);
void __fastcall detach_input_operand(C2Expression *expression, C2Expression *operand);
int __fastcall expression_is_unary_operation(C2Expression *expression);
C2Expression *__fastcall eliminate_expression_node(C2Expression *expression, C2Expression *context);
int __fastcall conversion_may_change_value(unsigned short from, unsigned short to)
{
    int from_integral, to_integral;
    if (from == to) return 0;
    if ((from & 0xfff) < (to & 0xfff)) return 1;
    from_integral = ((1U << (from >> 12)) & 0x0e) != 0;
    to_integral = ((1U << (to >> 12)) & 0x0e) != 0;
    if (!((from ^ to) & 0xfff) && from_integral != to_integral) return 1;
    return from_integral && (to & 0xf000) == 0x4000;
}
int __fastcall simplify_unary_constant(C2Expression *operand, C2Expression *context, int mode);
int __fastcall compare_expression_trees(C2Expression *left, C2Expression *right);
int __fastcall input_has_side_effects(C2Expression *expression);
int __fastcall expression_has_unsafe_inputs(C2Expression *expression);
int __fastcall expression_has_unsafe_effects(C2Expression *expression)
{
    if (input_has_side_effects(expression)) return 1;
    return expression_has_unsafe_inputs(expression);
}
int __fastcall expression_values_are_integral(C2Expression *operands);
int __fastcall expression_is_power_safe(C2Expression *expression);
int __fastcall expression_is_nonnegative(C2Expression *expression);
int __fastcall integer_constant_differs(C2Expression *expression, unsigned __int64 value);
unsigned __int64 __fastcall expression_shift_mask(C2Expression *expression);
int __fastcall retain_full_shift_mask(C2Expression *operand, C2Expression *expression);
void __fastcall apply_integer_mask(C2Expression *expression, unsigned __int64 mask);
__int64 __stdcall integer_log2(unsigned __int64 value)
{
    unsigned result = 0;
    for (value >>= 1; value; value >>= 1) ++result;
    return (__int64)result;
}
int c2_real_divide(C2Real *left, C2Real *right, C2Real *result);
int __fastcall evaluate_comparison_rewrite(unsigned short type, C2Expression *result,
    C2Expression *left, C2Expression *right, unsigned condition, int mode);
void __fastcall remove_statement_expression(C2Record *statement, C2Expression *expression);
C2Expression *__fastcall simplify_nested_comparison(C2Expression *expression, C2Expression *context);
C2Expression *__fastcall simplify_boolean_comparison(C2Expression *expression, C2Expression *context);

#define VALUE_OPERAND(value) ((1U << ((value)->kind & 31)) & 0x7fe)
#define INTEGER_TYPE(type) ((1U << ((type) >> 12)) & 14)
#define ZERO_VALUE(type) (INTEGER_TYPE(type) ? new_integer_expression((type), 0) : \
                                                new_real_expression((type), &c2_real_zero))
#if defined(RECON_NATIVE_TEST) && defined(__linux__)
#define INVERSE_CONDITION(code) ((unsigned)(code) <= 16 ? input_inverse_condition[(code)] : \
    (recon_unsupported("unmodeled optimizer comparison inverse"), 0))
#else
#define INVERSE_CONDITION(code) input_inverse_condition[(code)]
#endif

C2Expression *__fastcall simplify_expression_operation(C2Expression *expression,
                                                       C2Expression *context)
{
    C2Expression *operand, *next, *first, *second, *value, *constant, *inner;
    C2Expression *following, *other, *other_next, *positive, *negative;
    C2Real *real;
    unsigned short type, operand_type, inner_type;
    unsigned __int64 number, reduced;
    unsigned char *condition, branch_condition, comparison;
    long difference;
    int identity, absorber, found;

    if (VALUE_OPERAND(expression)) return expression;
restart:
    for (operand = expression->left; operand; operand = next) {
        next = operand->next;
        if (operand == expression->left && !next) break;
        if (!((1U << (operand->kind & 31)) & 0x180) ||
            (operand == expression->left && !expression_can_flatten(expression))) {
            if (expression->opcode == operand->opcode && expression->type == operand->type &&
                expression_can_flatten(expression)) {
                if (!next) next = operand->left;
                flatten_expression_inputs(expression, operand);
                remove_input_operand(expression, operand);
            }
            continue;
        }
        constant = operation_identity(expression->opcode, expression->type);
        if ((expression->type & 0xf000) == 0x4000)
            identity = constant && c2_real_compare(1, operand->value.real, constant->value.real);
        else
            identity = constant && operand->integer.value == constant->integer.value;
        if (constant) release_small_node(constant);
        constant = operation_absorber(expression->opcode, expression->type);
        if ((expression->type & 0xf000) == 0x4000)
            absorber = constant && c2_real_compare(1, operand->value.real, constant->value.real);
        else
            absorber = constant && operand->integer.value == constant->integer.value;
        if (constant) release_small_node(constant);
        if (identity) {
            clear_expression_values(operand);
            remove_input_operand(expression, operand);
            expression_rewrite_changed = 1;
        } else if (absorber) {
            detach_input_operand(expression, operand);
            clear_expression_values(expression);
            if (expression->left) release_small_node_list(expression->left);
            expression->left = operand;
            goto reduce;
        }
    }
    if (!expression->left->next && !expression_is_unary_operation(expression))
        return eliminate_expression_node(expression, context);
    first = expression->left;
    second = first->next;
    switch (expression->opcode) {
    case 0x135:
        while (first->opcode == 0x135 &&
            (!conversion_may_change_value(first->type, expression->type) ||
             !conversion_may_change_value(first->type, first->left->type))) {
            operand_type = first->type;
            inner_type = first->left->type;
            if (INTEGER_TYPE(operand_type) && INTEGER_TYPE(inner_type)) {
                if ((operand_type & 0xf000) == 0x2000 && (inner_type & 0xf000) == 0x1000)
                    break;
                if (!((operand_type ^ inner_type) & 0xfff) &&
                    (operand_type & 0xf000) == 0x1000 && (inner_type & 0xf000) == 0x2000)
                    break;
            }
            if ((expression->type & 0xf000) == 0x4000 && operand_type == 0x1008 &&
                inner_type == 0x2008) break;
            first = eliminate_expression_node(first, expression);
            expression_rewrite_changed = 1;
        }
        if (expression->type != first->type) return expression;
        goto reduce;
    case 0x136:
        if (first->opcode == 0x136) {
            value = eliminate_expression_node(expression, context);
            value = eliminate_expression_node(value, context);
            expression_rewrite_changed = 1;
            return value;
        }
        if (first->opcode != 0x137) return expression;
        eliminate_expression_node(first, expression);
        expression->opcode = 0x141;
        copy_input_operand(expression, new_integer_expression(expression->type, ~(unsigned __int64)0));
        goto changed;
    case 0x137:
        if (!simplify_unary_constant(first, expression, 3)) return expression;
        goto reduce;
    case 0x141:
        for (operand = expression->left; operand; operand = following) {
            following = operand->next;
            if (operand->opcode != 0x137) continue;
            for (other = expression->left; other; other = other_next) {
                other_next = other->next;
                if (!compare_expression_trees(other, operand->left) && !expression_has_unsafe_effects(other)) {
                    if (following == other) following = other_next;
                    clear_expression_values(other);
                    remove_input_operand(expression, other);
                    clear_expression_values(operand);
                    value = ZERO_VALUE(operand->type);
                } else if (other->kind == 3 && operand->left->kind == 3 &&
                           other->value.reg->canonical == operand->left->value.reg->canonical) {
                    difference = (long)((unsigned)other->value.reg->displacement -
                                        (unsigned)operand->left->value.reg->displacement);
                    value = new_integer_expression(expression->type, (__int64)difference);
                    if (following == other) following = other_next;
                    remove_input_operand(expression, other);
                } else continue;
                attach_input_expression(expression, operand, value);
                expression_rewrite_changed = 1;
                break;
            }
        }
        return expression;
    case 0x142:
        if (!compare_expression_trees(first, second) && !expression_has_unsafe_effects(first))
            goto zero;
        if ((first->kind == 7 && !first->integer.value) ||
            (first->kind == 8 && c2_real_compare(1, first->value.real, &c2_real_zero))) {
            negate_rewritten_operand(second, expression);
            goto remove_first;
        }
        if (first->kind != 3 || second->kind != 3 ||
            first->value.reg->canonical != second->value.reg->canonical) return expression;
        difference = (long)((unsigned)first->value.reg->displacement -
                            (unsigned)second->value.reg->displacement);
        value = new_integer_expression(expression->type, (__int64)difference);
        if (context) value = attach_input_expression(context, expression, value);
        expression_rewrite_changed = 1;
        return value;
    case 0x145:
        if (first->opcode == 0x143 && expression_values_are_integral(first->left) &&
            expression_is_power_safe(expression)) {
            flatten_expression_inputs(expression, first);
            remove_input_operand(expression, first);
            return fold_expression_constants(expression, context);
        }
        /* Fall through to the common shift combination. */
    case 0x14b:
        if (first->opcode == expression->opcode) {
            if (second->kind != 7) return expression;
            inner = first->left;
            constant = inner->next;
            if (constant->kind == 7) {
                expression->left = inner;
                inner->next = first;
                first->next = NULL;
                first->opcode = 0x141;
                if ((first->type & 0xfff) > 4) {
                    first->type = 0x1004;
                    set_input_expression_type(first->tail.right, 0x1004);
                }
                first->left = constant;
                constant->next = second;
                second->next = NULL;
                expression_rewrite_changed = 1;
                return expression;
            }
        }
        if (second->kind != 7 || retain_full_shift_mask(first, expression)) return expression;
        apply_integer_mask(first, expression_shift_mask(expression));
        return expression;
    case 0x146: case 0x147:
        for (operand = expression->left; operand; operand = operand->next) {
            if (operand->kind == 7) continue;
            for (constant = expression->left; constant; constant = constant->next) {
                if (constant->kind != 7) continue;
                number = constant->integer.value;
                if (expression->opcode != 0x146)
                    number = normalize_integer_value(expression->type, ~number);
                apply_integer_mask(operand, number);
            }
        }
        return expression;
    case 0x148:
        for (operand = first; operand; operand = operand->next)
            if (operand->kind == 7 && !integer_constant_differs(operand, ~(unsigned __int64)0) &&
                operand != expression->left) break;
        if (!operand) return expression;
        remove_input_operand(expression, operand);
        first = expression->left;
        detach_input_operand(expression, first);
        value = new_assignment_expression(0x136, expression->type, first, NULL, NULL);
        prune_detached_operands(value);
        value->control_flags = 2;
        value->next = expression->left;
        expression->left = value;
        goto changed;
    case 0x149:
        if (!c2_float_precision && (expression->type & 0xf000) == 0x4000 && second &&
            !second->next && second->kind == 8 && !c2_real_compare(1, second->value.real, &c2_real_zero)) {
            expression->opcode = 0x143;
            real = c2_pool_allocate(0, sizeof(C2Real));
            c2_real_divide(&c2_real_one, expression->left->next->value.real, real);
            value = new_real_expression(expression->left->next->type, real);
            attach_input_expression(expression, expression->left->next, value);
            expression_rewrite_changed = 1;
            return expression;
        }
        /* Integral division and multiplication share the following reduction. */
    case 0x143:
        for (operand = expression->left; operand; operand = operand->next) {
            if (expression->opcode == 0x143) {
                if ((operand->opcode == 0x145 || operand->opcode == 0x14b) &&
                    expression_values_are_integral(expression->left) && operand->opcode == 0x145 &&
                    expression_is_power_safe(expression)) {
                    flatten_expression_inputs(expression, operand);
                    remove_input_operand(expression, operand);
                    return fold_expression_constants(expression, context);
                }
            } else if (operand == expression->left->next && operand->kind == 7) {
                number = operand->integer.value;
                if (!((number - 1) & number)) {
                    if (!number) return expression;
                    if ((expression->type & 0xf000) == 0x2000 || expression_is_nonnegative(expression->left)) {
                        type = expression->type;
                        if ((type & 0xfff) > 4)
                            type = (type & 0xf000) == 0x1000 ? 0x1004 : 0x2004;
                        number = normalize_integer_value(type, integer_log2(number));
                        operand->integer.value = number;
                        operand->type = type;
                        first = expression->left;
                        first->next = operand;
                        operand->next = NULL;
                        expression->opcode = 0x14b;
                        expression_rewrite_changed = 1;
                        goto restart;
                    }
                }
            }
            if (expression->opcode != 0x143 && (expression->type & 0xf000) == 0x2000) continue;
            if (operand->opcode == 0x137) {
                eliminate_expression_node(operand, expression);
                expression_rewrite_changed = 1;
                goto negate;
            }
            if (operand->kind != 7) continue;
            if (!integer_constant_differs(operand, ~(unsigned __int64)0)) {
                remove_input_operand(expression, operand);
                goto negate;
            }
            if ((operand->type & 0xf000) == 0x2000) {
                number = operand->integer.value;
                reduced = normalize_integer_value(operand->type, 0 - number);
                if (reduced < number) {
                    detach_input_operand(expression, operand);
                    found = simplify_unary_constant(expression, context, 2);
                    copy_input_operand(expression, operand);
                    if (!found) return expression;
                    negate_rewritten_operand(operand, expression);
                    fold_expression_constants(operand, expression);
                    return expression;
                }
            }
            if (simplify_unary_constant(expression, context, 0)) {
                negate_rewritten_operand(operand, expression);
                fold_expression_constants(operand, expression);
                return expression;
            }
        }
        return expression;
    case 0x14a:
        if (second->kind == 7) {
            number = second->integer.value;
            if (!number) return expression;
            if (second->integer.low == 1 && !second->integer.high) goto zero;
        }
        if ((expression->type & 0xf000) == 0x1000 && simplify_unary_constant(second, expression, 0))
            return expression;
        if ((expression->type & 0xf000) != 0x2000 || second->kind != 7) return expression;
        number = second->integer.value;
        if ((number - 1) & number) return expression;
        expression->opcode = 0x146;
        second->integer.value = normalize_integer_value(expression->type, number - 1);
        expression_rewrite_changed = 1;
        return expression;
    case 0x14c:
        if ((first->kind != 7 || (first->location.expression.comparison & 0x40)) &&
            first->kind != 8) return expression;
        remove_input_operand(expression, second);
        goto reduce;
    case 0x151:
        if (first->opcode != 0x15e || first->left->opcode != 0x151) return expression;
        inner = first->left;
        condition = context ? (unsigned char *)&context->detail.data :
                              &((C2Expression *)expression_statement_context)->convention;
        positive = inner->next;
        negative = positive->next;
        branch_condition = *condition;
        if (branch_condition != 1 && branch_condition != 2) return expression;
        if (!compare_expression_trees(positive, negative)) return expression;
        if (!compare_expression_trees(second, negative) && !expression_has_unsafe_effects(second) &&
            positive->kind == 7) {
            comparison = (unsigned char)first->detail.intrinsic;
            if (branch_condition == 1) comparison = INVERSE_CONDITION(comparison);
        } else {
            if (compare_expression_trees(second, positive) || expression_has_unsafe_effects(second) ||
                negative->kind != 7) return expression;
            comparison = (unsigned char)first->detail.intrinsic;
            if (branch_condition == 2) comparison = INVERSE_CONDITION(comparison);
        }
        if (context) {
            other = context->left->next;
            other_next = other->next;
            if (!VALUE_OPERAND(other) || !VALUE_OPERAND(other_next)) return expression;
            if (!evaluate_comparison_rewrite(context->type, inner->tail.right, other, other_next,
                                            INVERSE_CONDITION(comparison), 0)) return expression;
        }
        *condition = comparison;
        detach_input_operand(first, inner);
        if (context) {
            attach_input_expression(context, expression, inner);
        } else {
            remove_statement_expression(expression_statement_context, expression);
            attach_call_result((C2Expression *)expression_statement_context, inner);
            release_small_node(expression);
        }
        return inner;
    case 0x15e:
        value = simplify_nested_comparison(expression, context);
        if (!value) value = simplify_boolean_comparison(expression, context);
        if (!value) return expression;
        expression_rewrite_changed = 1;
        return value;
    default:
        return expression;
    }
zero:
    clear_expression_values(expression);
    value = ZERO_VALUE(expression->type);
    if (context) value = attach_input_expression(context, expression, value);
    expression_rewrite_changed = 1;
    return value;
remove_first:
    remove_input_operand(expression, first);
reduce:
    value = eliminate_expression_node(expression, context);
    expression_rewrite_changed = 1;
    return value;
negate:
    value = simplify_expression_operation(expression, context);
    expression = negate_rewritten_operand(value, context);
changed:
    expression_rewrite_changed = 1;
    if (VALUE_OPERAND(expression)) return expression;
    goto restart;
}

C2Expression *__fastcall operation_identity(unsigned opcode, unsigned short type)
{
    C2Real *value, *identity;
    if (INTEGER_TYPE(type)) {
        switch (opcode) {
        case 0x141: case 0x142: case 0x145: case 0x147: case 0x148: case 0x14b:
            return new_integer_expression(type, 0);
        case 0x143: case 0x149:
            return new_integer_expression(type, 1);
        case 0x14a:
            if ((type & 0xf000) == 0x1000) return NULL;
            /* Unsigned remainder uses the all-ones identity. */
        case 0x146:
            return new_integer_expression(type, normalize_integer_value(type, ~(unsigned __int64)0));
        }
    } else if ((type & 0xf000) == 0x4000) {
        switch (opcode) {
        case 0x141: case 0x142:
            identity = &c2_real_zero;
            break;
        case 0x143: case 0x149:
            identity = &c2_real_one;
            break;
        default:
            return NULL;
        }
        value = c2_pool_allocate(0, sizeof(C2Real));
        *value = *identity;
        return new_real_expression(type, value);
    }
    return NULL;
}

C2Expression *__fastcall operation_absorber(unsigned opcode, unsigned short type)
{
    C2Real *value;
    if (opcode == 0x143 || opcode == 0x146) {
        if (INTEGER_TYPE(type)) return new_integer_expression(type, 0);
        if ((type & 0xf000) == 0x4000) {
            value = c2_pool_allocate(0, sizeof(C2Real));
            *value = c2_real_zero;
            return new_real_expression(type, value);
        }
    } else if (opcode != 0x147) {
        return NULL;
    }
    return new_integer_expression(type, normalize_integer_value(type, ~(unsigned __int64)0));
}

unsigned __int64 __fastcall expression_sign_mask(C2Expression *expression, unsigned bits);

void __fastcall apply_integer_mask(C2Expression *expression, unsigned __int64 mask)
{
    C2Expression *operand, *constant;
    unsigned __int64 limit, number, clipped, alternative;
    unsigned char shift;
    for (;;) {
        switch (expression->opcode) {
        case 0x145:
            operand = expression->left;
            constant = operand->next;
            if (constant->kind != 7) return;
            shift = (unsigned char)constant->integer.low;
            number = shift >= 64 ? 0 : mask >> shift;
            mask = expression_shift_mask(expression) & number;
            expression = operand;
            break;
        case 0x14b:
            operand = expression->left;
            constant = operand->next;
            number = 0;
            if (constant->kind != 7) return;
            if ((long)constant->integer.high < 0 ||
                (!constant->integer.high && constant->integer.low < 32)) {
                shift = (unsigned char)constant->integer.low;
                number = shift >= 64 ? 0 : mask << shift;
            }
            if ((expression->type & 0xf000) == 0x1000)
                number |= expression_sign_mask(expression, 0x80000000U);
            mask = normalize_integer_value(expression->type, number);
            expression = operand;
            break;
        case 0x136:
            expression = expression->left;
            break;
        case 0x141: case 0x142:
            shift = (unsigned char)integer_log2(mask);
            limit = (shift >= 64 ? 0 : (unsigned __int64)2 << shift) - 1;
            for (operand = expression->left; operand; operand = operand->next) {
                apply_integer_mask(operand, limit);
                constant = operand;
                if (operand->opcode == 0x146 || operand->opcode == 0x145)
                    constant = operand->left;
                if (constant->kind != 7) continue;
                if (operand->opcode == 0x145) {
                    number = 0;
                    if ((long)constant->integer.high < 0 ||
                        (!constant->integer.high && constant->integer.low < 32)) {
                        shift = (unsigned char)constant->integer.low;
                        number = shift >= 64 ? 0 : (unsigned __int64)1 << shift;
                    }
                } else {
                    number = constant->integer.value;
                }
                mask |= number;
                number = (0 - mask) & mask;
                limit &= 0 - number;
            }
            return;
        case 0x143:
            shift = (unsigned char)integer_log2(mask);
            mask = (shift >= 64 ? 0 : (unsigned __int64)2 << shift) - 1;
            /* Multiplication then shares the bitwise operators' operand walk. */
        case 0x146: case 0x147: case 0x148:
            for (operand = expression->left; operand; operand = operand->next)
                apply_integer_mask(operand, mask);
            return;
        case 0x11d:
            number = expression->integer.value;
            clipped = number & mask;
            alternative = normalize_integer_value(expression->type, (~number) & mask);
            if (alternative < clipped)
                clipped = number | normalize_integer_value(expression->type, ~mask);
            if (expression->integer.value != clipped) {
                expression->integer.value = clipped;
                expression_rewrite_changed = 1;
            }
            return;
        default:
            return;
        }
    }
}

int __fastcall expression_is_binary_operation(C2Expression *expression);
C2Expression *__fastcall fold_integer_operation(C2Expression *expression, unsigned short type,
                                               C2Expression *left, C2Expression *right);
C2Expression *__fastcall fold_real_operation(C2Expression *expression, unsigned short type,
                                            C2Expression *left, C2Expression *right);

C2Expression *__fastcall fold_expression_constants(C2Expression *expression, C2Expression *context)
{
    C2Expression *operand, *next, *accumulator, *address, *value;
    C2FunctionRegister *reg;
    long displacement;
    if (!expression_can_flatten(expression)) {
        operand = expression->left;
        if (expression_is_unary_operation(expression)) {
            if (operand->kind == 7)
                value = fold_integer_operation(expression, expression->type, operand, NULL);
            else if (operand->kind == 8)
                value = fold_real_operation(expression, expression->type, operand, NULL);
            else return expression;
            if (!value) return expression;
        } else {
            if (!expression_is_binary_operation(expression)) return expression;
            if (operand->kind == 7 && operand->next->kind == 7)
                value = fold_integer_operation(expression, expression->type, operand, operand->next);
            else if (operand->kind == 8 && operand->next->kind == 8)
                value = fold_real_operation(expression, expression->type, operand, operand->next);
            else return expression;
            if (!value) return expression;
            remove_input_operand(expression, expression->left->next);
        }
        expression = eliminate_expression_node(expression, context);
        expression_rewrite_changed = 1;
        return expression;
    }
    if (INTEGER_TYPE(expression->type)) {
        accumulator = NULL;
        address = NULL;
        for (operand = expression->left; operand; operand = next) {
            next = operand->next;
            if (operand->kind == 7) {
                if (!accumulator) accumulator = operand;
                else if (fold_integer_operation(expression, expression->type, accumulator, operand)) {
                    remove_input_operand(expression, operand);
                    expression_rewrite_changed = 1;
                }
            } else if (operand->kind == 3 && expression->opcode == 0x141) {
                address = operand;
            }
        }
        if (address && accumulator) {
            reg = address->value.reg;
            displacement = (long)accumulator->integer.low;
            if ((long)((unsigned)reg->displacement + (unsigned)displacement) >= 0 &&
                ((reg->type & 0xf000) == 0x5000 || displacement < (long)(reg->type & 0xfff))) {
                address->value.reg = new_address_register(reg, 0x5000, 0, displacement);
                remove_input_operand(expression, accumulator);
            }
        }
    } else if ((expression->type & 0xf000) == 0x4000) {
        accumulator = NULL;
        for (operand = expression->left; operand; operand = next) {
            next = operand->next;
            if (operand->kind != 8) continue;
            if (!accumulator) accumulator = operand;
            else {
                accumulator = fold_real_operation(expression, expression->type, accumulator, operand);
                if (accumulator) {
                    remove_input_operand(expression, operand);
                    expression_rewrite_changed = 1;
                }
            }
        }
    }
    if (!expression->left->next) return eliminate_expression_node(expression, context);
    return expression;
}

int __fastcall operation_is_associative(unsigned opcode);

int __fastcall expression_can_flatten(C2Expression *expression)
{
    return operation_is_associative(expression->opcode);
}

int __fastcall operation_is_associative(unsigned opcode)
{
    switch (opcode) {
    case 0x141:
    case 0x143:
    case 0x146:
    case 0x147:
    case 0x148:
    case 0x152:
    case 0x153:
        return 1;
    default:
        return 0;
    }
}

int __fastcall expression_is_unary_operation(C2Expression *expression)
{
    switch (expression->opcode) {
    case 0x130:
    case 0x131:
    case 0x132:
    case 0x133:
    case 0x134:
    case 0x135:
    case 0x136:
    case 0x137:
    case 0x138:
    case 0x139:
    case 0x13a:
    case 0x140:
    case 0x15a:
    case 0x15d:
        return 1;
    default:
        return 0;
    }
}

int __fastcall expression_is_binary_operation(C2Expression *expression)
{
    switch (expression->opcode) {
    case 0x141:
    case 0x142:
    case 0x143:
    case 0x145:
    case 0x146:
    case 0x147:
    case 0x148:
    case 0x149:
    case 0x14a:
    case 0x14b:
    case 0x14c:
    case 0x151:
    case 0x152:
    case 0x153:
    case 0x154:
    case 0x155:
    case 0x156:
    case 0x157:
        return 1;
    default:
        return 0;
    }
}

int __fastcall mark_expression_execution(C2Expression *expression, C2Expression *result);
C2Expression *__fastcall previous_expression_operand(C2Expression *head, C2Expression *value)
{
    C2Expression *previous = NULL;
    while (head && head != value) {
        previous = head;
        head = head->next;
    }
    return previous;
}
/* The second input is the previous link: replace its next field by the
 * inserted expression, returning the actual (possibly copied) node. */
C2Expression *__fastcall replace_expression_operand(C2Expression *context,
    C2Expression *previous, C2Expression *replacement)
{
    unsigned mask = 1U << (replacement->kind & 31);
    if (mask & 0x7fe) {
        if (replacement->location.expression.properties & 0x10)
            replacement = copy_input_expression(replacement);
        replacement->location.expression.properties |= 0x10;
    }
    replacement->next = previous->next;
    previous->next = replacement;
    mask = 1U << (replacement->kind & 31);
    if ((mask & 0x7fe) && (mask & 0x60) &&
        !(replacement->location.expression.comparison & 0x20)) {
        if (replacement->aggregate_value) {
            replacement->aggregate_value->location.expression.properties &= ~0x10;
            copy_input_operand(context, replacement->aggregate_value);
        }
        if (replacement->aggregate_second) {
            replacement->aggregate_second->location.expression.properties &= ~0x10;
            copy_input_operand(context, replacement->aggregate_second);
        }
    }
    return replacement;
}

C2Expression *__fastcall fold_expression_value(C2Expression *expression, C2Expression *context)
{
    C2Expression *component = expression, *definition, *source, *result, *previous;
    C2FunctionRegister *reg;
    unsigned aggregate = (1U << (expression->kind & 31)) & 0x60;
    unsigned source_kind;
    unsigned short type;

    if (aggregate) component = expression->aggregate_value;
    definition = component->value.reg->value;
    source = definition->left;
    if (source->kind == 4 || definition->control_flags == 1 || source->next ||
        (aggregate && !((1U << (source->kind & 31)) & 0x1e)))
        return expression;
    if (((source->type ^ component->type) & 0xfff) ||
        ((source->flags & 1) && mark_expression_execution(source, NULL)))
        return expression;
    source_kind = 1U << (source->kind & 31);
    type = source_kind & 0x18 ? expression->type : component->type;
    result = source;
    if ((source_kind & 6) || ((source_kind & 0x18) && aggregate)) {
        reg = source->value.reg;
        if ((long)reg->canonical->value_bytes <
            (long)((unsigned)reg->displacement + (type & 0xfff)))
            return expression;
        source->value.reg = new_address_register(reg, type, type & 0xfff,
                                                component->value.reg->displacement);
    } else if ((source->flags & 1) && expression->type != source->type) {
        if (definition) {
            previous = previous_expression_operand(definition->left, source);
            detach_input_operand(definition, source);
        } else previous = context;
        result = new_assignment_expression(0x135, expression->type,
                    new_temporary_operand(expression->type), NULL, NULL);
        attach_input_expression(result, result->left, source);
        result->control_flags = 2;
        if (definition) {
            if (previous) replace_expression_operand(definition, previous, result);
            else {
                result->next = definition->left;
                definition->left = result;
            }
        }
    }
    detach_input_operand(definition, result);
    if (VALUE_OPERAND(result)) result->type = type;
    if (aggregate) {
        if (result->kind != 3) {
            expression->aggregate_value = result;
            result->location.expression.properties |= 0x30;
            release_small_node(component);
            result = expression;
            goto release_definition;
        }
        mark_first_input_expression(result);
    }
    if (context) attach_input_expression(context, expression, result);
release_definition:
    release_function_register(definition->tail.right->value.reg->canonical);
    release_small_node(definition);
    return result;
}
C2Expression *__fastcall propagate_conversion_to_operand(C2Expression *expression,
    C2Expression *operand, unsigned short type, C2Expression *context);
C2Expression *__fastcall distribute_expression_conversion(C2Expression *expression,
    C2Expression *operand, unsigned short type, C2Expression *context);

C2Expression *__fastcall propagate_expression_conversion(C2Expression *expression,
    C2Expression *operand, C2Expression *context)
{
    unsigned short source_type = operand->left->type;
    unsigned short result_type = expression->type, type = source_type;
    unsigned char condition;
    C2Expression *other, *previous, *conversion, *result;
    if ((source_type & 0xfff) > (result_type & 0xfff) ||
        !((1U << (source_type >> 12)) & 0x0e) ||
        !((1U << (result_type >> 12)) & 0x0e)) return NULL;
    switch (expression->opcode) {
    case 0x146: case 0x147: case 0x148:
        return distribute_expression_conversion(expression, operand, type, context);
    case 0x14b:
        if (((source_type & 0xf000) == 0x1000) !=
            ((result_type & 0xf000) == 0x1000) &&
            (source_type & 0xf000) != 0x2000) return NULL;
        if (operand != expression->left) return NULL;
        other = expression->left->next;
        if (expression) {
            previous = previous_expression_operand(expression->left, other);
            detach_input_operand(expression, other);
        }
        conversion = new_assignment_expression(0x135, type,
            new_temporary_operand(type), NULL, NULL);
        attach_input_expression(conversion, conversion->left, other);
        conversion->control_flags = 2;
        if (expression) {
            if (previous) replace_expression_operand(expression, previous, conversion);
            else {
                conversion->next = expression->left;
                expression->left = conversion;
            }
        }
        prune_detached_operands(conversion);
        conversion->control_flags = 2;
        return propagate_conversion_to_operand(expression, operand, type, context);
    case 0x151:
        condition = context ? context->detail.condition :
            ((C2Expression *)expression_statement_context)->convention;
        if (condition != 1 && condition != 2) {
            if (!((result_type ^ source_type) & 0xfff)) return NULL;
            if ((result_type & 0xf000) == 0x2000 &&
                (source_type & 0xf000) == 0x1000)
                type = (source_type & 0xfff) | 0x2000;
        }
        other = expression->left;
        if (other->opcode == 0x135 && other->next->opcode == 0x135) {
            if (other == operand) other = other->next;
            if (((source_type & 0xf000) == 0x1000) !=
                ((other->left->type & 0xf000) == 0x1000)) {
                if ((source_type & 0xf000) == 0x2000 ||
                    (source_type & 0xfff) <= (other->left->type & 0xfff))
                    return NULL;
            }
        }
        result = distribute_expression_conversion(expression, operand, type, context);
        if (result && result != expression)
            return eliminate_expression_node(result, context);
        return result;
    }
    return NULL;
}

C2Expression *__fastcall fold_expression_conversion(C2Expression *expression,
    C2Expression *context)
{
    C2Expression *operand = expression->left, *output, *next, *previous, *conversion;
    unsigned short type = expression->type;
    unsigned bytes = type & 0xfff;
    if ((operand->type & 0xfff) < bytes ||
        !((1U << (operand->type >> 12)) & 0x0e) ||
        !((1U << (type >> 12)) & 0x0e)) return expression;
    switch (operand->kind) {
    case 1: case 2: case 3: case 4:
        if (operand->kind == 1 && operand->value.reg->kind == 3 &&
            operand->value.reg->value && (operand->type & 0xfff) > 4)
            return expression;
        expression = eliminate_expression_node(expression, context);
        if ((1U << (expression->kind & 31)) & 6U)
            expression->value.reg = new_address_register(expression->value.reg,
                type, bytes, 0);
        expression->type = type;
        expression_rewrite_changed = 1;
        return expression;
    case 6:
        expression = eliminate_expression_node(expression, context);
        expression->type = type;
        expression_rewrite_changed = 1;
        return expression;
    case 7: case 8:
        return fold_expression_constants(expression, context);
    case 11:
        switch (operand->opcode) {
        case 0x136: case 0x137: case 0x141: case 0x142:
        case 0x143: case 0x146: case 0x147: case 0x148:
            expression = eliminate_expression_node(expression, context);
            output = expression->tail.right;
            expression->type = type;
            output->value.reg = new_address_register(output->value.reg, type, bytes, 0);
            output->type = type;
            for (operand = expression->left; operand; operand = next) {
                next = operand->next;
                if (expression) {
                    previous = previous_expression_operand(expression->left, operand);
                    detach_input_operand(expression, operand);
                }
                conversion = new_assignment_expression(0x135, type,
                    new_temporary_operand(type), NULL, NULL);
                attach_input_expression(conversion, conversion->left, operand);
                conversion->control_flags = 2;
                if (expression) {
                    if (previous) replace_expression_operand(expression, previous, conversion);
                    else {
                        conversion->next = expression->left;
                        expression->left = conversion;
                    }
                }
                prune_detached_operands(conversion);
                conversion->control_flags = 2;
            }
            expression_rewrite_changed = 1;
            break;
        case 0x145:
            expression = eliminate_expression_node(expression, context);
            output = expression->tail.right;
            expression->type = type;
            output->value.reg = new_address_register(output->value.reg, type, bytes, 0);
            output->type = type;
            operand = expression->left;
            if (expression) {
                previous = previous_expression_operand(operand, operand);
                detach_input_operand(expression, operand);
            }
            conversion = new_assignment_expression(0x135, type,
                new_temporary_operand(type), NULL, NULL);
            attach_input_expression(conversion, conversion->left, operand);
            conversion->control_flags = 2;
            if (expression) {
                if (previous) replace_expression_operand(expression, previous, conversion);
                else {
                    conversion->next = expression->left;
                    expression->left = conversion;
                }
            }
            prune_detached_operands(conversion);
            conversion->control_flags = 2;
            expression_rewrite_changed = 1;
            break;
        }
    }
    return expression;
}
int __fastcall distribute_expression_operator(C2Expression **expression, unsigned opcode, C2Expression *context);

C2Expression *__fastcall fold_expression_tree(C2Expression *expression, C2Expression *context)
{
    C2Expression *result = expression, *operand, *next, *first;
    int changed;
    switch (expression->kind) {
    case 1:
    case 2:
        if (expression->value.reg->value && expression->value.reg->value->opcode == 0x131)
            result = fold_expression_value(expression, context);
        break;
    case 5:
    case 6:
        operand = expression->aggregate_value;
        if (operand && ((1U << (operand->kind & 31)) & 6) &&
            operand->value.reg->value && operand->value.reg->value->opcode == 0x131)
            result = fold_expression_value(expression, context);
        break;
    case 11:
        first = expression->left;
        switch (expression->opcode) {
        case 0x135:
            result = fold_expression_conversion(expression, context);
            break;
        case 0x137:
            if (first->opcode == 0x141) {
                result = eliminate_expression_node(expression, context);
                for (operand = result->left; operand; operand = next) {
                    next = operand->next;
                    negate_rewritten_operand(operand, result);
                }
                expression_rewrite_changed = 1;
            }
            break;
        case 0x143:
            distribute_expression_operator(&result, 0x141, context);
            break;
        case 0x145:
            if (first->opcode == 0x141 && distribute_expression_operator(&result, 0x141, context))
                break;
            /* A failed shift distribution continues with the bitwise cases. */
        case 0x14b:
            if (first->opcode == 0x146 && distribute_expression_operator(&result, 0x146, context))
                break;
            if (first->opcode == 0x147 && distribute_expression_operator(&result, 0x147, context))
                break;
            if (first->opcode == 0x148) distribute_expression_operator(&result, 0x148, context);
            break;
        case 0x146:
            if (!distribute_expression_operator(&result, 0x147, context))
                distribute_expression_operator(&result, 0x148, context);
            break;
        case 0x147:
            if (first->opcode == 0x146 &&
                !((1U << (first->left->next->kind & 31)) & 0x180))
                distribute_expression_operator(&result, 0x146, context);
            break;
        }
        break;
    }
    if (!((1U << (result->kind & 31)) & 0x7fe)) {
        changed = 0;
        for (operand = result->left; operand; operand = next) {
            next = operand->next;
            if (fold_expression_tree(operand, result) != operand) changed = 1;
        }
        if (changed) result = fold_expression_tree(result, context);
    }
    return result;
}

int __fastcall factor_common_operands(C2Expression *expression, unsigned opcode, unsigned marker);
int __fastcall contains_factor_operation(C2Expression *expression, unsigned opcode, unsigned marker);
int __fastcall factor_constant_operands(C2Expression *expression, unsigned opcode, unsigned marker);
int __fastcall combine_bitwise_operands(C2Expression *expression);
C2Expression *__fastcall replace_expression_operand(C2Expression *context, C2Expression *previous, C2Expression *replacement);
void __fastcall finish_expression_rewrite(C2Expression *expression);
void __fastcall bind_input_temporary(C2FunctionRegister *reg, C2Expression *value);

C2Expression *__fastcall rewrite_expression_operands(C2Expression *expression, C2Expression *context)
{
    C2Expression *operand, *inner, *result;
    unsigned char condition;
    switch (expression->opcode) {
    case 0x141:
        for (operand = expression->left; operand; operand = operand->next) {
            if (operand->opcode == 0x145 && expression_is_power_safe(operand))
                finish_expression_rewrite(operand);
        }
        if (factor_common_operands(expression, 0x143, 0x137))
            return simplify_expression_operation(expression, context);
        break;
    case 0x146:
        if (factor_common_operands(expression, 0x147, 0x11c))
            return simplify_expression_operation(expression, context);
        if (contains_factor_operation(expression, 0x145, 0x11c) &&
            factor_common_operands(expression, 0x145, 0x11c))
            return simplify_expression_operation(expression, context);
        if (contains_factor_operation(expression, 0x14b, 0x11c) &&
            factor_common_operands(expression, 0x14b, 0x11c))
            return simplify_expression_operation(expression, context);
        return expression;
    case 0x147:
    case 0x148:
        if (factor_common_operands(expression, 0x146, 0x11c))
            return simplify_expression_operation(expression, context);
        if (contains_factor_operation(expression, 0x145, 0x11c) &&
            factor_common_operands(expression, 0x145, 0x11c))
            return simplify_expression_operation(expression, context);
        if (contains_factor_operation(expression, 0x14b, 0x11c) &&
            factor_common_operands(expression, 0x14b, 0x11c))
            return simplify_expression_operation(expression, context);
        break;
    case 0x151:
        condition = context ? (unsigned char)(unsigned)context->detail.data :
                             ((C2Expression *)expression_statement_context)->convention;
        if (contains_factor_operation(expression, 0x14b, 0x11c) &&
            factor_constant_operands(expression, 0x14b, 0x11c))
            goto shifted_comparison;
        if (((expression->type & 0xf000) != 0x1000 || condition == 1 || condition == 2) &&
            contains_factor_operation(expression, 0x145, 0x11c) &&
            factor_constant_operands(expression, 0x145, 0x11c)) {
shifted_comparison:
            operand = expression->left;
            if (operand->next) {
                release_small_node(operand->next);
                operand = expression->left;
                operand->next = NULL;
            }
            inner = operand->left;
            expression->left = inner->left;
            inner->left = NULL;
            if (context) {
                replace_expression_operand(context, expression, operand);
                detach_input_operand(context, expression);
            }
            attach_input_expression(operand, inner, expression);
            operand->left->next->integer.value = expression_shift_mask(operand);
            operand->left->next->type = operand->type;
            operand->opcode = 0x146;
            result = operand;
            distribute_expression_operator(&result, 0x151, context);
            return result;
        }
        if ((condition == 1 || condition == 2) && expression->left->opcode == 0x146 &&
            expression->left->next->opcode == 0x146 &&
            factor_constant_operands(expression, 0x146, 0x11c)) {
            copy_input_operand(expression, new_integer_expression(expression->type, 0));
            inner = expression->left->left->next;
            inner->opcode = 0x148;
            replace_input_result(inner, inner->tail.right, new_temporary_operand(inner->type));
            bind_input_temporary(inner->tail.right->value.reg, inner);
        }
        return expression;
    default:
        return expression;
    }
    if (combine_bitwise_operands(expression))
        return simplify_expression_operation(expression, context);
    return expression;
}

/* These scalar pass settings are written before the factoring helpers run.
 * Their original storage is 48fb40 and 48fb44, respectively. */
unsigned factoring_opcode, factoring_marker;
void initialize_factoring_storage(void);
void __fastcall collect_factor_operands(C2Expression *expression, unsigned opcode, unsigned marker);
void __fastcall arrange_factor_operands(C2Expression *expression, unsigned opcode, unsigned marker);
C2Expression *__fastcall get_factor_operand(C2Expression *expression);
int __fastcall factor_operands_are_equal(C2Expression *left, C2Expression *right);
void finish_factoring_storage(void);
C2Expression *__fastcall common_factored_operand(C2Expression *first, C2Expression *second,
    unsigned opcode)
{
    C2Expression *left = get_factor_operand(first), *right = get_factor_operand(second);
    C2Expression *result;
    if (left->kind != 7) return copy_input_expression(left);
    switch (opcode) {
    case 0x143:
        result = copy_input_expression(right);
        if ((__int64)result->integer.value < 0)
            result->integer.value = 0 - result->integer.value;
        return result;
    case 0x145:
    case 0x14b:
        if (first->kind == 7) return copy_input_expression(right);
        if (second->kind == 7) return copy_input_expression(left);
        return copy_input_expression((__int64)right->integer.value <=
            (__int64)left->integer.value ? right : left);
    case 0x146:
        return copy_input_expression(left);
    case 0x147:
        return copy_input_expression(right);
    default:
        return NULL;
    }
}
C2Expression *__fastcall subtract_common_factor(C2Expression *common,
    C2Expression *expression, unsigned opcode)
{
    C2Expression *factor = get_factor_operand(expression);
    unsigned __int64 number, subtrahend, difference;
    unsigned __int64 unwritten_value;
    unsigned shift;
    if (factor->kind != 7) return operation_identity(opcode, factor->type);
    number = factor->integer.value;
    subtrahend = common->integer.value;
    if (number == subtrahend &&
        (expression->kind != 7 || (opcode != 0x145 && opcode != 0x14b)))
        return operation_identity(opcode, factor->type);
    switch (opcode) {
    case 0x143:
        number = (__int64)number / (__int64)subtrahend;
        break;
    case 0x145:
        if (expression->kind == 7) {
            shift = (unsigned char)subtrahend;
            number = shift >= 64 ? ((__int64)number < 0 ? ~(unsigned __int64)0 : 0) :
                (unsigned __int64)((__int64)number >> shift);
            break;
        }
        goto subtract;
    case 0x14b:
        if (expression->kind == 7) {
            shift = (unsigned char)subtrahend;
            number = shift >= 64 ? 0 : number << shift;
            break;
        }
subtract:
        difference = number - subtrahend;
        number = (__int64)difference < 0 ? subtrahend - number : difference;
        break;
    case 0x146:
        break;
    case 0x147:
        number ^= subtrahend;
        break;
    default:
        /* Original defect: the default constructs an integer from both fresh,
         * unwritten stack DWORDs. Keep that C local undefined in Win32. */
#ifdef __linux__
        /* Incidental Win32 stack bytes have no equivalent native value. */
        __asm { int 3 }
#endif
        number = unwritten_value;
        break;
    }
    return new_integer_expression(factor->type, number);
}
C2Expression *__fastcall reduce_factored_operand(C2Expression *expression,
    C2Expression *common, unsigned opcode, unsigned marker)
{
    C2Expression *replacement = subtract_common_factor(common, expression, opcode);
    C2Expression *operand = expression, *parent = NULL;
    if (operand->opcode == marker) {
        parent = operand;
        operand = operand->left;
    }
    if (operand->opcode == opcode) {
        parent = operand;
        operand = operand->left;
        if (opcode == 0x145 || opcode == 0x14b) operand = operand->next;
        if (operand->opcode == marker) {
            parent = operand;
            operand = operand->left;
        }
    }
    if (parent) {
        attach_input_expression(parent, operand, replacement);
        return simplify_expression_operation(expression, NULL);
    }
    release_small_node(operand);
    return simplify_expression_operation(replacement, NULL);
}
void __fastcall merge_factored_operands(C2Expression *owner, C2Expression *first,
    C2Expression *second, unsigned opcode, unsigned marker)
{
    C2Expression *common, *left, *right, *inner, *outer;
    unsigned short type;
    common = common_factored_operand(first, second, opcode);
    detach_input_operand(owner, first);
    detach_input_operand(owner, second);
    left = reduce_factored_operand(first, common, opcode, marker);
    right = reduce_factored_operand(second, common, opcode, marker);
    type = owner->type;
    if (owner->opcode == 0x151)
        inner = new_comparison_expression(0x151, type, left, right, NULL, NULL);
    else inner = new_folded_binary_expression(owner->opcode, type, left, right, NULL, NULL);
    prune_detached_operands(inner);
    inner->control_flags = 2;
    inner = fold_expression_constants(inner, NULL);
    if (opcode == 0x145 || opcode == 0x14b)
        outer = new_folded_binary_expression(opcode, type, inner, common, NULL, NULL);
    else outer = new_folded_binary_expression(opcode, type, common, inner, NULL, NULL);
    prune_detached_operands(outer);
    outer->control_flags = 2;
    outer->next = owner->left;
    owner->left = outer;
}

int __fastcall factor_common_operands(C2Expression *expression, unsigned opcode, unsigned marker)
{
    C2Expression *first = expression->left, *second = first->next, *next, *value;
    int changed = 0;
    factoring_marker = marker;
    factoring_opcode = opcode;
    if (second->next || first->opcode == opcode || second->opcode == opcode ||
        (first->opcode == marker && first->left->opcode == opcode) ||
        (second->opcode == marker && second->left->opcode == opcode)) {
        finish_expression_rewrite(expression);
        initialize_factoring_storage();
        collect_factor_operands(expression, opcode, marker);
        arrange_factor_operands(expression, opcode, marker);
    }
    for (second = expression->left->next; second; second = next) {
        next = second->next;
        if ((opcode == 0x145 || opcode == 0x14b) &&
            (expression->left->opcode != opcode || second->opcode != opcode))
            break;
        first = expression->left;
        value = get_factor_operand(second);
        if (!factor_operands_are_equal(get_factor_operand(first), value)) break;
        expression_rewrite_changed = 1;
        changed = 1;
        merge_factored_operands(expression, expression->left, second, opcode, marker);
    }
    finish_factoring_storage();
    if (!changed) return factor_constant_operands(expression, opcode, marker);
    return 1;
}

C2Expression *__fastcall get_factor_operand(C2Expression *expression)
{
    if (expression->opcode == factoring_marker) expression = expression->left;
    if (expression->opcode == factoring_opcode) {
        expression = expression->left;
        if (factoring_opcode == 0x145 || factoring_opcode == 0x14b)
            expression = expression->next;
        if (expression->opcode == factoring_marker) expression = expression->left;
    }
    return expression;
}

int __fastcall factor_operands_are_equal(C2Expression *left, C2Expression *right)
{
    if (compare_expression_trees(left, right)) return 0;
    if (expression_has_unsafe_effects(left)) return 0;
    return 1;
}

void finish_factoring_storage(void)
{
    reset_input_pool(4);
}

void __fastcall reorder_constant_factors(C2Expression *expression, unsigned opcode, unsigned marker);
int __fastcall can_merge_factored_constants(C2Expression *owner, C2Expression *first,
    C2Expression *second, unsigned opcode);

int __fastcall factor_constant_operands(C2Expression *expression, unsigned opcode, unsigned marker)
{
    C2Expression *operand, *value, *first, *second;
    int count = 0, changed = 0;
    factoring_marker = marker;
    factoring_opcode = opcode;
    for (operand = expression->left; operand && count < 2; operand = operand->next) {
        value = operand;
        if (value->opcode == marker) value = value->left;
        if (value->kind == 7) {
            ++count;
        } else if (value->opcode == opcode) {
            value = value->left;
            if (opcode == 0x145 || opcode == 0x14b) {
                if (value->next->kind == 7) ++count;
            } else {
                for (; value; value = value->next) {
                    if (value->kind == 7) {
                        ++count;
                        break;
                    }
                }
            }
        }
    }
    if (count < 2) return 0;
    finish_expression_rewrite(expression);
    reorder_constant_factors(expression, opcode, marker);
    for (first = expression->left; first; first = first->next) {
        value = get_factor_operand(first);
        if (value->kind != 7) break;
        if (expression->opcode == 0x147 && opcode == 0x146 && value == first) break;
        for (second = first->next; second; second = second->next) {
            value = get_factor_operand(second);
            if (value->kind != 7) break;
            if (expression->opcode == 0x147 && opcode == 0x146 && value == second) break;
            if (can_merge_factored_constants(expression, first, second, opcode)) {
                expression_rewrite_changed = changed = 1;
                merge_factored_operands(expression, first, second, opcode, marker);
                break;
            }
        }
        if (changed) break;
    }
    return changed;
}

int __fastcall contains_factor_operation(C2Expression *expression, unsigned opcode, unsigned marker)
{
    C2Expression *operand = expression->left;
    while (operand) {
        if (operand->opcode == marker) operand = operand->left;
        if (operand->opcode == opcode) return 1;
        operand = operand->next;
    }
    return 0;
}

int __fastcall expression_can_be_duplicated(C2Expression *expression);
C2Expression *__fastcall copy_reusable_expression(C2Expression *expression);

int __fastcall combine_bitwise_operands(C2Expression *expression)
{
    C2Expression *first = expression->left, *second = first->next;
    C2Expression *a, *b, *left, *right, *swap, *result;
    unsigned short type;
    int exchanged;
    if (first->opcode != 0x146 || !second || second->opcode != 0x146) return 0;
    for (a = first->left; a; a = a->next) {
        if (a->kind != 7) continue;
        for (b = second->left; b; b = b->next) {
            if (b->kind != 7) continue;
            if ((a->integer.value ^ b->integer.value) !=
                normalize_integer_value(expression->type, ~(unsigned __int64)0))
                continue;
            exchanged = a->integer.value < b->integer.value;
            if (!expression_can_be_duplicated(exchanged ? second : expression->left)) return 0;
            first = expression->left;
            expression_rewrite_changed = 1;
            detach_input_operand(first, a);
            left = simplify_expression_operation(first, expression);
            detach_input_operand(expression, left);
            detach_input_operand(second, b);
            right = simplify_expression_operation(second, expression);
            detach_input_operand(expression, right);
            if (exchanged) {
                swap = a; a = b; b = swap;
                swap = left; left = right; right = swap;
            }
            release_small_node(a);
            type = expression->type;
            result = new_folded_binary_expression(0x148, type, left, right, NULL, NULL);
            prune_detached_operands(result);
            result->control_flags = 2;
            result = new_folded_binary_expression(0x146, type, result, b, NULL, NULL);
            prune_detached_operands(result);
            result->control_flags = 2;
            result = new_folded_binary_expression(0x148, type, result,
                copy_reusable_expression(left), NULL, NULL);
            prune_detached_operands(result);
            result->control_flags = 2;
            copy_input_operand(expression, result);
            return 1;
        }
    }
    return 0;
}

#define HAS_REGISTER_DEFINITION(e) (((1U << ((e)->kind & 31)) & 6) && (e)->value.reg->value)
#define AGGREGATE_HAS_KEYS(e) (((1U << ((e)->kind & 31)) & 0x60) && \
    (!(e)->aggregate_value || HAS_REGISTER_DEFINITION((e)->aggregate_value)) && \
    (!(e)->aggregate_second || HAS_REGISTER_DEFINITION((e)->aggregate_second)) && \
    (e)->aggregate_value != (e)->aggregate_second)

void __fastcall finish_expression_rewrite(C2Expression *expression)
{
    C2Expression *operand;
    unsigned key;
    if ((1U << (expression->kind & 31)) & 0x7fe) return;
    for (operand = expression->left; operand; operand = operand->next) {
        if (HAS_REGISTER_DEFINITION(operand)) {
            operand->order_key = operand->value.reg->value->order_key;
        } else if (AGGREGATE_HAS_KEYS(operand)) {
            operand->order_key = operand->aggregate_value->value.reg->value->order_key;
        } else if ((1U << (operand->kind & 31)) & 0x7fe) {
            operand->order_key = expression_order_key(operand);
        } else if (expression_can_flatten(operand)) {
            operand->left = sort_expression_inputs(operand->left, compare_expression_keys);
        }
    }
    for (operand = expression->tail.right; operand; operand = operand->next) {
        if (AGGREGATE_HAS_KEYS(operand))
            operand->order_key = operand->aggregate_value->value.reg->value->order_key;
    }
    if (expression_can_flatten(expression))
        expression->left = sort_expression_inputs(expression->left, compare_expression_keys);
    key = expression_order_key(expression);
    expression->order_key = key | expression_value_hash(expression);
}

#undef HAS_REGISTER_DEFINITION
#undef AGGREGATE_HAS_KEYS

C2Expression *__fastcall distribute_widening_conversion(C2Expression *expression, C2Expression *context);
C2Expression *__fastcall factor_negations_and_comparisons(C2Expression *expression, C2Expression *context);

C2Expression *__fastcall simplify_expression_conditions(C2Expression *expression, C2Expression *context)
{
    C2Expression *result, *operand, *next;
    result = distribute_widening_conversion(expression, context);
    for (operand = result->left; operand; operand = next) {
        next = operand->next;
        if (!((1U << (operand->kind & 31)) & 0x7fe))
            simplify_expression_conditions(operand, result);
    }
    result = factor_negations_and_comparisons(result, context);
    result = simplify_expression_operation(result, context);
    return fold_expression_constants(result, context);
}

C2Expression *__fastcall previous_expression_operand(C2Expression *head, C2Expression *value);

C2Expression *__fastcall distribute_widening_conversion(C2Expression *expression, C2Expression *context)
{
    C2Expression *inner, *result, *output, *operand, *next, *previous, *conversion;
    unsigned short to, from, widened, width, difference;
    if (expression->opcode != 0x135) return expression;
    inner = expression->left;
    to = expression->type;
    from = inner->type;
    width = to & 0xfff;
    difference = (to ^ from) & 0xfff;
    widened = from ^ difference;
    if ((from & 0xfff) >= width || !((1U << (from >> 12)) & 0x0e) ||
        !((1U << (to >> 12)) & 0x0e) || width > 4)
        return expression;
    switch (inner->opcode) {
    case 0x137:
        if ((from & 0xf000) == 0x2000 && difference) return expression;
        break;
    case 0x146:
    case 0x147:
    case 0x148:
        break;
    case 0x14b:
        if ((from & 0xf000) == 0x1000 && (to & 0xf000) == 0x2000) return expression;
        break;
    default:
        return expression;
    }
    if (width == 2) return expression;
    inner = eliminate_expression_node(expression, context);
    inner->type = widened;
    output = inner->tail.right;
    output->value.reg = new_address_register(output->value.reg, widened, widened & 0xfff, 0);
    output->type = widened;
    result = inner;
    if (widened != to) {
        previous = NULL;
        if (context) {
            previous = previous_expression_operand(context->left, inner);
            detach_input_operand(context, inner);
        }
        result = new_assignment_expression(0x135, to, new_temporary_operand(to), NULL, NULL);
        attach_input_expression(result, result->left, inner);
        result->control_flags = 2;
        if (context) {
            if (previous) replace_expression_operand(context, previous, result);
            else {
                result->next = context->left;
                context->left = result;
            }
        }
        prune_detached_operands(result);
        result->control_flags = 2;
    }
    for (operand = inner->left; operand; operand = next) {
        next = operand->next;
        previous = NULL;
        if (inner) {
            previous = previous_expression_operand(inner->left, operand);
            detach_input_operand(inner, operand);
        }
        conversion = new_assignment_expression(0x135, widened,
            new_temporary_operand(widened), NULL, NULL);
        attach_input_expression(conversion, conversion->left, operand);
        conversion->control_flags = 2;
        if (inner) {
            if (previous) replace_expression_operand(inner, previous, conversion);
            else {
                conversion->next = inner->left;
                inner->left = conversion;
            }
        }
        prune_detached_operands(conversion);
        conversion->control_flags = 2;
    }
    expression_rewrite_changed = 1;
    return result;
}

C2Expression *__fastcall factor_unary_operands(C2Expression *expression,
    C2Expression *context, unsigned opcode)
{
    C2Expression *operand, *next;
    for (operand = expression->left; operand; operand = operand->next)
        if (operand->opcode != opcode) return expression;
    for (operand = expression->left; operand; operand = next) {
        next = operand->next;
        eliminate_expression_node(operand, expression);
    }
    expression = negate_rewritten_operand(expression, context);
    expression_rewrite_changed = 1;
    return simplify_expression_operation(expression, context);
}

C2Expression *__fastcall factor_negations_and_comparisons(C2Expression *expression, C2Expression *context)
{
    C2Expression *result = expression;
    switch (expression->opcode) {
    case 0x135:
        break;
    case 0x141:
        result = factor_unary_operands(expression, context, 0x137);
        break;
    case 0x14b:
        if (expression->left->opcode == 0x146)
            distribute_expression_operator(&result, 0x146, context);
        break;
    }
    return result;
}

C2Expression *__fastcall canonicalize_expression_tree(C2Expression *expression, C2Expression *context)
{
    C2Expression *operand, *next, *previous, *before_previous;
    C2Expression *replacement, *prefix, *saved;
    for (operand = expression->left; operand; operand = next) {
        next = operand->next;
        if (operand->flags & 1) canonicalize_expression_tree(operand, expression);
    }
    if (expression->opcode != 0x141) return expression;
    previous = before_previous = NULL;
    operand = expression->left;
    while (operand) {
        next = operand->next;
        if (expression->opcode != 0x141 || !simplify_unary_constant(operand, expression, 1)) {
            before_previous = previous;
            previous = operand;
            operand = next;
            continue;
        }
        if (!previous) {
            previous = next;
            while (previous->opcode == 0x137) {
                previous = previous->next;
                if (!previous) {
                    previous = expression->left->next;
                    break;
                }
            }
            detach_input_operand(expression, previous);
            previous->next = expression->left;
            expression->left = previous;
            if ((1U << (previous->kind & 31)) & 0x7fe)
                previous->location.expression.properties |= 0x10;
            next = previous->next->next;
        }
        if (!before_previous && !next) {
            expression->opcode = 0x142;
            operand = next;
            continue;
        }
        operand = previous->next;
        detach_input_operand(expression, operand);
        replacement = new_assignment_expression(0x142, expression->type, operand, NULL, NULL);
        prune_detached_operands(replacement);
        replacement->control_flags = 2;
        if (before_previous) {
            detach_input_operand(expression, previous);
            prefix = new_assignment_expression(0x141, expression->type, previous, NULL, NULL);
            prune_detached_operands(prefix);
            prefix->control_flags = 2;
            before_previous->next = previous;
            previous->next = NULL;
            prefix->left = expression->left;
            previous = prefix;
        }
        saved = replacement->left;
        replacement->left = previous;
        previous->next = saved;
        expression->left = replacement;
        replacement->next = next;
        before_previous = NULL;
        previous = replacement;
        operand = next;
    }
    if (!expression->left->next) return eliminate_expression_node(expression, context);
    return expression;
}

C2Expression *__fastcall prepare_final_expression(C2Expression *expression);
C2Expression *__fastcall restore_expression_operands(C2Expression *expression);

C2Expression *__fastcall finalize_expression_tree(C2Expression *expression)
{
    C2Expression *result, *definition;
    result = prepare_final_expression(expression);
    result = restore_expression_operands(result);
    if ((1U << (result->kind & 31)) & 0x7fe) {
        definition = result->value.reg->value;
        release_small_node(result);
        result = definition;
    }
    return result;
}

void __fastcall flatten_expression_inputs(C2Expression *owner, C2Expression *child)
{
    C2Expression *operand, *last = NULL;
    for (operand = owner->left; operand; operand = operand->next) last = operand;
    if (last) last->next = child->left;
    else owner->left = child->left;
    child->left = NULL;
}

int __fastcall distribute_expression_operator(C2Expression **expression, unsigned opcode, C2Expression *context)
{
    C2Expression *owner = *expression, *constant = NULL, *operand, *next, *nested, *value;
    unsigned outer_opcode;
    unsigned short type;
    for (operand = owner->left; operand; operand = operand->next) {
        if (operand->opcode == opcode) continue;
        if (!((1U << (operand->kind & 31)) & 0x180) || constant) return 0;
        constant = operand;
    }
    if (!constant) return 0;
    detach_input_operand(owner, constant);
    nested = owner->left;
    operand = nested->left;
    nested->left = NULL;
    type = owner->type;
    outer_opcode = owner->opcode;
    while (operand) {
        next = operand->next;
        if ((1U << (operand->kind & 31)) & 0x7fe)
            operand->location.expression.properties &= ~0x10;
        operand->next = NULL;
        value = new_folded_binary_expression(outer_opcode, type, operand,
            copy_input_expression(constant), NULL, NULL);
        value->control_flags = 2;
        copy_input_operand(nested, value);
        value = simplify_expression_operation(value, nested);
        fold_expression_constants(value, nested);
        operand = next;
    }
    release_small_node(constant);
    expression_rewrite_changed = 1;
    *expression = simplify_expression_operation(owner, context);
    return 1;
}

int __fastcall simplify_unary_constant(C2Expression *expression, C2Expression *context, int mode)
{
    C2Expression *operand;
    unsigned kind;
    int pass;
    for (;;) {
        switch (expression->opcode) {
        case 0x135:
            kind = expression->type >> 12;
            if (((1U << kind) & 0xe) &&
                (expression->left->type & 0xfff) < (expression->type & 0xfff)) return 0;
            /* Fall through: permitted conversions are transparent to this search. */
        case 0x145:
            context = expression;
            expression = expression->left;
            continue;
        case 0x149:
            if ((expression->type & 0xf000) == 0x2000) return 0;
            /* Fall through to the staged child search. */
        case 0x143:
            for (pass = 0; pass == 0 || (pass <= mode && pass <= 3); ++pass)
                for (operand = expression->left; operand; operand = operand->next)
                    if (simplify_unary_constant(operand, expression, pass)) return 1;
            return 0;
        case 0x142:
            if (!simplify_unary_constant(expression->left, expression, mode)) return 0;
            expression->opcode = 0x141;
            expression_rewrite_changed = 1;
            return 1;
        case 0x11d:
            if (mode >= 3 || (mode >= 2 && (expression->type & 0xf000) == 0x1000) ||
                (mode >= 1 && normalize_integer_value(expression->type,
                    0 - expression->integer.value) < expression->integer.value) ||
                (mode >= 0 && (expression->type & 0xf000) == 0x2000 &&
                    normalize_integer_value(expression->type,
                        0 - expression->integer.value) < expression->integer.value)) {
                expression->integer.value = normalize_integer_value(expression->type,
                    0 - expression->integer.value);
                expression_rewrite_changed = 1;
                return 1;
            }
            return 0;
        case 0x11e:
            if (mode < 1 && !c2_real_compare(4, expression->value.real, &c2_real_zero)) return 0;
            expression = negate_rewritten_operand(expression, context);
            fold_expression_constants(expression, context);
            expression_rewrite_changed = 1;
            return 1;
        case 0x137:
            eliminate_expression_node(expression, context);
            expression_rewrite_changed = 1;
            return 1;
        case 0x136:
            if (mode != 3) return 0;
            expression->opcode = 0x141;
            copy_input_operand(expression, new_integer_expression(expression->type, 1));
            expression_rewrite_changed = 1;
            return 1;
        default:
            return 0;
        }
    }
}

C2Expression *__fastcall negate_rewritten_operand(C2Expression *expression, C2Expression *context)
{
    C2Expression *previous, *result;
    if (context) {
        previous = previous_expression_operand(context->left, expression);
        detach_input_operand(context, expression);
    }
    result = new_assignment_expression(0x137, expression->type,
        new_temporary_operand(expression->type), NULL, NULL);
    attach_input_expression(result, result->left, expression);
    result->control_flags = 2;
    if (context) {
        if (previous) replace_expression_operand(context, previous, result);
        else {
            result->next = context->left;
            context->left = result;
        }
    }
    return result;
}

int __fastcall retain_full_shift_mask(C2Expression *operand, C2Expression *expression)
{
    C2Expression *inner_constant, *constant, *child;
    unsigned opcode = expression->opcode, shift;
    unsigned __int64 amount, number, mask, difference;
    unsigned short type;
    if (opcode == 0x14b) {
        if ((expression->type & 0xf000) == 0x1000) return 0;
    } else if (operand->opcode == 0x143) {
        for (child = operand->left; child; child = child->next)
            if (retain_full_shift_mask(child, expression)) return 1;
        return 0;
    }
    if (operand->opcode != (opcode == 0x14b ? 0x145 : 0x14b)) return 0;
    inner_constant = operand->left->next;
    if (inner_constant->kind != 7) return 0;
    amount = inner_constant->integer.value;
    for (constant = expression->left; constant && constant->kind != 7; constant = constant->next)
        ;
    /* The original assumes a constant exists and reads through NULL for a
     * malformed list. Keep that fully recovered defect, not a guessed value. */
    number = constant->integer.value;
    mask = expression_shift_mask(operand);
    if (opcode == 0x143) {
        if (integer_log2((0 - number) & number) < (__int64)amount) return 0;
        shift = (unsigned char)amount;
        /* The original uses the signed two-word compiler shift helper. */
        difference = shift >= 64 ? ((__int64)number < 0 ? ~(unsigned __int64)0 : 0) :
            (unsigned __int64)((__int64)number >> shift);
    } else {
        difference = amount - number;
        if ((__int64)difference < 0) difference = number - amount;
        if ((__int64)amount > (__int64)number) expression->opcode = operand->opcode;
    }
    constant->integer.value = normalize_integer_value(constant->type, difference);
    type = operand->type;
    inner_constant->integer.value = normalize_integer_value(type, mask);
    inner_constant->type = type;
    operand->opcode = 0x146;
    expression_rewrite_changed = 1;
    return 1;
}

unsigned __int64 __fastcall expression_shift_mask(C2Expression *expression)
{
    unsigned char shift = (unsigned char)expression->left->next->integer.value;
    unsigned __int64 mask;
    if (expression->opcode == 0x145) {
        mask = normalize_integer_value((expression->type & 0xfff) | 0x2000,
            ~(unsigned __int64)0);
        return shift >= 64 ? 0 : mask >> shift;
    }
    mask = shift >= 64 ? 0 : ~(unsigned __int64)0 << shift;
    return normalize_integer_value(expression->type, mask);
}

C2Expression *__fastcall eliminate_expression_node(C2Expression *expression, C2Expression *context)
{
    C2Expression *operand = expression->left;
    expression->left = NULL;
    if (VALUE_OPERAND(operand))
        operand->location.expression.properties &= ~0x10;
    release_function_register(expression->tail.right->value.reg->canonical);
    if (context) return attach_input_expression(context, expression, operand);
    release_small_node(expression);
    return operand;
}

void __fastcall release_input_aggregate_operands(C2Expression *owner, C2Expression *operand)
{
    C2Expression *component = operand->aggregate_value;
    if (component && !(operand->location.expression.comparison & 0x20)) {
        component->location.expression.properties &= ~0x20;
        detach_input_operand(owner, component);
        component->location.expression.properties |= 0x30;
    }
    component = operand->aggregate_second;
    if (component && !(operand->location.expression.comparison & 0x20)) {
        component->location.expression.properties &= ~0x20;
        detach_input_operand(owner, component);
        component->location.expression.properties |= 0x30;
    }
}

void __fastcall detach_input_operand(C2Expression *expression, C2Expression *operand)
{
    C2Expression *current, *previous = NULL;
    unsigned kind_bit;
    for (current = expression->left; current && current != operand; current = current->next)
        previous = current;
    if (previous) previous->next = operand->next;
    else expression->left = operand->next;
    kind_bit = 1U << (operand->kind & 31);
    operand->next = NULL;
    if (kind_bit & 0x7fe) operand->location.expression.properties &= ~0x10;
    if (kind_bit & 0x60) release_input_aggregate_operands(expression, operand);
}

void __fastcall remove_input_operand(C2Expression *expression, C2Expression *operand)
{
    C2Expression *current, *previous = NULL;
    for (current = expression->left; current && current != operand; current = current->next)
        previous = current;
    if (previous) previous->next = operand->next;
    else expression->left = operand->next;
    if ((1U << (operand->kind & 31)) & 0x60)
        release_result_aggregate_operands(expression, operand);
    release_small_node(operand);
}

int __fastcall expression_has_unsafe_inputs(C2Expression *expression)
{
    C2Expression *part, *base, *index;
    unsigned kinds = 1U << (expression->kind & 31);
    if (!(kinds & 0x7fe)) {
        for (part = expression->left; part; part = part->next)
            if (expression_has_unsafe_inputs(part)) return 1;
        return 0;
    }
    if ((kinds & 6) && expression->value.reg->value)
        return expression_has_unsafe_effects(expression->value.reg->value);
    if (expression->kind != 6) return 0;
    base = expression->aggregate_value;
    if (base && (!((1U << (base->kind & 31)) & 6) || !base->value.reg->value)) return 0;
    index = expression->aggregate_second;
    if (index && (!((1U << (index->kind & 31)) & 6) || !index->value.reg->value)) return 0;
    /* The original admits NULL in the checks above, then reads the base
     * unconditionally. Preserve that fully recovered malformed-input defect. */
    return expression_has_unsafe_effects(base->value.reg->value);
}
