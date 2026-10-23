/* The compiler's 80-bit representation is absent from this C compiler's
   scalar types. The shared store entry additionally receives its destination
   in ESI and its value on ST(0), requiring this narrow inline-assembly bridge. */
#include "extended.h"
#include <recon.h>
/* Original [00071391,000713b1). */
void integer_to_extended(const int *integer, Extended *number)
{
    __asm {
        mov edi, integer
        fild dword ptr [edi]
        mov esi, number
        mov word ptr [esi+10], 4
        call store_extended
    }
}

/* Original [00071681,00071685). Shared tail of the format normalizer. */
__declspec(naked) void store_extended(void)
{
    RECON_TRACE_NAKED_ENTRY(store_extended)
    __asm {
        fstp tbyte ptr [esi]
        fwait
        ret
    }
}

/* Original [00071180,00071195): the tag is a signed word. */
int extended_format(const Extended *number)
{
    return (short)number->format;
}

/* Original double range bound [0008614a,00086154). Ten significant
   bytes; the structure's format/padding word is not read by FLD. */
static const Extended double_conversion_limit = {0xfffffbffUL, 0xffffffffUL, 0x43fe, 0};

/* Original [000712f4,00071317). Required ABI exception: this compiler
   has no80-bit scalar type. The callee consumes the raw x87 stack and
   leaves its value on ST(0) and its range result in EAX. Ordinary C cannot
   express that call, so only the load/call/store bridge uses assembly. */
int extended_to_double(const Extended *number, DoubleBits *result)
{
    int range_error;
    __asm {
        mov esi, number
        fld tbyte ptr [esi]
        fld tbyte ptr double_conversion_limit
        call check_extended_range
        mov edi, result
        fstp qword ptr [edi]
        fwait
        mov range_error, eax
    }
    return range_error;
}

/* Original shared conversion scratch [0008616c,00086174). The status
   word aliases the upper half of the rounded double temporary. */
static union {
    DoubleBits rounded_double;
    float rounded_single;
    struct { unsigned negative; unsigned short status, reserved; } range;
} extended_scratch;

/* Original [00071685,000716cb). Private ABI: ST(0) is the magnitude
   bound, ST(1) the value. Return the original value on ST(0) and the
   comparison's carry bit in EAX. Only x87 primitives need assembly; sign
   handling and the status predicate remain ordinary C. FTST's carry
   treatment (including unordered) is retained rather than a C comparison. */
int check_extended_range(void)
{
    int negative;
    extended_scratch.range.negative = 0;
    __asm {
        fxch st(1)
        ftst
        fwait
        fnstsw word ptr extended_scratch+4
        fwait
    }
    negative = (extended_scratch.range.status & 0x100) != 0;
    if (negative) {
        ++extended_scratch.range.negative;
        __asm fchs
    }
    __asm {
        fxch st(1)
        fcomp st(1)
        fwait
        fnstsw word ptr extended_scratch+4
        fwait
    }
    if (negative) {
        __asm fchs
    }
    return (extended_scratch.range.status & 0x100) != 0;
}
