#include "hash-table.h"
/* C++ context stack starts with a null sentinel, original 0x2e620..0x2e655. */
#include "cpp-context.h"

InlineVector *cpp_context_stack; /* original 0x9f32c */

void initialize_cpp_context_stack(void)
{
    InlineVector *stack = new_inline_vector(10);
    cpp_context_stack = stack;
    if (stack->count == stack->capacity)
        stack = grow_inline_vector(stack);
    stack->items[stack->count++] = 0;
    cpp_context_stack = stack;
}

/* A lazily created mapping between additional member contexts. */
HashTable *member_context_links; /* original 0x9f34c */

/* Original 0x54550..0x54575. No mapping means the context chain ends. */
struct CppSymbol *__stdcall next_member_context(struct CppSymbol *context)
{
    if (member_context_links)
        return find_hash_value(member_context_links, context);
    return 0;
}
