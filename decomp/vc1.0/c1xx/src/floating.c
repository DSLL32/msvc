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

/* Original 0xa670a..0xa670f: sign and x87 comparison status. */
ExtendedRangeState extended_range_state;

/* Original 0x750a5..0x750ea. ST(0) supplies the limit, ST(1) the
   value; the limit is popped and the signed value retained in ST(0).
   Period C cannot express these operands or the x87 condition bits.
   The narrow blocks transfer/compare them; sign and range decisions are C. */
int check_extended_range(void)
{
    unsigned short status;
    extended_range_state.negative = 0;
    __asm {
        fxch st(1)
        ftst
        fstsw status
        fwait
    }
    extended_range_state.status = status;
    if (status & 0x100) {
        ++extended_range_state.negative;
        __asm { fchs }
    }
    __asm {
        fxch st(1)
        fcomp st(1)
        fstsw status
        fwait
    }
    extended_range_state.status = status;
    if (extended_range_state.negative) {
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
