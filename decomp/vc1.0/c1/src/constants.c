/* Frontend expression constants. */
#include "initialization.h"
#include "parser.h"
#include "expressions.h"

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

/* Original 0x27780..0x27884, switch table 0x27884..0x278a0. Numeric
   constants have a const-qualified interned type and retain a zero marker. */
IrToken *__stdcall make_constant_token(unsigned char token, void *value)
{
    ArenaPool *pool = &compiler_arena.pools[2];
    IrToken *node;
    TypeDescription *description;
    unsigned short flags; /* Original unspecified for nonnumeric token codes. */
    if (pool->remaining < (int)sizeof(IrToken)) refill_arena(&compiler_arena, pool);
    node = (IrToken *)pool->next;
    pool->remaining -= sizeof(IrToken);
    pool->next += sizeof(IrToken);
    node->flags = 3;
    node->kind = token;
    switch (token) {
    case TOK_FLOAT_CONSTANT: flags = 0x104; node->value.integer = *(unsigned *)value; break;
    case TOK_DOUBLE_CONSTANT: flags = 0x108; node->value.integer = *(unsigned *)value; break;
    case TOK_LONG_DOUBLE_CONSTANT: flags = 0x128; node->value.integer = *(unsigned *)value; break;
    case TOK_INT_CONSTANT: flags = 0x64; goto integer_value;
    case TOK_LONG_CONSTANT: flags = 0x44; goto integer_value;
    case TOK_UINT_CONSTANT: flags = 0x4064; goto integer_value;
    case TOK_ULONG_CONSTANT:
        flags = 0x4044;
integer_value:
        node->value.integer = *(unsigned *)value;
        if (!node->value.integer) node->flags |= 0x80;
        break;
    }
    description = &type_description_stack[++type_description_depth];
    description->flags = flags;
    description->attributes = 0x1000;
    description->related = 0;
    description->field08 = 0;
    description->field0c = 0;
    node->type = intern_type(description);
    --type_description_depth;
    return node;
}

/* Original 0x276c0..0x27777. String lengths include the terminator and
   are measured in bytes; wide strings therefore have half as many elements. */
void __stdcall assign_string_type(IrToken *token)
{
    TypeDescription *description = &type_description_stack[++type_description_depth];
    unsigned count;
    description->flags = 0;
    description->attributes = 0;
    description->related = 0;
    description->field08 = 0;
    description->field0c = 0;
    if (token->auxiliary_flags & 1) description->flags = 0x4042;
    else {
        description->flags = 0x41;
        if (options.extensions_enabled && options.option_J) description->flags = 0x4041;
    }
    description->attributes = 0;
    count = token->length / ((token->auxiliary_flags & 1) ? 2 : 1);
    description->related = intern_type_shape(new_type_shape(2, 1, &count));
    token->type = intern_type(description);
    --type_description_depth;
}

/* Original 0x284f0..0x28521. Parser-owned constants use arena two;
   persistent/scanner modes retain their extended value permanently. */
Extended *__stdcall save_extended_constant(Extended *value)
{
    return allocate_extended_value(*value, scanner_flags & 0x20000020UL ? 0 : 2);
}

/* Original 0x28530..0x28587, value passed as twelve stack bytes. */
Extended *__stdcall allocate_extended_value(Extended value, int arena)
{
    Extended *result;
    ArenaPool *pool;
    if (!arena) result = allocate_permanent(sizeof(Extended));
    else {
        if (arena == -1) arena = 0;
        pool = &compiler_arena.pools[arena];
        if (pool->remaining < (int)sizeof(Extended)) refill_arena(&compiler_arena, pool);
        result = (Extended *)pool->next;
        pool->remaining -= sizeof(Extended);
        pool->next += sizeof(Extended);
    }
    *result = value;
    return result;
}

/* Original 0x4c260..0x4c2ab. Only subtraction of opposite signed
   operands can overflow; retain the common addition range checker. */
int __stdcall subtraction_overflows(unsigned left, unsigned right,
    unsigned short is_unsigned, unsigned short bytes)
{
    if (is_unsigned) return 0;
    if (((int)left >= 0) != ((int)right >= 0))
        return addition_overflows(left, 0U - right, 0, bytes);
    return 0;
}

/* Original 0x2d250..0x2d298. Enumerators become ordinary signed int
   constants, with their value stored in the enum-specific symbol prefix. */
IrToken *__stdcall make_enumerator_constant(EnumeratorSymbol *symbol)
{
    ArenaPool *pool = &compiler_arena.pools[2];
    IrToken *constant;
    if (pool->remaining < (int)sizeof(IrToken)) refill_arena(&compiler_arena, pool);
    constant = (IrToken *)pool->next;
    pool->remaining -= sizeof(IrToken);
    pool->next += sizeof(IrToken);
    constant->flags = 3;
    constant->kind = TOK_INT_CONSTANT;
    constant->value.integer = symbol->value;
    constant->type = type_0064;
    return constant;
}
