/* Typed dispatch from encountered C6 leaf records to their converters. */
#include "types.h"
#include "image.h"
#include <recon.h>

typedef struct OldTypeConverter {
    unsigned char leaf;
    void (__cdecl *convert)(TypeRecord *);
} OldTypeConverter;
typedef char OldTypeConverterSize[(sizeof(OldTypeConverter)==8)?1:-1];

OldTypeConverter old_type_converters[13] = {
    { 0x7a, convert_old_pointer },
    { 0x91, convert_old_based_pointer },
    { 0x79, convert_old_structure },
    { 0x78, convert_old_array },
    { 0x75, convert_old_procedure },
    { 0x80, convert_old_leaf_80 },
    { 0x5c, convert_old_leaf_5c },
    { 0x72, convert_old_leaf_72 },
    { 0x8d, convert_old_leaf_8d },
    { 0x8e, convert_old_leaf_8e },
    { 0x8c, convert_old_barray },
    { 0xa6, convert_old_modifier },
    { 0xa7, convert_old_leaf_a7 },
};

void convert_old_type_record(unsigned short relative)
{
    TypeRecord *entry;
    unsigned short unused;
    unsigned char leaf;
    unsigned i;
    if (!type_signature && relative == (unsigned short)(0xffff-first_user_type))
        entry = empty_type;
    else {
        entry = &local_type_records[relative];
        if (entry->flags & 0x1000)
            entry = get_type_record((unsigned short)(entry->forward_index-0x1000), &unused);
    }
    leaf = entry->data[3];
    for (i = 0; i < 13; ++i)
        if (old_type_converters[i].leaf == leaf) break;
    if (i != 13) old_type_converters[i].convert(entry);
    else {
        warning(5, module_name(current_module), NULL);
        convert_old_leaf_80(entry);
    }
}

/* LF_PROCEDURE holds a counted return type and a legacy argument-list index. */
void convert_old_procedure(TypeRecord *entry)
{
    unsigned char *cursor = entry->data+5, *record;
    unsigned short result, arguments, count;
    unsigned long numeric;
    unsigned char convention = 0, kind;
    entry->flags |= 0x400;
    if (*cursor == 0x83) {
        ++cursor;
        result = *(unsigned short *)cursor;
        cursor += 2;
    } else result = *cursor++;
    kind = *cursor++;
    switch (kind) {
    case 0x63: convention = 0; break;
    case 0x64: convention = 1; break;
    case 0x73: convention = 3; break;
    case 0x74: convention = 2; break;
    case 0x95: convention = 4; break;
    case 0x96: convention = 5; break;
    case 0x97: convention = 6; break;
    default:
        /* The original leaves this output byte uninitialized for an unknown
           convention. Keep the reconstructed byte deterministically zero. */
        break;
    }
    numeric = read_old_numeric(&cursor, NULL);
    count = numeric > 0xffff ? 0xffff : (unsigned short)numeric;
    ++cursor;
    if (count) {
        arguments = *(unsigned short *)cursor;
        convert_old_argument_list(arguments, count);
    } else arguments = 0xffff;
    if ((unsigned long)*(unsigned short *)(cursor+1)+1 >= 10)
        record = entry->data;
    else record = resize_old_type(entry, 12);
    *(unsigned short *)record = 10;
    *(unsigned short *)(record+2) = 8;
    *(unsigned short *)(record+4) = result;
    record[6] = convention;
    record[7] = 0;
    *(unsigned short *)(record+8) = count;
    *(unsigned short *)(record+10) = arguments;
}

/* C6 numeric leaves return an unsigned value and their complete byte count. */
unsigned long read_old_numeric(unsigned char **cursor, unsigned short *size)
{
    unsigned char *value = *cursor;
    unsigned char kind = *value++;
    unsigned short length;
    unsigned long result;
    if (kind < 0x80) {
        result = kind;
        length = 1;
    } else {
        switch (kind) {
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
            result = *(unsigned long *)value;
            value += 4;
            length = 5;
            break;
        default:
            fatal_error(18, module_name(current_module), NULL);
            result = 0;
            length = 0;
            break;
        }
    }
    *cursor = value;
    if (size) *size = length;
    return result;
}

