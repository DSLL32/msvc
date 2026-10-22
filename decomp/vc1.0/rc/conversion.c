/* Original 0x010080c0..0x0100810e. NT counted-string conversion bridge. */
#include "resource.h"
struct AnsiString { USHORT length, capacity; char *text; };
struct UnicodeString { USHORT length, capacity; WCHAR *text; };
LONG __stdcall RtlAnsiStringToUnicodeString(struct UnicodeString *, struct AnsiString *, BYTE);

BOOL __stdcall ansi_to_wide(WCHAR *destination, char *source, unsigned int length)
{
    struct AnsiString ansi;
    struct UnicodeString unicode;
    ansi.length = ansi.capacity = (USHORT)length;
    ansi.text = source;
    unicode.capacity = (USHORT)(length * 2 + 2);
    unicode.text = destination;
    return RtlAnsiStringToUnicodeString(&unicode, &ansi, FALSE) == 0;
}
