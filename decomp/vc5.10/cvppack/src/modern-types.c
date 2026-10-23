/* Modern CodeView type compaction [004030a0,00403a09). */
#include "types.h"
#include "pack.h"
#include <recon.h>

unsigned __fastcall compact_type_index(unsigned index)
{
    TypeRecord *record, *target;
    CachedTypeName *named_target;
    unsigned slot, target_slot, result, global_index = 0, previous_chain;
    unsigned value, count;
    unsigned char from_global;
    unsigned char *data, *saved_data;
    unsigned short flags, kind;
    int limit, scan, match;

    if (index < 0x1000) {
        recursive_type_pending = 0;
        return index;
    }
    if (index >= type_record_count + 0x1000) {
        recursive_type_pending = 0;
        return 7;
    }
    slot = index - 0x1000;
    if (!module_type_low && slot == 0xffff - module_type_base) record = empty_type_record;
    else {
        record = &type_records[slot];
        if (record->status & 0x1000) {
            global_index = record->global_index;
            record = lookup_global_type(global_index - 0x1000, &from_global);
        }
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
        if (discard_types) {
            recursive_type_pending = 0;
            target->status |= 2;
            target->mapped_index = 0;
            return 0;
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
        value = global_index ? global_index : index;
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
    record->status = flags | 0x40;
    type_recursion_stack[type_recursion_depth++] = index;
    previous_chain = type_recursion_chain;
    type_recursion_chain = 0;
    data = record->data + 2;
    kind = *(unsigned short *)data;

    if (kind == 0xa || kind == 0xc || kind == 0xe || kind == 0xf ||
        kind == 0x20c || kind == 0x1205) {
        /* These leaves contain no type indices. */
    } else if (kind == 0x1001 || kind == 0x100a || kind == 0x100b ||
               kind == 0x1202 || kind == 0x1207 || kind == 0x1208) {
        result = compact_type_index(*(unsigned *)(data + 2));
        *(unsigned *)(record->data + 4) = result;
        if (recursive_type_pending) record->reference_offsets[record->reference_count++] = 4;
    } else if (kind == 0x1002) {
        compact_legacy_pointer(record);
    } else if (kind == 0x1003 || kind == 0x100c) {
        result = compact_type_index(*(unsigned *)(data + 2));
        *(unsigned *)(record->data + 4) = result;
        if (recursive_type_pending) record->reference_offsets[record->reference_count++] = 4;
        result = compact_type_index(*(unsigned *)(record->data + 8));
        *(unsigned *)(record->data + 8) = result;
        if (recursive_type_pending) record->reference_offsets[record->reference_count++] = 8;
    } else if (kind == 0x1004 || kind == 0x1005 || kind == 0x1006 || kind == 0x1007) {
        *(unsigned short *)(data + 4) &= 0xfffe;
        if (data[4] & 0x80) {
            match = match_forward_type(record, &named_target, 1);
            if (match == 1) {
                --type_recursion_depth;
                record->status |= 0x1000;
                record->global_index = named_target->type_index;
                type_recursion_chain = previous_chain;
                return compact_type_index(named_target->type_index);
            }
            if (match == 2 || match == 3) {
                --type_recursion_depth;
                recursive_type_pending = 0;
                record->status |= 0x82;
                record->mapped_index = named_target->type_index;
                type_recursion_chain = previous_chain;
                return named_target->type_index;
            }
        } else {
            if (match_forward_type(record, &named_target, 0) == 3) {
                --type_recursion_depth;
                associate_forward_type(index, named_target);
                recursive_type_pending = 0;
                type_recursion_chain = previous_chain;
                record->status |= 2;
                record->mapped_index = named_target->type_index;
                return record->mapped_index;
            }
            if (kind == 0x1004 || kind == 0x1005) {
                saved_data = record->data;
                result = compact_type_index(*(unsigned *)(saved_data + 16));
                *(unsigned *)(saved_data + 16) = result;
                if (recursive_type_pending) record->reference_offsets[record->reference_count++] = 16;
                result = compact_type_list(*(unsigned *)(saved_data + 8), *(unsigned short *)(saved_data + 4));
                *(unsigned *)(record->data + 8) = result;
                if (recursive_type_pending) record->reference_offsets[record->reference_count++] = 8;
            } else if (kind == 0x1006) {
                saved_data = record->data;
                result = compact_type_list(*(unsigned *)(saved_data + 8), *(unsigned short *)(saved_data + 4));
                *(unsigned *)(saved_data + 8) = result;
                if (recursive_type_pending) record->reference_offsets[record->reference_count++] = 8;
            } else {
                result = compact_type_index(*(unsigned *)(record->data + 8));
                *(unsigned *)(record->data + 8) = result;
                if (recursive_type_pending) record->reference_offsets[record->reference_count++] = 8;
                result = compact_type_list(*(unsigned *)(record->data + 12), *(unsigned short *)(record->data + 4));
                *(unsigned *)(record->data + 12) = result;
                if (recursive_type_pending) record->reference_offsets[record->reference_count++] = 12;
            }
        }
    } else if (kind == 0x1008) {
        result = compact_type_index(*(unsigned *)(data + 2));
        *(unsigned *)(record->data + 4) = result;
        if (recursive_type_pending) record->reference_offsets[record->reference_count++] = 4;
        result = compact_type_list(*(unsigned *)(record->data + 12), *(unsigned short *)(record->data + 10));
        *(unsigned *)(record->data + 12) = result;
        if (recursive_type_pending) record->reference_offsets[record->reference_count++] = 12;
    } else if (kind == 0x1009) {
        result = compact_type_index(*(unsigned *)(data + 2));
        *(unsigned *)(record->data + 4) = result;
        if (recursive_type_pending) record->reference_offsets[record->reference_count++] = 4;
        result = compact_type_index(*(unsigned *)(record->data + 8));
        *(unsigned *)(record->data + 8) = result;
        if (recursive_type_pending) record->reference_offsets[record->reference_count++] = 8;
        result = compact_type_index(*(unsigned *)(record->data + 12));
        *(unsigned *)(record->data + 12) = result;
        if (recursive_type_pending) record->reference_offsets[record->reference_count++] = 12;
        result = compact_type_list(*(unsigned *)(record->data + 20), *(unsigned short *)(record->data + 18));
        *(unsigned *)(record->data + 20) = result;
        if (recursive_type_pending) record->reference_offsets[record->reference_count++] = 20;
    } else if (kind == 0x100f) {
        count = *(unsigned *)(data + 6);
        while (count) {
            result = compact_type_index(*(unsigned *)(data + 10 + count * 4));
            data = record->data + 2;
            *(unsigned *)(data + 10 + count * 4) = result;
            /* The original stores a four-bit shift, including byte wrap. */
            if (recursive_type_pending) record->reference_offsets[record->reference_count++] = (unsigned char)((count << 4) + 12);
            --count;
        }
    } else if (kind == 0x1209 || kind == 0x120a) {
        result = compact_type_index(*(unsigned *)(data + 6));
        *(unsigned *)(record->data + 8) = result;
        if (recursive_type_pending) record->reference_offsets[record->reference_count++] = 8;
        data = record->data + 2;
        count = *(unsigned *)(data + 2);
        if (*(unsigned short *)data == 0x120a) count *= 2;
        while (count) {
            --count;
            result = compact_type_index(*(unsigned *)(data + 10 + count * 4));
            data = record->data + 2;
            *(unsigned *)(data + 10 + count * 4) = result;
            if (recursive_type_pending) record->reference_offsets[record->reference_count++] = (unsigned char)((count << 4) + 12);
        }
    } else {
        fatal_error(1011, module_name(current_module_info), NULL);
    }
    --type_recursion_depth;
    if (type_recursion_chain == index) {
        recursive_type_pending = 0;
        record->mapped_index = resolve_recursive_type(record, index);
        type_recursion_chain = previous_chain;
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
