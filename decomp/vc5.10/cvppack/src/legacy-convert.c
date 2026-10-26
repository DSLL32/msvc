/* Dispatch marker/WORD-length/byte-leaf C6 records to their typed converters. */
#include "legacy-types.h"
#include "legacy-symbols.h"
#include "pack.h"
#include <recon.h>

#pragma intrinsic(memcpy)
LegacyTypeConverter legacy_type_converters[13] = {
    { 0x7a, convert_legacy_pointer },
    { 0x91, convert_legacy_based_pointer },
    { 0x79, convert_legacy_structure },
    { 0x78, convert_legacy_array },
    { 0x75, convert_legacy_procedure },
    { 0x80, convert_legacy_nil },
    { 0x5c, convert_legacy_bitfield },
    { 0x72, convert_legacy_label },
    { 0x8d, convert_legacy_scalar_array },
    { 0x8e, convert_legacy_untranslated },
    { 0x8c, convert_legacy_based_array },
    { 0xa6, convert_legacy_cobol0 },
    { 0xa7, convert_legacy_cobol1 }
};

/* [00407690,0040773a). */
void convert_legacy_record(unsigned index)
{
    TypeRecord *record;
    unsigned from_global, slot;
    unsigned char leaf;
    if (!module_type_low && index == 0xffff - module_type_base)
        record = empty_type_record;
    else {
        record = &type_records[index];
        if (record->status & 0x1000)
            record = lookup_global_type(record->global_index - 0x1000, &from_global);
    }
    leaf = record->data[3];
    for (slot = 0; slot < 13; ++slot)
        if (legacy_type_converters[slot].leaf == leaf) break;
    if (slot < 13) legacy_type_converters[slot].convert(record);
    else {
        warning(4005, module_name(current_module_info), NULL);
        convert_legacy_nil(record);
    }
}

/* C6 procedure return/argument indices become DWORD-wide LF_PROCEDURE1008.
 * [00407e20,00407f4c), convention-dispatch data[00407f4c,00407fa1). */
void convert_legacy_procedure(TypeRecord *record)
{
    unsigned char *cursor = record->data + 5, *data;
    unsigned result, arguments, numeric;
    unsigned short count;
    /* Unknown original convention tags leave this byte uninitialized.
     * Zero that undefined output; this case uses the defined cdecl tag63. */
    unsigned char convention = 0, tag;
    record->status |= 0x400;
    if (*cursor == 0x83) {
        ++cursor;
        result = *(unsigned *)cursor;
        cursor += 4;
    } else result = *cursor++;
    tag = *cursor++;
    switch (tag) {
    case 0x63: convention = 0; break;
    case 0x64: convention = 1; break;
    case 0x73: convention = 3; break;
    case 0x74: convention = 2; break;
    case 0x95: convention = 4; break;
    case 0x96: convention = 5; break;
    case 0x97: convention = 6; break;
    }
    numeric = read_legacy_numeric(&cursor, NULL);
    count = numeric > 0xffff ? 0xffff : (unsigned short)numeric;
    ++cursor;
    if (count) {
        arguments = *(unsigned *)cursor;
        convert_legacy_argument_list(arguments, count);
    } else arguments = 0xffff;
    if ((unsigned)*(unsigned short *)(cursor + 1) + 1 >= 14)
        data = record->data;
    else data = resize_legacy_type_payload(record, 16);
    *(unsigned short *)data = 14;
    *(unsigned short *)(data + 2) = 0x1008;
    *(unsigned *)(data + 4) = result;
    data[8] = convention;
    data[9] = 0;
    *(unsigned short *)(data + 10) = count;
    *(unsigned *)(data + 12) = arguments;
}

/* C6 numeric tags retain the raw unsigned representation, including the
 * signed byte/WORD tags. [0040fd20,0040fdb9), dispatch[0040fdbc,0040fdd4). */
unsigned read_legacy_numeric(unsigned char **cursor, unsigned short *bytes)
{
    unsigned char *value = *cursor;
    unsigned char tag = *value++;
    unsigned result, length;
    if (tag < 0x80) {
        result = tag;
        length = 1;
    } else {
        switch (tag) {
        case 0x88:
            result = *value++;
            length = 2;
            break;
        case 0x85: case 0x89:
            result = *(unsigned short *)value;
            value += 2;
            length = 3;
            break;
        case 0x86: case 0x8a:
            result = *(unsigned *)value;
            value += 4;
            length = 5;
            break;
        default:
            fatal_error(1006, module_name(current_module_info), NULL);
            result = length = 0;
            break;
        }
    }
    *cursor = value;
    if (bytes) *bytes = (unsigned short)length;
    return result;
}

