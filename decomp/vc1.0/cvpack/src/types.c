/* Allocation and initial state of the type compactor's tables. */
#include "types.h"
#include "image.h"
#include <recon.h>

void **name_hash_buckets, **type_hash_buckets;
unsigned short *name_hash_counts, *type_hash_counts;
void **type_hash_extra_a, **public_symbol_hash, **type_hash_extra_c, **type_hash_extra_d;
TypeDerivation *derived_types;
unsigned short derived_type_capacity, type_page_count;
unsigned char *type_work_a, *type_work_b;
GrowingBuffer type_record_storage, type_symbol_storage;
TypeRecord *empty_type;

void initialize_type_tables(void)
{
    unsigned short *record;
    name_hash_buckets = checked_zero(0x4000);
    name_hash_counts = checked_zero(0x2000);
    type_hash_buckets = checked_zero(0x4000);
    type_hash_counts = checked_zero(0x2000);
    type_hash_extra_a = checked_zero(0x4000);
    public_symbol_hash = checked_zero(0x4000);
    type_hash_extra_c = checked_zero(0x4000);
    type_hash_extra_d = checked_zero(0x4000);
    derived_types = checked_zero(128 * sizeof(*derived_types));
    type_work_a = checked_zero(0x800);
    type_work_b = checked_zero(0x800);
    derived_type_capacity = 128;
    type_page_count = 0;
    initialize_buffer(&type_record_storage, 0x1000);
    initialize_buffer(&type_symbol_storage, 0x7f00);
    empty_type = checked_zero(sizeof(*empty_type));
    empty_type->data = checked_alloc(6);
    empty_type->flags |= 0x400;
    record = (unsigned short *)empty_type->data;
    record[0] = 6;
    record[1] = 0x201;
    record[2] = 0;
}

void *checked_zero(unsigned long size)
{
    unsigned char *data = malloc(size);
    unsigned long i;
    if (!data) fatal_error(13, NULL, NULL);
    for (i = 0; i < size; ++i) data[i] = 0;
    return data;
}

void initialize_buffer(GrowingBuffer *buffer, unsigned long size)
{
    unsigned long *words = (unsigned long *)buffer;
    int i;
    for (i = 0; i < 6; ++i) words[i] = 0;
    buffer->block_size = size;
}

void *checked_alloc(unsigned long size)
{
    void *data = malloc(size);
    if (!data) fatal_error(13, NULL, NULL);
    return data;
}

int type_recursion_error;

unsigned short __fastcall convert_old_type(unsigned short type)
{
    unsigned short converted;
    if (type < 0x200) return map_primitive(type);
    if (type_recursion_error) return 0;
    converted = compact_old_type(type);
    finish_recursive_types();
    return converted;
}

/* Old primitive encodings: numeric, Boolean and special types, repeated for
   direct, near, far and huge pointer modes. Unsupported slots remain zero. */
#define OLD_NUMERIC(p) (p)|0x10,(p)|0x11,(p)|0x12,0, \
    (p)|0x20,(p)|0x21,(p)|0x22,0, \
    (p)|0x40,(p)|0x41,(p)|0x42,(p)|0x22, \
    (p)|0x51,(p)|0x53,(p)|0x52,0
#define OLD_SPECIAL(p,absolute) (p)|0x30,(p)|0x31,(p)|0x32,0, \
    7,7,7,0,absolute,0,0,0,(p)|3,0,0,0
unsigned short legacy_special_types[3] = { 0, 1, 2 };
unsigned short legacy_primitives[128] = {
    OLD_NUMERIC(0), OLD_SPECIAL(0,5),
    OLD_NUMERIC(0x100), OLD_SPECIAL(0x100,0),
    OLD_NUMERIC(0x200), OLD_SPECIAL(0x200,0),
    OLD_NUMERIC(0x300), OLD_SPECIAL(0x300,0)
};
#undef OLD_SPECIAL
#undef OLD_NUMERIC

unsigned short map_primitive(unsigned short type)
{
    if (type & 0x80) return legacy_primitives[type & 0x7f];
    if (type < 3) return legacy_special_types[type];
    return 7;
}

