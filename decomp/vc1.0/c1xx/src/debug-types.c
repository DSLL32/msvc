/* CodeView type references and deferred type records. */
#include "debug-types.h"
#include "symbol-output.h"
#include <recon.h>

DebugTypeRecord *debug_types; /* original 0x8ccb0 */
unsigned short next_debug_type_index = 0x1000; /* original 0x9f454 */

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
