/* Historical CRT graph body; no unique variant is asserted. */
#include <wchar.h>
#include <recon.h>

/* Original 0x01013d70..0x01013d96. */
wchar_t * __cdecl wcscpy(wchar_t *destination, const wchar_t *source)
{
    wchar_t *result = destination;
    do {
        *destination++ = *source;
    } while (*source++);
    return result;
}
