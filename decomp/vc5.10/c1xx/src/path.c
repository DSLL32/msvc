#include <stdlib.h>
#include <mbctype.h>
#include <mbstring.h>
#include "frontend.h"
#include <recon.h>

/* Keep the original direct data import; the VC5 /MD SDK macro otherwise
 * adds an accessor call absent from the original function graph. */
#if defined(_MSC_VER) && defined(_M_IX86) && defined(_DLL)
#undef _mbctype
_CRTIMP extern unsigned char _mbctype[];
#endif

char *normalize_path(char *path);

char *resolve_option_path(char *output, const char *input, unsigned capacity)
{
    char path[260];
    unsigned length, index;
    int unchanged = 0, unc = 0, rooted = 0;

    for (length = 0; input[length]; ++length)
        ;
    if (length >= capacity)
        report_compiler_diagnostic(81, input, capacity);
    for (index = 0; (path[index] = input[index]) != 0; ++index)
        ;
    for (length = 0; path[length]; ++length)
        ;
    if (length > 2) {
        normalize_path(path);
        if (!(_mbctype[(unsigned char)path[0] + 1] & _M1)) {
            if (path[0] == '\\' && path[1] == '\\')
                unc = rooted = 1;
            else if ((unsigned char)path[0] < 0x80 && path[1] == ':')
                rooted = 1;
            if (rooted) {
                unchanged = !_mbsstr((unsigned char *)path,
                                     (const unsigned char *)".\\");
                if (unchanged && !unc)
                    unchanged = !!_mbsstr((unsigned char *)path,
                                          (const unsigned char *)":\\");
            }
        }
    }
    if (unchanged || !_fullpath(output, input, capacity))
        for (index = 0; (output[index] = input[index]) != 0; ++index)
            ;
    return normalize_path(output);
}


/* The original uses its dead incoming pointer slot as a byte temporary.
 * Both callers discard that by-value slot immediately after the call. */
char *normalize_path(char *path)
{
    char *cursor = path;
    unsigned character;
    WORD folded;

    while ((character = (unsigned char)*cursor) != 0) {
        folded = frontend_bss.filename_case_map[character];
        if (folded == 0xffff)
            cursor += 2;
        else {
            *cursor = (char)folded;
            ++cursor;
        }
    }
    return path;
}
