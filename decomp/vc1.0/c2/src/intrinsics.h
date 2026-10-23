#ifndef INTRINSICS_H
#define INTRINSICS_H
#include "tree-rewrite.h"
#include <recon.h>
/* Original intrinsic entries are16 bytes. Only encountered descriptors are
   defined; the interpreter never exposes their address or computes between
   descriptor addresses. Numeric IDs remain the frontend's serialized IDs. */
typedef struct IntrinsicDescription {
    unsigned short result_type, reserved;
    const char *commands, *runtime_name;
    unsigned flags;
} IntrinsicDescription;
typedef char IntrinsicDescriptionSizeCheck[sizeof(IntrinsicDescription) == 16 ? 1 : -1];
enum { INTRINSIC_AGGREGATE_FILL = 173 };
extern const IntrinsicDescription aggregate_fill_intrinsic;
/* An explicit data frontier, not a substitute implementation for another
   intrinsic. Inline INT3 is the intentional unreconstructed-data trap; keeping
   this dispatch inline introduces no nonoriginal function/call edge. */
#define SELECT_INTRINSIC_DESCRIPTION(id, description) do { \
    switch (id) { \
    case INTRINSIC_AGGREGATE_FILL: (description) = &aggregate_fill_intrinsic; break; \
    default: __asm { int 3 } \
        (description) = 0; break; \
    } \
} while (0)
extern Node *current_intrinsic;
void __fastcall prepare_intrinsic_arguments(Node *node, unsigned intrinsic, Node **arguments);
void __fastcall append_intrinsic_expression(Node *expression);
void __fastcall release_intrinsic_operands(Node *node);
unsigned __fastcall intrinsic_operator(unsigned intrinsic);
unsigned short __fastcall intrinsic_argument_width(Node *node, int skip_first);
const unsigned *__fastcall intrinsic_argument_types(unsigned intrinsic);
#endif
