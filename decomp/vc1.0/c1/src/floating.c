/* Frontend floating-point environment initialization. */
#include <float.h>
#include "initialization.h"
#include "floating.h"
#include "initializer-records.h"
#include <recon.h>

extern int _atoldbl(Extended *result, const char *text);
extern Extended extended_round_limit;

/* Original 0x4c380..0x4c38d. */
void initialize_floating_point(void)
{
    _control87(4, 4);
}

/* Original 0x4c390..0x4c4a1. Parse through the CRT's ten-byte form,
   then round to the requested target representation and expand it again.
   This target gives long double the same precision as double. */
int __stdcall parse_floating_literal(char *text, Extended *number)
{
    Extended parsed;
    char suffix, *end;
    float single;
    double wide;
    _atoldbl(&parsed, text);
    number->low = parsed.low;
    number->high = parsed.high;
    number->exponent = parsed.exponent;
    end = text;
    do { ++end; } while (*end);
    suffix = end[-1] & 0xdf;
    if (suffix == 'L') {
        if (extended_to_double(number, &wide)) {
            number->format = 8;
            return 1;
        }
        double_to_extended(&wide, number);
        number->format = 8;
        return 0;
    }
    if (suffix == 'F') {
        number->format = 4;
        if (extended_to_float(number, &single)) return 1;
        float_to_extended(&single, number);
        return 0;
    }
    number->format = 8;
    if (extended_to_double(number, &wide)) return 1;
    double_to_extended(&wide, number);
    return 0;
}

/* Original 0x4c4b0..0x4c4c5. The representation word is sign-extended. */
int extended_format(const Extended *value)
{
    return (short)value->format;
}

/* Original 0x4c4c5..0x4c4e9. Six forward word transfers include the
   representation tag, retaining the original word-copy ordering. */
int copy_extended(const Extended *source, Extended *destination)
{
    const unsigned short *from = (const unsigned short *)source;
    unsigned short *to = (unsigned short *)destination;
    unsigned index;
    for (index = 0; index < 6; ++index) to[index] = from[index];
    return 0;
}

/* Largest float-rounding threshold in the native ten-byte representation. */
Extended float_round_limit = { 0xffffffffU, 0xffffff7fU, 0x407e, 0 };

/* Original 0x4c625..0x4c64c. MSVC 1.0 C has no ten-byte scalar type,
   so narrow x87 transfers preserve that value and the private helper ABI.
   Range decisions and output control remain ordinary C. */
int extended_to_float(const Extended *value, float *result)
{
    int overflow;
    __asm {
        mov eax, value
        fld tbyte ptr [eax]
        fld tbyte ptr float_round_limit
    }
    overflow = check_extended_range();
    if (!overflow) {
        __asm {
            mov eax, result
            fstp dword ptr [eax]
            fwait
        }
    }
    return overflow;
}

FloatingWorkspace floating_workspace; /* original 0x710aa..0x710b2 */

/* Original 0x4c8f5..0x4c93b. The private ABI supplies a ten-byte limit
   in ST(0), value in ST(1), and leaves the signed value in ST(0).
   C cannot express those inputs or preserve x87 condition codes; the narrow
   blocks only perform the comparisons, while C handles sign and range. */
int check_extended_range(void)
{
    unsigned short status;
    floating_workspace.check.negative = 0;
    __asm {
        fxch st(1)
        ftst
        fstsw status
    }
    floating_workspace.check.status = status;
    if (status & 0x100) {
        ++floating_workspace.check.negative;
        __asm { fchs }
    }
    __asm {
        fxch st(1)
        fcomp st(1)
        fstsw status
    }
    floating_workspace.check.status = status;
    if (floating_workspace.check.negative) {
        __asm { fchs }
    }
    return (status & 0x100) != 0;
}

/* Original 0x4c673..0x4c696. Period C has no ten-byte scalar type;
   these narrow x87 transfers retain the private range-check ABI. Unlike
   the float/double interfaces, this store occurs even after a range error. */
