#ifndef C1_KEYWORDS_H
#define C1_KEYWORDS_H
#include "c1.h"
#include <recon.h>

/* Compact view: original next/name/flags at0/4/8 and token/category at16/17.
 * Original DWORD12 and the allocation tail are UNMODELED, never copied here.
 * The interner allocates the authentic20 bytes independently of this view. */
struct CompilerNameEntry {
    CompilerNameEntry *next;
    char *name;
    unsigned flags;
    BYTE token, category;
};
#endif
