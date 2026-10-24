/* Structured-exception scope discovery and expression preparation. */
#include "exception-scope.h"
#include <recon.h>
ExceptionScope *prepared_exception_scopes;
Node *exception_return_label;
unsigned exception_finally_depth, next_exception_scope_identifier;

/* Original [000656e0,00065725). The established provisional symbol name
   predates recovery of this SEH path. The neighboring routines are separate
   functions, not part of the former 1616-byte analysis envelope. */
void __fastcall prepare_inline_function(Function *function)
{
    next_exception_scope_identifier = 0;
    exception_finally_depth = 0;
    prepared_exception_scopes = 0;
    exception_return_label = 0;
    reset_exception_scope_stack();
    collect_exception_scopes(function);
    reset_exception_scope_stack();
    rewrite_exception_expressions(function);
}
