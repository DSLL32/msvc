/* Expression constants. */
#include "initialization.h"
#include "lexer.h"
#include "types.h"
#include "binary-expressions.h"
#include "numeric-conversion.h"
#include "class-members.h"
#include <recon.h>

/* Original 0x3b8c0..0x3b924, stdcall 4. Enum constants share the
   integral payload representation. Error recovery distinguishes a missing
   expression, a nonconstant expression and a nonintegral constant. */
unsigned __stdcall evaluate_integer_constant(IrToken *expression)
{
    if (!expression) {
        report_syntax_error(0x38);
        return 0;
    }
    expression = fold_symbol_constant(expression);
    if ((expression->flags & 7) != 3) {
        report_syntax_error(0x39);
        return 1;
    }
    if (expression->type->flags & 0x240) return expression->value.integer;
    report_syntax_error(0x3a);
    return 0;
}

/* Original 0x2c600..0x2c71f; seven-way dispatch at 0x2c720..0x2c73c.
   Floating payloads are pointers to their separately saved values. */
IrToken *__stdcall new_literal_constant(unsigned char kind, void *value)
{
    ArenaPool *pool;
    IrToken *constant;
    TypeDescription *description;
    unsigned short flags;
    if (token_allocation_flags & 0x20000020)
        constant = allocate_permanent(sizeof(IrToken));
    else {
        pool = &compiler_arena.pools[2];
        if (pool->remaining < (int)sizeof(IrToken))
            refill_arena(&compiler_arena, pool);
        constant = (IrToken *)pool->next;
        pool->remaining -= sizeof(IrToken);
        pool->next += sizeof(IrToken);
    }
    constant->flags = 3;
    constant->kind = kind;
    switch (kind) {
    case 4: flags = 0x0104; goto floating_value;
    case 5: flags = 0x0108; goto floating_value;
    case 6: flags = 0x0128;
floating_value:
        constant->value.integer = *(unsigned *)value;
        break;
    case 7: flags = 0x0064; goto integer_value;
    case 8: flags = 0x0044; goto integer_value;
    case 9: flags = 0x4064; goto integer_value;
    case 10: flags = 0x4044;
integer_value:
        constant->value.integer = *(unsigned *)value;
        if (!constant->value.integer) constant->flags |= 0x80;
        break;
    /* The native default reads an unwritten local flags word. Its callers
       supply the seven literal token kinds above; no invented default type. */
    }
    description = &type_description_stack[++type_description_depth];
    description->flags = flags;
    description->attributes = 0x1000;
    description->related = 0;
    description->field08 = description->field0a = 0;
    constant->type = intern_type(description);
    --type_description_depth;
    return constant;
}

/* Original 0x2c7b0..0x2c7c7. */
IrToken *__stdcall make_integer_constant(unsigned value)
{
    IrToken *constant = new_integer_constant(value);
    if (!value)
        constant->flags |= 0x80;
    return constant;
}

/* Original 0x2c740..0x2c7a1. Persistent tokens use the permanent arena. */
IrToken *__stdcall new_integer_constant(unsigned value)
{
    ArenaPool *pool;
    IrToken *constant;
    if (token_allocation_flags & 0x20000020)
        constant = allocate_permanent(sizeof(IrToken));
    else {
        pool = &compiler_arena.pools[2];
        if (pool->remaining < (int)sizeof(IrToken))
            refill_arena(&compiler_arena, pool);
        constant = (IrToken *)pool->next;
        pool->remaining -= sizeof(IrToken);
        pool->next += sizeof(IrToken);
    }
    constant->flags = 3;
    constant->kind = 7;
    constant->type = type_0064;
    constant->value.integer = value;
    return constant;
}

/* Original 0x2c7f0..0x2c826: a storage reference to an integer-valued
   constant symbol becomes an ordinary literal with that symbol's type. */
IrToken *__stdcall fold_symbol_constant(IrToken *expression)
{
    IrToken *reference, *constant;
    CppSymbol *symbol;
    if (token_categories[expression->kind] == 0x46) {
        reference = expression->value.pointer;
        if (reference->kind == 2) {
            symbol = reference->value.pointer;
            if (symbol->attributes & 0x4000) {
                constant = make_integer_constant(symbol->auxiliary.value);
                constant->type = symbol->type;
                return constant;
            }
        }
    }
    return expression;
}

