/* Frontend floating-point environment initialization. */
#include <float.h>
#include "initialization.h"
#include "floating.h"
#include "numeric-conversion.h"
#include <trace-abi.h>
#include <recon.h>

/* Original 0x74b30..0x74b3d. */
void initialize_floating_point(void)
{
    _control87(4, 4);
}

Extended extended_zero = { 0, 0, 0, 4 }; /* original 0x9fd50 */

Extended scanned_floating_value; /* original 0x97ba8, scanner conversion result */

/* The shipped CRT converts decimal spelling to ten x87 value bytes. Its
   public boundary has no Extended representation tag. */
int _atoldbl(void *value, const char *text);

/* Original 0x74b40..0x74c50, stdcall8. Both ordinary and L-suffixed
   values are rounded through double; F uses single precision. */
int __stdcall convert_floating_literal(char *text, Extended *value)
{
    Extended parsed;
    char *end;
    unsigned char suffix;
    float single;
    double real;
    _atoldbl(&parsed, text);
    value->low = parsed.low;
    value->high = parsed.high;
    value->exponent = parsed.exponent;
    for (end = text + 1; *end; ++end) ;
    suffix = (unsigned char)end[-1] & 0xdf;
    if (suffix == 'L') {
        if (extended_to_double(value, &real)) {
            value->format = 8;
            return 1;
        }
        double_to_extended(&real, value);
        value->format = 8;
    } else if (suffix == 'F') {
        value->format = 4;
        if (extended_to_float(value, &single)) return 1;
        float_to_extended(&single, value);
    } else {
        value->format = 8;
        if (extended_to_double(value, &real)) return 1;
        double_to_extended(&real, value);
    }
    return 0;
}

/* Original 0xa66de..0xa66e7, exactly ten bytes. The following original
   integer threshold is a separate object, not an Extended format field. */
ExtendedLimit double_round_limit = { 0xfffffbffU, 0xffffffffU, 0x43fe };

/* Original 0x74dfc..0x74e22. MSVC 1.0 has no ten-byte scalar type;
   these narrow transfers retain it and the private range-helper x87 ABI.
   Ordinary C controls the range result and conditional output write. */
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

/* Original 0xa670a..0xa6711: comparison state overlays the encountered
   single/double precision-rounding storage. */
ExtendedWorkspace extended_workspace;

/* Original 0x750a5..0x750ea. ST(0) supplies the limit, ST(1) the
   value; the limit is popped and the signed value retained in ST(0).
   Period C cannot express these operands or the x87 condition bits.
   The narrow blocks transfer/compare them; sign and range decisions are C. */
int check_extended_range(void)
{
    unsigned short status;
    extended_workspace.range.negative = 0;
    __asm {
        fxch st(1)
        ftst
        fstsw status
        fwait
    }
    extended_workspace.range.status = status;
    if (status & 0x100) {
        ++extended_workspace.range.negative;
        __asm { fchs }
    }
    __asm {
        fxch st(1)
        fcomp st(1)
        fstsw status
        fwait
    }
    extended_workspace.range.status = status;
    if (extended_workspace.range.negative) {
        __asm { fchs }
    }
    return (status & 0x100) != 0;
}

/* Original 0x74e68..0x74e89. The ten-byte store has private ESI/ST(0)
   inputs, so only those transfers require assembly. */
int double_to_extended(const double *value, Extended *result)
{
    __asm {
        mov eax, value
        fld qword ptr [eax]
    }
    result->format = 8;
    __asm {
        mov esi, result
        call store_extended
    }
    return 0;
}

/* Original 0x750a1..0x750a4. This shared ten-byte store tail takes
   ESI/ST(0), has no C frame, and pops the x87 value. Period C cannot
   express that value width or calling convention. */
__declspec(naked) void store_extended(void)
{
    RECON_TRACE_NAKED_ENTRY(store_extended)
    __asm {
        fstp tbyte ptr [esi]
        fwait
        ret
    }
}

/* Original 0x74d37..0x74d85. Ten-byte arithmetic and the ESI/ST(0)
   finishing ABI require narrow bridges; representation selection stays C. */
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

/* Each bound is exactly ten bytes; no representation tag or padding. */
ExtendedLimit float_round_limit = { 0xffffffffU, 0xffffff7fU, 0x407e }; /* 0xa66d4 */
ExtendedLimit extended_round_limit = { 0xffffffffU, 0xffffffffU, 0x7ffe }; /* 0xa66e8 */

/* Original 0x75034..0x750a0, ending at the shared store entry.
   ESI supplies the destination and ST(0) the arithmetic value. The
   narrow blocks retain that private ABI and ten-byte scalar width;
   range/precision selection and returned status remain ordinary C. */
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
                fstp dword ptr extended_workspace
                fld dword ptr extended_workspace
            }
        } else {
            __asm {
                fstp qword ptr extended_workspace
                fld qword ptr extended_workspace
            }
        }
    }
    __asm {
        mov esi, number
        call store_extended
    }
    return status;
}

/* Original0x74dd5..0x74dfc, cdecl. As in the double converter, the
   original compiler cannot express ten-byte loads or the helper's x87 ABI;
   only those transfers and the conditional single store use assembly. */
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

/* Original0x74e46..0x74e68, cdecl. Preserve the shared ESI/ST(0)
   ten-byte finishing ABI; the representation tag remains ordinary C. */
int float_to_extended(const float *value, Extended *result)
{
    __asm {
        mov eax, value
        fld dword ptr [eax]
    }
    result->format = 4;
    __asm {
        mov esi, result
        call store_extended
    }
    return 0;
}
