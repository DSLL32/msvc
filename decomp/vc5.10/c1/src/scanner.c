#include <mbctype.h>
#include <mbstring.h>
#include "compiler-state.h"
#include <recon.h>

/* This target imports the public data array directly. The VC5 /MD header
 * defaults to its optional accessor macro, which would add a function call. */
#if defined(_MSC_VER) && defined(_M_IX86) && defined(_DLL)
#undef _mbctype
_CRTIMP extern unsigned char _mbctype[];
#endif

void __cdecl initialize_multibyte_scanner(void)
{
    unsigned character;
    _setmbcp(_MB_CP_LOCALE);
    for (character = 0; character < 256; ++character) {
        if (_mbctype[(unsigned char)character + 1] & _M1) {
            c1_data.scanner.flags[character] |= 0x80;
            c1_data.scanner.classes[character] = SC_LEAD;
        }
    }
}

void __cdecl initialize_path_fold_table(void)
{
    unsigned character;
    for (character = 0; character < 256; ++character) {
        if (c1_data.scanner.classes[(unsigned char)character] == SC_LEAD)
            c1_bss.path_fold[character] = 0xffff;
        else
            c1_bss.path_fold[character] = (WORD)_mbctolower(character);
    }
    c1_bss.path_fold['/'] = '\\';
}
