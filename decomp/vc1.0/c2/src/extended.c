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