/* [00408360,00408376). */
void convert_legacy_nil(TypeRecord *record)
{
    record->status |= 0x400;
    *(unsigned short *)record->data = 2;
    *(unsigned short *)(record->data + 2) = 0x0f;
}

/* [00408380,00408396). */
void convert_legacy_untranslated(TypeRecord *record)
{
    record->status |= 0x400;
    *(unsigned short *)record->data = 2;
    *(unsigned short *)(record->data + 2) = 0x10;
}

/* [004083a0,00408405). */
void convert_legacy_label(TypeRecord *record)
{
    unsigned char *old = record->data, *data;
    record->status |= 0x400;
    data = allocate_bytes(6);
    *(unsigned short *)data = 4;
    *(unsigned short *)(data + 2) = 0x0e;
    *(unsigned short *)(data + 4) = old[5] == 0x73 ? 4 : 0;
    if (record->status & 0x38) release_type_payload(record);
    record->status |= 8;
    record->data = data;
}

/* [00408410,0040849f). */
void convert_legacy_bitfield(TypeRecord *record)
{
    unsigned char *old = record->data + 4, *data;
    unsigned type;
    record->status |= 0x400;
    data = allocate_bytes(10);
    *(unsigned short *)data = 8;
    *(unsigned short *)(data + 2) = 0x1205;
    data[8] = old[0];
    type = old[0] <= 16 ? 0x11 : 0x12;
    if (old[1] != 0x7d) type += 0x10;
    *(unsigned *)(data + 4) = type;
    data[9] = old[2];
    if (record->status & 0x38) release_type_payload(record);
    record->status |= 8;
    record->data = data;
}

/* [00407d50,00407e1e). Scalar-array mode0 carries a numeric byte size;
 * mode1 denotes LF_NOTTRAN. Other modes preserve the original unchanged body. */
void convert_legacy_scalar_array(TypeRecord *record)
{
    unsigned char *cursor = record->data, *data;
    unsigned char numeric[16];
    unsigned value, index;
    unsigned short consumed, size;
    record->status |= 0x400;
    if (cursor[4] == 0) {
        cursor += 5;
        value = read_legacy_numeric(&cursor, &consumed);
        size = write_unsigned_numeric(numeric, value);
        data = resize_legacy_type_payload(record, (unsigned short)(size + 13));
        *(unsigned short *)data = size + 11;
        *(unsigned short *)(data + 2) = 0x1003;
        *(unsigned *)(data + 4) = 0x10;
        *(unsigned *)(data + 8) = 0x21;
        for (index = 0; index < size; ++index) data[12 + index] = numeric[index];
        data[12 + size] = 0;
    } else if (cursor[4] == 1) {
        *(unsigned short *)cursor = 2;
        *(unsigned short *)(cursor + 2) = 0x10;
    }
}

/* [0040fc70,0040fcad). */
unsigned short write_unsigned_numeric(unsigned char *output, unsigned value)
{
    if (value < 0x8000) {
        *(unsigned short *)output = (unsigned short)value;
        return 2;
    }
    if (!(value & 0xffff0000)) {
        *(unsigned short *)output = 0x8002;
        *(unsigned short *)(output + 2) = (unsigned short)value;
        return 4;
    }
    *(unsigned short *)output = 0x8004;
    *(unsigned *)(output + 2) = value;
    return 6;
}

/* Allocate a replacement payload without copying the obsolete C6 bytes.
 * The original consumes only the lowWORD size, then reserves two extra bytes.
 * [004084a0,0040852d). */
unsigned char *resize_legacy_type_payload(TypeRecord *record, unsigned short size)
{
    unsigned bytes = (unsigned)size + 2;
    unsigned char *data;
    if (bytes > 900) data = allocate_bytes(bytes);
    else if (bytes > 32) data = allocate_medium_type();
    else data = allocate_small_type();
    if (record->status & 0x38) release_type_payload(record);
    if (bytes > 900) record->status |= 8;
    else if (bytes > 32) record->status |= 0x20;
    else record->status |= 0x10;
    record->data = data;
    return data;
}

