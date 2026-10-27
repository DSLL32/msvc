#ifndef C1XX_INCLUDES_H
#define C1XX_INCLUDES_H
#include "frontend.h"
#include <recon.h>

extern "C" {
extern const char compiler_directive_else[], compiler_directive_endif[];
}

/* The lookup reads these five DWORD fields. This does not establish an
 * allocation extent for later alias writers. String pointers are borrowed. */
struct CompilerIncludeAlias {
    CompilerIncludeAlias *next;
    const char *name, *replacement;
    unsigned kind, replacement_kind;
};
struct CompilerIncludeAliases {
    CompilerIncludeAlias *head;
    const char *RECON_THISCALL Find(const char *, unsigned *);
};
typedef char CompilerIncludeAliases_root_size[sizeof(CompilerIncludeAliases) == 4 ? 1 : -1];
#endif
