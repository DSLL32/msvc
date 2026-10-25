#ifndef C1XX_POINTER_ARRAY_H
#define C1XX_POINTER_ARRAY_H
#include <recon.h>

/* Count/capacity are signed SHORTs; the allocation carries inline pointers.
 * The element domain is still opaque until an actual consumer is reached. */
struct CompilerPointerArray {
    short count, capacity;
    void *items[1];
};
extern "C" CompilerPointerArray *__cdecl new_pointer_array(unsigned capacity);
extern "C" CompilerPointerArray *__cdecl grow_pointer_array(CompilerPointerArray *array);
#endif
