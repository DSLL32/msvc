#ifndef EXCEPTION_SCOPE_H
#define EXCEPTION_SCOPE_H
#include "flow-edges.h"
#include <recon.h>
/* Per-function structured-exception preparation state. These four owners
   are independently addressed in the original, not an indexed image view. */
extern ExceptionScope *prepared_exception_scopes; /* 0007b998 */
extern Node *exception_return_label; /* 0007b99c */
extern unsigned exception_finally_depth; /* 0007b9a0 */
extern unsigned next_exception_scope_identifier; /* 0007b9a4 */
void __cdecl reset_exception_scope_stack(void);
void __fastcall collect_exception_scopes(Function *function);
void __fastcall rewrite_exception_expressions(Function *function);
#endif
