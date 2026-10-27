#include <string.h>
#include "compiler-state.h"
#include "scanner.h"
#include <recon-tool.h>
#include <recon.h>
extern int __cdecl add_extended_constants(const InitializerExtendedReal *, const InitializerExtendedReal *, InitializerExtendedReal *);
extern int __cdecl multiply_extended_constants(const InitializerExtendedReal *, const InitializerExtendedReal *, InitializerExtendedReal *);
static int arithmetic_extent(void)
{
    struct { DWORD before; InitializerExtendedReal value; DWORD after; } owned;
    const WORD tags[] = {4, 8, 10};
    unsigned operation, i, mode;
    for (operation = 0; operation != 2; ++operation)
    for (i = 0; i != 3; ++i) for (mode = 0; mode != 2; ++mode) {
        InitializerExtendedReal left = {0, 0xa0000000U, 0x3fff, tags[i]};
        InitializerExtendedReal right = {0, 0x80000000U, 0x3ffe, 4};
        InitializerExtendedReal saved_left = left, saved_right = right;
        WORD before, after;
        int result;
        c1_data.options.precise_float = mode;
        memset(&owned, 0xa5, sizeof(owned));
        __asm { fstsw before }
        result = operation ? multiply_extended_constants(&left, &right, &owned.value) :
            add_extended_constants(&left, &right, &owned.value);
        __asm { fstsw after }
        if (result != (i == 2 ? 0 : (int)mode) || owned.before != 0xa5a5a5a5U ||
            owned.after != 0xa5a5a5a5U || owned.value.significand_low ||
            owned.value.significand_high != (operation ? 0xa0000000U : 0xe0000000U) ||
            owned.value.exponent != (operation ? 0x3ffe : 0x3fff) ||
            owned.value.precision != tags[i] || memcmp(&left, &saved_left, 12) ||
            memcmp(&right, &saved_right, 12) || ((before ^ after) & 0x3800)) return 0;
    }
    return 1;
}
extern int __cdecl initializer_double_to_real(const double *, InitializerExtendedReal *);
extern int __cdecl initializer_real_to_integer(const InitializerExtendedReal *, unsigned __int64 *);
static int integer_conversion(void)
{
    const double inputs[] = { 9.75, -9.75, 0.0, 1024.125, 18446744073709551616.0 };
    const unsigned __int64 expected[] = { 9, (unsigned __int64)-9, 0, 1024, 0 };
    struct { DWORD before; unsigned __int64 value; DWORD after; } owned;
    unsigned i;
    for (i = 0; i != 5; ++i) {
        InitializerExtendedReal real, saved;
        WORD before, after;
        int result;
        initializer_double_to_real(&inputs[i], &real);
        saved = real;
        memset(&owned, 0xa5, sizeof(owned));
        __asm { fstsw before }
        result = initializer_real_to_integer(&real, &owned.value);
        __asm { fstsw after }
        if (result != (i == 4) || owned.value != expected[i] ||
            owned.before != 0xa5a5a5a5U || owned.after != 0xa5a5a5a5U ||
            memcmp(&real, &saved, 12) || ((before ^ after) & 0x3800)) return 0;
    }
    return 1;
}
extern unsigned __int64 __cdecl parse_integer_digits(const char *, int, int);
static int wide_spelling(void)
{
    unsigned i;
    c1_data.internal_zb_value = 8;
    for (i = '0'; i <= '9'; ++i) c1_data.scanner.classes[i] = SC_DIGIT;
    return parse_integer_digits("123456789abcdef0", 16, 16) == 0x123456789abcdef0ULL &&
        parse_integer_digits("18446744073709551615", 20, 10) == ~0ULL &&
        parse_integer_digits("1777777777777777777777", 22, 8) == ~0ULL;
}
const ReconTest constant_expression_tests[] = {
    { "extended add and multiply preserve precision, inputs, neighbors and x87 depth", 1, arithmetic_extent },
    { "real-to-integer conversion truncates toward zero and checks decimal overflow", 1, integer_conversion },
    { "64-bit hexadecimal, decimal and octal spellings retain all payload bits", 1, wide_spelling }
};
