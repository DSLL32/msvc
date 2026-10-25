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

extern void __cdecl normalize_frontend_options(void);
static int processor_instruction_mask(void)
{
    c1_bss.processor_instruction_mask = 0xffffffff;
    c1_data.options.processor = 0;
    c1_data.options.minimal_rebuild = 0;
    normalize_frontend_options();
    if (c1_bss.processor_instruction_mask != 1) return 0;
    c1_data.options.processor = 4;
    c1_data.options.minimal_rebuild = 1;
    normalize_frontend_options();
    return c1_bss.processor_instruction_mask == 0x50;
}
extern int c1_test_browser_factory(void);
extern int c1_test_backend_getter(void);
const ReconTest minimal_tests[5] = {
    { "packing alignment inheritance and explicit override", 1, packing_alignment },
    { "input factory interfaces and recorded-buffer defaults", 1, c1_test_input_factory },
    { "processor mask replaces prior value and adds compatibility bit", 1, processor_instruction_mask },
    { "browser factory shared slots and owned filename prefix", 1, c1_test_browser_factory },
    { "backend getter preserves existing identity and lazily constructs", 1, c1_test_backend_getter }
};
