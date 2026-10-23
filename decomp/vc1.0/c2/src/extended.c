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

/* Original [00071351,00071371). Required80-bit/ESI/ST0 bridge, as
   for integer_to_extended: load the source double and reuse the shared
   store entry; the format tag is an ordinary contextual field. */
void double_to_extended(const DoubleBits *number, Extended *result)
{
    result->format = 8;
    __asm {
        mov edi, number
        fld qword ptr [edi]
        mov esi, result
        call store_extended
    }
}

/* Original [000715ac,000715f8). The m80 load/compare primitive is
   unexpressible in this toolchain; relation selection and the caller's
   mask are C. As in the original, unordered status selects equality
   through C3 before consulting the less-than C0 bit. */
int compare_extended(unsigned operation, const Extended *left, const Extended *right)
{
    unsigned relation;
    __asm {
        mov esi, left
        mov edi, right
        fld tbyte ptr [esi]
        fld tbyte ptr [edi]
        fxch st(1)
        fcompp
        fwait
        fnstsw word ptr extended_scratch+4
        fwait
    }
    if (extended_scratch.range.status & 0x4000) relation = 1;
    else if (extended_scratch.range.status & 0x100) relation = 4;
    else relation = 2;
    return (relation & (unsigned short)operation) != 0;
}

/* Original [00071195,000711e4). C selects the combined format tag;
   m80 arithmetic and the ESI/ST0 normalizer call require the private bridge. */
int __cdecl add_extended(const Extended *left, const Extended *right, Extended *result)
{
    unsigned format;
    int range_error;
    __asm {
        mov esi, left
        mov edi, right
        fld tbyte ptr [esi]
        fld tbyte ptr [edi]
        faddp st(1), st(0)
    }
    format = left->format | right->format;
    result->format = (format & 2) ? 10 : ((format & 8) ? 8 : 4);
    __asm {
        mov esi, result
        call normalize_extended
        mov range_error, eax
    }
    return range_error;
}

/* Original single range bound [00086140,0008614a); only ten bytes
   participate, independently of the contextual C structure's final word. */
static const Extended single_conversion_limit = {0xffffffffUL, 0xffffff7fUL, 0x407e, 0};
extern int optimize_precision;

/* Original [00071621,00071681), followed by the already accepted
   store_extended tail. ESI/ST0 input cannot be expressed in C; format
   selection and optional rounding policy are otherwise ordinary C. */
int normalize_extended(void)
{
    Extended *number;
    int range_error = 0;
    __asm mov number, esi
    if (number->format == 4) {
        __asm {
            fld tbyte ptr single_conversion_limit
            call check_extended_range
            mov range_error, eax
        }
        if (optimize_precision) {
            __asm {
                fstp dword ptr extended_scratch
                fld dword ptr extended_scratch
            }
        }
    } else if (number->format != 10) {
        __asm {
            fld tbyte ptr double_conversion_limit
            call check_extended_range
            mov range_error, eax
        }
        if (optimize_precision) {
            __asm {
                fstp qword ptr extended_scratch
                fld qword ptr extended_scratch
            }
        }
    }
    __asm {
        mov esi, number
        call store_extended
    }
    return range_error;
}
