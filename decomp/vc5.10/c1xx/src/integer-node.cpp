#include "frontend.h"
#include "ir.h"
#include <recon.h>

extern "C" CompilerExpression *__cdecl allocate_integer_constant(int value);

extern "C" void *__cdecl make_integer_constant(int value)
{
    CompilerExpression *constant = allocate_integer_constant(value);
    if (!value)
        constant->flags |= 0x80;
    return constant;
}
extern "C" CompilerExpression *__cdecl allocate_integer_constant(int value)
{
    CompilerArena *arena = &frontend_bss.arenas[frontend_data.constant_arena_index];
    CompilerExpression *constant = (CompilerExpression *)arena->cursor;
    arena->cursor += 24;
    if (arena->cursor > arena->committed_limit)
        arena->CommitExtent();
    constant->flags = 3;
    constant->kind = 0x65;
    constant->type = frontend_data.builtin_types.type_0063;
    constant->value.integer = (__int64)value;
    return constant;
}
