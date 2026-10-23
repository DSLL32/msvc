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

/* A compacted index can still refer to a recursive type under construction. */
unsigned short recursive_type_reference;

unsigned short __fastcall convert_new_type(unsigned short type)
{
    unsigned short converted;
    if (type < 0x1000) {
        recursive_type_reference = 0;
        return type;
    }
    if (type_recursion_error) return 0;
    converted = compact_new_type(type);
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
            entry->index = 0;
            entry->type = NULL;
            entry->name = NULL;
            entry = entry->next;
        }
    }
}

unsigned short extra_type_count;

void index_new_types(void *types, unsigned long size,
                     unsigned short *base, unsigned short *count)
{
    unsigned char *record = (unsigned char *)types+4;
    unsigned char *end = (unsigned char *)types+size;
    unsigned short length, kind, padding, index, relative, unused;
    unsigned short old_kind;
    unsigned char *name, *type, *cursor;
    TypeRecord *entry;
    TypeCacheEntry *cached;
    unsigned bucket, i;

    extra_type_count = 0;
    while (record < end) {
        if (!((unsigned long)record & 1)) {
            length = *(unsigned short *)record+2;
            kind = *(unsigned short *)(record+2);
        } else {
            length = (unsigned short)(record[0]+(record[1]<<8)+2);
            kind = (unsigned short)(record[2]+(record[3]<<8));
        }
        if (kind == 7) *count += length;
        else {
            padding = 3-((length-1)&3);
            *base += padding;
            switch (kind) {
            case 4:
                index = *(unsigned short *)(record+4);
                name = record+6;
                if (index < 0x1000) break;
                relative = index-first_user_type;
                entry = &local_type_records[relative];
                if (entry->flags & 0x4000) break;
                if (!type_signature && relative == (unsigned short)(0xffff-first_user_type))
                    entry = empty_type;
                else if (entry->flags & 0x1000)
                    entry = get_type_record((unsigned short)(entry->index-0x1000), &unused);
                type = entry->data;
                if (!type) break;
                old_kind = *(unsigned short *)(type+2);
                if (old_kind != 4 && old_kind != 5 && old_kind != 6 && old_kind != 7) break;
                bucket = name[0];
                cursor = name+1;
                for (i = name[0]; i; --i) bucket += *cursor++;
                bucket &= 0x1ff;
                cached = ((TypeCacheEntry **)type_work_a)[bucket];
                while (cached) {
                    if (!cached->type) break;
                    if (!strncmp((char *)cached->name, (char *)name, name[0]+1)) {
                        if (cached->index != index) module_type_bytes |= module_types_indexed;
                        break;
                    }
                    cached = cached->next;
                }
                if (!cached) {
                    cached = checked_zero(sizeof(*cached));
                    if (!cached) fatal_error(13, NULL, NULL);
                    cached->next = ((TypeCacheEntry **)type_work_a)[bucket];
                    ((TypeCacheEntry **)type_work_a)[bucket] = cached;
                }
                cached->type = type;
                cached->name = name;
                cached->index = index;
                break;
            case 0x104: case 0x105:
                reserve_extra_type(*(unsigned short *)(record+24), base);
                break;
            case 0x204: case 0x205:
                reserve_extra_type(*(unsigned short *)(record+32), base);
                break;
            }
        }
        record += length;
    }
    if (module_type_bytes) {
        reset_local_types();
        load_type_server_records(NULL);
        if (!previous_type_count) previous_type_count = local_type_count;
    }
}

unsigned short extra_types[254];

void reserve_extra_type(unsigned short type, unsigned short *size)
{
    if (!has_extra_type(type)) {
        *size += 13;
        /* The original checks the limit after inserting the index. */
        extra_types[extra_type_count++] = type;
        if (extra_type_count > 254)
            fatal_error(27, module_name(current_module), NULL);
    }
}

int has_extra_type(unsigned short type)
{
    unsigned short i = 0, count = extra_type_count;
    while (i < count) {
        if (extra_types[i] == type) return 1;
        ++i;
    }
    return 0;
}

/* The historical pack_new_types name is provisional: sstAlignSym contains
   CodeView symbol records, including UDT references and procedure scopes. */
typedef struct SymbolConversion {
    unsigned short kind;
    void (*convert)(void);
} SymbolConversion;

