/* Locale-aware uppercase conversion, original 0x01013a50..0x01013bc4.
 * Unlike the shipped LIBC routine, RC also tests the active code page before
 * selecting its C-locale path. Its two-byte return ordering is preserved. */
#include "driver.h"
#include <ctype.h>
#include <locale.h>
extern unsigned long _lc_handle[];
extern unsigned int _lc_codepage;

int __cdecl rc_toupper(int character)
{
    char input[3];
    WCHAR wide[2], mapped[2];
    int count, bytes;
    unsigned int lead;

    if (!_lc_handle[LC_CTYPE] && !_lc_codepage)
        return islower(character) ? character - ('a' - 'A') : character;
    if (character < 256 && !islower(character))
        return character;
    lead = (character >> 8) & 255;
    if (_pctype[lead] & 0x8000) {
        input[0] = (char)lead;
        input[1] = (char)character;
        input[2] = 0;
    } else {
        input[0] = (char)character;
        input[1] = 0;
    }
    count = MultiByteToWideChar(_lc_codepage, 1, input, -1, wide, 2);
    if (!count)
        return character;
    count = LCMapStringW(_lc_handle[LC_CTYPE], LCMAP_UPPERCASE,
                         wide, count, mapped, 2);
    if (!count)
        return character;
    bytes = WideCharToMultiByte(_lc_codepage, 0x210, mapped, count,
                                input, 2, 0, 0);
    if (!bytes)
        return character;
    if (bytes == 1)
        return input[0];
    return (input[1] << 8) | input[0];
}

/* Lowercase counterpart, original 0x01013890..0x01013a04. */
int __cdecl rc_tolower(int character)
{
    char input[3];
    WCHAR wide[2], mapped[2];
    int count, bytes;
    unsigned int lead;

    if (!_lc_handle[LC_CTYPE] && !_lc_codepage)
        return isupper(character) ? character + ('a' - 'A') : character;
    if (character < 256 && !isupper(character))
        return character;
    lead = (character >> 8) & 255;
    if (_pctype[lead] & 0x8000) {
        input[0] = (char)lead;
        input[1] = (char)character;
        input[2] = 0;
    } else {
        input[0] = (char)character;
        input[1] = 0;
    }
    count = MultiByteToWideChar(_lc_codepage, 1, input, -1, wide, 2);
    if (!count)
        return character;
    count = LCMapStringW(_lc_handle[LC_CTYPE], LCMAP_LOWERCASE,
                         wide, count, mapped, 2);
    if (!count)
        return character;
    bytes = WideCharToMultiByte(_lc_codepage, 0x210, mapped, count,
                                input, 2, 0, 0);
    if (!bytes)
        return character;
    if (bytes == 1)
        return input[0];
    return (input[1] << 8) | input[0];
}
