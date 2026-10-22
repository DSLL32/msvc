#ifndef TREE_SCRATCH_H
#define TREE_SCRATCH_H
#include "functions.h"
/* Original [0007e820,0007e844). The typer permits a push at its limit,
   which aliases the low word of the cleanup cursor at 0007e840. */
typedef union TreeScratch {
    struct {
        unsigned short types[16];
        Expression **cleanup_link;
    } phase;
    unsigned short type_stack_with_limit[18];
} TreeScratch;
typedef char TreeScratchSizeCheck[sizeof(TreeScratch) == 36 ? 1 : -1];
extern TreeScratch tree_scratch;
#define expression_cleanup_link tree_scratch.phase.cleanup_link
#endif
