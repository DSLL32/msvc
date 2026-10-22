/* Language-linkage nesting, original 0x3cd30..0x3cd62. */
#include "options.h"

int linkage_depth = -1;
LanguageLinkage linkage_stack[10];

void __stdcall push_linkage(LanguageLinkage linkage)
{
    ++linkage_depth;
    if (linkage_depth == 10)
        fatal_error(45);
    linkage_stack[linkage_depth] = linkage;
}
