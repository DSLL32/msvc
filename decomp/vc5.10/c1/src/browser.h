#ifndef C1_BROWSER_H
#define C1_BROWSER_H
#include "c1.h"
#include <recon.h>

/* The original stack record is24 bytes. Only the named fields are written;
 * alignment bytes2..3 and22..23 are neither initialized nor semantic data. */
typedef struct CompilerBrowserRecord {
    BYTE kind, category;
    BYTE unmodeled_02[2];
    unsigned line, symbol_id;
    const char *name;
    struct CompilerNameEntry *identifier;
    WORD attributes;
    BYTE unmodeled_16[2];
} CompilerBrowserRecord;
typedef char BrowserRecord_size[sizeof(CompilerBrowserRecord) == 24 ? 1 : -1];

#ifdef __cplusplus
extern "C" {
#endif
void __cdecl emit_browser_record(CompilerBrowserRecord *);
#ifdef __cplusplus
}
#endif
#endif
