#ifndef C1_KEYWORDS_H
#define C1_KEYWORDS_H
#include "c1.h"
#include <recon.h>

/* Ordinary names are16 bytes. Keyword records extend this same header;
 * generic names never own or expose the token/category bytes. */
struct CompilerMacro;
struct CompilerNameEntry {
    CompilerNameEntry *next;
    char *name;
    unsigned flags;
    CompilerMacro *macro; /* Original+12; retained until a definition writes it. */
};
struct CompilerKeywordEntry : CompilerNameEntry {
    BYTE token, category; /* Original+16/+17; final padding stays unmodeled. */
};
typedef char CompilerNameEntry_size[sizeof(CompilerNameEntry) == 16 ? 1 : -1];
typedef char CompilerKeywordEntry_size[sizeof(CompilerKeywordEntry) == 20 ? 1 : -1];
#endif
