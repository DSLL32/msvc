#include "frontend.h"
#include "pointer-array.h"
#include <recon.h>

extern "C" unsigned __cdecl pointer_array_size(unsigned capacity);

extern "C" CompilerPointerArray *__cdecl new_pointer_array(unsigned capacity)
{
    unsigned size = pointer_array_size(capacity);
    CompilerArena *arena = &frontend_bss.arenas[0];
    CompilerPointerArray *array = (CompilerPointerArray *)arena->cursor;
    arena->cursor += size;
    if (arena->cursor > arena->committed_limit)
        arena->CommitExtent();
    array->count = 0;
    array->capacity = (short)capacity;
    if (!frontend_data.empty_pointer_array) {
        /* The original uses pointer value one solely as an initialization
         * guard while recursively creating the zero-capacity header. */
        frontend_data.empty_pointer_array = (CompilerPointerArray *)1;
        frontend_data.empty_pointer_array = new_pointer_array(0);
    }
    return array;
}
extern "C" CompilerPointerArray *__cdecl grow_pointer_array(CompilerPointerArray *array) { __asm { int 3 } return NULL; }
extern "C" unsigned __cdecl pointer_array_size(unsigned capacity)
{
    return (unsigned)(short)capacity * 4U + 4U;
}
