#include "optimizer.h"
#include "function-records.h"
#include <recon.h>

int __fastcall compare_input_operands(C2Expression *left, C2Expression *right);
int __fastcall expression_type_is_significant(C2Expression *expression);

/* These views expand in the caller: they do not introduce helper vertices. */
#define IS_VALUE(e) ((1U << ((e)->kind & 31)) & 0x7fe)
#define HAS_DEFINITION(e) (((1U << ((e)->kind & 31)) & 6) && (e)->value.reg->value)
#define DEFINED_AGGREGATE(e) ((e)->kind == 6 && \
    (!(e)->aggregate_value || HAS_DEFINITION((e)->aggregate_value)) && \
    (!(e)->aggregate_second || HAS_DEFINITION((e)->aggregate_second)))

int __fastcall compare_expression_trees(C2Expression *left, C2Expression *right)
{
    C2Expression *a, *b, *definition;
    unsigned short type;
    int comparison;
retry:
    if (left == right) return 0;
    if (!left) return -1;
    if (!right) return 1;
    if (left->opcode != right->opcode) {
        if (left->opcode == 0x135 && register_types_match(left->type, left->left->type)) {
            left = left->left;
            goto retry;
        }
        if (right->opcode == 0x135 && register_types_match(right->type, right->left->type)) {
            right = right->left;
            goto retry;
        }
        return left->opcode < right->opcode ? -1 : 1;
    }
    if (IS_VALUE(left)) {
        if (!IS_VALUE(right)) return -1;
        comparison = compare_input_operands(left, right);
        if (!comparison) return 0;
        if (HAS_DEFINITION(left)) {
            if (!HAS_DEFINITION(right)) return comparison;
            left = left->value.reg->value;
            right = right->value.reg->value;
            goto retry;
        }
        if (!DEFINED_AGGREGATE(left) || !DEFINED_AGGREGATE(right)) return comparison;
        /* The original assumes the first component exists once this aggregate
         * case is selected; a missing component is not a fabricated default. */
        left = left->aggregate_value->value.reg->value;
        right = right->aggregate_value->value.reg->value;
        goto retry;
    }
    if (IS_VALUE(right)) return 1;
    if (left->type != right->type &&
        (expression_type_is_significant(left) || !register_types_match(left->type, right->type)))
        return left->type < right->type ? -1 : 1;
    if (left->opcode == 0x135) {
        type = left->left->type;
        if ((type & 0xfff) < (left->type & 0xfff) && type != right->left->type)
            return type < right->left->type ? -1 : 1;
    }
    for (a = left->tail.right, b = right->tail.right; a && b; a = a->next, b = b->next) {
        if (HAS_DEFINITION(a) && HAS_DEFINITION(b)) {
            if (a->kind != b->kind) return a->kind < b->kind ? -1 : 1;
            if (!register_types_match(a->type, b->type))
                return a->type < b->type ? -1 : 1;
        } else {
            comparison = compare_input_operands(a, b);
            if (comparison) return comparison;
        }
    }
    if (a) return 1;
    if (b) return -1;
    for (a = left->left, b = right->left; a && b; a = a->next, b = b->next) {
        if (!IS_VALUE(a) || !IS_VALUE(b)) {
            comparison = compare_expression_trees(a, b);
        } else {
            if (a->kind != b->kind) return a->kind < b->kind ? -1 : 1;
            if (HAS_DEFINITION(a) && HAS_DEFINITION(b)) {
                comparison = compare_expression_trees(a->value.reg->value, b->value.reg->value);
            } else if (DEFINED_AGGREGATE(a) && DEFINED_AGGREGATE(b)) {
                definition = a->aggregate_value->value.reg->value;
                comparison = compare_expression_trees(definition, b->aggregate_value->value.reg->value);
            } else {
                comparison = compare_input_operands(a, b);
            }
        }
        if (comparison) return comparison;
    }
    if (a) return 1;
    if (b) return -1;
    switch (left->kind) {
    case 15:
        if ((unsigned char)(unsigned)left->detail.data != (unsigned char)(unsigned)right->detail.data)
            return (unsigned char)(unsigned)left->detail.data < (unsigned char)(unsigned)right->detail.data ? -1 : 1;
        break;
    case 16:
        if (left->convention != right->convention)
            return left->convention < right->convention ? -1 : 1;
        break;
    case 17:
        if (left->detail.intrinsic != right->detail.intrinsic)
            return left->detail.intrinsic < right->detail.intrinsic ? -1 : 1;
        break;
    }
    return 0;
}

#undef IS_VALUE
#undef HAS_DEFINITION
#undef DEFINED_AGGREGATE

extern int global_optimization_active; /* Same48e2b0 owner as the global pass. */
int c2_real_compare(int condition, C2Real *left, C2Real *right);

int __fastcall compare_input_operands(C2Expression *left, C2Expression *right)
{
    int comparison;
    unsigned a, b;
    if (left == right) return 0;
    if (!left) return -1;
    if (!right) return 1;
    if (left->kind != right->kind) return left->kind < right->kind ? -1 : 1;
    if (!register_types_match(left->type, right->type))
        return left->type < right->type ? -1 : 1;
    switch (left->kind) {
    case 1:
        if (left->original_register != right->original_register)
            return left->original_register->number < right->original_register->number ? -1 : 1;
    case 2: case 3:
        return compare_function_registers(left->value.reg, right->value.reg);
    case 4:
        if (left->value.symbol != right->value.symbol)
            return left->value.symbol->serial < right->value.symbol->serial ? -1 : 1;
        break;
    case 5: case 6:
        if (left->detail.aggregate_displacement != right->detail.aggregate_displacement)
            return left->detail.aggregate_displacement < right->detail.aggregate_displacement ? -1 : 1;
        a = left->location.expression.comparison & 7;
        b = right->location.expression.comparison & 7;
        if (a != b) return a < b ? -1 : 1;
        a = left->location.expression.properties & 15;
        b = right->location.expression.properties & 15;
        if (a != b) return a < b ? -1 : 1;
        comparison = compare_function_registers(left->tail.aggregate_register,
            right->tail.aggregate_register);
        if (comparison) return comparison;
        comparison = compare_input_operands(left->aggregate_value, right->aggregate_value);
        if (comparison) return comparison;
        comparison = compare_input_operands(left->aggregate_second, right->aggregate_second);
        if (comparison) return comparison;
        if (global_optimization_active && left->immediate != right->immediate)
            return left->immediate < right->immediate ? -1 : 1;
        break;
    case 7:
        if (left->integer.value != right->integer.value)
            return (__int64)left->integer.value < (__int64)right->integer.value ? -1 : 1;
        break;
    case 8:
        if (!c2_real_compare(1, left->value.real, right->value.real))
            return c2_real_compare(4, left->value.real, right->value.real) ? -1 : 1;
        break;
    case 10:
        if (left->value.control != right->value.control)
            return left->value.control < right->value.control ? -1 : 1;
        break;
    }
    return 0;
}
