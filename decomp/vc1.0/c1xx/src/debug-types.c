/* CodeView type references and deferred type records. */
#include "debug-types.h"
#include "symbol-output.h"
#include "cpp-context.h"
#include <recon.h>

DebugTypeRecord *debug_types; /* original 0x8ccb0 */
unsigned short next_debug_type_index = 0x1000; /* original 0x9f454 */

/* CodeView predefined types indexed by the frontend's byte-width field.
   Keep the encountered logical rows, without neighbouring alignment bytes
   or the ordinary string following the final integral table. */
unsigned short cv_real_types[11] = {
    0, 0, 0, 0, 0x40, 0, 0, 0, 0x41, 0, 0x42
}; /* original 0xa25c8 */
unsigned short cv_unsigned_int_types[9] = {
    0, 0x70, 0x73, 0, 0x75, 0, 0, 0, 0x77
}; /* original 0xa25e0 */
unsigned short cv_signed_int_types[9] = {
    0, 0x70, 0x72, 0, 0x74, 0, 0, 0, 0x76
}; /* original 0xa25f8 */
unsigned short cv_unsigned_size_types[9] = {
    0, 0x20, 0x21, 0, 0x22, 0, 0, 0, 0x23
}; /* original 0xa2610 */
unsigned short cv_signed_size_types[9] = {
    0, 0x10, 0x11, 0, 0x12, 0, 0, 0, 0x13
}; /* original 0xa2628 */

/* Original 0x6d920..0x6d9b7. Integer spellings distinguish the int
   family from width-specific scalar types, including plain char. */
unsigned short __stdcall primitive_debug_type(unsigned short flags)
{
    unsigned width;
    if (flags & 0x80) return 3;
    if (!(flags & 0x140)) return 0;
    width = flags & 15;
#ifdef RECON_NATIVE_TEST
    /* Internal widths outside the recovered logical tables have no native
       support. This does not classify their public reachability or claim
       an original bug/equivalence for neighbouring out-of-domain reads. */
    if (width >= ((flags & 0x100) ? 11 : 9)) { __asm { int 3 } }
#endif
    if (flags & 0x100) return cv_real_types[width];
    if ((flags & 0x60) == 0x60 || (width == 1 && !(flags & 0xc000))) {
        if (flags & 0x4000) return cv_unsigned_int_types[width];
        return cv_signed_int_types[width];
    }
    if (flags & 0x4000) return cv_unsigned_size_types[width];
    return cv_signed_size_types[width];
}

/* Original 0x6d9c0..0x6da4d. Tags retain their declaration identity;
   unshaped builtin types can use CodeView's predefined type indices. */
unsigned short __stdcall base_debug_type(CType *type)
{
    unsigned short flags = type->flags;
    if (flags & 0x200)
        return debug_tag_type(cpp_context_stack->items[type->field08]);
    if (flags & 0x400)
        return debug_tag_type(cpp_context_stack->items[type->field08]);
    if (flags & 0x80) return 3;
    if (flags & 0x800) return 2;
    if (!flags) return 0;
    if (flags == 0x802) return 2;
    return primitive_debug_type(flags);
}

/* Original 0x6d6e0..0x6d856. Reserve a type's index before queuing its
   record, recursively referring to the type beneath its first shape. */
void __stdcall reference_debug_type(CType *type)
{
    TypeShape *shape;
    CType description, *base;
    unsigned record;
    if (options.configuration.line_records) {
        reference_line_record_type(type);
        return;
    }
    /* The original reads related before its late null test. */
    shape = type->related;
    if (options.prototypes_only) return;
    if (!type) {
        type->debug_index = 0;
        return;
    }
    if (type->debug_index) return;
    if (shape) {
        type->debug_index = primitive_pointer_debug_type(type);
        if (type->debug_index) return;
        description = *type;
        description.related = shape->related;
        base = intern_type((TypeDescription *)&description);
        reference_debug_type(base);
        switch (shape->flags & 0x77) {
        case 0:
            type->debug_index = base->debug_index;
            return;
        case 1: case 2: case 3: case 0x10: case 0x20: case 0x40:
            type->debug_index = next_debug_type_index;
            record = reserve_debug_type(0);
            debug_types[record].subject.type = type;
            debug_types[record].related_type = base->debug_index;
            return;
        case 4:
            reference_function_debug_type(type);
            return;
        }
        return;
    }
    if (type->attributes & 0x3000) {
        type->debug_index = next_debug_type_index;
        record = reserve_debug_type(0);
        debug_types[record].subject.type = type;
        return;
    }
    type->debug_index = base_debug_type(type);
}
