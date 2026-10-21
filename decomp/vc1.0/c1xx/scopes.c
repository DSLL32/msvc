/* Parser scope context stack, original 0x3cd30..0x3cd62. */
#include "options.h"

int scope_depth = -1;
Scope *scope_contexts[10];

void __stdcall push_scope(Scope *scope)
{
    ++scope_depth;
    if (scope_depth == 10)
        fatal_error(45);
    scope_contexts[scope_depth] = scope;
}
