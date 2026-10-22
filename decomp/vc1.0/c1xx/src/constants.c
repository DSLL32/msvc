/* Expression constants. */
#include "initialization.h"
#include "lexer.h"
#include "types.h"
#include <recon.h>

/* Original 0x2c600..0x2c71f; seven-way dispatch at 0x2c720..0x2c73c.
   Floating payloads are pointers to their separately saved values. */
IrToken *__stdcall new_literal_constant(unsigned char kind, void *value)
{
    ArenaPool *pool;
    IrToken *constant;
    TypeDescription *description;
    unsigned short flags;
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
    constant->kind = kind;
    switch (kind) {
    case 4: flags = 0x0104; goto floating_value;
    case 5: flags = 0x0108; goto floating_value;
    case 6: flags = 0x0128;
floating_value:
        constant->value.integer = *(unsigned *)value;
        break;
    case 7: flags = 0x0064; goto integer_value;
    case 8: flags = 0x0044; goto integer_value;
    case 9: flags = 0x4064; goto integer_value;
    case 10: flags = 0x4044;
integer_value:
        constant->value.integer = *(unsigned *)value;
        if (!constant->value.integer) constant->flags |= 0x80;
        break;
    /* The native default reads an unwritten local flags word. Its callers
       supply the seven literal token kinds above; no invented default type. */
    }
    description = &type_description_stack[++type_description_depth];
    description->flags = flags;
    description->attributes = 0x1000;
    description->related = 0;
    description->field08 = description->field0a = 0;
    constant->type = intern_type(description);
    --type_description_depth;
    return constant;
}

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

/* Original 0x2c7d0..0x2c7e5. Sizes retain the unsigned-long builtin type. */
IrToken *__stdcall make_size_constant(unsigned value)
{
    IrToken *constant = make_integer_constant(value);
    constant->type = type_4064_first;
    return constant;
}