/* C6 arrays store their size in bits and WORD element/index references;
 * the output LF_ARRAY carries DWORD indices and a byte-sized numeric value.
 * [00407bd0,00407d42). */
void convert_legacy_array(TypeRecord *record)
{
    unsigned char *old = record->data, *cursor = old + 4, *data;
    unsigned char name[257], numeric[20];
    unsigned short old_length, consumed, size, element, index, length;
    unsigned i, count;
    for (i = 0; i < 257; ++i) name[i] = 0;
    record->status |= 0x400;
    old_length = *(unsigned short *)(old + 1);
    count = read_legacy_numeric(&cursor, &consumed) >> 3;
    size = write_unsigned_numeric(numeric, count);
    ++cursor;
    element = *(unsigned short *)cursor;
    cursor += 2;
    if ((unsigned)consumed + 4 < old_length) {
        ++cursor;
        index = *(unsigned short *)cursor;
        cursor += 2;
    } else index = 0x85;
    if ((unsigned short)(consumed + 7) < old_length) {
        if (*cursor == 0x82) ++cursor;
        /* Original counted copy includes the length and omits its last
         * character. The precleared local supplies that final zero byte. */
        for (i = 0; i < *cursor; ++i) name[i] = cursor[i];
    }
    length = (unsigned short)(name[0] + size + 11);
    if ((unsigned)length <= (unsigned)old_length + 1) data = old;
    else data = resize_legacy_type_payload(record, (unsigned short)(length + 2));
    *(unsigned short *)data = length;
    *(unsigned short *)(data + 2) = 0x1003;
    *(unsigned *)(data + 4) = element;
    *(unsigned *)(data + 8) = index;
    for (i = 0; i < size; ++i) data[12 + i] = numeric[i];
    data += 12 + size;
    for (i = 0; i < (unsigned)name[0] + 1; ++i) data[i] = name[i];
}

/* Preserve the original list owner while replacing marker83/WORD entries
 * with LF_ARGLIST's DWORD entries [004081f0,00408358). */
void convert_legacy_argument_list(unsigned index, unsigned short count)
{
    TypeRecord *record;
    unsigned slot = index - 0x200, from_global, bytes, i;
    unsigned short length;
    unsigned char *cursor, *data;
    int reuse;
    if (!module_type_low && slot == 0xffff - module_type_base)
        record = empty_type_record;
    else {
        record = &type_records[slot];
        if (record->status & 0x1000)
            record = lookup_global_type(record->global_index - 0x1000, &from_global);
    }
    if (record->status & 0x400) return;
    record->status |= 0x400;
    cursor = record->data;
    length = count * 4 + 6;
    bytes = (unsigned)length + 2;
    reuse = bytes <= (unsigned)*(unsigned short *)(cursor + 1) + 3;
    data = reuse ? allocate_type_buffer(bytes) : allocate_bytes(bytes);
    *(unsigned short *)data = length;
    *(unsigned short *)(data + 2) = 0x1201;
    *(unsigned *)(data + 4) = count;
    cursor += 4;
    for (i = 0; i < count; ++i) {
        if (*cursor++ == 0x83) {
            *(unsigned *)(data + 8 + i * 4) = (short)*(unsigned short *)cursor;
            cursor += 2;
        } else *(unsigned *)(data + 8 + i * 4) = -1;
    }
    if (reuse) {
        for (i = 0; i < bytes; ++i) record->data[i] = data[i];
    } else {
        if (record->status & 0x38) release_type_payload(record);
        record->status |= 8;
        record->data = data;
    }
}

/* A C6 aggregate references separate member-type and member-name lists.
 * [00407980,00407b7d). The actual WORD413268 also controls later UDT aliases. */
