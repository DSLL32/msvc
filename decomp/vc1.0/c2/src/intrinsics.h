#ifndef INTRINSICS_H
#define INTRINSICS_H
#include "tree-rewrite.h"
#include <recon.h>
/* Original intrinsic entries are16 bytes. Only encountered descriptors are
   defined; the interpreter never exposes their address or computes between
   descriptor addresses. Numeric IDs remain the frontend's serialized IDs. */
typedef struct IntrinsicDescription {
    /* Default scalar argument normalization, not the call's result type:
       setjmp is the concrete pointer404 argument/int104 result distinction. */
    unsigned short argument_type, reserved;
    const char *commands, *runtime_name;
    unsigned flags;
} IntrinsicDescription;
typedef char IntrinsicDescriptionSizeCheck[sizeof(IntrinsicDescription) == 16 ? 1 : -1];
extern Node *current_intrinsic;
void __fastcall prepare_intrinsic_arguments(Node *node, unsigned intrinsic, Node **arguments);
void __fastcall append_intrinsic_expression(Node *expression);
void __fastcall release_intrinsic_operands(Node *node);
unsigned __fastcall intrinsic_operator(unsigned intrinsic);
unsigned short __fastcall intrinsic_argument_width(Node *node, int skip_first);
const unsigned *__fastcall intrinsic_argument_types(unsigned intrinsic);
void __fastcall set_generated_intrinsic_call(FunctionSymbol *symbol, unsigned short convention, unsigned short result, unsigned short arguments);
#endif
