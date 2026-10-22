/* Native utilities with no identified production caller. Their historical
 * library/compiler provenance remains uncertain; direct cases identify the ABI.
 */
#include "rc.h"

/* Original 0x0100f3d0..0x0100f3fb, signed low byte, RET 4.
 * This is not C isspace: VT/FF are false, while ff (signed EOF) is true. */
int __stdcall is_space_byte(signed char character)
{
    return character == -1 || character == '\t' || character == '\n' ||
           character == '\r' || character == ' ' ? -1 : 0;
}

/* Original 0x0100f530..0x0100f58d, RET 8. The count includes the NUL. */
unsigned int __stdcall copy_basename(const char *path, char *destination)
{
    const char *end = path, *name;
    unsigned int count;
    while (*end)
        ++end;
    name = end;
    while (name != path && name[-1] != '\\' && name[-1] != '/' && name[-1] != ':')
        --name;
    count = (unsigned int)(end - name) + 1;
    while ((*destination++ = *name++) != 0)
        ;
    return count;
}
