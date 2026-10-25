#include "compiler-state.h"
#include "packing.h"
#include "case.h"
#include <recon.h>

static int packing_alignment(void)
{
    unsigned storage[6];
    const char tag[] = "owned tag";
    CompilerPackingFrame predecessor = { NULL, 8, tag };
    CompilerPacking stack = { &predecessor, NULL };
    CompilerPackingFrame *first, *second;
    CompilerArena *arena = &c1_bss.arenas[0];

    arena->cursor = (BYTE *)storage;
    arena->committed_limit = (BYTE *)(storage + 6);
    c1_data.options.packing_stack = &stack;
    first = push_packing(0, NULL);
    if (first != (CompilerPackingFrame *)storage || first->previous != &predecessor ||
        first->alignment != 8 || first->tag || stack.active != first ||
        arena->cursor != (BYTE *)(storage + 3)) return 0;
    second = push_packing(4, tag);
    return second == (CompilerPackingFrame *)(storage + 3) &&
        second->previous == first && second->alignment == 4 && second->tag == tag &&
        stack.active == second && stack.free_frames == NULL &&
        arena->cursor == (BYTE *)(storage + 6) && first->alignment == 8 &&
        first->tag == NULL && first->previous == &predecessor &&
        predecessor.previous == NULL && predecessor.alignment == 8 && predecessor.tag == tag;
}

extern int c1_test_input_factory(void);

const ReconTest minimal_tests[2] = {
    { "packing alignment inheritance and explicit override", 1, packing_alignment },
    { "input factory interfaces and recorded-buffer defaults", 1, c1_test_input_factory }
};
