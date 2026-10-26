/* Deliberately unsupported target forms retained in this NT backend. These
   are App assertions, not CRT implementations or added language support. */
#include "tree-rewrite.h"
#include "algebra.h"
#include <recon.h>
/* Original85308 points to this shared stub-module signature. */
static char *target_stub_source = "@(#)stub.c:1.23";
/* Original6f5e0..6f5f9 (25 bytes), unused ECX node, RET0. */
int __fastcall lower_far_pointer_cast(Node *node)
{
    (void)node;
    compiler_internal_error(target_stub_source, 0x3b);
    return 0;
}
/* Original6f600..6f617 (23 bytes), ECX node, nominal EAX same node, RET0.
   Preserve the original result if the fatal diagnostic were ever to return. */
Node *__fastcall lower_target_address(Node *node)
{
    compiler_internal_error(target_stub_source, 0x42);
    return node;
}
