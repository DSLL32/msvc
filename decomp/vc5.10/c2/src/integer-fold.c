#include "code-input.h"
#include <recon.h>

C2Expression *__fastcall fold_integer_operation(C2Expression *expression,
    unsigned short type, C2Expression *left, C2Expression *right)
{
    /* Original defect: when a binary operation has no second operand, the
     * temporary has only this16-bit type write; its other48 bits are unwritten.
     * Keep that storage undefined in Win32, and reject that unsupported native
     * domain before any such read. Ordinary unary operations need no payload. */
    union { unsigned __int64 value; unsigned short type; } second;
    second.type = type;
    if (!((1U << ((type >> 12) & 31)) & 0x0e) && expression->opcode != 0x135)
        return NULL;
    if (left->kind != 7) return NULL;
    if (right) {
        if (right->kind != 7) return NULL;
        second.value = right->integer.value;
    }
#if defined(RECON_NATIVE_TEST) && defined(__linux__)
    else {
        switch (expression->opcode) {
        case 0x141: case 0x142: case 0x143: case 0x145:
        case 0x146: case 0x147: case 0x148: case 0x149: case 0x14a: case 0x14b:
            recon_unsupported("binary integer fold with uninitialized absent operand");
        }
    }
#endif
    switch (expression->opcode) {
    case 0x130: case 0x131: case 0x133: case 0x140:
    case 0x151: case 0x15d: case 0x15f:
        return NULL;
    case 0x136:
        left->integer.value = ~left->integer.value;
        break;
    case 0x137:
        left->integer.value = 0 - left->integer.value;
        break;
    case 0x141:
        left->integer.value += second.value;
        break;
    case 0x142:
        left->integer.value -= second.value;
        break;
    case 0x143:
        left->integer.value = second.value * left->integer.value;
        break;
    case 0x145:
#if defined(RECON_NATIVE_TEST) && defined(__linux__)
        if ((unsigned char)second.value >= 64) left->integer.value = 0;
        else
#endif
        left->integer.value <<= (unsigned char)second.value;
        break;
    case 0x146:
        left->integer.value &= second.value;
        break;
    case 0x147:
        left->integer.value |= second.value;
        break;
    case 0x148:
        left->integer.value ^= second.value;
        break;
    case 0x149:
    case 0x14a:
        if (!second.value) return NULL;
#if defined(RECON_NATIVE_TEST) && defined(__linux__)
        if ((type & 0xf000) != 0x2000 &&
            left->integer.low == 0 && left->integer.high == 0x80000000U &&
            second.value == (unsigned __int64)-1)
            recon_unsupported("signed integer fold overflow boundary");
#endif
        if (expression->opcode == 0x149) {
            if ((type & 0xf000) == 0x2000) left->integer.value /= second.value;
            else left->integer.value = (__int64)left->integer.value / (__int64)second.value;
        } else {
            if ((type & 0xf000) == 0x2000) left->integer.value %= second.value;
            else left->integer.value = (__int64)left->integer.value % (__int64)second.value;
        }
        break;
    case 0x14b:
        if ((type & 0xf000) == 0x2000) {
#if defined(RECON_NATIVE_TEST) && defined(__linux__)
            if ((unsigned char)second.value >= 64) left->integer.value = 0;
            else
#endif
            left->integer.value >>= (unsigned char)second.value;
        } else {
#if defined(RECON_NATIVE_TEST) && defined(__linux__)
            if ((unsigned char)second.value >= 64)
                left->integer.value = (left->integer.high & 0x80000000U) ?
                    (unsigned __int64)-1 : 0;
            else
#endif
            left->integer.value = (__int64)left->integer.value >> (unsigned char)second.value;
        }
        break;
    }
    return fold_input_constant(left, type, expression);
}

int __fastcall integer_constant_differs(C2Expression *expression, unsigned __int64 value)
{
    __int64 normalized;

    /* Existing callers use the nonzero result, but the original returns a
     * complete signed ordering, including -1 rather than a Boolean 1. */
    normalized = (__int64)normalize_integer_value(expression->type, value);
    if ((__int64)expression->integer.value == normalized) return 0;
    if ((__int64)expression->integer.value < normalized) return -1;
    return 1;
}

unsigned __fastcall count_integer_leading_zero_bits(C2Expression *operand)
{
    unsigned value = operand->integer.low;
    unsigned high_bit, count = 0;

    switch (operand->type & 0xfff) {
    case 1:
        value &= 0xff;
        if (!value) return 8;
        high_bit = 0x80;
        break;
    case 2:
        value &= 0xffff;
        if (!value) return 16;
        high_bit = 0x8000;
        break;
    case 4:
        if (!value) return 32;
        high_bit = 0x80000000;
        break;
    case 16:
        /* The original explicitly declines this width, even for zero. */
        return 0;
    default:
        return 0;
    }
    while (!(value & high_bit)) {
        value <<= 1;
        ++count;
    }
    return count;
}
