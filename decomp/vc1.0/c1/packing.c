/* Structure packing state, original initialize_packing at 0x1e4e0. */
#include "options.h"

void initialize_packing(void)
{
    PackingEntry *entry;
    packing_stack = new_packing_stack();
    entry = push_packing(packing_stack);
    if (options.option_Zp == -1)
        entry->alignment = 8;
    else
        entry->alignment = options.option_Zp;
    entry->field08 = 0;
}

/* Original 0x1e4c0..0x1e4d5. */
PackingStack *new_packing_stack(void)
{
    PackingStack *stack = allocate_permanent(sizeof(PackingStack));
    stack->active = 0;
    stack->free = 0;
    return stack;
}

/* Original 0x1e480..0x1e4ba. */
PackingEntry *push_packing(PackingStack *stack)
{
    PackingEntry *entry = stack->free;
    if (!entry)
        entry = allocate_permanent(sizeof(PackingEntry));
    else {
        stack->free = entry->next;
        clear_packing(entry);
    }
    entry->next = stack->active;
    stack->active = entry;
    return entry;
}

/* Original 0x1e450..0x1e457. */
PackingEntry *top_packing(PackingStack *stack)
{
    return stack->active;
}
