/* Embedded CodeView type records and their deferred emission queue. */
#include <stdlib.h>
#include "debug-info.h"
#include "frontend.h"
#include "parser.h"
#include "codeview-types.h"
#include "literal-names.h"
#include "hash-table.h"
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

LegacyTypeFixup *legacy_type_fixups; /* original0x6e070 */

/* Original0x488e0..0x489f1. Pad and length-prefix the type, retaining
   symbolic relocation records between any raw byte spans. */
void __stdcall write_legacy_type_record(int bytes)
{
    unsigned char *base = (unsigned char *)debug_record_buffer;
    unsigned char *cursor, *tail;
    unsigned padding, remaining;
    unsigned char pad;
    int length, gap;
    if (bytes > 4096) fatal_error(1,pch_diagnostic_name,2626);
    padding = -(bytes + 2) & 3;
    tail = base + bytes + 2;
    pad = (unsigned char)(padding - 16);
    for (remaining = padding; remaining; --remaining) *tail++ = pad--;
    length = bytes + padding;
    *(unsigned short *)debug_record_buffer = (unsigned short)length;
    if (pch_separate_debug)
        pragma_record_number = checksum_bytes((unsigned char *)debug_record_buffer,
            3,pragma_record_number);
    cursor = base;
    if (!legacy_type_fixups) write_debug_record(11,debug_record_buffer);
    else {
        do {
            gap = legacy_type_fixups->offset - (cursor - (unsigned char *)debug_record_buffer);
            if (gap) write_debug_record(3,gap,cursor);
            write_debug_record(2,legacy_type_fixups->symbol,legacy_type_fixups->bytes);
            cursor += gap + legacy_type_fixups->bytes;
            legacy_type_fixups = legacy_type_fixups->next;
        } while (legacy_type_fixups);
        gap = (unsigned char *)debug_record_buffer + length + 2 - cursor;
        if (gap) write_debug_record(3,gap,cursor);
    }
    debug_type_bytes += length + 2;
}

/* Pointer encodings indexed by the frontend's four ordinary models. */
unsigned char legacy_pointer_models[4] = { 0, 10, 11, 2 }; /* original0x6e078 */

/* Original0x47d20..0x47ee5. Ordinary models fit the pointer attributes;
   based models append an embedded symbol or a symbolic IR relocation. */
void __stdcall write_legacy_pointer_type(CType *type, unsigned short nested)
{
    unsigned short *record = (unsigned short *)debug_record_buffer + 1;
    TypeShape *shape = type->related;
    unsigned short attributes = shape->attributes;
    unsigned model, kind, bytes = 0;
    FrontendSymbol *based;
    record[0] = 2;
    record[1] = 0;
    record[1] &= ~31;
    record[2] = nested;
    if (attributes & 0x1000) record[1] |= 0x400;
    if (attributes & 0x2000) record[1] |= 0x200;
    if (!(attributes & 12)) {
        model = attributes & 15;
        record[1] = (record[1] & ~31) | (legacy_pointer_models[model] & 31);
        kind = shape->flags & 7;
        if (!model && kind) {
            if (kind < 3)
                record[1] = (record[1] & ~31) | ((options.option_M & 2) ? 11 : 10);
            else if (kind <= 4)
                record[1] = (record[1] & ~31) | ((options.option_M & 1) ? 11 : 10);
        }
    } else if (attributes & 4) {
        based = (FrontendSymbol *)shape->field08;
        switch (attributes & 7) {
        case 4:
            record[1] = (record[1] & ~31) | 8;
            record[3] = 3;
            *((unsigned char *)(record + 3) + 2) = 0;
            bytes = 3;
            break;
        case 5:
            if ((based->kind & 31) == 13) {
                if (based->index == 6) {
                    record[1] = (record[1] & ~31) | 5;
                    record[3] = 7;
                    record[4] = 2;
                    record[5] = 0;
                    record[6] = 27;
                    *((unsigned char *)(record + 3) + 8) = 0;
                    bytes = 9;
                } else {
                    record[1] = (record[1] & ~31) | 3;
                    bytes = 4;
                    add_legacy_type_fixup(based,(unsigned char *)(record + 3),4);
                }
            } else {
                record[1] = (record[1] & ~31) | 4;
                bytes = serialize_legacy_symbol((unsigned char *)(record + 3),based);
            }
            break;
        case 6:
            record[1] = (record[1] & ~31) | 7;
            bytes = serialize_legacy_symbol((unsigned char *)(record + 3),based);
            break;
        case 7:
            record[1] = (record[1] & ~31) | 5;
            bytes = serialize_legacy_symbol((unsigned char *)(record + 3),based);
            break;
        }
    } else if (attributes & 8) record[1] = (record[1] & ~31) | 9;
    write_legacy_type_record(bytes + 6);
}

