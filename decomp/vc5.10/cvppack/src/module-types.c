/* Stream CodeView type records into segmented storage [0040c100,0040c68f). */
#include "files.h"
#include "pack.h"
#include "widen.h"
#include <recon.h>

unsigned type_record_capacity, source_type_input_bytes, current_type_server;
void read_new_module_types(unsigned size, int precompiled)
{
    unsigned processed = 0, carried = 0, available, amount, raw_length;
    unsigned old_count, skip, index, clear_bytes, prior_available, flag;
    unsigned char *raw, *end, *record, *converted = NULL, *output;
    WidenTi *object;
    TypeRecord *entry;
    int widen = module_type_low == 1;
    if (widen) {
        if (!create_widen(&object)) fatal_error(1001, NULL, NULL);
        output = converted = allocate_bytes(0xffe8);
        available = 0xffe8;
    }
    initialize_module_type_storage();
    type_segment_index = 0;
    while (processed < size) {
        raw = type_segments[type_segment_index];
        amount = size - processed;
        if (amount >= 0xffe8) amount = 0xffe8;
        amount -= carried;
        if (read_cached_file(input_file, raw + carried, amount) != amount)
            fatal_error(1007, "Types", module_name(current_module_info));
        end = raw + carried + amount;
        while ((unsigned)(end - raw) >= 4) {
            raw_length = *(unsigned short *)raw;
            if ((unsigned)(end - raw) < raw_length + 2) break;
            if (widen) {
                prior_available = available;
                if (!record_widen(object, output, raw, &available)) {
                    if (available > prior_available) break;
                    fatal_error(1011, module_name(current_module_info), NULL);
                }
                record = output;
                available -= *(unsigned short *)record + 2;
            } else record = raw;
            if (*(unsigned short *)(record + 2) == 0x100e) {
                if (precompiled) fatal_error(1028, module_name(current_module_info), NULL);
                use_precompiled_types(record + 2);
            } else if (*(unsigned short *)(record + 2) == 0x16) {
                register_type_server(record + 2);
                current_type_server = type_record_count + 0x1000;
            } else if (*(unsigned short *)(record + 2) == 0x14) {
                current_type_server = type_record_count + 0x1000;
                find_type_server(NULL, 1, NULL, NULL, NULL);
                current_module_info->fields_before[1] = *(unsigned *)(record + 4);
            } else {
                skip = *(unsigned short *)(record + 2) == 0x1200 ?
                    *(unsigned *)(record + 4) - type_record_count - 0x1000 : 0;
                old_count = type_record_count;
                if (module_precompiled_limit) {
                    clear_bytes = module_precompiled_limit * sizeof(TypeRecord);
                    for (index = 0; index < clear_bytes; ++index) ((unsigned char *)type_records)[index] = 0;
                    module_precompiled_limit = 0;
                }
                type_record_count += skip ? skip : 1;
                if (type_record_count >= type_record_capacity) {
                    type_record_capacity += skip + 6000;
                    type_records = realloc(type_records, type_record_capacity * sizeof(TypeRecord));
                    if (!type_records) fatal_error(1001, NULL, NULL);
                    clear_bytes = (skip + 6000) * sizeof(TypeRecord);
                    for (index = 0; index < clear_bytes; ++index)
                        ((unsigned char *)&type_records[old_count])[index] = 0;
                }
                entry = &type_records[old_count];
                entry->data = record;
                entry->mapped_index = 0;
                entry->status |= 0x400;
                entry->status &= 0xf7ff;
                flag = record && *(unsigned short *)(record + 2) >= 0x1004 &&
                    *(unsigned short *)(record + 2) <= 0x1007 ? (record[6] >> 7) & 1 : 0;
                entry->status = (entry->status & 0xbfff) | (flag << 14);
            }
            raw += raw_length + 2;
            processed += raw_length + 2;
            output = record + *(unsigned short *)record + 2;
        }
        if (processed >= size) break;
        if ((int)type_segment_index + 1 >= (int)type_segment_count)
            fatal_error(1011, module_name(current_module_info), NULL);
        if (!type_segments[type_segment_index + 1]) {
            type_segments[type_segment_index + 1] = malloc(0xffe8);
            if (!type_segments[type_segment_index + 1]) fatal_error(1001, NULL, NULL);
        }
        carried = end - raw;
        memmove(type_segments[type_segment_index + 1], raw, carried);
        ++type_segment_index;
        if (widen) {
            free(type_segments[type_segment_index - 1]);
            type_segments[type_segment_index - 1] = converted;
            output = converted = malloc(0xffe8);
            if (!output) fatal_error(1001, NULL, NULL);
            available = 0xffe8;
        }
    }
    source_type_input_bytes += size;
    if (widen) {
        if (type_segments[type_segment_index] != converted) {
            free(type_segments[type_segment_index]);
            type_segments[type_segment_index] = converted;
        }
        release_widen(object);
        module_type_low = 2;
        line_data = type_segments[0];
    }
}
