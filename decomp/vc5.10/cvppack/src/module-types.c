/* Stream CodeView type records into segmented storage [0040c100,0040c68f). */
#include <mbstring.h>
#include "files.h"
#include "pack.h"
#include "widen.h"
#include <recon.h>

#pragma intrinsic(memset, strcpy)

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
                append_module_type(NULL, 1, 0, 0, 0);
                current_module_info->type_server = *(unsigned *)(record + 4);
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

/* Begin a module's type records, retaining the reusable table [0040c720,0040c75d). */
unsigned type_module_count;
void initialize_module_type_storage(void)
{
    ++type_module_count;
    if (!type_records) {
        type_record_capacity = 12000;
        type_records = allocate_cleared(type_record_capacity * sizeof(TypeRecord));
    }
    type_record_count = 0;
}

/* Clear cached names and record associations, keeping bucket chains
 * [0040c760,0040c7a1). */
void reset_type_name_cache(void)
{
    unsigned index;
    CachedTypeName *entry;
    CachedTypeName **buckets = (CachedTypeName **)type_references;
    for (index = 0; index < 512; ++index) {
        entry = buckets[index];
        while (entry) {
            entry->type_index = 0;
            entry->record = NULL;
            free(entry->name);
            entry->name = NULL;
            entry = entry->next;
        }
    }
}

/* Append one local type slot, including LF_ENDPRECOMP placeholders.
 * [0040c7b0,0040c917), fastcall with three DWORD stack arguments.
 * The old unreconstructed name find_type_server did not describe this code. */
void __fastcall append_module_type(unsigned char *data, unsigned local,
    unsigned precompiled, unsigned mapped_index, unsigned skip)
{
    unsigned old_count = type_record_count, flags;
    TypeRecord *entry;
    if (module_precompiled_limit) {
        memset(type_records, 0, module_precompiled_limit * sizeof(TypeRecord));
        module_precompiled_limit = 0;
    }
    type_record_count += skip ? skip : 1;
    if (type_record_count >= type_record_capacity) {
        type_record_capacity += skip + 6000;
        type_records = realloc(type_records, type_record_capacity * sizeof(TypeRecord));
        if (!type_records) fatal_error(1001, NULL, NULL);
        memset(&type_records[old_count], 0, (skip + 6000) * sizeof(TypeRecord));
    }
    entry = &type_records[old_count];
    entry->data = data;
    entry->mapped_index = mapped_index;
    entry->status = (entry->status & ~0x400) | ((local & 1) << 10);
    entry->status = (entry->status & ~0x800) | ((precompiled & 1) << 11);
    flags = data && *(unsigned short *)(data + 2) >= 0x1004 &&
        *(unsigned short *)(data + 2) <= 0x1007 ? (data[6] >> 7) & 1 : 0;
    entry->status = (entry->status & ~0x4000) | (flags << 14);
}

/* Save compacted precompiled indices [00402c90,00402e0e).
 * The following dispatch tables end at00402e46 and are separately owned. */
void prepare_precompiled_types(ModuleInfo *info)
{
    PrecompiledTypeMap *map;
    TypeRecord *record;
    unsigned short index;
    unsigned global_record, value;
    unsigned short kind;
    unsigned char *data;
    info->precompiled_bytes = (current_type_server - 0x1000 + 3) * 4;
    info->precompiled_types = malloc(info->precompiled_bytes);
    if (!info->precompiled_types) fatal_error(1001, NULL, NULL);
    map = info->precompiled_types;
    memset(map, 0, info->precompiled_bytes);
    map->signature = info->type_server;
    map->first = 0x1000;
    map->count = current_type_server - 0x1000;
    for (index = 0; index < current_type_server - 0x1000; ++index) {
        if (!module_type_low && index == 0xffff - module_type_base)
            record = empty_type_record;
        else {
            record = &type_records[index];
            if (record->status & 0x1000)
                record = lookup_global_type(record->global_index - 0x1000, &global_record);
        }
        if (record->status & 6) value = record->mapped_index;
        else {
            data = record->data;
            kind = *(unsigned short *)(data + 2);
            if (kind == 0x1201)
                value = compact_type_list(index + 0x1000, *(unsigned *)(data + 4));
            else if ((kind >= 0x1001 && kind <= 0x100c) || kind == 0x100f ||
                kind == 0x20c || kind == 0x0a || kind == 0x0c ||
                kind == 0x0e || kind == 0x0f || kind == 0x1202 ||
                kind == 0x1205 || (kind >= 0x1207 && kind <= 0x120a))
                value = map_type_index(index + 0x1000);
            else continue;
        }
        map->indices[index] = value;
    }
    current_type_server = 0;
}

/* Restore a module's cached precompiled type indices [0040c930,0040cb67). */
static ModuleInfo *last_precompiled_module;
static unsigned last_precompiled_sequence, last_precompiled_count;
void use_precompiled_types(void *record)
{
    unsigned char *leaf = record;
    ModuleInfo *info = find_precompiled_module(leaf + 14);
    PrecompiledTypeMap *map = info->precompiled_types;
    char module[260];
    unsigned short index;
    if (!map) fatal_error(1001, NULL, NULL);
    if (info->type_server != *(unsigned *)(leaf + 10)) {
        strcpy(module, module_name(current_module_info));
        fatal_error(1021, module, module_name(info));
    }
    if (map->count != *(unsigned *)(leaf + 6) ||
        type_record_count + 0x1000 != *(unsigned *)(leaf + 2)) {
        strcpy(module, module_name(current_module_info));
        fatal_error(1018, module, module_name(info));
    }
    if (type_module_count == last_precompiled_sequence + 1 &&
        info == last_precompiled_module) {
        last_precompiled_sequence = type_module_count;
        type_record_count = last_precompiled_count;
        if (module_precompiled_limit > last_precompiled_count)
            memset(&type_records[last_precompiled_count], 0,
                (module_precompiled_limit - last_precompiled_count) * sizeof(TypeRecord));
        module_precompiled_limit = 0;
        return;
    }
    if (module_precompiled_limit) {
        memset(type_records, 0, module_precompiled_limit * sizeof(TypeRecord));
        module_precompiled_limit = 0;
    }
    for (index = 0; index < *(unsigned *)(leaf + 6); ++index) {
        type_records[type_record_count].mapped_index = map->indices[index];
        type_records[type_record_count].status |= 0xc00;
        ++type_record_count;
    }
    last_precompiled_module = info;
    last_precompiled_sequence = type_module_count;
    last_precompiled_count = type_record_count;
}

/* Match LF_PRECOMP's counted object name [0040cb70,0040cc01).
 * These are the original multibyte CRT calls, including signed-char length. */
ModuleInfo *find_precompiled_module(const unsigned char *name)
{
    ModuleInfo *info = module_list;
    unsigned char object[260];
    int length = (signed char)name[0];
    while (info) {
        if (info->precompiled_object_name && name[0] == info->precompiled_object_name[0] &&
            !_mbsnbicmp(name + 1, info->precompiled_object_name + 1, length))
            return info;
        info = info->next;
    }
    _mbsnbcpy(object, name + 1, length);
    object[length] = 0;
    fatal_error(1017, (char *)object, NULL);
    return info;
}