/* Original0x47f40..0x48273. The member count is known before the
   field list is emitted, so its index is reserved in a later queue record. */
void __stdcall write_legacy_record_type(FrontendSymbol *tag,
    unsigned short fields, unsigned short auxiliary)
{
    unsigned short *record = (unsigned short *)debug_record_buffer + 1;
    unsigned short flags = tag->type->flags;
    unsigned queued, bytes, count;
    int length;
    char *name_start, *name_end;
    HashWalk walk;
    EnumeratorSymbol **enumerators;
    record[1] = fields;
    if (flags == 0x402) {
        record[0] = 5;
        record[5] = 0;
        record[3] = auxiliary;
        if (!(tag->flags & 1)) {
            record[2] = 0;
            record[3] |= 0x80;
        } else {
            record[3] = (record[3] & ~1) | (top_packing(packing_stack)->alignment == 1);
            record[2] = next_debug_type_index;
            queued = queue_debug_record(2);
            debug_type_records[queued].value.symbol = tag;
            debug_type_records[queued].index = 0;
        }
        record[4] = 0;
        record[3] = (record[3] & ~8) | ((tag->metadata.tag.flags & 2) ? 8 : 0);
        bytes = write_legacy_unsigned_number((unsigned char *)(record + 6),tag->detail.value);
        name_start = (char *)(record + 6) + bytes;
        name_end = append_codeview_name(name_start,tag->name);
        length = 12 + bytes + (name_end - name_start);
    } else if (flags == 0x404) {
        record[0] = 6;
        record[3] = auxiliary;
        if (!(tag->flags & 1)) {
            record[2] = 0;
            record[3] |= 0x80;
        } else {
            record[2] = next_debug_type_index;
            queued = queue_debug_record(2);
            debug_type_records[queued].value.symbol = tag;
            debug_type_records[queued].index = 0;
        }
        record[3] = (record[3] & ~1) | (top_packing(packing_stack)->alignment == 1);
        record[3] = (record[3] & ~8) | ((tag->metadata.tag.flags & 2) ? 8 : 0);
        bytes = write_legacy_unsigned_number((unsigned char *)(record + 4),tag->detail.value);
        name_start = (char *)(record + 4) + bytes;
        name_end = append_codeview_name(name_start,tag->name);
        length = 8 + bytes + (name_end - name_start);
    } else if (flags & 0x200) {
        record[0] = 7;
        record[4] = auxiliary;
        if (!(tag->flags & 1)) {
            record[3] = 0;
            record[4] |= 0x80;
            record[2] = 0;
        } else {
            record[3] = next_debug_type_index;
            record[2] = legacy_primitive_type_index((flags & ~0x200) | 0x64);
            queued = queue_debug_record(7);
            debug_type_records[queued].index = fields;
            enumerators = malloc((unsigned)fields * sizeof(EnumeratorSymbol *));
            debug_type_records[queued].value.pointer = enumerators;
            if (!enumerators) fatal_error(60);
            debug_type_records[queued].auxiliary = 0;
            initialize_hash_walk(&walk,tag->associated.enumerators);
            count = 0;
            while (next_hash_entry(&walk)) enumerators[count++] = walk.value;
            qsort(enumerators,fields,sizeof(EnumeratorSymbol *),compare_legacy_enumerators);
        }
        record[4] = (record[4] & ~8) | ((tag->metadata.tag.flags & 2) ? 8 : 0);
        name_start = (char *)(record + 5);
        name_end = append_codeview_name(name_start,tag->name);
        length = 10 + (name_end - name_start);
    }
    write_legacy_type_record(length);
}

