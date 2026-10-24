#ifndef C1_PACKING_H
#define C1_PACKING_H
#include <recon.h>

/* Encountered pragma-pack stack. The tag is an opaque identity here; its
 * pointed-to representation is not consumed by the initializer. */
typedef struct CompilerPackingFrame {
    struct CompilerPackingFrame *previous;
    unsigned alignment;
    const void *tag;
} CompilerPackingFrame;
typedef struct CompilerPacking {
    CompilerPackingFrame *active;
    CompilerPackingFrame *free_frames;
#ifdef __cplusplus
    CompilerPackingFrame *RECON_THISCALL Push(void);
#endif
} CompilerPacking;
#ifdef __cplusplus
extern "C" {
#endif
void __cdecl initialize_packing(void);
void *__cdecl allocate_packing_memory(unsigned bytes);
void __cdecl push_packing(unsigned alignment, const void *tag);
void __cdecl clear_packing_frame(CompilerPackingFrame *frame);
#ifdef __cplusplus
}
#endif
#endif
