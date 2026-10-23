#include <wctype.h>
#include "directives.h"
#include <recon.h>

int __stdcall rc_digit_value(int character)
{
    if(rc_character_class((unsigned short)character)&4)return character-L'0';
    return (int)towupper((WCHAR)character)+10-(unsigned)towupper(L'A');
}
