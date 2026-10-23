/* Recursive compaction of old-format type indices [004025f0,00402b9e). */
#include "types.h"
#include "pack.h"
#include <recon.h>

unsigned type_record_count, module_type_base;
TypeRecord *type_records;
unsigned short recursive_type_pending, type_recursion_depth;
unsigned type_recursion_chain, type_recursion_stack[256];
int discard_types;

unsigned __fastcall compact_legacy_index(unsigned index)
{
    TypeRecord *record, *target;
    unsigned slot, target_slot, result, previous_chain, value, position;
    unsigned char first_pending = 0, count = 0, global_record;
    unsigned char *data;
    unsigned short flags;
    int scan, limit;
    if (index < 0x200) {
        recursive_type_pending = 0;
        return map_primitive_type(index);
    }
    if (index >= type_record_count + 0x200) {
        recursive_type_pending = 0;
        return 0;
    }
    slot = index - 0x200;
    if (!module_type_low && slot == 0xffff - module_type_base) record = empty_type_record;
    else {
        record = &type_records[slot];
        if (record->status & 0x1000)
            record = lookup_global_type(record->global_index - 0x1000, &global_record);
    }
    flags = record->status;
    if (flags & (0x6 | 0x8000)) {
        recursive_type_pending = 0;
        return record->mapped_index;
    }
    if (flags & 0x80) {
        for (;;) {
            target_slot = record->mapped_index - 0x200;
            if (!module_type_low && target_slot == 0xffff - module_type_base) target = empty_type_record;
            else {
                target = &type_records[target_slot];
                if (target->status & 0x1000)
                    target = lookup_global_type(target->global_index - 0x1000, &first_pending);
            }
            if (!(target->status & 0x80)) break;
            record = target;
        }
        recursive_type_pending = 1;
        result = record->mapped_index;
        if (result) {
            if (!type_recursion_chain) type_recursion_chain = result;
            else {
                limit = type_recursion_depth - 1;
                while (type_recursion_stack[limit] != type_recursion_chain) --limit;
                for (scan = 0; scan < limit; ++scan)
                    if (type_recursion_stack[scan] == result) { type_recursion_chain = result; break; }
            }
        }
        return index;
    }
    if (discard_types) {
        recursive_type_pending = 0;
        record->status |= 2;
        record->mapped_index = 0;
        return 0;
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
    previous_chain = type_recursion_chain;
    type_recursion_chain = 0;
    type_recursion_stack[type_recursion_depth++] = index;
    value = record->data[3];
    if (value == 0x8c || value == 0xa6) {
        convert_legacy_record(slot);
        data = record->data + 2;
        *(unsigned *)(data + 2) = compact_legacy_index(*(unsigned *)(data + 2));
        if (recursive_type_pending) { record->reference_offsets[0] = 4; count = 1; }
    } else if (value == 0x7a || value == 0x91) {
        convert_legacy_record(slot);
        count = compact_legacy_pointer(record);
    } else if (value == 0x79) {
        convert_legacy_record(slot);
        if (match_forward_type(record, &target, 0) == 3) {
            --type_recursion_depth;
            associate_forward_type(index, target);
            recursive_type_pending = 0;
            type_recursion_chain = previous_chain;
            record->status |= 2;
            record->mapped_index = target->global_index;
            return record->mapped_index;
        }
        data = record->data + 2;
        *(unsigned *)(data + 6) = compact_type_list(*(unsigned *)(data + 6), *(unsigned short *)(data + 2));
        if (recursive_type_pending) { record->reference_offsets[0] = 8; count = 1; }
    } else if (value == 0x78) {
        convert_legacy_record(slot);
        data = record->data + 2;
        *(unsigned *)(data + 2) = compact_legacy_index(*(unsigned *)(data + 2));
        if (recursive_type_pending) { record->reference_offsets[0] = 4; first_pending = count = 1; }
        *(unsigned *)(data + 6) = compact_legacy_index(*(unsigned *)(data + 6));
        if (recursive_type_pending) record->reference_offsets[count++] = 8;
    } else if (value == 0x75) {
        convert_legacy_record(slot);
        data = record->data + 2;
        *(unsigned *)(data + 2) = compact_legacy_index(*(unsigned *)(data + 2));
        if (recursive_type_pending) { record->reference_offsets[0] = 4; first_pending = count = 1; }
        *(unsigned *)(data + 10) = compact_type_list(*(unsigned *)(data + 10), *(unsigned short *)(data + 8));
        if (recursive_type_pending) record->reference_offsets[count++] = 12;
    } else if (value == 0x76) {
        record->mapped_index = compact_legacy_index(*(unsigned *)(record->data + 5));
        record->status |= 0x8000;
    } else if (value != 0x5d && value != 0x68 && value != 0x99 && value != 0xa3) {
        convert_legacy_record(slot);
    }
    --type_recursion_depth;
    record->reference_count = count;
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
    if (!(record->status & 0x8000)) {
        record->status |= 4;
        intern_type(record);
    }
    return record->mapped_index;
}

/* Resolve and release queued recursive type patches [00402ea0,00403064). */
PendingType *pending_types_head, *pending_types_tail;
void flush_type_stack(void)
{
    PendingType *pending = pending_types_head, *next;
    TypeRecord *record;
    unsigned char *data, *source;
    unsigned slot, length;
    unsigned short kind;
    unsigned char from_global;
    while (pending) {
        slot = pending->index - module_type_base;
        if (!module_type_low && slot == 0xffff - module_type_base) record = empty_type_record;
        else {
            record = &type_records[slot];
            if (record->status & 0x1000)
                record = lookup_global_type(record->global_index - 0x1000, &from_global);
        }
        data = record->data;
        source = data + 2;
        kind = *(unsigned short *)(data + 2);
        if (kind == 0x1004 || kind == 0x1005) {
            *(unsigned *)(data + 16) = compact_type_index(*(unsigned *)(data + 16));
            if (recursive_type_pending) record->reference_offsets[record->reference_count++] = 16;
            *(unsigned *)(record->data + 8) = compact_type_list(*(unsigned *)(data + 8), *(unsigned short *)(data + 6));
            if (recursive_type_pending) record->reference_offsets[record->reference_count++] = 8;
        } else if (kind == 0x1006) {
            *(unsigned *)(data + 8) = compact_type_list(*(unsigned *)(data + 8), *(unsigned short *)(data + 6));
            if (recursive_type_pending) record->reference_offsets[record->reference_count++] = 8;
        } else if (kind == 0x1007) {
            *(unsigned *)(record->data + 8) = compact_type_index(*(unsigned *)(data + 8));
            if (recursive_type_pending) record->reference_offsets[record->reference_count++] = 8;
            *(unsigned *)(record->data + 12) = compact_type_list(*(unsigned *)(record->data + 12), *(unsigned short *)(record->data + 4));
            if (recursive_type_pending) record->reference_offsets[record->reference_count++] = 12;
        }
        if (!(pending->patch->flags & 1)) {
            length = *(unsigned short *)data;
            if (length >= *(unsigned short *)pending->patch->data) length = *(unsigned short *)pending->patch->data;
            memmove(pending->patch->data + 2, source, length);
            pending->patch->flags |= 1;
        }
        next = pending->next;
        free(pending);
        pending = next;
    }
    pending_types_head = pending_types_tail = NULL;
}