TypeRecord *local_type_records;
unsigned short local_type_capacity;
unsigned long total_local_type_bytes;

void read_new_global_types(unsigned long size, int precompiled)
{
    unsigned long consumed = 0, carry = 0, count, length, i;
    unsigned char *record, *end;
    unsigned short kind, skip, old_count, attributes;
    TypeRecord *type;
    reset_local_types();
    global_symbol_block = 0;
    while (consumed < size) {
        record = global_symbol_blocks[global_symbol_block];
        count = size-consumed;
        if (count >= 0xffe8) count = 0xffe8;
        count -= carry;
        if (read_file(input_file, record+carry, count) != count)
            fatal_error(19, "Types", module_name(current_module));
        end = record+carry+count;
        while ((unsigned long)(end-record) >= 4) {
            length = *(unsigned short *)record;
            kind = *(unsigned short *)(record+2);
            if ((unsigned long)(end-record) < length+2) break;
            switch (kind) {
            case 0x13:
                load_precompiled_types(record+2);
                break;
            case 0x14:
                module_flags = local_type_count+0x1000;
                add_old_type(NULL, 1, 0, 0, 0);
                current_module->precompiled_signature = *(unsigned long *)(record+4);
                break;
            case 0x16:
                import_type_server(record+2);
                module_flags = local_type_count+0x1000;
                break;
            default:
                skip = kind == 0x200 ?
                    (unsigned short)(*(unsigned short *)(record+4)-local_type_count-0x1000) : 0;
                old_count = local_type_count;
                if (previous_type_count) {
                    unsigned char *bytes = (unsigned char *)local_type_records;
                    for (i = 0; i < (unsigned long)previous_type_count*sizeof(*local_type_records); ++i)
                        bytes[i] = 0;
                    previous_type_count = 0;
                }
                if (skip) local_type_count += skip;
                else ++local_type_count;
                if (local_type_count >= local_type_capacity) {
                    local_type_capacity += skip+6000;
                    local_type_records = realloc(local_type_records,
                        local_type_capacity*sizeof(*local_type_records));
                    if (!local_type_records) fatal_error(13, NULL, NULL);
                    type = &local_type_records[old_count];
                    for (i = 0; i < (unsigned long)(skip+6000)*sizeof(*type); ++i)
                        ((unsigned char *)type)[i] = 0;
                }
                type = &local_type_records[old_count];
                type->data = record;
                type->index = 0;
                type->flags |= 0x400;
                type->flags &= ~0x800;
                attributes = 0;
                if (record) {
                    if (kind == 4 || kind == 5 || kind == 6)
                        attributes = *(unsigned short *)(record+8)&0x80;
                    else if (kind == 7) attributes = *(unsigned short *)(record+10)&0x80;
                }
                type->flags = (type->flags&~0x4000)|(attributes ? 0x4000 : 0);
                break;
            }
            record += length+2;
            consumed += length+2;
        }
        if (consumed < size) {
            if (!global_symbol_blocks[global_symbol_block+1]) {
                global_symbol_blocks[global_symbol_block+1] = malloc(0xffe8);
                if (!global_symbol_blocks[global_symbol_block+1]) fatal_error(13, NULL, NULL);
                ++global_symbol_block_count;
            }
            carry = end-record;
            memmove(global_symbol_blocks[global_symbol_block+1], record, carry);
            ++global_symbol_block;
        }
    }
    total_local_type_bytes += size;
}

unsigned short type_module_count;

void reset_local_types(void)
{
    ++type_module_count;
    if (!local_type_records) {
        local_type_capacity = 12000;
        local_type_records = checked_zero(12000*sizeof(*local_type_records));
    }
    local_type_count = 0;
}

void reset_type_hash(void)
{
    unsigned i;
    TypeCacheEntry *entry;
    for (i = 0; i < 512; ++i) {
        entry = ((TypeCacheEntry **)type_work_a)[i];
        while (entry) {
            entry->count = 0;
            entry->state[1] = 0;
            entry->state[0] = 0;
            entry = entry->next;
        }
    }
}