int extended_to_ten_bytes(const Extended *source, void *destination)
{
    int overflow;
    __asm {
        mov eax, source
        fld tbyte ptr [eax]
        fld tbyte ptr extended_round_limit
    }
    overflow = check_extended_range();
    __asm {
        mov eax, destination
        fstp tbyte ptr [eax]
        fwait
    }
    return overflow;
}

/* Original 0x4c696..0x4c6b8. The store entry takes ESI and ST(0),
   so these narrow blocks bridge that original private x87 ABI. */
int float_to_extended(const float *value, Extended *number)
{
    __asm {
        mov eax, value
        fld dword ptr [eax]
    }
    number->format = 4;
    __asm {
        mov esi, number
        call store_extended
    }
    return 0;
}

/* Original 0x4c8f1..0x4c8f5 is a shared four-byte x87 store tail.
   Its ESI/ST(0) ABI and ten-byte destination cannot be declared in C.
   No C frame is needed or permitted at this private entry. */
__declspec(naked) void store_extended(void)
{
    /* Native trace entry preserves the live private ESI/x87 ABI. Empty Win32. */
    RECON_TRACE_NAKED_ENTRY(store_extended)
    __asm {
        fstp tbyte ptr [esi]
        fwait
        ret
    }
}

/* Double-rounding threshold, original ten bytes at 0x7107e. */
Extended double_round_limit = { 0xfffffbffU, 0xffffffffU, 0x43fe, 0 };

/* Original 0x4c64c..0x4c673. Narrow transfers preserve the ten-byte
   input and private x87 range-check ABI, which period C cannot express. */
int extended_to_double(const Extended *value, double *result)
{
    int overflow;
    __asm {
        mov eax, value
        fld tbyte ptr [eax]
        fld tbyte ptr double_round_limit
    }
    overflow = check_extended_range();
    if (!overflow) {
        __asm {
            mov eax, result
            fstp qword ptr [eax]
            fwait
        }
    }
    return overflow;
}

/* Original 0x4c6b8..0x4c6da. ESI/ST(0) bridges the original ten-byte
   store entry; ordinary C records the target representation. */
int double_to_extended(const double *value, Extended *number)
{
    __asm {
        mov eax, value
        fld qword ptr [eax]
    }
    number->format = 8;
    __asm {
        mov esi, number
        call store_extended
    }
    return 0;
}

/* Original 0x4c538..0x4c587. Period MSVC C has no 80-bit scalar type.
   The narrow x87 blocks perform that arithmetic and bridge the ESI/ST(0)
   tail ABI; target-format selection and status handling remain ordinary C. */
int subtract_extended(const Extended *left, const Extended *right, Extended *result)
{
    unsigned short format;
    int status;
    __asm {
        mov eax, left
        fld tbyte ptr [eax]
        mov eax, right
        fld tbyte ptr [eax]
        fsubp st(1), st(0)
    }
    format = left->format | right->format;
    if (format & 2) result->format = 10;
    else if (format & 8) result->format = 8;
    else result->format = 4;
    __asm {
        mov esi, result
        call finish_extended_arithmetic
        mov status, eax
    }
    return status;
}

/* Greatest finite x87 extended value, original ten bytes at 0x71088. */
Extended extended_round_limit = { 0xffffffffU, 0xffffffffU, 0x7ffe, 0 };

/* Original 0x4c884..0x4c8f1, falling into shared store_extended.
   This private entry receives ESI and an 80-bit value in ST(0). C cannot
   express either input. Narrow transfers capture the pointer, load the
   ten-byte limit, and optionally round via float/double storage; the range
   and precision decisions remain C and retain the separate store entry. */
