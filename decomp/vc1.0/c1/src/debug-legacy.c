/* Embedded CodeView type records and their deferred emission queue. */
#include <stdlib.h>
#include "debug-info.h"
#include "parser.h"
#include "codeview-types.h"
#include <recon.h>

/* Original0x47790..0x477fc. Tags use the shared symbol queue; the
   unadorned scalar case delegates to the legacy primitive-code selector. */
unsigned short __stdcall primitive_debug_index(CType *type)
{
    unsigned short flags = type->flags;
    if (flags & 0x600)
        return debug_symbol_type_index((FrontendSymbol *)type->field08);
    if (flags & 0x80) return 3;
    if (flags & 0x800) return 2;
    if (!flags) return 0;
    if (flags == 0x802) return 2;
    return legacy_primitive_type_index(flags);
}

/* Legacy scalar tables are independent objects from the database tables.
   Widths are indexed in bytes; the signed integer table ends at width9. */
unsigned short legacy_floating_codes[12] = {
    0, 0, 0, 0, CV_REAL32, 0, 0, 0, CV_REAL64, 0, CV_REAL80, 0
};
unsigned short legacy_natural_unsigned_codes[12] = {
    0, CV_RCHAR, CV_UINT2, 0, CV_UINT4, 0, 0, 0, CV_UINT8, 0, 0, 0
};
unsigned short legacy_natural_signed_codes[12] = {
    0, CV_RCHAR, CV_INT2, 0, CV_INT4, 0, 0, 0, CV_INT8, 0, 0, 0
};
unsigned short legacy_explicit_unsigned_codes[12] = {
    0, CV_UCHAR, CV_USHORT, 0, CV_ULONG, 0, 0, 0, CV_UQUAD, 0, 0, 0
};
unsigned short legacy_explicit_signed_codes[10] = {
    0, CV_CHAR, CV_SHORT, 0, CV_LONG, 0, 0, 0, CV_QUAD, 0
};

/* Original0x476f0..0x47786. Keep the original selector as a separate
   graph vertex even though its type-code rules parallel the PDB selector. */
unsigned short __stdcall legacy_primitive_type_index(unsigned short flags)
{
    unsigned width;
    if (flags & 0x80) return 3;
    if (!(flags & 0x140)) return 0;
    width = flags & 15;
    if (flags & 0x100) return legacy_floating_codes[width];
    if (((flags & 0x20) && (flags & 0x40)) || (width == 1 && !(flags & 0xc000)))
        return (flags & 0x4000) ? legacy_natural_unsigned_codes[width] : legacy_natural_signed_codes[width];
    return (flags & 0x4000) ? legacy_explicit_unsigned_codes[width] : legacy_explicit_signed_codes[width];
}

/* Original0x47630..0x476e2. Primitive pointers encode their address
   model directly; compound and based pointer shapes require a record. */
unsigned short __stdcall primitive_pointer_debug_index(CType *type)
{
    TypeShape *shape = type->related;
    unsigned short index, model;
    if (shape->related || (shape->flags & 7) != 1 || (shape->attributes & 12))
        return 0;
    index = legacy_primitive_type_index(type->flags);
    if (!index) return 0;
    model = shape->attributes & 15;
    switch (model) {
    case 1: return (index & 0xfcff) | 0x400;
    case 2: return (index & 0xfdff) | 0x500;
    case 3: return (index & 0xfbff) | 0x300;
    default: return (index & 0xf8ff) | ((options.option_M & 2) ? 0x500 : 0x400);
    }
}

unsigned debug_type_record_capacity; /* original0x6e07c */

/* Original0x479e0..0x47a79. Reserve the next type index before growing
   the twelve-byte record vector; callers fill the selected payload fields. */
unsigned __stdcall queue_debug_record(unsigned char kind)
{
    unsigned index = debug_type_record_count++;
    ++next_debug_type_index;
    if (!debug_type_record_capacity) {
        debug_type_record_capacity = 128;
        debug_type_records = malloc(128 * sizeof(DebugTypeRecord));
        if (!debug_type_records) fatal_error(60);
    } else if (debug_type_record_count > debug_type_record_capacity) {
        debug_type_record_capacity += (debug_type_record_capacity >> 2) + 1;
        debug_type_records = realloc(debug_type_records,
            debug_type_record_capacity * sizeof(DebugTypeRecord));
        if (!debug_type_records) fatal_error(60);
    }
    debug_type_records[index].kind = kind;
    return index;
}

/* Original0x48800..0x488d4. Intern an arena-owned unqualified
   descriptor before writing the modifier record for const/volatile. */
void __stdcall write_legacy_qualified_type(CType *type)
{
    unsigned short *record = (unsigned short *)debug_record_buffer + 1;
    ArenaPool *pool = &compiler_arena.pools[1];
    CType *description, *unqualified;
    unsigned short attributes;
    if (pool->remaining < (int)sizeof(CType)) refill_arena(&compiler_arena,pool);
    description = (CType *)pool->next;
    pool->next += sizeof(CType);
    pool->remaining -= sizeof(CType);
    *description = *type;
    description->attributes &= ~0x3000;
    *(unsigned short *)&description->field0c = 0;
    unqualified = intern_type(description);
    if (!(unsigned short)unqualified->field0c) emit_debug_type(unqualified);
    record[0] = 1;
    attributes = legacy_tag_auxiliary;
    record[1] = attributes;
    attributes = (attributes & ~1) | ((type->attributes & 0x1000) ? 1 : 0);
    record[1] = attributes;
    record[1] = (attributes & ~2) | ((type->attributes & 0x2000) ? 2 : 0);
    record[2] = (unsigned short)unqualified->field0c;
    write_legacy_type_record(6);
}

/* Original0x47800..0x4784e. Argument types reserve their list index
   before the function's index; the queue stores the byte argument count. */
void __stdcall emit_function_debug_type(CType *type)
{
    unsigned char count;
    unsigned record;
    count = prepare_legacy_argument_types((ParameterInfo *)type->related->field08);
    *(unsigned short *)&type->field0c = next_debug_type_index;
    record = queue_debug_record(0);
    debug_type_records[record].value.type = type;
    debug_type_records[record].index = count;
}

/* Original0x47850..0x478b3. The legacy parameter-list count is a
   byte, and its assigned list index occupies only the auxiliary low word. */
unsigned char __stdcall prepare_legacy_argument_types(ParameterInfo *parameters)
{
    unsigned char count = 0;
    unsigned index, record;
    if (parameters->types[0] != type_0080) count = (unsigned char)parameters->count;
    if (!(unsigned short)parameters->auxiliary) {
        for (index = 0; index < count; ++index)
            if (parameters->types[index]) emit_debug_type(parameters->types[index]);
        *(unsigned short *)&parameters->auxiliary = next_debug_type_index;
        record = queue_debug_record(3);
        debug_type_records[record].value.pointer = parameters;
    }
    return count;
}
