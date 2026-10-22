/* Reserved preprocessing directive names, grouped by first character. */
#include "directives.h"
/* Original [0100f8d0,0100f958). The original stores the initial-character
 * group index separately from the suffix spellings and result codes. */
unsigned char __stdcall rc_directive_code(const char *word)
{
    static const struct {
        const char *suffix;
        unsigned char code;
    } names[] = {
        { "efine", PP_DEFINE },
        { "rror", PP_ERROR }, { "ndif", PP_ENDIF },
        { "lse", PP_ELSE }, { "lif", PP_ELIF },
        { "nclude", PP_INCLUDE }, { "fndef", PP_IFNDEF },
        { "fdef", PP_IFDEF }, { "f", PP_IF }, { "dent", PP_IDENT },
        { "ine", PP_LINE }, { "ragma", PP_PRAGMA }, { "ndef", PP_UNDEF }
    };
    unsigned first, last, i;
    const char *a, *b;
    switch (*word) {
    case 'd': first = 0; last = 1; break;
    case 'e': first = 1; last = 5; break;
    case 'i': first = 5; last = 10; break;
    case 'l': first = 10; last = 11; break;
    case 'p': first = 11; last = 12; break;
    case 'u': first = 12; last = 13; break;
    default: return PP_UNKNOWN;
    }
    for (i = first; i < last; ++i) {
        a = names[i].suffix;
        b = word + 1;
        while (*a == *b && *a) { ++a; ++b; }
        if (*a == *b) return names[i].code;
    }
    return PP_UNKNOWN;
}
