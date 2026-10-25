#ifndef C1_GLOBAL_RECORD_H
#define C1_GLOBAL_RECORD_H
#include "c1.h"
#include <recon.h>

/* Encountered twelve-byte location payload; only the writer's transfer is
 * described here, without assigning a wider debug-provider object layout. */
struct CompilerGlobalLocation { unsigned first, second, third; };
/* Source-only option-list view. Its owners remain behind the native guard. */
struct CompilerGlobalOption {
    CompilerGlobalOption *next;
    unsigned kind;
    const BYTE *value;
};
/* The existing current_context.object supplies this receiver. Only its
 * encountered index DWORD is exposed; no allocation/first12-byte claim. */
struct CompilerSourceFile {
    BYTE unmodeled_prefix[12];
    unsigned record_id;
    unsigned RECON_THISCALL OutputIndex(void);
};
#endif
