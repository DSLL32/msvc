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
