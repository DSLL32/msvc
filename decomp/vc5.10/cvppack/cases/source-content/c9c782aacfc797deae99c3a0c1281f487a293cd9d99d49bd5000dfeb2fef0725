/* Compare recursive records apart from their pending indices [00408650,004088f5). */
#include "types.h"
#include "pack.h"
#include <recon.h>

int __fastcall compare_recursive_type(TypeRecord *record, unsigned index,
                                    unsigned char *other_data, unsigned other_index)
{
    unsigned char *data = record->data, *global_data;
    TypeRecord *target;
    union { unsigned global_index; unsigned short length; } saved;
    unsigned short cursor = 0, ordinal = 0, offset;
    unsigned value, slot, global_index, bits, mask;
    int matches;
    record->status |= 1;
    record->mapped_index = other_index;
    saved.length = (unsigned short)(*(unsigned short *)data + 2);
    while (cursor < saved.length) {
        while (ordinal != record->reference_count) {
            bits = record->flags & 1 ? record->reference_flags[ordinal >> 3] :
                record->reference_offsets[8];
            mask = 0x80 >> (record->flags & 1 ? ordinal & 7 : ordinal);
            if (!(bits & mask)) break;
            ++ordinal;
        }
        offset = ordinal == record->reference_count ? saved.length :
            record->flags & 1 ? record->extended_references[ordinal++] :
                record->reference_offsets[ordinal++];
        if ((unsigned short)(offset - cursor) &&
            memcmp(data + cursor, other_data + cursor, (unsigned short)(offset - cursor)))
            goto different;
        cursor = (unsigned short)(offset + 4);
    }
    for (ordinal = 0; ordinal < record->reference_count; ++ordinal) {
        bits = record->flags & 1 ? record->reference_flags[ordinal >> 3] :
            record->reference_offsets[8];
        mask = 0x80 >> (record->flags & 1 ? ordinal & 7 : ordinal);
        if (bits & mask) continue;
        offset = record->flags & 1 ? record->extended_references[ordinal] :
            record->reference_offsets[ordinal];
        value = *(unsigned *)(data + offset);
        slot = value - module_type_base;
        if (!module_type_low && slot == 0xffff - module_type_base) target = empty_type_record;
        else {
            target = &type_records[slot];
            if (target->status & 0x1000)
                /* The first-pass length is dead; its DWORD slot holds this output. */
                target = lookup_global_type(target->global_index - 0x1000,
                                            &saved.global_index);
        }
        global_index = *(unsigned *)(other_data + offset);
        if (global_index < module_type_base) goto different;
        global_data = global_type_blocks[(global_index - 0x1000) >> 9][global_index & 511];
        if (target->status & 1) matches = target->mapped_index == global_index;
        else if (target->status & 2) matches = 1;
        else if (target->status & 4) matches = 1;
        else matches = compare_recursive_type(target, value, global_data, global_index);
        if (!matches) goto different;
    }
    record->status = (record->status & ~1) | 2;
    return 1;

different:
    record->status &= ~1;
    return 0;
}
