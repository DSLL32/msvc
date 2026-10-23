/* Recursive argument/method/field-list compaction [00403c20,00404330). */
#include "types.h"
#include "pack.h"
#include <recon.h>

TypeIndexMapper type_index_mappers[3] = {
    compact_legacy_index, compact_type_index, compact_type_index
};

unsigned __fastcall compact_type_list(unsigned index, unsigned count)
{
    TypeRecord *record, *target;
    TypeIndexMapper map;
    unsigned slot, target_slot, value, previous_chain, offset, remaining, i;
    unsigned short flags, kind, field_kind, reference, attributes;
    unsigned char from_global;
    unsigned char *data, *field;
    int limit, scan;
    slot = index - module_type_base;
    if (!module_type_low && slot == 0xffff - module_type_base) record = empty_type_record;
    else {
        record = &type_records[slot];
        if (record->status & 0x1000)
            record = lookup_global_type(record->global_index - 0x1000, &from_global);
    }
    flags = record->status;
    if (flags & (0x6 | 0x0800)) {
        recursive_type_pending = 0;
        return record->mapped_index;
    }
    if (flags & 0x80) {
        for (;;) {
            target_slot = record->mapped_index - 0x1000;
            if (!module_type_low && target_slot == 0xffff - module_type_base) target = empty_type_record;
            else {
                target = &type_records[target_slot];
                if (target->status & 0x1000)
                    target = lookup_global_type(target->global_index - 0x1000, &from_global);
            }
            if (!(target->status & 0x80)) break;
            record = target;
        }
        recursive_type_pending = 1;
        value = record->mapped_index;
        if (value) {
            if (!type_recursion_chain) type_recursion_chain = value;
            else {
                limit = type_recursion_depth - 1;
                while (type_recursion_stack[limit] != type_recursion_chain) --limit;
                for (scan = 0; scan < limit; ++scan)
                    if (type_recursion_stack[scan] == value) { type_recursion_chain = value; break; }
            }
        }
        return index;
    }
    if (flags & 0x40) {
        recursive_type_pending = 1;
        if (index) {
            if (!type_recursion_chain) type_recursion_chain = index;
            else {
                limit = type_recursion_depth - 1;
                while (type_recursion_stack[limit] != type_recursion_chain) --limit;
                for (scan = 0; scan < limit; ++scan)
                    if (type_recursion_stack[scan] == index) { type_recursion_chain = index; break; }
            }
        }
        return index;
    }
    record->status = flags | 0x40;
    type_recursion_stack[type_recursion_depth++] = index;
    previous_chain = type_recursion_chain;
    type_recursion_chain = 0;
    map = type_index_mappers[module_type_low];
    record->reference_count = 0;
    data = record->data + 2;
    kind = *(unsigned short *)data;
    if (kind == 0x1201) {
        for (i = 0; i < count; ++i) {
            value = map(*(unsigned *)(data + 6 + i * 4));
            *(unsigned *)(record->data + 8 + i * 4) = value;
            data = record->data + 2;
            if (recursive_type_pending) add_type_reference(record, (unsigned short)(i * 4 + 8));
        }
    } else if (kind == 0x1206) {
        offset = 0;
        for (remaining = count; remaining; --remaining) {
            value = map(*(unsigned *)(record->data + 8 + (unsigned short)offset));
            *(unsigned *)(record->data + 8 + (unsigned short)offset) = value;
            field = record->data + 4 + (unsigned short)offset;
            reference = (unsigned short)(field - record->data + 4);
            attributes = *(unsigned short *)field & 0x1c;
            offset += attributes == 0x10 || attributes == 0x18 ? 12 : 8;
            if (reference && recursive_type_pending) add_type_reference(record, reference);
        }
    } else if (kind == 0x1203) {
        offset = 0;
        remaining = count;
        while (remaining) {
            data = record->data + 4;
            field = data + (unsigned short)offset;
            field_kind = *(unsigned short *)field;
            reference = 0;
            if (field_kind == 0x0403) {
                offset += 4;
                field = data + (unsigned short)offset;
                offset += *(unsigned short *)field < 0x8000 ? 2 : numeric_leaf_size(field);
                offset += data[(unsigned short)offset] + 1;
            } else if (field_kind == 0x1400 || field_kind == 0x1405) {
                value = map(*(unsigned *)(field + 4));
                data = record->data + 4;
                field = data + (unsigned short)offset;
                *(unsigned *)(field + 4) = value;
                reference = (unsigned short)(field - record->data + 4);
                offset += 8;
                field = data + (unsigned short)offset;
                offset += *(unsigned short *)field < 0x8000 ? 2 : numeric_leaf_size(field);
                if (field_kind == 0x1405) offset += data[(unsigned short)offset] + 1;
            } else if (field_kind == 0x1401 || field_kind == 0x1402) {
                value = map(*(unsigned *)(field + 4));
                field = record->data + 4 + (unsigned short)offset;
                *(unsigned *)(field + 4) = value;
                if (recursive_type_pending)
                    add_type_reference(record, (unsigned short)(field - record->data + 4));
                value = map(*(unsigned *)(field + 8));
                data = record->data + 4;
                field = data + (unsigned short)offset;
                *(unsigned *)(field + 8) = value;
                if (recursive_type_pending)
                    add_type_reference(record, (unsigned short)(field - record->data + 8));
                offset += 12;
                field = data + (unsigned short)offset;
                offset += *(unsigned short *)field < 0x8000 ? 2 : numeric_leaf_size(field);
                field = data + (unsigned short)offset;
                offset += *(unsigned short *)field < 0x8000 ? 2 : numeric_leaf_size(field);
            } else if (field_kind == 0x1403 || field_kind == 0x1406 || field_kind == 0x1408) {
                value = map(*(unsigned *)(field + 4));
                data = record->data + 4;
                field = data + (unsigned short)offset;
                *(unsigned *)(field + 4) = value;
                reference = (unsigned short)(field - record->data + 4);
                offset += field[8] + 9;
            } else if (field_kind == 0x1404) {
                append_type_field_list(field, record);
                data = record->data + 4;
                ++remaining;
            } else if (field_kind == 0x1407) {
                value = compact_type_list(*(unsigned *)(field + 4), *(unsigned short *)(field + 2));
                data = record->data + 4;
                field = data + (unsigned short)offset;
                *(unsigned *)(field + 4) = value;
                reference = (unsigned short)(field - record->data + 4);
                remaining += 1 - *(unsigned short *)(field + 2);
                offset += field[8] + 9;
            } else if (field_kind == 0x1409 || field_kind == 0x140a) {
                value = map(*(unsigned *)(field + 4));
                data = record->data + 4;
                field = data + (unsigned short)offset;
                *(unsigned *)(field + 4) = value;
                reference = (unsigned short)(field - record->data + 4);
                offset += 8;
            } else if (field_kind == 0x140b) {
                value = map(*(unsigned *)(field + 4));
                data = record->data + 4;
                field = data + (unsigned short)offset;
                *(unsigned *)(field + 4) = value;
                reference = (unsigned short)(field - record->data + 4);
                attributes = *(unsigned short *)(field + 2) & 0x1c;
                offset += 8;
                if (attributes == 0x10 || attributes == 0x18) offset += 4;
                offset += data[(unsigned short)offset] + 1;
            }
            if (remaining > 1 && data[(unsigned short)offset] >= 0xf0)
                offset += data[(unsigned short)offset] & 15;
            if (reference && recursive_type_pending) add_type_reference(record, reference);
            --remaining;
        }
    }
    --type_recursion_depth;
    if (type_recursion_chain == index) {
        recursive_type_pending = 0;
        type_recursion_chain = previous_chain;
        record->mapped_index = resolve_recursive_type(record, index);
        return record->mapped_index;
    }
    if (type_recursion_chain) {
        recursive_type_pending = 1;
        record->mapped_index = type_recursion_chain;
        if (previous_chain) {
            if (!type_recursion_chain) type_recursion_chain = previous_chain;
            else {
                limit = type_recursion_depth - 1;
                while (type_recursion_stack[limit] != type_recursion_chain) --limit;
                for (scan = 0; scan < limit; ++scan)
                    if (type_recursion_stack[scan] == previous_chain) { type_recursion_chain = previous_chain; break; }
            }
        }
        record->status |= 0x80;
        return index;
    }
    recursive_type_pending = 0;
    type_recursion_chain = previous_chain;
    record->status |= 4;
    intern_type(record);
    return record->mapped_index;
}
