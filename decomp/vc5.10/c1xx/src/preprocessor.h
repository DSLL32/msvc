#ifndef C1XX_PREPROCESSOR_H
#define C1XX_PREPROCESSOR_H
#include "names.h"
#include <recon.h>

/* Only the reached kind and text fields are interpreted. The leading four
 * bytes retain their real storage and remain semantically unmodeled. */
struct CompilerMacroBody {
    BYTE unmodeled_prefix[4];
    signed char kind;
    char payload[1];
};
struct CompilerMacro {
    CompilerNameEntry *identifier;
    CompilerMacroBody *body;
};

/* The push/read/pop consumers establish all twelve fields of this real
 * 48-byte arena-owned frame; its buffer is a separate 0x7004-byte span. */
typedef struct CompilerSourceFrame {
    int descriptor; /* +0 */
    void *saved_origin; /* +4, existing context_stack.origin */
    unsigned saved_position; /* +8 */
    struct CompilerSourceFile *file; /* +c */
    BYTE *saved_cursor, *buffer; /* +10/+14 */
    int available_bytes;
    unsigned consumed_bytes;
    unsigned packing_alignment;
    int conditional_depth;
    unsigned flags;
    struct CompilerSourceFrame *next;
} CompilerSourceFrame;
typedef char CompilerSourceFrame_size[sizeof(CompilerSourceFrame) == 48 ? 1 : -1];
#endif