void convert_legacy_structure(TypeRecord *record)
{
    unsigned char *old = record->data, *cursor = old + 4, *end, *data;
    unsigned char numeric[20], name[257];
    unsigned short old_length, numeric_size, count, fields, properties, length;
    int types, names;
    unsigned value;
    record->status |= 0x400;
    old_length = *(unsigned short *)(old + 1);
    end = old + old_length + 3;
    retain_structure_names = 1;
    value = read_legacy_numeric(&cursor, NULL);
    if (value) value = ((value - 1) >> 3) + 1;
    numeric_size = write_unsigned_numeric(numeric, value);
    value = read_legacy_numeric(&cursor, NULL);
    count = value > 0xffff ? 0xffff : (unsigned short)value;
    if (*cursor++ == 0x83) {
        types = *(short *)cursor;
        cursor += 2;
    } else types = -1;
    if (*cursor++ == 0x83) {
        names = *(short *)cursor;
        cursor += 2;
    } else names = -1;
    fields = convert_legacy_field_list(types, names, count);
    name[0] = 0;
    if (cursor < end) {
        if (*cursor == 0x82) {
            ++cursor;
            memcpy(name, cursor, (unsigned)cursor[0] + 1);
            cursor += (unsigned)cursor[0] + 1;
            properties = cursor < end && *cursor == 0x68;
        } else properties = *cursor == 0x68;
    } else properties = 0;
    length = name[0] + numeric_size + 19;
    if (length <= (unsigned)old_length + 1) data = old;
    else data = resize_legacy_type_payload(record, length + 2);
    *(unsigned short *)data = length;
    *(unsigned short *)(data + 2) = 0x1005;
    *(unsigned short *)(data + 4) = count;
    *(unsigned short *)(data + 6) = properties;
    *(unsigned *)(data + 8) = fields;
    *(unsigned *)(data + 12) = 0;
    *(unsigned *)(data + 16) = 0;
    memcpy(data + 20, numeric, numeric_size);
    memcpy(data + 20 + numeric_size, name, (unsigned)name[0] + 1);
}

/* Join the C6 parallel type/name lists into the original aligned member
 * encoding [00407fb0,004081e6). Keep the original type/attribute field order. */
unsigned short convert_legacy_field_list(unsigned short types, unsigned short names,
    unsigned short count)
{
    TypeRecord *record, *name_record;
    unsigned slot, from_global, bytes, i, padding;
    unsigned short allocation, remaining;
    unsigned char *type_cursor, *name_cursor, *after_name, *cursor, *start, *data;
    slot = (unsigned)types - 0x200;
    if (!module_type_low && slot == 0xffff - module_type_base)
        record = empty_type_record;
    else {
        record = &type_records[slot];
        if (record->status & 0x1000)
            record = lookup_global_type(record->global_index - 0x1000, &from_global);
    }
    slot = (unsigned)names - 0x200;
    if (!module_type_low && slot == 0xffff - module_type_base)
        name_record = empty_type_record;
    else {
        name_record = &type_records[slot];
        if (name_record->status & 0x1000)
            name_record = lookup_global_type(name_record->global_index - 0x1000, &from_global);
    }
    type_cursor = record->data + 4;
    name_cursor = name_record->data;
    allocation = *(unsigned short *)(name_cursor + 1) + count * 12 + 6;
    name_cursor += 4;
    cursor = allocate_type_buffer(allocation);
    while ((unsigned)cursor & 3) ++cursor;
    start = cursor;
    cursor += 2;
    *(unsigned short *)cursor = 0x1203;
    cursor += 2;
    remaining = count;
    while (remaining) {
        ++name_cursor;
        *(unsigned short *)cursor = 0x1405;
        cursor += 2;
        if (*type_cursor++ == 0x83) {
            *(unsigned short *)cursor = *(short *)type_cursor;
            type_cursor += 2;
        } else *(unsigned short *)cursor = 0xffff;
        cursor += 2;
        *(unsigned short *)cursor = 3;
        cursor += 2;
        after_name = name_cursor + *name_cursor + 1;
        convert_legacy_numeric_leaf(&after_name, &cursor);
        for (i = 0; i < (unsigned)*name_cursor + 1; ++i) *cursor++ = name_cursor[i];
        name_cursor = after_name;
        padding = (unsigned)cursor & 3;
        if (padding) {
            *cursor++ = (unsigned char)(0xf4 - padding);
            while ((unsigned)cursor & 3) *cursor++ = 0;
        }
        --remaining;
    }
    bytes = cursor - start;
    *(unsigned short *)start = bytes - 2;
    data = resize_legacy_type_payload(record, bytes);
    record->status |= 0x400;
    for (i = 0; i < (unsigned short)bytes; ++i) data[i] = start[i];
    return types;
}

/* The based-array referent becomes a DWORD LF_BARRAY index
 * [00407b80,00407bc4). */
void convert_legacy_based_array(TypeRecord *record)
{
    unsigned referent = *(unsigned short *)(record->data + 5);
    unsigned char *data;
    record->status |= 0x400;
    data = resize_legacy_type_payload(record, 8);
    *(unsigned short *)data = 6;
    *(unsigned short *)(data + 2) = 0x100b;
    *(unsigned *)(data + 4) = referent;
}