/* Replace a C6 argument list with LF_ARGLIST, preserving its owning slot. */
void convert_old_argument_list(unsigned short type, unsigned short count)
{
    unsigned short relative = type-0x200, unused, length, i;
    TypeRecord *entry;
    unsigned char *cursor, *converted;
    unsigned long bytes, n;
    int reuse;
    if (!type_signature && relative == (unsigned short)(0xffff-first_user_type))
        entry = empty_type;
    else {
        entry = &local_type_records[relative];
        if (entry->flags & 0x1000)
            entry = get_type_record((unsigned short)(entry->forward_index-0x1000), &unused);
    }
    if (entry->flags & 0x400) return;
    entry->flags |= 0x400;
    cursor = entry->data;
    length = (unsigned short)(count*2+4);
    bytes = (unsigned long)length+2;
    reuse = (unsigned long)*(unsigned short *)(cursor+1)+3 >= bytes;
    converted = reuse ? type_scratch(bytes) : checked_alloc(bytes);
    cursor += 4;
    *(unsigned short *)converted = length;
    *(unsigned short *)(converted+2) = 0x201;
    *(unsigned short *)(converted+4) = count;
    for (i = 0; i < count; ++i) {
        if (*cursor++ == 0x83) {
            *(unsigned short *)(converted+6+i*2) = *(unsigned short *)cursor;
            cursor += 2;
        } else *(unsigned short *)(converted+6+i*2) = 0xffff;
    }
    if (reuse) {
        for (n = 0; n < bytes; ++n) entry->data[n] = converted[n];
    } else {
        if (entry->flags & 0x38) release_type_data(entry);
        entry->flags |= 8;
        entry->data = converted;
    }
}

/* Basic C6 pointer records become the six-byte LF_POINTER payload. */
void convert_old_pointer(TypeRecord *entry)
{
    unsigned char *record = entry->data;
    unsigned short referent = *(unsigned short *)(record+6), attributes = 0;
    entry->flags |= 0x400;
    switch (record[4]) {
    case 0x5e: attributes = 2; break;
    case 0x73: attributes = 1; break;
    case 0x74: attributes = 0; break;
    default: break;
    }
    /* The original leaves reserved attribute bit 0x0800 uninitialized, and
       also leaves the mode uninitialized for unknown pointer encodings.
       Zero these unwritten bits in reconstructed output. */
    attributes |= (unsigned short)(current_module_pe&1)<<8;
    *(unsigned short *)record = 6;
    *(unsigned short *)(record+2) = 2;
    *(unsigned short *)(record+4) = attributes;
    *(unsigned short *)(record+6) = referent;
}

/* Convert a bit-sized C6 array to LF_ARRAY's byte-sized numeric leaf. */
void convert_old_array(TypeRecord *entry)
{
    unsigned char *old = entry->data, *cursor = old+4, *record;
    unsigned char name[257], numeric[20];
    unsigned short old_length, number_size, numeric_size, element, index;
    unsigned short length, i;
    unsigned long count;
    for (i = 0; i < 257; ++i) name[i] = 0;
    entry->flags |= 0x400;
    old_length = *(unsigned short *)(old+1);
    count = read_old_numeric(&cursor, &number_size)>>3;
    numeric_size = write_numeric_leaf(numeric, count);
    ++cursor;
    element = *(unsigned short *)cursor;
    cursor += 2;
    if ((unsigned long)number_size+4 < old_length) {
        ++cursor;
        index = *(unsigned short *)cursor;
        cursor += 2;
    } else index = 0x85;
    if ((unsigned short)(number_size+7) < old_length) {
        if (*cursor == 0x82) ++cursor;
        /* Preserve the original counted copy: it copies the length byte and
           one fewer character, leaving the final name byte zero. */
        for (i = 0; i < *cursor; ++i) name[i] = cursor[i];
    }
    length = (unsigned short)(name[0]+numeric_size+7);
    if ((unsigned long)length <= (unsigned long)old_length+1) record = old;
    else record = resize_old_type(entry, (unsigned short)(length+2));
    *(unsigned short *)record = length;
    *(unsigned short *)(record+2) = 3;
    *(unsigned short *)(record+4) = element;
    *(unsigned short *)(record+6) = index;
    for (i = 0; i < numeric_size; ++i) record[8+i] = numeric[i];
    record += 8+numeric_size;
    for (i = 0; i < (unsigned short)name[0]+1; ++i) record[i] = name[i];
}

