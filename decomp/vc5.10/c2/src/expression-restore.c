#include "optimizer.h"
#include "expression-order.h"
#include "function-records.h"
#include "small-nodes.h"
#include <recon.h>

extern C2Expression *expression_following_record;
int __fastcall expression_is_binary_operation(C2Expression *expression);
int __fastcall expression_is_unary_operation(C2Expression *expression);
void __fastcall finish_restored_expression(C2Expression *expression);
C2Expression *__fastcall order_call_operands(C2Expression *head)
{
    C2Expression *result = NULL, *next;
    while (head) {
        next = head->next;
        head->next = result;
        result = head;
        head = next;
    }
    return result;
}
C2Expression *__fastcall restore_expression_operand(C2Expression *expression);
C2Expression *__fastcall retain_input_result(C2Expression *expression);
void __fastcall retain_restored_aggregate(C2Expression *owner, C2Expression *operand);
void __fastcall prune_detached_operands(C2Expression *expression);
C2Expression *__fastcall replace_input_result(C2Expression *owner,
    C2Expression *old, C2Expression *replacement);

#define IS_VALUE(e) ((1U << ((e)->kind & 31)) & 0x7fe)
#define HAS_DEFINITION(e) (((1U << ((e)->kind & 31)) & 6) && (e)->value.reg->value)
#define DEFINED_AGGREGATE(e) (((1U << ((e)->kind & 31)) & 0x60) && \
    (!(e)->aggregate_value || HAS_DEFINITION((e)->aggregate_value)) && \
    (!(e)->aggregate_second || HAS_DEFINITION((e)->aggregate_second)) && \
    (e)->aggregate_value != (e)->aggregate_second)