/* Original0x48c10..0x48c5f. Values below LF_NUMERIC are immediate;
   larger values carry an unsigned16- or unsigned32-bit numeric leaf. */
unsigned __stdcall write_legacy_unsigned_number(unsigned char *destination,
    unsigned number)
{
    if (number < 0x8000) {
        *(unsigned short *)destination = (unsigned short)number;
        return 2;
    }
    if (number < 0x10000) {
        *(unsigned short *)destination = 0x8002;
        *(unsigned short *)(destination + 2) = (unsigned short)number;
        return 4;
    }
    *(unsigned short *)destination = 0x8004;
    *(unsigned *)(destination + 2) = number;
    return 6;
}

/* Original0x47ef0..0x47f33. Arrays carry their element index, an
   integer index type, byte size, and the empty counted name. */
void __stdcall write_legacy_array_type(CType *type, unsigned short nested)
{
    unsigned short *record = (unsigned short *)debug_record_buffer + 1;
    unsigned bytes;
    record[0] = 3;
    record[1] = nested;
    record[2] = CV_SHORT;
    bytes = write_legacy_unsigned_number((unsigned char *)(record + 3),type_size(type));
    *((unsigned char *)(record + 3) + bytes) = 0;
    write_legacy_type_record(bytes + 7);
}

/* Original0x48780..0x487ff. A void sentinel has zero arguments;
   null entries otherwise retain the zero index used for ellipsis. */
void __stdcall write_legacy_argument_list(ParameterInfo *parameters)
{
    unsigned short *record = (unsigned short *)debug_record_buffer + 1;
    unsigned count = 0, index;
    record[0] = 0x201;
    if (parameters->types[0] != type_0080) count = (unsigned char)parameters->count;
    record[1] = (unsigned short)count;
    if (!count) {
        write_legacy_type_record(4);
        return;
    }
    for (index = 0; index < count; ++index)
        record[index + 2] = parameters->types[index] ?
            (unsigned short)parameters->types[index]->field0c : 0;
    write_legacy_type_record(count * 2 + 4);
}

/* Original0x48720..0x48776. Function records distinguish the byte
   convention from the argument count and queued parameter-list index. */
void __stdcall write_legacy_function_type(CType *type, unsigned short result,
    unsigned short count)
{
    unsigned short *record = (unsigned short *)debug_record_buffer + 1;
    ParameterInfo *parameters = (ParameterInfo *)type->related->field08;
    record[0] = 8;
    record[1] = result;
    *((unsigned char *)(record + 2)) = (unsigned char)codeview_calling_convention(type->related->attributes);
    *((unsigned char *)(record + 2) + 1) = 0;
    record[3] = count;
    record[4] = (unsigned short)parameters->auxiliary;
    write_legacy_type_record(10);
}

unsigned legacy_field_bytes; /* original0x5b250 */

/* Original0x48300..0x484e7. Fields are aligned relative to the first
   member entry. A full264-byte list links to a newly queued continuation. */
