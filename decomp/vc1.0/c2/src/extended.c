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
    struct { unsigned negative; unsigned short status, saved_control; } range;
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

/* Original [000711e4,00071233). Same tag policy and private x87
   bridge as addition; the m80 arithmetic primitive subtracts right from left. */
int __cdecl subtract_extended(const Extended *left, const Extended *right, Extended *result)
{
    unsigned format;
    int range_error;
    __asm {
        mov esi, left
        mov edi, right
        fld tbyte ptr [esi]
        fld tbyte ptr [edi]
        fsubp st(1), st(0)
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

/* Original signed integer range [00086154,0008615e). */
static const Extended integer_conversion_limit = {0, 0xfffffffeUL, 0x401d, 0};

/* Original [000713e4,0007140c). Required m80/raw-ST0 bridge. The
   rounding helper leaves a truncated value on ST(0); the range helper
   supplies EAX while FISTP writes the signed DWORD destination. */
int extended_to_integer(const Extended *number, int *result)
{
    int range_error;
    __asm {
        mov esi, number
        fld tbyte ptr [esi]
        call truncate_extended
        fld tbyte ptr integer_conversion_limit
        call check_extended_range
        mov edi, result
        fistp dword ptr [edi]
        fwait
        mov range_error, eax
    }
    return range_error;
}

/* Original temporary control word [00086174,00086176); saved control
   is the contextual alias at scratch+6 ([00086172,00086174)). */
static unsigned short truncation_control;

/* Original [000715f8,00071621). Private ST0 input/result. Preserve the
   caller's control word around truncation, including original invalid-mask
   bit0 and round-toward-zero bits10/11. No scalar80-bit C type exists. */
void truncate_extended(void)
{
    __asm {
        fwait
        fnstcw word ptr extended_scratch+6
        fwait
    }
    truncation_control = extended_scratch.range.saved_control | 0x0c01;
    __asm {
        fwait
        fldcw truncation_control
        frndint
        fwait
        fldcw word ptr extended_scratch+6
    }
}

/* Original71233..71282. As for add_extended, ordinary C cannot load or
   multiply the80-bit payload or call the ESI/ST0 normalizer. Only those
   primitives/ABI transfers use assembly; tag selection remains C. */
int __cdecl multiply_extended(const Extended *left, const Extended *right, Extended *result)
{
    unsigned format;
    int range_error;
    __asm {
        mov esi, left
        mov edi, right
        fld tbyte ptr [esi]
        fld tbyte ptr [edi]
        fmulp st(1), st(0)
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

/* Original71282..712d1. Required80-bit arithmetic/private ESI-ST0
   bridge as above. Divide left by right; do not reverse the x87 operands. */
int __cdecl divide_extended(const Extended *left, const Extended *right, Extended *result)
{
    unsigned format;
    int range_error;
    __asm {
        mov esi, left
        mov edi, right
        fld tbyte ptr [esi]
        fld tbyte ptr [edi]
        fdivp st(1), st(0)
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

/* Original71560..715ac. FABS on the80-bit payload and the private
   normalizer call cannot be expressed by this compiler's double type.
   Preserve their x87 behavior; format selection is ordinary C. */
int __cdecl absolute_extended(const Extended *number, Extended *result)
{
    unsigned format;
    int range_error;
    __asm {
        mov esi, number
        fld tbyte ptr [esi]
        fabs
    }
    format = number->format;
    result->format = (format & 2) ? 10 : ((format & 8) ? 8 : 4);
    __asm {
        mov esi, result
        call normalize_extended
        mov range_error, eax
    }
    return range_error;
}

/* Original713b1..713e4. Zero-extend the unsigned DWORD into the actual
   signed-qword FILD operand. Only that load and the ESI/ST0 store ABI need
   assembly; the original compiler has no80-bit C scalar representation. */
void unsigned_integer_to_extended(const unsigned *integer, Extended *number)
{
    struct { unsigned low, high; } widened;
    widened.low = *integer;
    widened.high = 0;
    number->format = 4;
    __asm {
        lea edi, widened
        fild qword ptr [edi]
        mov esi, number
        call store_extended
    }
}

/* Original71494..714fa. The private80-bit comparison leaves the carry
   condition in the shared status word. Keep value selection/tag policy in C;
   only raw x87 loads, exchange/discard and the ESI/ST0 bridge require asm.
   Equality retains left; unordered follows the original carry-bit branch. */
int __cdecl minimum_extended(const Extended *left, const Extended *right, Extended *result)
{
    unsigned format;
    int range_error;
    __asm {
        mov esi, left
        mov edi, right
        fld tbyte ptr [esi]
        fld tbyte ptr [edi]
        fcom st(1)
        fwait
        fnstsw word ptr extended_scratch+4
        fwait
    }
    if (extended_scratch.range.status & 0x100) {
        __asm fxch st(1)
    }
    __asm fstp st(0)
    format = left->format | right->format;
    result->format = (format & 2) ? 10 : ((format & 8) ? 8 : 4);
    __asm {
        mov esi, result
        call normalize_extended
        mov range_error, eax
    }
    return range_error;
}

/* Original712d1..712f4. Same documented m80/raw-ST0 range-check bridge
   as extended_to_double, followed by the actual32-bit floating store. */
int extended_to_float(const Extended *number, float *result)
{
    int range_error;
    __asm {
        mov esi, number
        fld tbyte ptr [esi]
        fld tbyte ptr single_conversion_limit
        call check_extended_range
        mov esi, result
        fstp dword ptr [esi]
        fwait
        mov range_error, eax
    }
    return range_error;
}

/* Original71331..71351. Load the actual32-bit scalar before writing
   its output tag, then reuse the raw ESI/ST0 store entry. An80-bit result
   cannot be represented by this compiler's C floating types. */
void float_to_extended(const float *number, Extended *result)
{
    __asm {
        mov edi, number
        fld dword ptr [edi]
    }
    result->format = 4;
    __asm {
        mov esi, result
        call store_extended
    }
}
