/* Structure packing state, original initialize_packing at 0x1e4e0. */
#include "options.h"
#include <recon.h>

void initialize_packing(void)
{
    PackingEntry *entry;
    packing_stack = new_packing_stack();
    entry = push_packing(packing_stack);
    if (options.option_Zp == -1)
        entry->alignment = 8;
    else
        entry->alignment = options.option_Zp;
    entry->label = 0;
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

/* Original 0x2a290..0x2a2f9. Padding uses the smaller of natural
   alignment and the active #pragma pack limit. */
unsigned __stdcall packing_padding(unsigned alignment, unsigned offset)
{
    unsigned effective = alignment, remainder;
    if ((unsigned)top_packing(packing_stack)->alignment < alignment)
        effective = top_packing(packing_stack)->alignment;
    remainder = offset % effective;
    if (remainder) {
        if ((unsigned)top_packing(packing_stack)->alignment < alignment)
            alignment = top_packing(packing_stack)->alignment;
        return alignment - remainder;
    }
    return 0;
}

/* Original 0x1e870..0x1e8ad. Invalid packing values retain the current
   alignment after their warning. */
int __stdcall validate_packing(int alignment)
{
    switch (alignment) {
    case 1: case 2: case 4: case 8: case 16: return alignment;
    }
    compiler_warning(1, 86);
    return top_packing(packing_stack)->alignment;
}

/* Original0x1e8b0..0x1e8e5. Named pushes inherit the current alignment
   when no replacement value was supplied. Labels are interned pointer keys. */
void __stdcall push_pragma_packing(int alignment, char *label)
{
    PackingEntry *entry;
    if (!alignment) alignment = top_packing(packing_stack)->alignment;
    entry = push_packing(packing_stack);
    entry->alignment = alignment;
    entry->label = label;
}

/* Original0x1e8f0..0x1e9d8. Named pops search the legacy entry/free-list
   linkage and retain its warning/default-alignment recovery behavior. */
void __stdcall pop_pragma_packing(int alignment, char *label)
{
    PackingEntry *entry = top_packing(packing_stack);
    if (!entry->next) {
        compiler_warning(3,161);
        if (!alignment) alignment = options.option_Zp == -1 ? 8 : options.option_Zp;
    } else {
        pop_packing(packing_stack);
        while (entry->label != label) {
            if (!label) { compiler_warning(3,159,entry->label); break; }
            if (!entry->next) {
                compiler_warning(3,160,label);
                if (!alignment) alignment = options.option_Zp == -1 ? 8 : options.option_Zp;
                break;
            }
            entry = top_packing(packing_stack);
            pop_packing(packing_stack);
        }
    }
    if (alignment) entry->alignment = alignment;
}

/* Original0x1e460..0x1e47a. Retired entries form a reusable free list. */
void pop_packing(PackingStack *stack)
{
    PackingEntry *entry;
    if (stack && stack->active) {
        entry = stack->active;
        stack->active = entry->next;
        entry->next = stack->free;
        stack->free = entry;
    }
}
