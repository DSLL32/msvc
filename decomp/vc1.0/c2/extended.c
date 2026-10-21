/* The compiler's 80-bit arithmetic representation uses a private x87 ABI. */
#include "extended.h"
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
    __asm {
        fstp tbyte ptr [esi]
        fwait
        ret
    }
}