/* Original 0x2c7d0..0x2c7e5. Sizes retain the unsigned-long builtin type. */
IrToken *__stdcall make_size_constant(unsigned value)
{
    IrToken *constant = make_integer_constant(value);
    constant->type = type_4064_first;
    return constant;
}

/* Original code 0x2c830..0x2cfd5, operation table 0x2cfd8..0x2d083.
   Fold integral operations with the original target widths and signedness;
   mixed floating operands use the separate extended-precision evaluator. */
IrToken *__stdcall fold_binary_constant(IrToken *expression, IrToken *left, IrToken *right)
{
    unsigned short left_flags = left->type->flags, right_flags = right->type->flags;
    unsigned short width, unsigned_arithmetic, folded_flags;
    unsigned char operation;
    unsigned value, lhs, rhs;
    int long_result = 0, left_signed, right_signed;
    CType *result_type = 0;
    Extended left_value, right_value;

    if (!expression) return 0;
    operation = expression->kind;
    left = fold_symbol_constant(left);
    if ((left->flags & 7) != 3) return expression;
    right = fold_symbol_constant(right);
    if ((right->flags & 7) != 3) {
        if (operation != 0x3b && operation != 0x58) return expression;
        if (left_flags & 0x100) {
            left_value = *(Extended *)left->value.pointer;
            value = compare_extended(6, &left_value, &extended_zero);
            unsigned_arithmetic = 0;
        } else {
            value = left->value.integer != 0;
            unsigned_arithmetic = (left_flags & 0x4000) ||
                (options.configuration.option_J &&
                 ((left_flags & 0x2fdf) != 0x41 || !(left_flags & 0x8000)));
        }
        if ((operation == 0x3b) == value) return expression;
        integer_constant_scratch = value;
        return new_literal_constant(unsigned_arithmetic ? 9 : 7, &integer_constant_scratch);
    }
    if ((left_flags | right_flags) & 0x100) {
        if (left_flags & 0x100) left_value = *(Extended *)left->value.pointer;
        else if ((left_flags & 0x4000) ?
                 unsigned_to_extended((unsigned long *)&left->value.integer, &left_value) :
                 integer_to_extended((long *)&left->value.integer, &left_value)) compiler_warning(2, 0x38);
        if (right_flags & 0x100) right_value = *(Extended *)right->value.pointer;
        else if ((right_flags & 0x4000) ?
                 unsigned_to_extended((unsigned long *)&right->value.integer, &right_value) :
                 integer_to_extended((long *)&right->value.integer, &right_value)) compiler_warning(2, 0x38);
        return fold_floating_binary(expression, &left_value, &right_value);
    }
    if (!left->type->related && !right->type->related) {
        if ((left_flags & 15) == (right_flags & 15)) {
            width = left_flags & 15;
            unsigned_arithmetic = width != 1 && ((left_flags | right_flags) & 0x4000);
            if (width < 5) width = 4;
        } else if ((left_flags & 15) > (right_flags & 15)) {
            width = left_flags & 15;
            unsigned_arithmetic = left_flags & 0x4000;
        } else {
            width = right_flags & 15;
            unsigned_arithmetic = right_flags & 0x4000;
        }
    } else {
        unsigned_arithmetic = 1;
        width = expression->type->flags & 15;
    }
    if (width == 4)
        long_result = !((!(left_flags & 15) || (left_flags & 0x60) == 0x60) &&
                        (!(right_flags & 15) || (right_flags & 0x60) == 0x60));
    lhs = left->value.integer;
    rhs = right->value.integer;
    if (operation != 0x3b && operation != 0x4c && operation != 0x58 && operation != 0x61 &&
        unsigned_arithmetic && operation != 0x3a && operation != 0x56 && operation != 0x64) {
        folded_flags = left->type->flags;
        left_signed = !(folded_flags & 0x4000) && (!options.configuration.option_J ||
            ((folded_flags & 0x2fdf) == 0x41 && (folded_flags & 0x8000)));
        folded_flags = right->type->flags;
        right_signed = !(folded_flags & 0x4000) && (!options.configuration.option_J ||
            ((folded_flags & 0x2fdf) == 0x41 && (folded_flags & 0x8000)));
        if ((left_signed && (int)lhs < 0) || (right_signed && (int)rhs < 0))
            compiler_warning(2, 0x134);
    }
    switch (operation) {
    case 0x3a: value = lhs & rhs; break;
    case 0x3b: value = lhs != 0 && rhs != 0; goto boolean_result;
    case 0x42:
        if (!rhs) { right->flags = 0x40; return right; }
        value = unsigned_arithmetic ? lhs / rhs : (unsigned)((int)lhs / (int)rhs);
        break;
    case 0x44: value = lhs == rhs; goto boolean_result;
    case 0x47: value = unsigned_arithmetic ? lhs > rhs : (int)lhs > (int)rhs; goto boolean_result;
    case 0x48: value = unsigned_arithmetic ? lhs >= rhs : (int)lhs >= (int)rhs; goto boolean_result;
    case 0x4c: value = lhs << (rhs & 31); result_type = expression->type; break;
    case 0x4d: value = unsigned_arithmetic ? lhs < rhs : (int)lhs < (int)rhs; goto boolean_result;
    case 0x4e: value = unsigned_arithmetic ? lhs <= rhs : (int)lhs <= (int)rhs; goto boolean_result;
    case 0x4f:
        value = lhs - rhs;
        if (subtraction_overflows((int)lhs, (int)rhs, unsigned_arithmetic, width)) compiler_warning(2, 0x133);
        break;
    case 0x51:
        if (!rhs) { right->flags = 0x40; return right; }
        value = unsigned_arithmetic ? lhs % rhs : (unsigned)((int)lhs % (int)rhs);
        break;
    case 0x53:
        value = lhs * rhs;
        if (multiplication_overflows(lhs, rhs, unsigned_arithmetic, width)) compiler_warning(2, 0x133);
        break;
    case 0x55: value = lhs != rhs; goto boolean_result;
    case 0x56: value = lhs | rhs; break;
    case 0x58: value = lhs != 0 || rhs != 0; goto boolean_result;
    case 0x5a: case 0x5c:
        value = lhs + rhs;
        if (addition_overflows((int)lhs, rhs, unsigned_arithmetic, width)) compiler_warning(2, 0x133);
        break;
    case 0x61:
        value = unsigned_arithmetic ? lhs >> (rhs & 31) : (unsigned)((int)lhs >> (rhs & 31));
        result_type = expression->type;
        break;
    case 0x64: value = lhs ^ rhs; break;
    default: return expression;
    }
    goto make_result;
boolean_result:
    result_type = type_0064;
    unsigned_arithmetic = 0;
make_result:
    if ((operation == 0x5a || operation == 0x4f) && expression->type->related &&
        (expression->type->related->flags & 1)) {
        left->value.integer = value;
        return left;
    }
    if (result_type) {
        folded_flags = result_type->flags;
        long_result = (folded_flags & 15) == 4 && (folded_flags & 0x60) != 0x60;
    }
    integer_constant_scratch = value;
    return new_literal_constant((unsigned_arithmetic ? 9 : 7) + long_result, &integer_constant_scratch);
}

/* Original 0x2d3f0..0x2d420. Ordinary parser constants use arena two;
   persistent/scanner allocation modes keep the value permanently. */
Extended *__stdcall save_extended_constant(Extended *value)
{
    return save_extended_in_arena(*value,
        token_allocation_flags & 0x20000020UL ? 0 : 2);
}

/* Original 0x2d430..0x2d486. The value occupies twelve stack bytes;
   arena -1 selects pool zero while arena zero requests permanent storage. */
Extended *__stdcall save_extended_in_arena(Extended value, int arena)
{
    Extended *result;
    ArenaPool *pool;
    if (!arena) result = allocate_permanent(sizeof(Extended));
    else {
        if (arena == -1) arena = 0;
        pool = &compiler_arena.pools[arena];
        if (pool->remaining < (int)sizeof(Extended))
            refill_arena(&compiler_arena, pool);
        result = (Extended *)pool->next;
        pool->remaining -= sizeof(Extended);
        pool->next += sizeof(Extended);
    }
    *result = value;
    return result;
}
