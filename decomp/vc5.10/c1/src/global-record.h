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
/* Source records are arena-owned20-byte nodes. Construction writes name,
 * kind and an optional previous link; the untouched bytes have no invented
 * defaults. The existing current_context.object supplies this same receiver. */
struct CompilerSourceFile {
    struct CompilerNameEntry *name;
    BYTE kind;
    BYTE unmodeled_05[3];
    unsigned reference_flags; /* Original+8, source-only non-IL view; no default claim */
    unsigned record_id;
    CompilerSourceFile *next;
    unsigned RECON_THISCALL OutputIndex(void);
    unsigned RECON_THISCALL AssignOutputIndex(void);
};
typedef char CompilerSourceFile_size[sizeof(CompilerSourceFile) == 20 ? 1 : -1];
#endif
