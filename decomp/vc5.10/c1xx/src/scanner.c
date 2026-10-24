#include <mbctype.h>
#include <mbstring.h>
#include "frontend.h"
#include <recon.h>

/* The original imports the public array at IAT c90c4. VC5's /MD SDK
 * accessor macro would introduce a function absent from the target graph. */
#if defined(_MSC_VER) && defined(_M_IX86) && defined(_DLL)
#undef _mbctype
_CRTIMP extern unsigned char _mbctype[];
#endif

void initialize_multibyte_categories(void)
{
    unsigned character;

    _setmbcp(_MB_CP_LOCALE);
    for (character = 0; character < 256; ++character) {
        if (_mbctype[(unsigned char)character + 1] & _M1) {
            frontend_data.scanner.flags[character] |= 0x80;
            frontend_data.scanner.classes[character] = SC_LEAD;
        }
    }
}
void initialize_lowercase_table(void)
{
    unsigned character;

    for (character = 0; character < 256; ++character) {
        if (frontend_data.scanner.classes[(unsigned char)character] == SC_LEAD)
            frontend_bss.filename_case_map[character] = 0xffff;
        else
            frontend_bss.filename_case_map[character] = (WORD)_mbctolower(character);
    }
    frontend_bss.filename_case_map['/'] = '\\';
}