SymbolConversion symbol_conversions[32] = {
    { 0x100, convert_bp_relative16 },
    { 0x200, convert_bp_relative32 },
    { 0x107, begin_symbol_block },
    { 0x207, begin_symbol_block },
    { 0x104, convert_procedure16 },
    { 0x204, convert_procedure32 },
    { 0x105, convert_procedure16 },
    { 0x205, convert_procedure32 },
    { 0x6, end_symbol_block },
    { 0x101, convert_local_data16 },
    { 0x201, convert_local_data32 },
    { 0x102, convert_global_data16 },
    { 0x202, convert_global_data32 },
    { 0x103, convert_local_data16 },
    { 0x203, convert_local_data32 },
    { 0x109, NULL },
    { 0x209, NULL },
    { 0x108, begin_symbol_block },
    { 0x208, begin_symbol_block },
    { 0x106, begin_symbol_block },
    { 0x206, begin_symbol_block },
    { 0x10a, NULL },
    { 0x20a, NULL },
    { 0x2, convert_register_symbol },
    { 0x3, convert_constant_symbol },
    { 0x20d, convert_local_data32 },
    { 0x20e, convert_global_data32 },
    { 0x4, convert_udt_symbol },
    { 0x5, NULL },
    { 0x7, NULL },
    { 0x1, NULL },
    { 0x9, convert_object_name }
};

unsigned char *symbol_input, *symbol_output;
unsigned long suppressed_symbol_depth, total_packed_symbol_bytes;

void pack_new_types(void *types, CvDirectoryEntry *entry, ModuleInfo *module,
                    unsigned short *padding_bytes, unsigned short *skipped_bytes)
{
    unsigned char *buffer, *start, *end;
    unsigned long output_size, i;
    unsigned short index, length, kind, padding;
    SymbolConversion *conversion;

    symbol_input = types;
    suppressed_symbol_depth = 0;
    public_symbol_bytes = 0;
    end = (unsigned char *)types+entry->size;
    output_size = entry->size+(unsigned long)*padding_bytes-(unsigned long)*skipped_bytes;
    buffer = malloc(output_size);
    if (!buffer) fatal_error(14, NULL, NULL);
    symbol_output = buffer;
    if (!buffer) fatal_error(14, NULL, NULL);
    start = symbol_output;
    *(unsigned long *)symbol_output = *(unsigned long *)symbol_input;
    symbol_input += 4;
    symbol_output += 4;
    for (index = 0; index < extra_type_count; ++index)
        symbol_output += write_search_symbol(symbol_output, extra_types[index]);
    while (symbol_input < end) {
        if (!((unsigned long)symbol_input & 1)) {
            length = *(unsigned short *)symbol_input+2;
            kind = *(unsigned short *)(symbol_input+2);
        } else {
            length = (unsigned short)(symbol_input[0]+(symbol_input[1]<<8)+2);
            kind = (unsigned short)(symbol_input[2]+(symbol_input[3]<<8));
        }
        if (kind != 7) {
            conversion = symbol_conversions;
            for (index = 0; index < 32; ++index, ++conversion)
                if (conversion->kind == kind) break;
#ifdef RECON_NATIVE_TEST
            /* The original falls into adjacent globals for an unknown kind.
               Native execution has no verified adjacency for that path. */
            if (index == 32) __asm { int 3 }
#endif
            if (conversion->convert) conversion->convert();
            if (*(unsigned short *)(symbol_input+2) != 8) {
                for (i = 0; i < length; ++i) symbol_output[i] = symbol_input[i];
                padding = (unsigned short)(3-((length-1)&3));
                *(unsigned short *)symbol_output = (unsigned short)(length+padding-2);
                symbol_output += length;
                while (padding--) *symbol_output++ = 0;
            }
        }
        symbol_input += length;
    }
    output_size = symbol_output-start;
    if (!link_symbol_scopes(start, output_size)) {
        warning(3, module_name(current_module), NULL);
        output_size = 0;
    } else record_symbol_offsets(start, output_size, module->module);
    entry->location.data = buffer;
    entry->size = output_size;
    total_packed_symbol_bytes += output_size;
    module->symbol_size = output_size;
    module->symbols = buffer;
}

unsigned short write_search_symbol(unsigned char *buffer, unsigned short segment)
{
    *(unsigned short *)buffer = 10;
    *(unsigned short *)(buffer+2) = 5;
    *(unsigned short *)(buffer+8) = segment;
    *(unsigned long *)(buffer+4) = 0;
    buffer[10] = 0;
    buffer[11] = 0;
    return 12;
}
