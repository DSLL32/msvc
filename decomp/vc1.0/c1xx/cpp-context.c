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