/* Emit the shortest unsigned CodeView numeric leaf for a 32-bit value. */
unsigned short write_numeric_leaf(unsigned char *buffer, unsigned long value)
{
    if (value < 0x8000) {
        *(unsigned short *)buffer = (unsigned short)value;
        return 2;
    }
    if (!(value&0xffff0000)) {
        *(unsigned short *)buffer = 0x8002;
        *(unsigned short *)(buffer+2) = (unsigned short)value;
        return 4;
    }
    *(unsigned short *)buffer = 0x8004;
    *(unsigned long *)(buffer+2) = value;
    return 6;
}

/* Select the application's small, medium or separately owned record storage. */
unsigned char *resize_old_type(TypeRecord *entry, unsigned short size)
{
    unsigned long bytes = (unsigned long)size+2;
    unsigned char *buffer;
    if (bytes > 900) buffer = checked_alloc(bytes);
    else if (bytes > 32) buffer = allocate_medium_type();
    else buffer = allocate_small_type();
    if (entry->flags & 0x38) release_type_data(entry);
    if (bytes > 900) entry->flags |= 8;
    else if (bytes > 32) entry->flags |= 0x20;
    else entry->flags |= 0x10;
    entry->data = buffer;
    return buffer;
}

/* C6 structures carry separate member-type and member-name/offset lists. */
void convert_old_structure(TypeRecord *entry)
{
    unsigned char *old = entry->data, *cursor = old+4, *end, *record;
    unsigned char numeric[20], name[257];
    unsigned short old_length, numeric_size, count, types, names, fields;
    unsigned short properties = 0, length, i;
    unsigned long value;
    entry->flags |= 0x400;
    old_length = *(unsigned short *)(old+1);
    end = old+old_length+3;
    emit_udt_symbols = 1;
    value = read_old_numeric(&cursor, NULL);
    if (value) value = ((value-1)>>3)+1;
    numeric_size = write_numeric_leaf(numeric, value);
    value = read_old_numeric(&cursor, NULL);
    count = value > 0xffff ? 0xffff : (unsigned short)value;
    types = names = 0xffff;
    if (*cursor++ == 0x83) {
        types = *(unsigned short *)cursor;
        cursor += 2;
    }
    if (*cursor++ == 0x83) {
        names = *(unsigned short *)cursor;
        cursor += 2;
    }
    fields = convert_old_field_list(types, names, count);
    name[0] = 0;
    if (cursor < end) {
        if (*cursor == 0x82) {
            ++cursor;
            for (i = 0; i < (unsigned short)*cursor+1; ++i) name[i] = cursor[i];
            cursor += *cursor+1;
        }
        if (cursor < end) properties = *cursor == 0x68 ? 1 : 0;
    }
    length = (unsigned short)(name[0]+numeric_size+13);
    if ((unsigned long)old_length+1 >= length) record = old;
    else record = resize_old_type(entry, (unsigned short)(length+2));
    *(unsigned short *)record = length;
    *(unsigned short *)(record+2) = 5;
    *(unsigned short *)(record+4) = count;
    *(unsigned short *)(record+6) = fields;
    *(unsigned short *)(record+8) = properties;
    *(unsigned short *)(record+10) = 0;
    *(unsigned short *)(record+12) = 0;
    for (i = 0; i < numeric_size; ++i) record[14+i] = numeric[i];
    record += 14+numeric_size;
    for (i = 0; i < (unsigned short)name[0]+1; ++i) record[i] = name[i];
}

