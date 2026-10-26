#include "expression-order.h"
#include "function-records.h"
#include <recon.h>

unsigned __fastcall expression_order_key(C2Expression *expression)
{
    C2Expression *operand;
    unsigned key, depth, weight, next_depth;
    if ((1U << (expression->kind & 31)) & 0x7fe) {
        key = expression_value_hash(expression) & 0xffff;
        if (!((1U << (expression->kind & 31)) & 0x180)) key |= 0x10000;
        return key;
    }
    operand = expression->left;
    key = operand->order_key;
    weight = (key >> 16) & 255;
    depth = key >> 24;
    for (operand = operand->next; operand; operand = operand->next) {
        key = operand->order_key;
        weight += (key >> 16) & 255;
        next_depth = key >> 24;
        if (next_depth > depth) depth = next_depth + 1;
        else if (next_depth == depth) ++depth;
    }
    if (expression->kind == 13) depth += 7;
    else if (expression->kind == 17) depth += 5;
    key = expression_value_hash(expression) & 0xffff;
    return key | (((weight & 255) | (depth << 8)) << 16);
}

unsigned __fastcall expression_value_hash(C2Expression *expression)
{
    C2Expression *operand;
    C2FunctionRegister *reg;
    unsigned value = 0, hash = 0;
    switch (expression->kind) {
    case 1:
    case 2:
    case 3:
        reg = expression->value.reg;
        if (((1U << (expression->kind & 31)) & 6) && reg->value)
            return expression_value_hash(reg->value) & 0xffff;
        if (((1U << (expression->kind & 31)) & 6) && reg->kind == 3)
            return reg->number & 0xffff;
        value = reg->canonical->symbol->serial;
        hash = value ^ (value >> 16);
        break;
    case 4:
        value = expression->value.symbol->serial;
        hash = value ^ (value >> 16);
        break;
    case 5:
    case 6:
        value = expression->detail.aggregate_displacement;
        hash = (value & 0xffff) ^ (unsigned short)expression->opcode ^ (value >> 16);
        if (expression->aggregate_value)
            hash ^= expression_value_hash(expression->aggregate_value);
        break;
    case 7:
        value = (unsigned)((__int64)expression->integer.value >> 32) ^ expression->integer.low;
        hash = value ^ (value >> 16);
        break;
    case 8:
        value = expression->value.real->significand_low;
        hash = value ^ (value >> 16);
        break;
    case 9:
    case 10:
        break;
    case 11:
    case 12:
    case 13:
    case 14:
    case 15:
    case 16:
    case 17:
    case 18:
    case 19:
    case 20:
        for (operand = expression->left; operand; operand = operand->next)
            hash ^= operand->order_key & 0xffff;
        hash ^= (unsigned short)expression->opcode;
        break;
    }
    return hash & 0xffff;
}

C2Expression *__fastcall sort_expression_inputs(C2Expression *head, C2ExpressionCompare compare)
{
    C2Expression *left = NULL, *right = NULL, *remaining, *last;
    C2Expression **link;
    int width = 1, count;
    if (!head) return head;
    do {
        if (right == head) return head;
        link = &head;
        remaining = head;
        while (remaining) {
            left = remaining;
            last = remaining;
            for (count = 1; last->next && count < width; ++count) last = last->next;
            right = last->next;
            if (!right) {
                *link = remaining;
                break;
            }
            last->next = NULL;
            last = right;
            for (count = 1; last->next && count < width; ++count) last = last->next;
            remaining = last->next;
            last->next = NULL;
            *link = merge_expression_inputs(left, right, compare);
            if (!remaining) break;
            for (count = width * 2; count > 0; --count) link = &(*link)->next;
        }
        width *= 2;
    } while (left != head);
    return head;
}

C2Expression *__fastcall merge_expression_inputs(C2Expression *left,
    C2Expression *right, C2ExpressionCompare compare)
{
    C2Expression *head, **link = &head;
    while (left && right) {
        if (compare(left, right) <= 0) {
            *link = left;
            link = &left->next;
            left = left->next;
        } else {
            *link = right;
            link = &right->next;
            right = right->next;
        }
    }
    *link = left ? left : right;
    return head;
}

int __fastcall compare_expression_keys(C2Expression *left, C2Expression *right)
{
    if (left->order_key < right->order_key) return 1;
    if (left->order_key != right->order_key) return -1;
    return 0;
}

int __fastcall expression_can_flatten(C2Expression *expression);
#define HAS_REGISTER_DEFINITION(e) (((1U << ((e)->kind & 31)) & 6) && (e)->value.reg->value)
#define AGGREGATE_HAS_KEYS(e) (((1U << ((e)->kind & 31)) & 0x60) && \
    (!(e)->aggregate_value || HAS_REGISTER_DEFINITION((e)->aggregate_value)) && \
    (!(e)->aggregate_second || HAS_REGISTER_DEFINITION((e)->aggregate_second)) && \
    (e)->aggregate_value != (e)->aggregate_second)

C2Expression *__fastcall prepare_final_expression(C2Expression *expression)
{
    C2Expression *operand, *definition;
    unsigned hash;
    if (!expression->left) return expression;
    for (operand = expression->left; operand; operand = operand->next) {
        if (HAS_REGISTER_DEFINITION(operand)) {
            definition = prepare_final_expression(operand->value.reg->value);
            operand->order_key = definition->order_key;
        } else if (AGGREGATE_HAS_KEYS(operand)) {
            definition = prepare_final_expression(operand->aggregate_value->value.reg->value);
            operand->order_key = definition->order_key;
        } else if ((1U << (operand->kind & 31)) & 0x7fe) {
            operand->order_key = expression_order_key(operand);
        } else {
            prepare_final_expression(operand);
        }
    }
    for (operand = expression->tail.right; operand; operand = operand->next) {
        if (AGGREGATE_HAS_KEYS(operand)) {
            definition = prepare_final_expression(operand->aggregate_value->value.reg->value);
            operand->order_key = definition->order_key;
        }
    }
    if (expression_can_flatten(expression))
        expression->left = sort_expression_inputs(expression->left, compare_expression_keys);
    hash = expression_value_hash(expression);
    expression->order_key = hash | expression_order_key(expression);
    return expression;
}

#undef HAS_REGISTER_DEFINITION
#undef AGGREGATE_HAS_KEYS
