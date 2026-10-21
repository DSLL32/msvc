/* Frontend expression constants. */
#include "initialization.h"

/* Original 0x278f0..0x27907. */
IrToken *__stdcall make_integer_constant(unsigned value)
{
    IrToken *constant = new_integer_constant(value);
    if (!value)
        constant->flags |= 0x80;
    return constant;
}

/* Original 0x278a0..0x278e5. */
IrToken *__stdcall new_integer_constant(unsigned value)
{
    ArenaPool *pool = &compiler_arena.pools[2];
    IrToken *constant;
    if (pool->remaining < (int)sizeof(IrToken))
        refill_arena(&compiler_arena, pool);
    constant = (IrToken *)pool->next;
    pool->remaining -= sizeof(IrToken);
    pool->next += sizeof(IrToken);
    constant->flags = 3;
    constant->kind = 7;
    constant->type = type_0064;
    constant->value.integer = value;
    return constant;
}