/* Join the separate C6 type and counted-name/offset lists into LF_FIELDLIST. */
unsigned short convert_old_field_list(unsigned short types, unsigned short names,
                                     unsigned short count)
{
    TypeRecord *entry, *name_entry;
    unsigned short relative, unused, allocation, i, remaining;
    unsigned char *type_cursor, *name_cursor, *after_name, *cursor, *start, *record;
    unsigned long bytes, n, padding;
#define OLD_FIELD_ENTRY(destination,index) \
    do { \
        relative = (unsigned short)((index)-0x200); \
        if (!type_signature && relative == (unsigned short)(0xffff-first_user_type)) \
            (destination) = empty_type; \
        else { \
            (destination) = &local_type_records[relative]; \
            if ((destination)->flags & 0x1000) \
                (destination) = get_type_record((unsigned short)((destination)->forward_index-0x1000), &unused); \
        } \
    } while (0)
    OLD_FIELD_ENTRY(entry, types);
    OLD_FIELD_ENTRY(name_entry, names);
    type_cursor = entry->data+4;
    name_cursor = name_entry->data;
    allocation = (unsigned short)(count*10+*(unsigned short *)(name_cursor+1)+6);
    name_cursor += 4;
    cursor = type_scratch(allocation);
    while ((unsigned long)cursor&3) ++cursor;
    start = cursor;
    cursor += 2;
    *(unsigned short *)cursor = 0x204;
    cursor += 2;
    remaining = count;
    while (remaining) {
        ++name_cursor;
        *(unsigned short *)cursor = 0x406;
        cursor += 2;
        if (*type_cursor++ == 0x83) {
            *(unsigned short *)cursor = *(unsigned short *)type_cursor;
            type_cursor += 2;
        } else *(unsigned short *)cursor = 0xffff;
        cursor += 2;
        *(unsigned short *)cursor = 3;
        cursor += 2;
        after_name = name_cursor+*name_cursor+1;
        convert_old_numeric_leaf(&after_name, &cursor);
        for (i = 0; i < (unsigned short)*name_cursor+1; ++i) *cursor++ = name_cursor[i];
        name_cursor = after_name;
        padding = (unsigned long)cursor&3;
        if (padding) {
            *cursor++ = (unsigned char)(0xf4-padding);
            while ((unsigned long)cursor&3) *cursor++ = 0;
        }
        --remaining;
    }
    bytes = cursor-start;
    *(unsigned short *)start = (unsigned short)(bytes-2);
    record = resize_old_type(entry, (unsigned short)bytes);
    entry->flags |= 0x400;
    for (n = 0; n < (unsigned short)bytes; ++n) record[n] = start[n];
    return types;
#undef OLD_FIELD_ENTRY
}

/* Translate a C6 numeric/string leaf, advancing both counted cursors. */
unsigned short convert_old_numeric_leaf(unsigned char **source,
                                       unsigned char **destination)
{
    unsigned char *input = *source, *output = *destination;
    unsigned char kind = *input++;
    unsigned short length = 0, count, value, i;
    if (kind < 0x80) {
        *(unsigned short *)output = kind;
        *source = input;
        *destination = output+2;
        return 2;
    }
    switch (kind) {
    case 0x82:                     /* Counted string. */
        count = *input++;
        *(unsigned short *)output = 0x8010;
        *(unsigned short *)(output+2) = count;
        output += 4;
        for (i = 0; i < count; ++i) *output++ = *input++;
        /* Preserve the original's count, one less than its written size. */
        length = count+3;
        break;
    case 0x85:                     /* Unsigned short, shortened when possible. */
        value = *(unsigned short *)input;
        if (value >= 0x8000) {
            *(unsigned short *)output = 0x8002;
            output += 2;
            length = 4;
        } else length = 2;
        *(unsigned short *)output = value;
        output += 2;
        input += 2;
        break;
    case 0x86:                     /* Unsigned long. */
        *(unsigned short *)output = 0x8004;
        *(unsigned long *)(output+2) = *(unsigned long *)input;
        input += 4;
        output += 6;
        length = 6;
        break;
    case 0x88:                     /* Signed char, retain its raw two's complement. */
        *(unsigned short *)output = 0x8000;
        output[2] = *input++;
        output += 3;
        length = 3;
        break;
    case 0x89:                     /* Signed short. */
        *(unsigned short *)output = 0x8001;
        *(unsigned short *)(output+2) = *(unsigned short *)input;
        input += 2;
        output += 4;
        length = 4;
        break;
    case 0x8a:                     /* Signed long. */
        *(unsigned short *)output = 0x8003;
        *(unsigned long *)(output+2) = *(unsigned long *)input;
        input += 4;
        output += 6;
        length = 6;
        break;
    default:
        /* The original returns an uninitialized count for unsupported kinds.
           Keep this unwritten value deterministically zero. */
        break;
    }
    *destination = output;
    *source = input;
    return length;
}

/* C6 NIL becomes the two-byte LF_NULL leaf. */
void convert_old_leaf_80(TypeRecord *entry)
{
    unsigned char *record = entry->data;
    entry->flags |= 0x400;
    *(unsigned short *)record = 2;
    *(unsigned short *)(record+2) = 0x0f;
}

void convert_old_leaf_8e(TypeRecord *entry)
{
    unsigned char *record = entry->data;
    entry->flags |= 0x400;
    *(unsigned short *)record = 2;
    *(unsigned short *)(record+2) = 0x10;
}