C2Expression *__fastcall restore_expression_operands(C2Expression *expression)
{
    C2Expression *operand, *next, *first, *second, *selected, *value;
    C2Expression *old_output, *output, *definition, *result;
    C2FunctionRegister *reg;
    unsigned opcode;
    int count, arity, i;
    if (expression->control_flags == 1) {
        finish_restored_expression(expression);
        if (expression->kind == 13 || expression->kind == 17)
            expression->left = order_call_operands(expression->left);
        if (expression_is_binary_operation(expression) &&
            compare_expression_keys(expression->left, expression->left->next) > 0 &&
            (expression->type & 0xf000) != 0x4000) {
            first = expression->left;
            second = first->next;
            /* The second operand is restored first on this ordering path. */
            for (i = 0; i < 2; ++i) {
                selected = i ? first : second;
                if (HAS_DEFINITION(selected)) {
                    value = restore_expression_operands(selected->value.reg->value);
                    if (IS_VALUE(value)) {
                        if (selected->type != value->type) {
                            value->value.reg = new_address_register(value->value.reg,
                                selected->type, selected->type & 0xfff, 0);
                            value->type = selected->type;
                        }
                        attach_input_expression(expression, selected, value);
                    }
                } else if (DEFINED_AGGREGATE(selected)) {
                    value = restore_expression_operands(selected->aggregate_value->value.reg->value);
                    if (IS_VALUE(value)) {
                        release_small_node(selected->aggregate_value);
                        selected->aggregate_value = value;
                        retain_restored_aggregate(expression, selected);
                    }
                }
            }
        } else {
            for (operand = expression->left; operand; operand = next) {
                next = operand->next;
                if (!HAS_DEFINITION(operand)) continue;
                value = restore_expression_operands(operand->value.reg->value);
                if (!IS_VALUE(value)) continue;
                if (operand->type != value->type) {
                    value->value.reg = new_address_register(value->value.reg,
                        operand->type, operand->type & 0xfff, 0);
                    value->type = operand->type;
                }
                attach_input_expression(expression, operand, value);
            }
        }
        if (expression->kind == 13 || expression->kind == 17)
            expression->left = order_call_operands(expression->left);
        prepend_input_record((C2Record *)expression_following_record, (C2Record *)expression);
        return expression;
    }
    old_output = expression->tail.right;
    old_output->location.expression.properties &= ~0x10;
    value = new_temporary_operand(old_output->type);
    expression->tail.right = retain_input_result(value);
    reg = expression->tail.right->value.reg;
    if (!reg->canonical->variant_next) reg->value = expression;
    else bind_input_temporary(reg, expression);
    if (expression->opcode == 0x15f) {
        arity = 0;
        for (operand = expression->left; operand; operand = operand->next) ++arity;
    } else if (expression->opcode == 0x15e) {
        arity = 3;
    } else {
        arity = expression_is_unary_operation(expression) ? 1 : 2;
    }
    operand = expression->left;
    expression->left = NULL;
    result = expression;
    count = 0;
    if (expression_is_binary_operation(expression) &&
        compare_expression_keys(operand, operand->next) > 0 &&
        (expression->type & 0xf000) != 0x4000) {
        second = restore_expression_operand(operand->next);
        first = restore_expression_operand(operand);
        second->next = first->next = NULL;
        copy_input_operand(expression, first);
        copy_input_operand(expression, second);
    } else {
        opcode = expression->opcode;
        while (operand) {
            next = operand->next;
            operand->next = NULL;
            if (count == arity) {
                finish_restored_expression(result);
                prepend_input_record((C2Record *)expression_following_record, (C2Record *)result);
            }
            value = restore_expression_operand(operand);
            if (count == arity) {
                result = new_folded_binary_expression(opcode, expression->type,
                    result->tail.right, value, NULL, NULL);
                count = 2;
            } else if (opcode == 0x135) {
                result = convert_input_expression(&value, result->type,
                    &expression_following_record->previous);
                release_small_node(expression);
                if (!result) {
                    if (HAS_DEFINITION(value) && HAS_DEFINITION(old_output)) {
                        definition = value->value.reg->value;
                        if (!(value->location.expression.properties & 0x10)) release_small_node(value);
                        value = copy_input_expression(old_output);
                        value->value.reg = new_address_register(value->value.reg,
                            definition->type, definition->type & 0xfff, 0);
                        value->type = definition->type;
                        output = definition->tail.right;
                        release_function_register(output->value.reg->canonical);
                        replace_input_result(definition, output, value);
                        reg = value->value.reg;
                        if (!reg->canonical->variant_next) reg->value = definition;
                        else bind_input_temporary(reg, definition);
                        return old_output;
                    }
                    result = new_assignment_expression(0x131, value->type, value, NULL, NULL);
                } else if (result != value->value.reg->value) {
                    finish_restored_expression(result);
                    result = value->value.reg->value;
                }
                ++count;
                unlink_input_record((C2Record *)result);
                prune_detached_operands(result);
            } else {
                copy_input_operand(result, value);
                ++count;
            }
            operand = next;
        }
    }
    output = result->tail.right;
    release_function_register(output->value.reg->canonical);
    replace_input_result(result, output, old_output);
    if (HAS_DEFINITION(old_output)) {
        reg = old_output->value.reg;
        if (!reg->canonical->variant_next) reg->value = result;
        else bind_input_temporary(reg, result);
    } else if (DEFINED_AGGREGATE(old_output)) {
        restore_expression_operand(old_output);
        old_output->location.expression.properties |= 0x10;
    }
    finish_restored_expression(result);
    prepend_input_record((C2Record *)expression_following_record, (C2Record *)result);
    return result;
}

C2Expression *__fastcall restore_expression_operand(C2Expression *expression)
{
    C2Expression *result;
    if (IS_VALUE(expression)) {
        expression->location.expression.properties &= ~0x10;
        if (HAS_DEFINITION(expression)) {
            restore_expression_operands(expression->value.reg->value);
        } else if (DEFINED_AGGREGATE(expression)) {
            restore_expression_operands(expression->aggregate_value->value.reg->value);
        }
        return expression;
    }
    result = restore_expression_operands(expression);
    if (result->flags & 1) result = result->tail.right;
    return result;
}

#undef IS_VALUE
#undef HAS_DEFINITION
#undef DEFINED_AGGREGATE

void __fastcall finish_restored_expression(C2Expression *expression)
{
    C2Expression *operand;
    for (operand = expression->tail.right; operand; operand = operand->next) {
        if ((1U << (operand->kind & 31)) & 0x60)
            operand->location.expression.comparison &= ~0x20;
    }
    for (operand = expression->left; operand; operand = operand->next) {
        if ((1U << (operand->kind & 31)) & 0x60)
            operand->location.expression.comparison &= ~0x20;
    }
    retain_aggregate_operands(expression);
}
