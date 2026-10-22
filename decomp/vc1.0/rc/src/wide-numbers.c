/* RC's own wide-number utilities, distinct from CRT conversion functions. */
#include "dialog.h"
#include <recon.h>

/* Original 0x0100f590..0x0100f5b6. Every nonzero WCHAR contributes its signed
 * low byte; there is no whitespace, sign or digit validation. */
int __stdcall wide_atoi(const WCHAR *text)
{
    DWORD value = 0;
    while (*text)
        value = value * 10 + (signed char)(BYTE)*text++ - '0';
    return (int)value;
}

/* Original 0x0100f5c0..0x0100f620. Zero produces an empty string. */
WCHAR * __stdcall wide_itoa(DWORD value, WCHAR *text, DWORD radix)
{
    DWORD quotient = value, digits = 0;
    WCHAR *next;
    while (quotient) {
        ++digits;
        quotient /= radix;
    }
    next = text + digits;
    *next = 0;
    while (value) {
        *--next = (WCHAR)((signed char)(BYTE)(value % radix) + '0');
        value /= radix;
    }
    return next;
}