int finish_extended_arithmetic(void)
{
    Extended *number;
    unsigned short format;
    int status;
    __asm { mov number, esi }
    format = number->format;
    if (format == 4) {
        __asm { fld tbyte ptr float_round_limit }
    } else if (format == 10) {
        __asm { fld tbyte ptr extended_round_limit }
    } else {
        __asm { fld tbyte ptr double_round_limit }
    }
    status = check_extended_range();
    if (format != 10 && normalized_Op) {
        if (format == 4) {
            __asm {
                fstp dword ptr floating_workspace
                fld dword ptr floating_workspace
            }
        } else {
            __asm {
                fstp qword ptr floating_workspace
                fld qword ptr floating_workspace
            }
        }
    }
    __asm {
        mov esi, number
        call store_extended
    }
    return status;
}

/* Original 0x4c4e9..0x4c537. Period C cannot represent the original
   80-bit operands or ESI/ST(0) arithmetic tail. Only that arithmetic and
   transfer use assembly; result precision follows the operand tags. */
int add_extended(const Extended *left, const Extended *right, Extended *result)
{
    unsigned short format;
    int status;
    __asm {
        mov eax, left
        fld tbyte ptr [eax]
        mov eax, right
        fld tbyte ptr [eax]
        faddp st(1), st(0)
    }
    format = left->format | right->format;
    if (format & 2) result->format = 10;
    else if (format & 8) result->format = 8;
    else result->format = 4;
    __asm {
        mov esi, result
        call finish_extended_arithmetic
        mov status, eax
    }
    return status;
}

/* Original 0x4c587..0x4c5d5. Period C cannot represent the original
   80-bit operands or ESI/ST(0) arithmetic tail. Only that arithmetic and
   transfer use assembly; result precision follows the operand tags. */
int multiply_extended(const Extended *left, const Extended *right, Extended *result)
{
    unsigned short format;
    int status;
    __asm {
        mov eax, left
        fld tbyte ptr [eax]
        mov eax, right
        fld tbyte ptr [eax]
        fmulp st(1), st(0)
    }
    format = left->format | right->format;
    if (format & 2) result->format = 10;
    else if (format & 8) result->format = 8;
    else result->format = 4;
    __asm {
        mov esi, result
        call finish_extended_arithmetic
        mov status, eax
    }
    return status;
}

/* Original 0x4c80f..0x4c85a. The status word shares the floating
   workspace. C cannot compare these ten-byte values; the x87 comparison
   supplies its condition bits, including the original unordered handling. */
int compare_extended(unsigned short operation, const Extended *left, const Extended *right)
{
    unsigned short relation;
    __asm {
        mov eax, right
        fld tbyte ptr [eax]
        mov eax, left
        fld tbyte ptr [eax]
        fcompp
        fstsw word ptr floating_workspace + 4
        fwait
    }
    if (floating_workspace.check.status & 0x4000) relation = 1;
    else if (floating_workspace.check.status & 0x100) relation = 4;
    else relation = 2;
    return (relation & operation) != 0;
}

/* Original 0x4c5d6..0x4c624. Period C cannot represent the original
   80-bit operands or ESI/ST(0) arithmetic tail. Only that arithmetic and
   transfer use assembly; result precision follows the operand tags. */
int divide_extended(const Extended *left, const Extended *right, Extended *result)
{
    unsigned short format;
    int status;
    __asm {
        mov eax, left
        fld tbyte ptr [eax]
        mov eax, right
        fld tbyte ptr [eax]
        fdivp st(1), st(0)
    }
    format = left->format | right->format;
    if (format & 2) result->format = 10;
    else if (format & 8) result->format = 8;
    else result->format = 4;
    __asm {
        mov esi, result
        call finish_extended_arithmetic
        mov status, eax
    }
    return status;
}


/* Greatest signed32-bit integer as an exact x87 value, original0x71092. */
Extended signed_integer_limit = { 0, 0xfffffffeU, 0x401d, 0 };

/* Original0x4c753..0x4c77e. Truncation and range checks retain their
   private x87 entries; a failed check leaves the result pointer untouched. */
int extended_to_integer(const Extended *value, int *result)
{
    int overflow;
    __asm {
        mov eax, value
        fld tbyte ptr [eax]
    }
    truncate_extended();
    __asm { fld tbyte ptr signed_integer_limit }
    overflow = check_extended_range();
    if (!overflow) {
        __asm {
            mov eax, result
            fistp dword ptr [eax]
            fwait
        }
    }
    return overflow;
}

