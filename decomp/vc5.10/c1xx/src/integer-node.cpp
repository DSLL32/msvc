#include "frontend.h"
#include <recon.h>

/* Only the encountered node discriminator bytes so far. The actual builder
 * remains pending before any other member can be consumed. */
struct CompilerIntegerConstant {
    BYTE kind, flags;
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
    __asm { int 3 }
    return NULL;
}
