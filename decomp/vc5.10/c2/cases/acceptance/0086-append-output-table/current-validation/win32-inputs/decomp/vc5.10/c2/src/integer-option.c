#include "c2.h"
#include <recon.h>

int __fastcall parse_option_integer(char *value)
{
    char *text = value, *end;
    long number;
    int (__cdecl *classify)(int, int) = _isctype;
    int *(__cdecl *error_number)(void);

    while (MB_CUR_MAX > 1 ? classify(*text, 8) : (_pctype[(int)*text] & 8))
        ++text;
    error_number = _errno;
    *error_number() = 0;
    number = strtol(text, &end, 0);
    if (*error_number() == ERANGE || *end)
        c2_fatal(49, value);
    return number;
}
