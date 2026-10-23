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
enum { INTRINSIC_SQRT_FLOAT = 92, INTRINSIC_SQRT_DOUBLE = 93,
    INTRINSIC_SIN_FLOAT = 106, INTRINSIC_SIN_DOUBLE = 107,
    INTRINSIC_COS_FLOAT = 110, INTRINSIC_COS_DOUBLE = 111,
    INTRINSIC_ATAN2_FLOAT = 132, INTRINSIC_ATAN2_DOUBLE = 133,
    INTRINSIC_AGGREGATE_COPY = 172, INTRINSIC_AGGREGATE_FILL = 173,
    INTRINSIC_POWER_DOUBLE = 174 };
extern const IntrinsicDescription root_float_intrinsic, root_double_intrinsic;
extern const IntrinsicDescription sine_float_intrinsic, sine_double_intrinsic;
extern const IntrinsicDescription cosine_float_intrinsic, cosine_double_intrinsic;
extern const IntrinsicDescription arctangent2_float_intrinsic, arctangent2_double_intrinsic;
extern const IntrinsicDescription power_double_intrinsic;
extern const IntrinsicDescription aggregate_copy_intrinsic, aggregate_fill_intrinsic;
/* An explicit data frontier, not a substitute implementation for another
   intrinsic. Inline INT3 is the intentional unreconstructed-data trap; keeping
   this dispatch inline introduces no nonoriginal function/call edge. */
#define SELECT_INTRINSIC_DESCRIPTION(id, description) do { \
    switch (id) { \
    case INTRINSIC_SQRT_FLOAT: (description) = &root_float_intrinsic; break; \
    case INTRINSIC_SQRT_DOUBLE: (description) = &root_double_intrinsic; break; \
    case INTRINSIC_SIN_FLOAT: (description) = &sine_float_intrinsic; break; \
    case INTRINSIC_SIN_DOUBLE: (description) = &sine_double_intrinsic; break; \
    case INTRINSIC_COS_FLOAT: (description) = &cosine_float_intrinsic; break; \
    case INTRINSIC_COS_DOUBLE: (description) = &cosine_double_intrinsic; break; \
    case INTRINSIC_ATAN2_FLOAT: (description) = &arctangent2_float_intrinsic; break; \
    case INTRINSIC_ATAN2_DOUBLE: (description) = &arctangent2_double_intrinsic; break; \
    case INTRINSIC_AGGREGATE_COPY: (description) = &aggregate_copy_intrinsic; break; \
    case INTRINSIC_AGGREGATE_FILL: (description) = &aggregate_fill_intrinsic; break; \
    case INTRINSIC_POWER_DOUBLE: (description) = &power_double_intrinsic; break; \
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
void __fastcall set_generated_intrinsic_call(FunctionSymbol *symbol, unsigned short convention, unsigned short result, unsigned short arguments);
#endif