unsigned short truncation_control; /* original0x710b2 */

/* Original0x4c85b..0x4c883. Its only argument/result is ST(0). The
   saved control word aliases the high word of the shared rounding workspace.
   Narrow x87 instructions are necessary to retain that private contract. */
void truncate_extended(void)
{
    __asm { fstcw word ptr floating_workspace + 6 }
    truncation_control = floating_workspace.check.control | 0xc01;
    __asm {
        fldcw truncation_control
        frndint
        fldcw word ptr floating_workspace + 6
    }
}

/* Original 0x4c6da..0x4c6fc. The ten-byte load and ESI/ST(0) store
   entry cannot be expressed by the original C compiler's double type. */
int ten_bytes_to_extended(const void *source, Extended *destination)
{
    __asm {
        mov eax, source
        fld tbyte ptr [eax]
    }
    destination->format = 10;
    __asm {
        mov esi, destination
        call store_extended
    }
    return 0;
}

/* Exact unsigned32-bit limit at original 0x7109c. Only its ten numeric
   bytes are consumed by the private range interface. */
Extended unsigned_integer_limit = { 0, 0xffffffffU, 0x401e, 0 };

/* Original 0x4c77f..0x4c7c4. The private helpers retain an 80-bit value
   that period C cannot name. Negative values return a sign status but still
   store the low32 bits; a genuine range failure alone skips the store. */
int extended_to_unsigned(const Extended *source, unsigned *destination)
{
    int overflow;
    unsigned words[2];
    __asm {
        mov eax, source
        fld tbyte ptr [eax]
    }
    truncate_extended();
    __asm { fld tbyte ptr unsigned_integer_limit }
    overflow = check_extended_range();
    if (!overflow) {
        overflow |= floating_workspace.check.negative;
        __asm {
            lea eax, words
            fistp qword ptr [eax]
            fwait
        }
        *destination = words[0];
    }
    return overflow;
}

/* Original 0x710a6. Addressable for the original x87 half-step transfer. */
float integer_round_half = 0.5F;

/* Original 0x4c7c4..0x4c80f. Preserve the 80-bit half-step and private
   x87 helper ABIs; C selects its sign and controls the range/output path. */
int round_extended_to_integer(const Extended *source, int *destination)
{
    int overflow;
    __asm {
        mov eax, source
        fld tbyte ptr [eax]
        ftst
        fstsw word ptr floating_workspace + 4
        fld dword ptr integer_round_half
    }
    if (floating_workspace.check.status & 0x100) {
        __asm { fchs }
    }
    __asm { faddp st(1), st(0) }
    truncate_extended();
    __asm { fld tbyte ptr signed_integer_limit }
    overflow = check_extended_range();
    if (!overflow) {
        __asm {
            mov eax, destination
            fistp dword ptr [eax]
            fwait
        }
    }
    return overflow;
}

/* Original0x4c6fc..0x4c71e. Signed integer expansion uses the existing
   ESI/ST(0) store entry. Period C cannot express that private x87 call ABI;
   these narrow transfers preserve it without adding a wrapper callee. */
int integer_to_extended(const int *value, Extended *number)
{
    __asm {
        mov eax, value
        fild dword ptr [eax]
    }
    number->format = 4;
    __asm {
        mov esi, number
        call store_extended
    }
    return 0;
}

/* Original0x4c71e..0x4c753. Zero-widen the unsigned input into two
   i386 words before loading it as a signed64-bit x87 integer. Only the
   load and original ESI/ST(0) store ABI require assembly. */
int unsigned_to_extended(const unsigned *value, Extended *number)
{
    unsigned words[2];
    words[0] = *value;
    words[1] = 0;
    __asm {
        lea eax, words
        fild qword ptr [eax]
    }
    number->format = 4;
    __asm {
        mov esi, number
        call store_extended
    }
    return 0;
}