void __stdcall write_legacy_field_list(FrontendSymbol *tag, unsigned count, int mode)
{
    unsigned short *record = (unsigned short *)debug_record_buffer + 1;
    unsigned short *entry;
    FrontendSymbol *member;
    unsigned seen = 0, kind, length, padding, remaining, bytes, queued;
    int include, offset;
    unsigned char *tail, pad;
    char *name_start, *name_end;
    legacy_field_bytes = 0;
    record[0] = 0x204;
    for (member = tag->associated.members; member; member = member->next) {
        kind = member->kind & 31;
        include = kind == 4 && member->name == name_unnamed &&
            (member->type->flags & 0x200);
        if (!include)
            include = member->name && member->name != name_unnamed &&
                member->name != name_alignment &&
                (!kind || kind > 13 || kind == 5 || member->type);
        if (!include) continue;
        if (count <= seen) {
            for (length = 0; member->name[length]; ++length) ;
            ++length;
            padding = -legacy_field_bytes & 3;
            tail = (unsigned char *)debug_record_buffer + 4 + legacy_field_bytes;
            pad = (unsigned char)(padding - 16);
            for (remaining = padding; remaining; --remaining) *tail++ = pad--;
            legacy_field_bytes += padding;
            if (legacy_field_bytes + length + 16 > 264) {
                begin_legacy_field_continuation(2,tag,seen,(unsigned)mode & 0xffff);
                return;
            }
            entry = (unsigned short *)((unsigned char *)debug_record_buffer + 4 + legacy_field_bytes);
            entry[0] = 0x406;
            entry[2] = legacy_tag_auxiliary;
            entry[2] |= 3;
            if (kind == 2) {
                entry[1] = next_debug_type_index;
                queued = queue_debug_record(1);
                debug_type_records[queued].value.symbol = member;
            } else entry[1] = (unsigned short)member->type->field0c;
            offset = (int)member->detail.value;
            if (offset < 0) bytes = write_legacy_signed_number((unsigned char *)(entry + 3),offset);
            else bytes = write_legacy_unsigned_number((unsigned char *)(entry + 3),(unsigned)offset);
            name_start = (char *)(entry + 3) + bytes;
            name_end = append_codeview_name(name_start,member->name);
            legacy_field_bytes += bytes + 6 + (name_end - name_start);
        }
        ++seen;
    }
    write_legacy_type_record(legacy_field_bytes + 2);
}

/* Original0x47a80..0x47ad9. Its compare/set sequence selects128
   as the shrink limit, retaining smaller allocations and clearing count. */
void reset_debug_type_records(void)
{
    if (debug_type_record_capacity > 128) {
        debug_type_record_capacity = 128;
        debug_type_records = realloc(debug_type_records,128 * sizeof(DebugTypeRecord));
        if (!debug_type_records) fatal_error(60);
    }
    debug_type_record_count = 0;
}

/* Original 0x484f0..0x48529. Bitfield width and starting bit follow
   the LF_BITFIELD leaf, followed by the underlying primitive index. */
void __stdcall write_legacy_bitfield_type(FrontendSymbol *member)
{
    unsigned short *record = (unsigned short *)debug_record_buffer + 1;
    unsigned char *bits = (unsigned char *)(record + 1);
    record[0] = 0x206;
    bits[0] = member->metadata.bitfield.width;
    bits[1] = member->metadata.bitfield.offset;
    record[2] = legacy_primitive_type_index(member->type->flags);
    write_legacy_type_record(6);
}

/* Original0x48280..0x482f5. Close a full field list with LF_INDEX,
   reserving a new queued record for the remaining members. Enum slices
   retain both their current position and limit; records only need position. */
void __cdecl begin_legacy_field_continuation(unsigned char kind, void *payload,
    unsigned first, unsigned limit)
{
    unsigned short *continuation = (unsigned short *)
        ((unsigned char *)debug_record_buffer + legacy_field_bytes + 4);
    unsigned queued;
    continuation[0] = 0x405;
    continuation[1] = next_debug_type_index;
    queued = queue_debug_record(kind);
    debug_type_records[queued].value.pointer = payload;
    if (kind != 2) {
        debug_type_records[queued].auxiliary = first;
        first = limit;
    }
    debug_type_records[queued].index = (unsigned short)first;
    write_legacy_type_record(legacy_field_bytes + 6);
}

/* Original0x48fc0..0x48fd2. Restore enumerator declaration order by
   symbol index, independently of signed or wide enumerator values. */
int __cdecl compare_legacy_enumerators(const void *left, const void *right)
{
    const EnumeratorSymbol *a = *(EnumeratorSymbol * const *)left;
    const EnumeratorSymbol *b = *(EnumeratorSymbol * const *)right;
    return (int)(a->index - b->index);
}

