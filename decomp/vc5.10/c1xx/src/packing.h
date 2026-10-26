#ifndef C1XX_PACKING_H
#define C1XX_PACKING_H
#include <recon.h>

/* Packing pragmas keep a linked stack and a separate reusable-frame list.
 * Identifier values are borrowed opaque identities; no pointee is modeled. */
typedef struct PackingFrame {
    struct PackingFrame *previous;
    unsigned alignment;
    void *identifier;
} PackingFrame;

typedef struct CompilerPacking {
    PackingFrame *top;
    PackingFrame *available;
#ifdef __cplusplus
    PackingFrame *RECON_THISCALL AcquireFrame(void);
    void RECON_THISCALL PopFrame(void);
#endif
} CompilerPacking;

#endif
