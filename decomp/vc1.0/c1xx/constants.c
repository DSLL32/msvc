/* Expression constants. */
#include "initialization.h"
#include "lexer.h"

/* Original 0x2c7b0..0x2c7c7. */
IrToken *__stdcall make_integer_constant(unsigned value)
{
    IrToken *constant = new_integer_constant(value);
    if (!value)
        constant->flags |= 0x80;
    return constant;
}

/* Original 0x2c740..0x2c7a1. Persistent tokens use the permanent arena. */
IrToken *__stdcall new_integer_constant(unsigned value)
{
    ArenaPool *pool;
    IrToken *constant;
    if (token_allocation_flags & 0x20000020)
        constant = allocate_permanent(sizeof(IrToken));
    else {
        pool = &compiler_arena.pools[2];
        if (pool->remaining < (int)sizeof(IrToken))
            refill_arena(&compiler_arena, pool);
        constant = (IrToken *)pool->next;
        pool->remaining -= sizeof(IrToken);
        pool->next += sizeof(IrToken);
    }
    constant->flags = 3;
    constant->kind = 7;
    constant->type = type_0064;
    constant->value.integer = value;
    return constant;
}

/* Original 0x2c7f0..0x2c826: a storage reference to an integer-valued
   constant symbol becomes an ordinary literal with that symbol's type. */
IrToken *__stdcall fold_symbol_constant(IrToken *expression)
{
    IrToken *reference, *constant;
    CppSymbol *symbol;
    if (token_categories[expression->kind] == 0x46) {
        reference = expression->value.pointer;
        if (reference->kind == 2) {
            symbol = reference->value.pointer;
            if (symbol->attributes & 0x4000) {
                constant = make_integer_constant(symbol->auxiliary.value);
                constant->type = symbol->type;
                return constant;
            }
        }
    }
    return expression;
}