/* Original0x48530..0x48683. Enumerators are already in declaration
   order. Each LF_ENUMERATE carries attributes, numeric value and counted
   name; a full list queues the remaining slice without freeing its array. */
void __stdcall write_legacy_enum_members(EnumeratorSymbol **enumerators, int first, int limit)
{
    unsigned short *record = (unsigned short *)debug_record_buffer + 1;
    unsigned short *entry;
    EnumeratorSymbol *symbol;
    int index, value;
    unsigned length, padding, remaining, bytes;
    unsigned char *tail, pad;
    char *name_start, *name_end;
    record[0] = 0x204;
    legacy_field_bytes = 0;
    for (index = first; index < limit; ++index) {
        symbol = enumerators[index];
        symbol->flags &= ~2;
        for (length = 0; symbol->name[length]; ++length) ;
        ++length;
        padding = -legacy_field_bytes & 3;
        tail = (unsigned char *)debug_record_buffer + legacy_field_bytes + 4;
        pad = (unsigned char)(padding - 16);
        for (remaining = padding; remaining; --remaining) *tail++ = pad--;
        legacy_field_bytes += padding;
        if (legacy_field_bytes + length + 14 > 264) {
            begin_legacy_field_continuation(7,enumerators,index,limit);
            return;
        }
        entry = (unsigned short *)((unsigned char *)debug_record_buffer + legacy_field_bytes + 4);
        entry[0] = 0x403;
        entry[1] = legacy_tag_auxiliary;
        entry[1] |= 3;
        value = (int)symbol->value;
        if (value < 0) bytes = write_legacy_signed_number((unsigned char *)(entry + 2),value);
        else bytes = write_legacy_unsigned_number((unsigned char *)(entry + 2),(unsigned)value);
        name_start = (char *)(entry + 2) + bytes;
        name_end = append_codeview_name(name_start,symbol->name);
        legacy_field_bytes += bytes + 4 + (name_end - name_start);
    }
    write_legacy_type_record(legacy_field_bytes + 2);
    free(enumerators);
}

/* Original0x48bb0..0x48c0a. Choose the smallest signed numeric leaf;
   each return value includes its two-byte leaf tag. */
unsigned __stdcall write_legacy_signed_number(unsigned char *destination, int number)
{
    if (number >= -128 && number < 128) {
        *(unsigned short *)destination = 0x8000;
        destination[2] = (unsigned char)number;
        return 3;
    }
    if (number >= -32768 && number < 32768) {
        *(unsigned short *)destination = 0x8001;
        *(short *)(destination + 2) = (short)number;
        return 4;
    }
    *(unsigned short *)destination = 0x8003;
    *(int *)(destination + 2) = number;
    return 6;
}

/* Original0x47ce0..0x47d17. End the separate PCH type range with its
   accumulated checksum. This record itself is outside that checksum. */
void finish_pch_type_records(void)
{
    unsigned short *record;
    pch_separate_debug = 0;
    if (!options.line_records) {
        record = (unsigned short *)debug_record_buffer + 1;
        record[0] = 0x14;
        *(unsigned *)(record + 1) = pragma_record_number;
        write_legacy_type_record(6);
        ++next_debug_type_index;
    }
}

/* Original0x47c70..0x47cdf. Start the consumer debug stream with a
   LF_PRECOMP reference to the producer's type range, checksum and object. */
void __stdcall restore_pch_debug_state(char *object_name)
{
    unsigned short *record, previous;
    char *name, *end;
    if (options.line_records) {
        begin_program_database();
        return;
    }
    debug_type_bytes = 0;
    begin_debug_info();
    record = (unsigned short *)debug_record_buffer + 1;
    record[0] = 0x13;
    record[1] = 0x1000;
    previous = next_debug_type_index--;
    record[2] = (unsigned short)(previous - 0x1001);
    *(unsigned *)(record + 3) = pragma_record_number;
    name = (char *)(record + 5);
    end = append_codeview_name(name,object_name);
    write_legacy_type_record(10 + (end - name));
}
