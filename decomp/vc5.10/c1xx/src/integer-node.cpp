#include "frontend.h"
#include <recon.h>

/* Known original node fields: kind/flags at0/1, type at4, signed value at16.
 * Original +2..3 and +8..15 stay unmodeled; this compact view has no image
 * padding placeholders. The allocation remains the original 24 bytes. */
struct CompilerIntegerConstant {
    BYTE kind, flags;
    CompilerType *type;
    __int64 value;
};
extern "C" CompilerIntegerConstant *__cdecl allocate_integer_constant(int value);

extern "C" void *__cdecl make_integer_constant(int value)
{
    CompilerIntegerConstant *constant = allocate_integer_constant(value);
    if (!value)
        constant->flags |= 0x80;
    return constant;
}
extern "C" CompilerIntegerConstant *__cdecl allocate_integer_constant(int value)
{
    CompilerArena *arena = &frontend_bss.arenas[frontend_data.constant_arena_index];
    CompilerIntegerConstant *constant = (CompilerIntegerConstant *)arena->cursor;
    arena->cursor += 24;
    if (arena->cursor > arena->committed_limit)
        arena->CommitExtent();
    constant->flags = 3;
    constant->kind = 0x65;
    constant->type = frontend_data.builtin_types.type_0063;
    constant->value = (__int64)value;
    return constant;
}