/* C6 near/far/huge pointers expand in place [00407740,004077b8).
 * The original carries unwritten attribute bits0x1800 (and an unknown mode)
 * from its stack local. Zero that undefined output. This case observes zero
 * for the carried bits and still compares the complete raw output strictly. */
void convert_legacy_pointer(TypeRecord *record)
{
    unsigned char *data = record->data;
    unsigned referent = *(unsigned short *)(data + 6), attributes = 0;
    record->status |= 0x400;
    switch (data[4]) {
    case 0x5e: attributes = 2; break;
    case 0x73: attributes = 1; break;
    case 0x74: attributes = 0; break;
    }
    attributes |= 0x100;
    *(unsigned short *)data = 10;
    *(unsigned short *)(data + 2) = 0x1002;
    *(unsigned *)(data + 4) = referent;
    *(unsigned *)(data + 8) = attributes;
}

/* C6 based pointers retain the actual base relation and consumed embedded
 * symbol [004077c0,00407948), switch data[00407948,00407980).
 * Original reserved attribute bits0x1800 are unwritten stack data, as in the
 * plain pointer converter. Zero only that undefined output, not the payload. */
void convert_legacy_based_pointer(TypeRecord *record)
{
    unsigned char *old = record->data, *data;
    unsigned char suffix[260];
    unsigned referent = *(unsigned short *)(old + 5), attributes = 0;
    unsigned short old_length = *(unsigned short *)(old + 1), length, bytes = 0;
    int symbol = 0;
    record->status |= 0x400;
    switch (old[7]) {
    case 0x92:
        attributes = 3;
        *(unsigned short *)suffix = *(unsigned short *)(old + 8);
        bytes = 2;
        break;
    case 0x93: attributes = 4; symbol = 1; break;
    case 0x94: attributes = 5; symbol = 1; break;
    case 0x97: attributes = 6; symbol = 1; break;
    case 0x98: attributes = 7; symbol = 1; break;
    case 0x83:
        attributes = 8;
        *(unsigned short *)suffix = 0x9c;
        suffix[2] = 0;
        bytes = 3;
        break;
    case 0x81: attributes = 9; break;
    }
    if (symbol)
        bytes += convert_legacy_base_symbol(suffix + bytes,
            legacy_symbol_at(*(unsigned short *)(old + 8)));
    attributes |= 0x100;
    length = bytes + 10;
    if (length <= (unsigned)old_length + 1) data = old;
    else data = resize_legacy_type_payload(record, length + 2);
    *(unsigned short *)data = length;
    *(unsigned short *)(data + 2) = 0x1002;
    *(unsigned *)(data + 4) = referent;
    *(unsigned *)(data + 8) = attributes;
    memcpy(data + 12, suffix, bytes);
}

/* Preserve the opaque COBOL parent/item bytes. Unlike the pointer converters,
 * these original routines do not widen or interpret the copied payload.
 * [00408530,004085b6), cdecl with one TypeRecord pointer. */
void convert_legacy_cobol0(TypeRecord *record)
{
    unsigned char *old = record->data, *data;
    unsigned short size = (unsigned short)(*(unsigned short *)(old + 1) + 3);
    current_type_count = 1;
    record->status |= 0x400;
    data = allocate_bytes(size);
    *(unsigned short *)data = (unsigned short)(size - 2);
    *(unsigned short *)(data + 2) = 0x100a;
    memmove(data + 4, old + 4, (unsigned)*(unsigned short *)(old + 1) - 1);
    if (record->status & 0x38) release_type_payload(record);
    record->status |= 8;
    record->data = data;
}

/* Same opaque ownership conversion for the unparented COBOL leaf.
 * [004085c0,00408646), cdecl with one TypeRecord pointer. */
void convert_legacy_cobol1(TypeRecord *record)
{
    unsigned char *old = record->data, *data;
    unsigned short size = (unsigned short)(*(unsigned short *)(old + 1) + 3);
    current_type_count = 1;
    record->status |= 0x400;
    data = allocate_bytes(size);
    *(unsigned short *)data = (unsigned short)(size - 2);
    *(unsigned short *)(data + 2) = 0x0c;
    memmove(data + 4, old + 4, (unsigned)*(unsigned short *)(old + 1) - 1);
    if (record->status & 0x38) release_type_payload(record);
    record->status |= 8;
    record->data = data;
}
