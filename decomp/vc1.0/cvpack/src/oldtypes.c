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
