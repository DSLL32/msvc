/* Recursive hash ignores embedded reference DWORDs [0040e640,0040e809). */
#include "types.h"
#include "pack.h"
#include <recon.h>

unsigned __fastcall hash_recursive_type(TypeRecord *record)
{
    unsigned char *data = record->data, *name = NULL;
    unsigned short hash, kind, ordinal;
    unsigned limit, cursor, stop, flags;
    int numeric = 0, count, i;
    if (record->recursive_hash) return record->recursive_hash;
    record->recursive_hash = 0xfff;
    kind = *(unsigned short *)(data + 2);
    hash = kind;
    switch (kind) {
    case 0x1004: case 0x1005: name = data + 20; numeric = 1; break;
    case 0x1006: name = data + 12; numeric = 1; break;
    case 0x1007: name = data + 16; break;
    case 0x1405: name = data + 10; numeric = 1; break;
    case 0x1406: name = data + 10; break;
    }
    if (name) {
        if (numeric) name += *(unsigned short *)name < 0x8000 ? 2 : numeric_leaf_size(name);
        /* Preserve the original loop's count minus one bound. */
        count = *name++ - 1;
        while (count-- > 0) {
            hash = (unsigned short)((hash << 2) ^ (hash >> 14) ^ *name);
            ++name;
        }
    } else {
        limit = *(unsigned short *)data + 2;
        cursor = 0;
        ordinal = 0;
        while (cursor < limit) {
            stop = limit;
            if (ordinal != record->reference_count) {
                stop = record->flags & 1 ? record->extended_references[ordinal] :
                    record->reference_offsets[ordinal];
                ++ordinal;
            }
            while (cursor < stop) {
                hash = (unsigned short)((hash << 2) ^ (hash >> 14) ^ data[cursor]);
                ++cursor;
            }
            cursor += 4;
        }
        i = record->reference_count < 3 ? record->reference_count : 3;
        while (--i >= 0) {
            flags = record->flags & 1 ? record->reference_flags[i >> 3] :
                record->reference_offsets[8];
            if (!(flags & (0x80 >> (i & 7))))
                hash ^= hash_type_reference(record, (unsigned short)i);
        }
    }
    record->recursive_hash = hash & 0xfff;
    return record->recursive_hash;
}

/* Follow one recursive reference [0040e820,0040e8c4). */
unsigned short __fastcall hash_type_reference(TypeRecord *record, unsigned short ordinal)
{
    unsigned short offset;
    unsigned value, slot;
    TypeRecord *target;
    offset = record->flags & 1 ? record->extended_references[ordinal] :
        record->reference_offsets[ordinal];
    value = *(unsigned *)(record->data + offset);
    if (value < module_type_base) return (unsigned short)value;
    slot = value - module_type_base;
    if (!module_type_low && slot == 0xffff - module_type_base) target = empty_type_record;
    else {
        target = &type_records[slot];
        if (target->status & 0x1000)
            /* Original low-byte output aliases the saved ordinal WORD. */
            target = lookup_global_type(target->global_index - 0x1000,
                                        (unsigned char *)&ordinal);
    }
    if (target->status & 4) return target->recursive_hash;
    return (unsigned short)hash_recursive_type(target);
}