void convert_old_leaf_72(TypeRecord *entry)
{
    unsigned char *old = entry->data, *record;
    entry->flags |= 0x400;
    record = checked_alloc(6);
    *(unsigned short *)record = 4;
    *(unsigned short *)(record+2) = 0x0e;
    *(unsigned short *)(record+4) = old[5] == 0x73 ? 4 : 0;
    if (entry->flags & 0x38) release_type_data(entry);
    entry->flags |= 8;
    entry->data = record;
}

void convert_old_leaf_5c(TypeRecord *entry)
{
    unsigned char *old = entry->data+4, *record;
    unsigned short type;
    entry->flags |= 0x400;
    record = checked_alloc(8);
    *(unsigned short *)record = 6;
    *(unsigned short *)(record+2) = 0x206;
    record[4] = old[0];
    type = old[0] <= 16 ? 0x11 : 0x12;
    if (old[1] != 0x7d) type += 0x10;
    *(unsigned short *)(record+6) = type;
    record[5] = old[2];
    if (entry->flags & 0x38) release_type_data(entry);
    entry->flags |= 8;
    entry->data = record;
}

/* Scalar arrays use a fixed scalar element/index pair and a numeric size. */
void convert_old_leaf_8d(TypeRecord *entry)
{
    unsigned char *cursor = entry->data, *record;
    unsigned char numeric[8];
    unsigned short consumed, size, i;
    unsigned long value;
    entry->flags |= 0x400;
    if (cursor[4] == 0) {
        cursor += 5;
        value = read_old_numeric(&cursor, &consumed);
        size = write_numeric_leaf(numeric, value);
        record = resize_old_type(entry, (unsigned short)(size+9));
        *(unsigned short *)record = size+7;
        *(unsigned short *)(record+2) = 3;
        *(unsigned short *)(record+4) = 0x10;
        *(unsigned short *)(record+6) = 0x21;
        for (i = 0; i < size; ++i) record[8+i] = numeric[i];
        record[8+size] = 0;
    } else if (cursor[4] == 1) {
        *(unsigned short *)cursor = 2;
        *(unsigned short *)(cursor+2) = 0x10;
    }
}

/* C6 based-pointer forms carry an optional segment/type/symbol extension. */
void convert_old_based_pointer(TypeRecord *entry)
{
    unsigned char *old = entry->data, *record;
    unsigned char extra[260];
    unsigned short old_length = *(unsigned short *)(old+1);
    unsigned short referent = *(unsigned short *)(old+5);
    unsigned short attributes = 0, extra_length = 0, length, i;
    int symbol = 0;
    entry->flags |= 0x400;
    switch (old[7]) {
    case 0x81: attributes = 9; break;
    case 0x83:
        attributes = 8;
        *(unsigned short *)extra = 0x9c;
        extra[2] = 0;
        extra_length = 3;
        break;
    case 0x92:
        attributes = 3;
        *(unsigned short *)extra = *(unsigned short *)(old+8);
        extra_length = 2;
        break;
    case 0x93: attributes = 4; symbol = 1; break;
    case 0x94: attributes = 5; symbol = 1; break;
    case 0x97: attributes = 6; symbol = 1; break;
    case 0x98: attributes = 7; symbol = 1; break;
    }
    if (symbol)
        extra_length += convert_old_based_symbol(extra+extra_length,
                         get_old_symbol(*(unsigned short *)(old+8)));
    /* The original leaves reserved attribute bit0x0800 unwritten; also an
       unknown encoding leaves its pointer mode unwritten. Emit zeroes. */
    attributes |= (unsigned short)(current_module_pe&1)<<8;
    length = extra_length+6;
    if ((unsigned long)old_length+1 >= length) record = old;
    else record = resize_old_type(entry, (unsigned short)(length+2));
    *(unsigned short *)record = length;
    *(unsigned short *)(record+2) = 2;
    *(unsigned short *)(record+4) = attributes;
    *(unsigned short *)(record+6) = referent;
    for (i = 0; i < extra_length; ++i) record[8+i] = extra[i];
}

/* Preserve the opaque C6 payload under the LF_COBOL0 leaf. */
void convert_old_modifier(TypeRecord *entry)
{
    unsigned char *old = entry->data, *record;
    unsigned short length = *(unsigned short *)(old+1);
    types_compacted = 1;
    entry->flags |= 0x400;
    record = checked_alloc((unsigned short)(length+3));
    *(unsigned short *)record = length+1;
    *(unsigned short *)(record+2) = 0x0b;
    memcpy(record+4, old+4, (unsigned long)length-1);
    if (entry->flags & 0x38) release_type_data(entry);
    entry->flags |= 8;
    entry->data = record;
}
