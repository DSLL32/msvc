#include "c2.h"
#include <recon.h>

C2Real c2_real_zero, c2_real_one, c2_real_two;

/* This compiler has no 80-bit scalar type, and the shared store receives its
 * address in ESI and value on ST(0). Only that ABI bridge requires assembly. */
void c2_real_from_int(int *input, C2Real *output)
{
    __asm {
        mov edi, input
        fild dword ptr [edi]
    }
    output->kind = 4;
    __asm {
        mov esi, output
        call store_extended
    }
}

/* Private ESI/ST(0) ABI; ordinary C cannot represent this incoming value.
 * Store only the ten-byte payload, leaving the caller's kind untouched. */
__declspec(naked) void store_extended(void)
{
    RECON_TRACE_NAKED_ENTRY(store_extended)
    __asm {
        fstp tbyte ptr [esi]
        fwait
        ret
    }
}

void c2_initialize_constants(void)
{
    int value = 0;
    c2_real_from_int(&value, &c2_real_zero);
    value = 1;
    c2_real_from_int(&value, &c2_real_one);
    value = 2;
    c2_real_from_int(&value, &c2_real_two);
}

/* Actual 49e278 scratch and 49a062 WORD are owners written by this
 * conversion, not part of either ten-byte real operand. */
C2RealOperationScratch real_operation_scratch;
const C2RealLimit double_conversion_limit = {0xfffffbffUL, 0xffffffffUL, 0x43fe};
static const C2RealLimit single_conversion_limit = {0xffffffffUL, 0xffffff7fUL, 0x407e};
extern int c2_float_precision;
unsigned short real_comparison_status;

/* Private x87 ABI: limit ST(0), value ST(1), EAX status and ST(0) value
 * returned, with no stack formals. VC5 C has no 80-bit scalar type, so only
 * the scalar x87 test/compare/sign operations require inline assembly.
 * Ordinary C expresses the sign handling and out-of-range result. FTST's
 * C0 also selects the original unordered path; it is not a sign-bit test. */
int constrain_input_real(void)
{
    int negative;
    real_operation_scratch.negative = 0;
    __asm {
        fxch st(1)
        ftst
        fstsw real_comparison_status
        fwait
    }
    negative = (real_comparison_status & 0x100) != 0;
    if (negative) {
        ++real_operation_scratch.negative;
        __asm { fchs }
    }
    __asm {
        fxch st(1)
        fcomp st(1)
        fstsw real_comparison_status
        fwait
    }
    if (negative) {
        __asm { fchs }
    }
    return (real_comparison_status & 0x100) != 0;
}

/* Compare ten-byte operands without rounding to VC5's 64-bit long double.
 * The original tests equality before less-than, so unordered also selects
 * the equality mask. Integer C retains that exact condition-mask behavior. */
int c2_real_compare(int condition, C2Real *left, C2Real *right)
{
    unsigned comparison;
    __asm {
        mov esi, left
        mov edi, right
        fld tbyte ptr [esi]
        fld tbyte ptr [edi]
        fxch st(1)
        fcompp
        fstsw real_comparison_status
        fwait
    }
    if (real_comparison_status & 0x4000) comparison = 1;
    else if (real_comparison_status & 0x100) comparison = 4;
    else comparison = 2;
    return (condition & comparison) != 0;
}

/* Genuine ESI destination/ST(0) value, EAX status, RET0. VC5 C cannot
 * express this register/x87 input, so capture ESI before the compiler uses it.
 * Ordinary C retains the original precision and result-kind decisions. */
int finish_real_operation(void)
{
    C2Real *result;
    int status;
    __asm { mov result, esi }
    status = 0;
    if (result->kind == 4) {
        __asm {
            fld tbyte ptr [single_conversion_limit]
            call constrain_input_real
            mov status, eax
        }
        if (c2_float_precision) {
            __asm {
                fstp dword ptr [real_operation_scratch]
                fld dword ptr [real_operation_scratch]
            }
        }
    } else if (result->kind != 10) {
        __asm {
            fld tbyte ptr [double_conversion_limit]
            call constrain_input_real
            mov status, eax
        }
        if (c2_float_precision) {
            __asm {
                fstp qword ptr [real_operation_scratch]
                fld qword ptr [real_operation_scratch]
            }
        }
    }
    __asm {
        mov esi, result
        call store_extended
    }
    return status;
}

int c2_real_subtract(C2Real *left, C2Real *right, C2Real *result)
{
    unsigned kind;
    int status;
    /* VC5 cannot express ten-byte arithmetic in C. Load left then right;
     * FSUBP computes ST(1)-ST(0), leaving the exact extended difference. */
    __asm {
        mov esi, left
        mov edi, right
        fld tbyte ptr [esi]
        fld tbyte ptr [edi]
        fsubp st(1), st
    }
    kind = left->kind | right->kind;
    result->kind = (kind & 2) ? 10 : (kind & 8) ? 8 : 4;
    __asm {
        mov esi, result
        call finish_real_operation
        mov status, eax
    }
    return status;
}
