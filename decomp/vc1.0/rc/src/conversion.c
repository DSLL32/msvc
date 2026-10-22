/* Original 0x010080c0..0x0100810e. NT counted-string conversion bridge. */
#include "resource.h"
#include <recon.h>
struct AnsiString { USHORT length, capacity; char *text; };
struct UnicodeString { USHORT length, capacity; WCHAR *text; };
LONG __stdcall RtlAnsiStringToUnicodeString(struct UnicodeString *, struct AnsiString *, BYTE);
LONG __stdcall RtlUnicodeStringToAnsiString(struct AnsiString *, struct UnicodeString *, BYTE);

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

/* Original 0x01008110..0x01008166. */
BOOL __stdcall wide_to_ansi(char *destination, WCHAR *source, WORD length)
{
    struct AnsiString ansi;
    struct UnicodeString unicode;
    unicode.length = (USHORT)(length * 2);
    unicode.capacity = (USHORT)(length * 2 + 2);
    unicode.text = source;
    ansi.capacity = unicode.capacity >> 1;
    ansi.text = destination;
    return RtlUnicodeStringToAnsiString(&ansi, &unicode, FALSE) == 0;
}
