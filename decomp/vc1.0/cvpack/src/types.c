/* Allocation and initial state of the type compactor's tables. */
#include "types.h"
#include "image.h"
#include <recon.h>

void **name_hash_buckets, **type_hash_buckets;
unsigned short *name_hash_counts, *type_hash_counts;
void **global_symbol_hash, **public_symbol_hash, **type_hash_extra_c, **type_hash_extra_d;
TypeDerivation *derived_types;
unsigned short derived_type_capacity, type_page_capacity;
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
    global_symbol_hash = checked_zero(0x4000);
    public_symbol_hash = checked_zero(0x4000);
    type_hash_extra_c = checked_zero(0x4000);
    type_hash_extra_d = checked_zero(0x4000);
    derived_types = checked_zero(128 * sizeof(*derived_types));
    type_work_a = checked_zero(0x800);
    type_work_b = checked_zero(0x800);
    derived_type_capacity = 128;
    type_page_capacity = 0;
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

void read_old_global_types(void *types, unsigned long size)
{
    unsigned char *record = types, *end = record+size;
    unsigned short skip;
    reset_local_types();
    while (record < end) {
        skip = 0;
        if (record[3] == 0x90)
            skip = (unsigned short)(*(unsigned short *)(record+4)-local_type_count-0x200);
        add_old_type(record, 0, 0, 0, skip);
        if (record[3] == 0x79)
            module_symbol_count += *(unsigned short *)(record+1)+10;
        record += *(unsigned short *)(record+1)+3;
    }
    total_local_type_bytes += size;
}

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
    symbol_scope_depth = 0;
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

void convert_object_name(void)
{
    unsigned char *name;
    if (current_precompiled == 1) {
        name = symbol_input+8;
        current_module->object_name = malloc(name[0]+1);
        if (!current_module->object_name) fatal_error(13, NULL, NULL);
        memcpy(current_module->object_name, name, name[0]+1);
    }
}

void convert_udt_symbol(void)
{
    unsigned short *index;
    if (suppressed_symbol_depth) *(unsigned short *)(symbol_input+2) = 8;
    if (!suppressed_symbol_depth) {
        index = (unsigned short *)(symbol_input+4);
        *index = convert_new_type(*index);
        if (!symbol_scope_depth) intern_global(symbol_input, hash_public);
    }
}

/* Current type path and the earliest unresolved member of its recursive group. */
unsigned short type_path[256], type_path_depth, recursive_type_root;

/* Compact C6 type indices while retaining the shared recursive-group path. */
unsigned short __fastcall compact_old_type(unsigned short type)
{
    unsigned short relative, unused, previous_root, candidate, field;
    unsigned char recursive_count = 0;
    TypeRecord *entry, *resolved;
    TypeCacheEntry *named;
    unsigned char *record;
    int last, i;

#ifdef RECON_NATIVE_TEST
#define OLD_TYPE_LIMIT(test) do { if (!(test)) __asm { int 3 } } while (0)
#else
#define OLD_TYPE_LIMIT(test) ((void)0)
#endif
#define OLD_TYPE_ENTRY(destination,index) \
    do { \
        relative = (index); \
        if (!type_signature && relative == (unsigned short)(0xffff-first_user_type)) \
            (destination) = empty_type; \
        else { \
            (destination) = &local_type_records[relative]; \
            if ((destination)->flags & 0x1000) \
                (destination) = get_type_record((unsigned short)((destination)->forward_index-0x1000), &unused); \
        } \
    } while (0)
#define NOTE_OLD_RECURSIVE(index) \
    do { \
        candidate = (index); \
        if (candidate) { \
            if (!recursive_type_root) recursive_type_root = candidate; \
            else { \
                last = (int)type_path_depth-1; \
                OLD_TYPE_LIMIT(last >= 0 && last < 256); \
                while (type_path[last] != recursive_type_root) { --last; OLD_TYPE_LIMIT(last >= 0); } \
                for (i = 0; i < last; ++i) \
                    if (type_path[i] == candidate) { recursive_type_root = candidate; break; } \
            } \
        } \
    } while (0)
#define OLD_RECURSIVE_OFFSET(offset) \
    do { \
        OLD_TYPE_LIMIT(recursive_count < 12); \
        entry->recursion.small[recursive_count++] = (offset); \
    } while (0)

    if (type < 0x200) {
        recursive_type_reference = 0;
        return map_primitive(type);
    }
    if ((unsigned long)type >= (unsigned long)local_type_count+0x200) {
        recursive_type_reference = 0;
        return 0;
    }
    OLD_TYPE_ENTRY(entry, (unsigned short)(type-0x200));
    if (entry->flags & 0x8006) {
        recursive_type_reference = 0;
        return entry->index;
    }
    if (entry->flags & 0x80) {
        resolved = entry;
        do {
            entry = resolved;
            OLD_TYPE_ENTRY(resolved, (unsigned short)(entry->index-0x200));
        } while (resolved->flags & 0x80);
        recursive_type_reference = 1;
        NOTE_OLD_RECURSIVE(entry->index);
        return type;
    }
    if (entry->flags & 0x40) {
        recursive_type_reference = 1;
        NOTE_OLD_RECURSIVE(type);
        return type;
    }
    entry->flags |= 0x40;
    OLD_TYPE_LIMIT(type_path_depth < 256);
    type_path[type_path_depth++] = type;
    previous_root = recursive_type_root;
    recursive_type_root = 0;
    relative = type-0x200;
    record = entry->data;
    switch (record[3]) {
    case 0x75:                      /* Procedure: result and argument list. */
        convert_old_type_record(relative);
        record = entry->data+2;
        *(unsigned short *)(record+2) = compact_old_type(*(unsigned short *)(record+2));
        if (recursive_type_reference) OLD_RECURSIVE_OFFSET(4);
        *(unsigned short *)(record+8) = compact_type_list(*(unsigned short *)(record+8), *(unsigned short *)(record+6));
        if (recursive_type_reference) OLD_RECURSIVE_OFFSET(10);
        break;
    case 0x76:                      /* Alias: retain the already compacted index. */
        field = compact_old_type(*(unsigned short *)(record+5));
        entry->flags |= 0x8000;
        entry->index = field;
        break;
    case 0x78:                      /* Array: element and index types. */
        convert_old_type_record(relative);
        record = entry->data+2;
        *(unsigned short *)(record+2) = compact_old_type(*(unsigned short *)(record+2));
        if (recursive_type_reference) OLD_RECURSIVE_OFFSET(4);
        *(unsigned short *)(record+4) = compact_old_type(*(unsigned short *)(record+4));
        if (recursive_type_reference) OLD_RECURSIVE_OFFSET(6);
        break;
    case 0x79:                      /* Structure: named identity and field list. */
        convert_old_type_record(relative);
        if (find_named_type(entry, &named, 0) == 3) {
            --type_path_depth;
            queue_type_match(type, named);
            recursive_type_reference = 0;
            recursive_type_root = previous_root;
            entry->flags |= 2;
            entry->index = named->index;
            return entry->index;
        }
        record = entry->data+2;
        *(unsigned short *)(record+4) = compact_type_list(*(unsigned short *)(record+4), *(unsigned short *)(record+2));
        if (recursive_type_reference) OLD_RECURSIVE_OFFSET(6);
        break;
    case 0x7a: case 0x91:          /* Pointer records share the new representation. */
        convert_old_type_record(relative);
        recursive_count = compact_pointer(entry);
        break;
    case 0x8c: case 0xa6:          /* Based array and modifier. */
        convert_old_type_record(relative);
        record = entry->data+2;
        *(unsigned short *)(record+2) = compact_old_type(*(unsigned short *)(record+2));
        if (recursive_type_reference) OLD_RECURSIVE_OFFSET(4);
        break;
    case 0x5d: case 0x68: case 0x99: case 0xa3:
        break;
    default:
        convert_old_type_record(relative);
        break;
    }
    entry->recursive_count = recursive_count;
    --type_path_depth;
    if (recursive_type_root == type) {
        recursive_type_reference = 0;
        recursive_type_root = previous_root;
        entry->index = intern_type(entry, type);
        return entry->index;
    }
    if (recursive_type_root) {
        recursive_type_reference = 1;
        entry->index = recursive_type_root;
        NOTE_OLD_RECURSIVE(previous_root);
        entry->flags |= 0x80;
        return type;
    }
    recursive_type_reference = 0;
    recursive_type_root = previous_root;
    if (!(entry->flags & 0x8000)) {
        entry->flags |= 4;
        store_type(entry);
    }
    return entry->index;
#undef OLD_RECURSIVE_OFFSET
#undef NOTE_OLD_RECURSIVE
#undef OLD_TYPE_ENTRY
#undef OLD_TYPE_LIMIT
}

unsigned short __fastcall compact_new_type(unsigned short type)
{
    unsigned short relative, mapped, unused, previous_root, candidate, field;
    TypeRecord *entry, *resolved;
    TypeCacheEntry *named;
    unsigned char *record;
    unsigned short *field_pointer;
    int result, last, i;
    unsigned long count;

#ifdef RECON_NATIVE_TEST
#define TYPE_LIMIT(test) do { if (!(test)) __asm { int 3 } } while (0)
#else
#define TYPE_LIMIT(test) ((void)0)
#endif
#define ADD_RECURSIVE_OFFSET(offset) \
    do { \
        TYPE_LIMIT(entry->recursive_count < 12); \
        entry->recursion.small[entry->recursive_count++] = (offset); \
    } while (0)

    /* These scans follow the original active path; they are not hash lookups. */
#define NOTE_RECURSIVE(index) \
    do { \
        candidate = (index); \
        if (candidate) { \
            if (!recursive_type_root) recursive_type_root = candidate; \
            else { \
                last = (int)type_path_depth-1; \
                TYPE_LIMIT(last >= 0 && last < 256); \
                while (type_path[last] != recursive_type_root) { --last; TYPE_LIMIT(last >= 0); } \
                for (i = 0; i < last; ++i) \
                    if (type_path[i] == candidate) { recursive_type_root = candidate; break; } \
            } \
        } \
    } while (0)
#define COMPACT_FIELD(offset) \
    do { \
        field = compact_new_type(*(unsigned short *)(entry->data+(offset))); \
        *(unsigned short *)(entry->data+(offset)) = field; \
        if (recursive_type_reference) \
            ADD_RECURSIVE_OFFSET(offset); \
    } while (0)

again:
    if (type < 0x1000) { recursive_type_reference = 0; return type; }
    if ((unsigned long)type >= (unsigned long)local_type_count+0x1000) {
        recursive_type_reference = 0;
        return 7;
    }
    relative = type-0x1000;
    mapped = 0;
    if (!type_signature && relative == (unsigned short)(0xffff-first_user_type))
        entry = empty_type;
    else {
        entry = &local_type_records[relative];
        if (entry->flags & 0x1000) {
            mapped = entry->forward_index;
            entry = get_type_record((unsigned short)(entry->forward_index-0x1000), &unused);
        }
    }
    if (entry->flags & 0x806) {
        recursive_type_reference = 0;
        return entry->index;
    }
    if (entry->flags & 0x80) {
        resolved = entry;
        do {
            entry = resolved;
            relative = resolved->index-0x1000;
            mapped = 0;
            if (!type_signature && relative == (unsigned short)(0xffff-first_user_type))
                resolved = empty_type;
            else {
                resolved = &local_type_records[relative];
                if (resolved->flags & 0x1000) {
                    mapped = resolved->forward_index;
                    resolved = get_type_record((unsigned short)(resolved->forward_index-0x1000), &unused);
                }
            }
        } while (resolved->flags & 0x80);
        recursive_type_reference = 1;
        NOTE_RECURSIVE(entry->index);
        return type;
    }
    if (entry->flags & 0x40) {
        recursive_type_reference = 1;
        NOTE_RECURSIVE(mapped ? mapped : type);
        return type;
    }
    entry->flags |= 0x40;
#ifdef RECON_NATIVE_TEST
    /* Only the proved 256-word active-path object belongs to this array. */
    if (type_path_depth == 256) __asm { int 3 }
#endif
    type_path[type_path_depth++] = type;
    previous_root = recursive_type_root;
    recursive_type_root = 0;
    record = entry->data;
    switch (*(unsigned short *)(record+2)) {
    case 1:                         /* LF_MODIFIER */
        COMPACT_FIELD(6);
        break;
    case 2:                         /* LF_POINTER */
        compact_pointer(entry);
        break;
    case 3:                         /* LF_ARRAY */
        COMPACT_FIELD(4);
        COMPACT_FIELD(6);
        break;
    case 4: case 5:                 /* LF_CLASS, LF_STRUCTURE */
        *(unsigned short *)(record+8) &= ~1;
        if (*(unsigned short *)(record+8) & 0x80) goto forward;
        result = find_named_type(entry, &named, 0);
        if (result == 3) goto matched;
        field_pointer = (unsigned short *)(record+12);
        *field_pointer = compact_new_type(*field_pointer);
        if (recursive_type_reference) ADD_RECURSIVE_OFFSET(12);
        *(unsigned short *)(record+6) = compact_type_list(*(unsigned short *)(record+6), *(unsigned short *)(record+4));
        if (recursive_type_reference) ADD_RECURSIVE_OFFSET(6);
        break;
    case 6:                         /* LF_UNION */
        *(unsigned short *)(record+8) &= ~1;
        if (*(unsigned short *)(record+8) & 0x80) goto forward;
        result = find_named_type(entry, &named, 0);
        if (result == 3) goto matched;
        record = entry->data;
        *(unsigned short *)(record+6) = compact_type_list(*(unsigned short *)(record+6), *(unsigned short *)(record+4));
        if (recursive_type_reference) ADD_RECURSIVE_OFFSET(6);
        break;
    case 7:                         /* LF_ENUM */
        *(unsigned short *)(record+10) &= ~1;
        if (*(unsigned short *)(record+10) & 0x80) goto forward;
        result = find_named_type(entry, &named, 0);
        if (result == 3) goto matched;
        COMPACT_FIELD(6);
        record = entry->data;
        *(unsigned short *)(record+8) = compact_type_list(*(unsigned short *)(record+8), *(unsigned short *)(record+4));
        if (recursive_type_reference) ADD_RECURSIVE_OFFSET(8);
        break;
    case 8:                         /* LF_PROCEDURE */
        COMPACT_FIELD(4);
        record = entry->data;
        *(unsigned short *)(record+10) = compact_type_list(*(unsigned short *)(record+10), *(unsigned short *)(record+8));
        if (recursive_type_reference) ADD_RECURSIVE_OFFSET(10);
        break;
    case 9:                         /* LF_MFUNCTION */
        COMPACT_FIELD(4);
        COMPACT_FIELD(6);
        COMPACT_FIELD(8);
        record = entry->data;
        *(unsigned short *)(record+14) = compact_type_list(*(unsigned short *)(record+14), *(unsigned short *)(record+12));
        if (recursive_type_reference) ADD_RECURSIVE_OFFSET(14);
        break;
    case 11: case 13:               /* LF_COBOL0, LF_BARRAY */
        COMPACT_FIELD(4);
        break;
    case 17:                        /* LF_DIMARRAY */
        COMPACT_FIELD(4);
        COMPACT_FIELD(6);
        break;
    case 0x202:                     /* LF_DEFARG */
        COMPACT_FIELD(4);
        break;
    case 0x208: case 0x209:         /* LF_DIMCONU, LF_DIMCONLU */
        COMPACT_FIELD(6);
        break;
    case 0x20a: case 0x20b:         /* LF_DIMVARU, LF_DIMVARLU */
        COMPACT_FIELD(6);
        record = entry->data+2;
        count = *(unsigned short *)(record+2);
        if (*(unsigned short *)record == 0x20b) count += count;
        while (count--) {
            field = compact_new_type(*(unsigned short *)(record+6+count*2));
            *(unsigned short *)(entry->data+8+count*2) = field;
            record = entry->data+2;
            if (recursive_type_reference)
                ADD_RECURSIVE_OFFSET((unsigned char)(count*2+8));
        }
        break;
    case 10: case 12: case 14: case 15: case 0x206: case 0x20c:
        break;
    default:
        fatal_error(23, module_name(current_module), NULL);
        break;
    }
finish:
    --type_path_depth;
    if (recursive_type_root == type) {
        recursive_type_reference = 0;
        entry->index = intern_type(entry, type);
        recursive_type_root = previous_root;
        return entry->index;
    }
    if (recursive_type_root) {
        recursive_type_reference = 1;
        entry->index = recursive_type_root;
        NOTE_RECURSIVE(previous_root);
        entry->flags |= 0x80;
        return type;
    }
    recursive_type_reference = 0;
    recursive_type_root = previous_root;
    entry->flags |= 4;
    store_type(entry);
    return entry->index;

forward:
    result = find_named_type(entry, &named, 1);
    switch (result) {
    case 1:
        --type_path_depth;
        entry->flags |= 0x1000;
        entry->forward_index = named->index;
        recursive_type_root = previous_root;
        type = named->index;
        goto again;
    case 2:
        recursive_type_reference = 0;
        --type_path_depth;
        entry->flags |= 0x80;
        entry->flags |= 2;
        entry->index = named->index;
        recursive_type_root = previous_root;
        return named->index;
    case 3:
        recursive_type_reference = 0;
        --type_path_depth;
        entry->flags |= 0x80;
        entry->flags |= 2;
        entry->index = named->index;
        recursive_type_root = previous_root;
        return named->index;
    }
    goto finish;
matched:
    --type_path_depth;
    queue_type_match(type, named);
    recursive_type_reference = 0;
    recursive_type_root = previous_root;
    entry->flags |= 2;
    entry->index = named->index;
    return entry->index;
#undef ADD_RECURSIVE_OFFSET
#undef TYPE_LIMIT
#undef COMPACT_FIELD
#undef NOTE_RECURSIVE
}

/* The file's CodeView type signature selects its old/new index converter. */
typedef unsigned short (__fastcall *TypeConverter)(unsigned short);
TypeConverter type_converters[2] = { compact_old_type, compact_new_type };

unsigned char __fastcall compact_pointer(TypeRecord *entry)
{
    TypeConverter convert;
    unsigned char *record;
    unsigned short offset, converted;
    unsigned int mode, kind;
#ifdef RECON_NATIVE_TEST
    if (type_signature >= 2) __asm { int 3 }
#endif
    convert = type_converters[type_signature];
    converted = convert(*(unsigned short *)(entry->data+6));
    *(unsigned short *)(entry->data+6) = converted;
    record = entry->data+2;
    if (recursive_type_reference) {
#ifdef RECON_NATIVE_TEST
        if (entry->recursive_count >= 12) __asm { int 3 }
#endif
        entry->recursion.small[entry->recursive_count++] = 6;
    }
    mode = record[2] >> 5;
    if (!mode) {
        kind = record[2]&0x1f;
        if (kind >= 4 && kind <= 7) {
            switch (*(unsigned short *)(record+8)) {
            case 2: offset = 12; break;
            case 0x100: offset = 14; break;
            case 0x101: case 0x102: offset = 16; break;
            case 0x200: offset = 16; break;
            case 0x201: case 0x202: offset = 18; break;
            default:
                /* Original offset remains uninitialized for unknown records. */
#ifdef RECON_NATIVE_TEST
                __asm { int 3 }
#endif
                break;
            }
            converted = convert(*(unsigned short *)(entry->data+offset));
            *(unsigned short *)(entry->data+offset) = converted;
            if (recursive_type_reference) {
#ifdef RECON_NATIVE_TEST
                if (entry->recursive_count >= 12) __asm { int 3 }
#endif
                entry->recursion.small[entry->recursive_count++] = (unsigned char)offset;
            }
        } else if (kind == 8) {
            converted = convert(*(unsigned short *)(record+6));
            *(unsigned short *)(entry->data+8) = converted;
            if (recursive_type_reference) {
#ifdef RECON_NATIVE_TEST
                if (entry->recursive_count >= 12) __asm { int 3 }
#endif
                entry->recursion.small[entry->recursive_count++] = 8;
            }
        }
    } else if (mode == 2 || mode == 3) {
        converted = convert(*(unsigned short *)(record+6));
        *(unsigned short *)(entry->data+8) = converted;
        if (recursive_type_reference) {
#ifdef RECON_NATIVE_TEST
            if (entry->recursive_count >= 12) __asm { int 3 }
#endif
            entry->recursion.small[entry->recursive_count++] = 8;
        }
    }
    return (unsigned char)entry->recursive_count;
}

unsigned short __fastcall compact_type_list(unsigned short type, unsigned short count)
{
    unsigned short relative = type-first_user_type, unused, previous_root;
    unsigned short candidate, cursor, remaining, converted, recursive_offset, item;
    TypeRecord *entry, *resolved;
    TypeConverter convert;
    unsigned char *record, *base, *field_record;
    int last, i;

#ifdef RECON_NATIVE_TEST
#define LIST_LIMIT(test) do { if (!(test)) __asm { int 3 } } while (0)
#else
#define LIST_LIMIT(test) ((void)0)
#endif
#define NOTE_LIST_RECURSIVE(index) \
    do { \
        candidate = (index); \
        if (candidate) { \
            if (!recursive_type_root) recursive_type_root = candidate; \
            else { \
                last = (int)type_path_depth-1; \
                LIST_LIMIT(last >= 0 && last < 256); \
                while (type_path[last] != recursive_type_root) { --last; LIST_LIMIT(last >= 0); } \
                for (i = 0; i < last; ++i) \
                    if (type_path[i] == candidate) { recursive_type_root = candidate; break; } \
            } \
        } \
    } while (0)
#define FIELD_TYPE(offset) \
    do { \
        converted = convert(*(unsigned short *)(field_record+(offset))); \
        base = entry->data+4; \
        field_record = base+cursor; \
        *(unsigned short *)(field_record+(offset)) = converted; \
        recursive_offset = (unsigned short)(field_record-entry->data+(offset)); \
    } while (0)
#define SKIP_NUMERIC() \
    do { \
        record = base+cursor; \
        cursor += *(unsigned short *)record < 0x8000 ? 2 : numeric_leaf_size(record); \
    } while (0)

    if (!type_signature && relative == (unsigned short)(0xffff-first_user_type))
        entry = empty_type;
    else {
        entry = &local_type_records[relative];
        if (entry->flags & 0x1000)
            entry = get_type_record((unsigned short)(entry->forward_index-0x1000), &unused);
    }
    if (entry->flags & 0x806) {
        recursive_type_reference = 0;
        return entry->index;
    }
    if (entry->flags & 0x80) {
        resolved = entry;
        do {
            entry = resolved;
            relative = entry->index-0x1000;
            if (!type_signature && relative == (unsigned short)(0xffff-first_user_type))
                resolved = empty_type;
            else {
                resolved = &local_type_records[relative];
                if (resolved->flags & 0x1000)
                    resolved = get_type_record((unsigned short)(resolved->forward_index-0x1000), &unused);
            }
        } while (resolved->flags & 0x80);
        recursive_type_reference = 1;
        NOTE_LIST_RECURSIVE(entry->index);
        return type;
    }
    if (entry->flags & 0x40) {
        recursive_type_reference = 1;
        NOTE_LIST_RECURSIVE(type);
        return type;
    }
    entry->flags |= 0x40;
    LIST_LIMIT(type_path_depth < 256);
    type_path[type_path_depth++] = type;
    previous_root = recursive_type_root;
    recursive_type_root = 0;
    record = entry->data;
    base = record+2;
    LIST_LIMIT(type_signature < 2);
    convert = type_converters[type_signature];
    entry->recursive_count = 0;
    switch (*(unsigned short *)base) {
    case 0x201:                     /* LF_ARGLIST */
        for (item = 0; item < count; ++item) {
            converted = convert(*(unsigned short *)(base+4+item*2));
            base = entry->data+2;
            *(unsigned short *)(base+4+item*2) = converted;
            if (recursive_type_reference)
                add_recursive_offset(entry, (unsigned short)(base-entry->data+4+item*2));
        }
        break;
    case 0x204:                     /* LF_FIELDLIST */
        cursor = 0;
        remaining = count;
        if (!remaining) break;
        do {
            base = entry->data+4;
            field_record = base+cursor;
            recursive_offset = 0;
            switch (*(unsigned short *)field_record) {
            case 0x400:             /* LF_BCLASS */
                FIELD_TYPE(2);
                cursor += 6;
                SKIP_NUMERIC();
                break;
            case 0x401: case 0x402:  /* LF_VBCLASS, LF_IVBCLASS */
                FIELD_TYPE(2);
                if (recursive_type_reference) add_recursive_offset(entry, recursive_offset);
                FIELD_TYPE(4);
                if (recursive_type_reference) add_recursive_offset(entry, recursive_offset);
                recursive_offset = 0;
                cursor += 8;
                SKIP_NUMERIC();
                SKIP_NUMERIC();
                break;
            case 0x403:             /* LF_ENUMERATE */
                cursor += 4;
                SKIP_NUMERIC();
                cursor += (unsigned short)(base[cursor]+1);
                break;
            case 0x404:             /* LF_FRIENDFCN */
                FIELD_TYPE(2);
                cursor += (short)(signed char)field_record[4]+5;
                break;
            case 0x405:             /* LF_INDEX: splice the continuation here. */
                merge_field_list(field_record, entry);
                base = entry->data+4;
                ++remaining;
                break;
            case 0x406:             /* LF_MEMBER */
                FIELD_TYPE(2);
                cursor += 6;
                SKIP_NUMERIC();
                cursor += (unsigned short)(base[cursor]+1);
                break;
            case 0x407:             /* LF_STMEMBER */
                FIELD_TYPE(2);
                cursor += (short)(signed char)field_record[6]+7;
                break;
            case 0x408:             /* LF_METHOD */
                converted = compact_type_list(*(unsigned short *)(field_record+4), *(unsigned short *)(field_record+2));
                base = entry->data+4;
                field_record = base+cursor;
                *(unsigned short *)(field_record+4) = converted;
                recursive_offset = (unsigned short)(field_record-entry->data+4);
                cursor += (short)(signed char)field_record[6]+7;
                remaining -= *(unsigned short *)(field_record+2);
                ++remaining;
                break;
            case 0x409:             /* LF_NESTTYPE */
                FIELD_TYPE(2);
                cursor += (unsigned short)(field_record[4]+5);
                break;
            case 0x40a: case 0x40b: /* LF_VFUNCTAB, LF_FRIENDCLS */
                FIELD_TYPE(2);
                cursor += 4;
                break;
            default:
                break;
            }
            if (remaining > 1 && base[cursor] >= 0xf0) cursor += base[cursor]&0xf;
            if (recursive_offset && recursive_type_reference)
                add_recursive_offset(entry, recursive_offset);
        } while (--remaining);
        break;
    case 0x207:                     /* LF_METHODLIST */
        cursor = 0;
        remaining = count;
        while (remaining) {
            converted = convert(*(unsigned short *)(record+cursor+6));
            record = entry->data;
            field_record = record+cursor+4;
            *(unsigned short *)(field_record+2) = converted;
            recursive_offset = (unsigned short)(field_record-record+2);
            cursor += (*(unsigned short *)field_record&0x1c)==0x10 ? 8 : 4;
            if (recursive_offset && recursive_type_reference)
                add_recursive_offset(entry, recursive_offset);
            --remaining;
        }
        break;
    }
    --type_path_depth;
    if (recursive_type_root == type) {
        recursive_type_reference = 0;
        recursive_type_root = previous_root;
        entry->index = intern_type(entry, type);
        return entry->index;
    }
    if (recursive_type_root) {
        recursive_type_reference = 1;
        entry->index = recursive_type_root;
        NOTE_LIST_RECURSIVE(previous_root);
        entry->flags |= 0x80;
        return type;
    }
    recursive_type_reference = 0;
    recursive_type_root = previous_root;
    entry->flags |= 4;
    store_type(entry);
    return entry->index;
#undef SKIP_NUMERIC
#undef FIELD_TYPE
#undef NOTE_LIST_RECURSIVE
#undef LIST_LIMIT
}

/* Exact-record type buckets hold compacted indices and their stored bytes. */
typedef struct TypeHashEntry {
    unsigned short index, reserved;
    unsigned char *data;
    struct TypeHashEntry *next;
} TypeHashEntry;
typedef char TypeHashEntrySize[(sizeof(TypeHashEntry)==12)?1:-1];

void __fastcall store_type(TypeRecord *entry)
{
    unsigned short bucket = hash_type(entry->data), kind;
    TypeHashEntry *node;
    TypeCacheEntry *named;
    unsigned char *record, *name;
    unsigned long length, i;

    for (node = type_hash_buckets[bucket]; node; node = node->next) {
        if (!node->data) break;
        length = *(unsigned short *)node->data+2;
        for (i = 0; i < length; ++i)
            if (node->data[i] != entry->data[i]) break;
        if (i != length) continue;
        entry->flags |= 2;
        entry->index = node->index;
        if (entry->flags & 0x38) release_type_data(entry);
        if (entry->flags & 0x100) {
#ifdef RECON_NATIVE_TEST
            /* Large recursive arrays require their own typed lifetime proof. */
            __asm { int 3 }
#endif
            free(entry->recursion.large.offsets);
            free(entry->recursion.large.marks);
            entry->flags &= ~0x100;
        }
        return;
    }
    record = entry->data;
    kind = *(unsigned short *)(record+2);
    switch (kind) {
    case 4: case 5:
        name = record+14;
        name += *(unsigned short *)name < 0x8000 ? 2 : numeric_leaf_size(name);
        break;
    case 6:
        name = record+10;
        name += *(unsigned short *)name < 0x8000 ? 2 : numeric_leaf_size(name);
        break;
    case 7:
        name = record+12;
        break;
    default:
        name = NULL;
        break;
    }
    if (name && find_global_type(kind, name, &named) == 3) {
        entry->index = named->index;
        memcpy(named->type, record, *(unsigned short *)record+2);
        entry->data = named->type;
    } else save_type(entry);
    node = checked_alloc(sizeof(*node));
    node->data = entry->data;
    node->index = entry->index;
    node->next = type_hash_buckets[bucket];
    type_hash_buckets[bucket] = node;
    ++type_hash_counts[bucket];
    kind = *(unsigned short *)(node->data+2);
    if ((kind == 4 || kind == 5) && !(node->data[8]&0x80))
        record_derived_types(node->index, *(unsigned short *)(node->data+6));
}

/* Linear byte transforms used by the record hash. The eight basis columns
   specify each original256-byte lookup exactly, without a runtime initializer. */
#define HASH_TRANSFORM(x,a,b,c,d,e,f,g,h) \
    (((x)&1?(a):0)^((x)&2?(b):0)^((x)&4?(c):0)^((x)&8?(d):0)^ \
     ((x)&16?(e):0)^((x)&32?(f):0)^((x)&64?(g):0)^((x)&128?(h):0))
#define HASH_LOW(x) HASH_TRANSFORM(x,1,3,6,0x0d,0x1a,0x34,0x69,0xd2)
#define HASH_HIGH(x) HASH_TRANSFORM(x,0xa5,0x4a,0x94,0x28,0x50,0xa1,0x42,0x85)
#define HASH_BYTE(x) HASH_TRANSFORM(x,0x97,0x2e,0x5c,0xb8,0x70,0xe0,0xc0,0x80)
#define HASH_ROW(f,n) f(n),f((n)+1),f((n)+2),f((n)+3), \
    f((n)+4),f((n)+5),f((n)+6),f((n)+7),f((n)+8),f((n)+9), \
    f((n)+10),f((n)+11),f((n)+12),f((n)+13),f((n)+14),f((n)+15)
unsigned char type_hash_low_mix[256] = {
    HASH_ROW(HASH_LOW,0),
    HASH_ROW(HASH_LOW,16),
    HASH_ROW(HASH_LOW,32),
    HASH_ROW(HASH_LOW,48),
    HASH_ROW(HASH_LOW,64),
    HASH_ROW(HASH_LOW,80),
    HASH_ROW(HASH_LOW,96),
    HASH_ROW(HASH_LOW,112),
    HASH_ROW(HASH_LOW,128),
    HASH_ROW(HASH_LOW,144),
    HASH_ROW(HASH_LOW,160),
    HASH_ROW(HASH_LOW,176),
    HASH_ROW(HASH_LOW,192),
    HASH_ROW(HASH_LOW,208),
    HASH_ROW(HASH_LOW,224),
    HASH_ROW(HASH_LOW,240)
};
unsigned char type_hash_high_mix[256] = {
    HASH_ROW(HASH_HIGH,0),
    HASH_ROW(HASH_HIGH,16),
    HASH_ROW(HASH_HIGH,32),
    HASH_ROW(HASH_HIGH,48),
    HASH_ROW(HASH_HIGH,64),
    HASH_ROW(HASH_HIGH,80),
    HASH_ROW(HASH_HIGH,96),
    HASH_ROW(HASH_HIGH,112),
    HASH_ROW(HASH_HIGH,128),
    HASH_ROW(HASH_HIGH,144),
    HASH_ROW(HASH_HIGH,160),
    HASH_ROW(HASH_HIGH,176),
    HASH_ROW(HASH_HIGH,192),
    HASH_ROW(HASH_HIGH,208),
    HASH_ROW(HASH_HIGH,224),
    HASH_ROW(HASH_HIGH,240)
};
unsigned char type_hash_byte[256] = {
    HASH_ROW(HASH_BYTE,0),
    HASH_ROW(HASH_BYTE,16),
    HASH_ROW(HASH_BYTE,32),
    HASH_ROW(HASH_BYTE,48),
    HASH_ROW(HASH_BYTE,64),
    HASH_ROW(HASH_BYTE,80),
    HASH_ROW(HASH_BYTE,96),
    HASH_ROW(HASH_BYTE,112),
    HASH_ROW(HASH_BYTE,128),
    HASH_ROW(HASH_BYTE,144),
    HASH_ROW(HASH_BYTE,160),
    HASH_ROW(HASH_BYTE,176),
    HASH_ROW(HASH_BYTE,192),
    HASH_ROW(HASH_BYTE,208),
    HASH_ROW(HASH_BYTE,224),
    HASH_ROW(HASH_BYTE,240)
};
#undef HASH_ROW
#undef HASH_BYTE
#undef HASH_HIGH
#undef HASH_LOW
#undef HASH_TRANSFORM

unsigned char type_hash_high, type_hash_low, type_hash_mix;

unsigned short __fastcall hash_type(const unsigned char *record)
{
    const unsigned char *cursor = record, *end = record+*(unsigned short *)record+2;
    unsigned short kind = *(unsigned short *)(record+2), remaining, group, i;
    switch (kind) {
    case 4: case 5:
        cursor += 14;
        cursor += *(unsigned short *)cursor < 0x8000 ? 2 : numeric_leaf_size(cursor);
        break;
    case 6:
        cursor += 10;
        cursor += *(unsigned short *)cursor < 0x8000 ? 2 : numeric_leaf_size(cursor);
        break;
    case 7:
        cursor += 12;
        break;
    }
    type_hash_high = type_hash_low = 0;
    remaining = (unsigned short)(end-cursor);
    while (remaining) {
        /* The original unrolls eight bytes backwards, starting with the
           leading short group, then advancing to the next complete group. */
        group = ((remaining+7)&7)+1;
        for (i = group; i; ) {
            --i;
            type_hash_mix = type_hash_low_mix[type_hash_low]^type_hash_high_mix[type_hash_high];
            type_hash_low = type_hash_high;
            type_hash_high = type_hash_byte[cursor[i]^type_hash_mix];
        }
        cursor += group;
        remaining = (unsigned short)(end-cursor);
    }
    return ((unsigned short)type_hash_high<<8|type_hash_low)&0xfff;
}

unsigned short next_type_index = 0x1000;
unsigned char **type_pages[120];
UDTSymbol *last_udt_symbol;

void __fastcall save_type(TypeRecord *entry)
{
    unsigned char *original, *record, *name;
    unsigned char **page;
    unsigned short length, saved_index, kind;
    unsigned long padding, record_size, i, bucket;
    int relative, page_index, slot;
    UDTSymbol *symbol;
    TypeCacheEntry *cached;

    if (type_recursion_error) { entry->index = 0; return; }
    original = entry->data;
    if (*(unsigned short *)(original+2) == 5 && emit_udt_symbols) {
        name = original+14;
        name += *(unsigned short *)name < 0x8000 ? 2 : numeric_leaf_size(name);
        if (*name && (*name != 10 || strncmp((char *)name+1, "(untagged)", 10))) {
            padding = 3-((*name+6)&3);
            record_size = *name+padding+5;
            symbol = checked_alloc(record_size+6);
            if (!module_udt_symbols) module_udt_symbols = symbol;
            else last_udt_symbol->next = symbol;
            symbol->length = (unsigned short)record_size;
            symbol->kind = 4;
            symbol->type = next_type_index;
            for (i = 0; i < (unsigned long)*name+1; ++i) symbol->name[i] = name[i];
            for (i = 0; i < padding; ++i) symbol->name[*name+1+i] = 0;
            last_udt_symbol = symbol;
            symbol->next = NULL;
        }
    }
    if ((int)next_type_index-(int)type_page_capacity == 0x1000) {
        type_page_capacity += 0x200;
        page = checked_alloc(0x800);
        page_index = ((int)next_type_index-0x1000)/512;
#ifdef RECON_NATIVE_TEST
        if (page_index < 0 || page_index >= 120) __asm { int 3 }
#endif
        type_pages[page_index] = page;
    }
    length = *(unsigned short *)original+2;
    if (length > 0xc000) {
        *(unsigned short *)original = 2;
        *(unsigned short *)(original+2) = 0xf;
        length = 4;
        warning(6, NULL, NULL);
    }
    relative = (int)next_type_index-0x1000;
    page_index = relative/512;
    slot = relative%512;
#ifdef RECON_NATIVE_TEST
    if (page_index < 0 || page_index >= 120 || slot < 0) __asm { int 3 }
#endif
    page = type_pages[page_index];
    page[slot] = append_buffer(&type_symbol_storage, original, length);
    if (entry->flags & 0x38) release_type_data(entry);
    entry->data = page[((int)next_type_index-0x1000)%512];
    entry->index = next_type_index++;
    saved_index = next_type_index-1;
    record = entry->data;
    kind = *(unsigned short *)(record+2);
    switch (kind) {
    case 4: case 5:
        name = record+14;
        name += *(unsigned short *)name < 0x8000 ? 2 : numeric_leaf_size(name);
        break;
    case 6:
        name = record+10;
        name += *(unsigned short *)name < 0x8000 ? 2 : numeric_leaf_size(name);
        break;
    case 7:
        name = record+12;
        break;
    default:
        goto complete;
    }
    bucket = *name;
    for (i = 0; i < *name; ++i) bucket += name[i+1];
    bucket &= 511;
    for (cached = ((TypeCacheEntry **)type_work_b)[bucket]; cached; cached = cached->next) {
        if (!cached->type) break;
        if (*(unsigned short *)(cached->type+2) == kind &&
            !strncmp((char *)cached->name, (char *)name, *name+1)) goto complete;
    }
    if (!cached) {
        cached = checked_zero(sizeof(*cached));
        if (!cached) fatal_error(13, NULL, NULL);
        cached->next = ((TypeCacheEntry **)type_work_b)[bucket];
        ((TypeCacheEntry **)type_work_b)[bucket] = cached;
    }
    cached->type = record;
    cached->name = name;
    cached->index = saved_index;
complete:
    if (!next_type_index) {
        warning(9, module_name(current_module), NULL);
        type_recursion_error = 1;
    }
}

void *__fastcall append_buffer(GrowingBuffer *buffer, const void *data, unsigned long size)
{
    BufferBlock *block;
    unsigned char *payload, *result;
    const unsigned char *source = data;
    unsigned long i;
    if (buffer->block_size < size) return NULL;
    if (buffer->available < size) {
        block = checked_zero(sizeof(*block));
        payload = checked_alloc(buffer->block_size);
        block->data = payload;
        if (buffer->last) buffer->last->next = block;
        else buffer->first = block;
        ++buffer->block_count;
        buffer->last = block;
        buffer->next = block->data;
        buffer->available = buffer->block_size;
    }
    result = buffer->next;
    for (i = 0; i < size; ++i) result[i] = source[i];
    buffer->next += size;
    buffer->available -= size;
    buffer->last->used += size;
    return result;
}

/* Deferred local definitions awaiting their named global type match. */
typedef struct TypeMatch {
    unsigned short type, reserved;
    TypeCacheEntry *named;
    struct TypeMatch *next;
} TypeMatch;
TypeMatch *first_type_match, *last_type_match;

void finish_recursive_types(void)
{
    TypeMatch *match = first_type_match;
    TypeRecord *entry;
    unsigned char *original, *record;
    unsigned short relative, unused, converted, *field;
    unsigned short length, other_length;
#ifdef RECON_NATIVE_TEST
    /* The Win32 loop reads next after free. No host allocator lifetime
       equivalence has been established for that nonempty queue path. */
    if (match) __asm { int 3 }
#define MATCH_OFFSET(offset) do { if (entry->recursive_count >= 12) __asm { int 3 } \
    entry->recursion.small[entry->recursive_count++] = (offset); } while (0)
#else
#define MATCH_OFFSET(offset) \
    (entry->recursion.small[entry->recursive_count++] = (offset))
#endif
    while (match) {
        relative = match->type-first_user_type;
        if (!type_signature && relative == (unsigned short)(0xffff-first_user_type))
            entry = empty_type;
        else {
            entry = &local_type_records[relative];
            if (entry->flags & 0x1000)
                entry = get_type_record((unsigned short)(entry->forward_index-0x1000), &unused);
        }
        original = entry->data;
        record = entry->data;
        switch (*(unsigned short *)(record+2)) {
        case 4: case 5:
            field = (unsigned short *)(record+12);
            *field = compact_new_type(*field);
            if (recursive_type_reference) MATCH_OFFSET(12);
            converted = compact_type_list(*(unsigned short *)(record+6), *(unsigned short *)(record+4));
            *(unsigned short *)(entry->data+6) = converted;
            if (recursive_type_reference) MATCH_OFFSET(6);
            break;
        case 6:
            field = (unsigned short *)(record+6);
            *field = compact_type_list(*field, *(unsigned short *)(record+4));
            if (recursive_type_reference) MATCH_OFFSET(6);
            break;
        case 7:
            converted = compact_new_type(*(unsigned short *)(record+6));
            record = entry->data;
            *(unsigned short *)(record+6) = converted;
            if (recursive_type_reference) MATCH_OFFSET(6);
            converted = compact_type_list(*(unsigned short *)(record+8), *(unsigned short *)(record+4));
            *(unsigned short *)(entry->data+8) = converted;
            if (recursive_type_reference) MATCH_OFFSET(8);
            break;
        }
        if (!(match->named->flags & 1)) {
            length = *(unsigned short *)match->named->type;
            other_length = *(unsigned short *)original;
            if (other_length < length) length = other_length;
            memcpy(match->named->type+2, original+2, length);
            match->named->flags |= 1;
        }
        free(match);
        match = match->next;
    }
    last_type_match = first_type_match = NULL;
#undef MATCH_OFFSET
}

int __fastcall find_named_type(TypeRecord *entry, TypeCacheEntry **named, int forward)
{
    unsigned char *record = entry->data, *name;
    unsigned short kind = *(unsigned short *)(record+2), properties;
    int result;
    switch (kind) {
    case 4: case 5:
        name = record+14;
        name += *(unsigned short *)name < 0x8000 ? 2 : numeric_leaf_size(name);
        break;
    case 6:
        name = record+10;
        name += *(unsigned short *)name < 0x8000 ? 2 : numeric_leaf_size(name);
        break;
    case 7:
        name = record+12;
        break;
    default:
        return 0;
    }
    if (forward == 1) {
        properties = *(unsigned short *)(record+(kind == 7 ? 10 : 8));
        result = find_local_type(kind, name, named, (properties&8)>>3);
        if (result == 1) return result;
    }
    return find_global_type(kind, name, named);
}

int find_global_type(unsigned short kind, const unsigned char *name, TypeCacheEntry **named)
{
    unsigned long bucket = *name, i;
    TypeCacheEntry *entry;
    unsigned short properties;
    for (i = 0; i < *name; ++i) bucket += name[i+1];
    bucket &= 511;
    for (entry = ((TypeCacheEntry **)type_work_b)[bucket]; entry && entry->type; entry = entry->next) {
        if (*(unsigned short *)(entry->type+2) != kind ||
            strncmp((const char *)entry->name, (const char *)name, (int)*(const signed char *)name+1)) continue;
        *named = entry;
        switch (*(unsigned short *)(entry->type+2)) {
        case 4: case 5: case 6:
            properties = *(unsigned short *)(entry->type+8);
            break;
        case 7:
            properties = *(unsigned short *)(entry->type+10);
            break;
        default:
            properties = 0;
            break;
        }
        return properties & 0x80 ? 3 : 2;
    }
    return 0;
}

int find_local_type(unsigned short kind, const unsigned char *name, TypeCacheEntry **named, int property)
{
    unsigned long bucket = *name, i;
    TypeCacheEntry *entry;
    unsigned short properties;
    for (i = 0; i < *name; ++i) bucket += name[i+1];
    bucket &= 511;
    for (entry = ((TypeCacheEntry **)type_work_a)[bucket]; entry && entry->type; entry = entry->next) {
        if (*(unsigned short *)(entry->type+2) != kind ||
            strncmp((const char *)entry->name, (const char *)name, (int)*(const signed char *)name+1)) continue;
        switch (*(unsigned short *)(entry->type+2)) {
        case 4: case 5: case 6:
            properties = *(unsigned short *)(entry->type+8);
            break;
        case 7:
            properties = *(unsigned short *)(entry->type+10);
            break;
        default:
            properties = 0;
            break;
        }
        if (property == ((properties&8)>>3)) {
            *named = entry;
            return 1;
        }
    }
    return 0;
}

void record_derived_types(unsigned short type, unsigned short list)
{
    int relative, page, slot, scanning = 1;
    unsigned char *field;
    if (!list) return;
    relative = (int)list-0x1000;
    page = relative/512;
    slot = relative%512;
#ifdef RECON_NATIVE_TEST
    if (page < 0 || page >= 120 || slot < 0) __asm { int 3 }
#endif
    field = type_pages[page][slot]+4;
    do {
        if (*field >= 0xf0) field += *field&15;
        switch (*(unsigned short *)field) {
        case 0x400:
            add_derived_type(type, *(unsigned short *)(field+2));
            field += 6;
            field += *(unsigned short *)field < 0x8000 ? 2 : numeric_leaf_size(field);
            break;
        case 0x401:
            add_derived_type(type, *(unsigned short *)(field+2));
            field += 8;
            field += *(unsigned short *)field < 0x8000 ? 2 : numeric_leaf_size(field);
            field += *(unsigned short *)field < 0x8000 ? 2 : numeric_leaf_size(field);
            break;
        default:
            scanning = 0;
            break;
        }
    } while (scanning == 1);
}

void convert_global_data32(void)
{
    unsigned short *type;
    if (suppressed_symbol_depth) *(unsigned short *)(symbol_input+2) = 8;
    if (!suppressed_symbol_depth) {
        type = (unsigned short *)(symbol_input+10);
        *type = convert_new_type(*type);
        intern_global(symbol_input, hash_public);
    }
}

TypeRecord *get_type_record(unsigned short type, unsigned short *flag)
{
    TypeRecord *entry;
    unsigned short unused;
    *flag = 0;
    if (!type_signature && type == (unsigned short)(0xffff-first_user_type))
        return empty_type;
#ifdef RECON_NATIVE_TEST
    if (type >= local_type_capacity) __asm { int 3 }
#endif
    entry = &local_type_records[type];
    if (entry->flags & 0x1000) {
        *flag = entry->forward_index;
        return get_type_record((unsigned short)(entry->forward_index-0x1000), &unused);
    }
    return entry;
}

void __fastcall add_recursive_offset(TypeRecord *entry, unsigned short offset)
{
    unsigned short *offsets, i;
    unsigned char *marks;
    if (offset <= 255 && entry->recursive_count < 5)
        entry->recursion.small[entry->recursive_count] = (unsigned char)offset;
    else {
        if (!(entry->flags & 0x100)) {
            offsets = checked_zero(20);
            marks = checked_zero(40);
            for (i = 0; i < entry->recursive_count; ++i)
                offsets[i] = entry->recursion.small[i];
            marks[0] = entry->recursion.small[8];
            entry->recursion.large.offsets = offsets;
            entry->recursion.large.marks = marks;
        } else if (!(entry->recursive_count%10)) {
            entry->recursion.large.offsets = checked_realloc(entry->recursion.large.offsets,
                (unsigned long)entry->recursive_count*2+20);
            if (!(entry->recursive_count&7))
                entry->recursion.large.marks = checked_realloc(entry->recursion.large.marks,
                    (unsigned long)entry->recursive_count+10);
        }
        entry->recursion.large.offsets[entry->recursive_count] = offset;
        entry->flags |= 0x100;
    }
    ++entry->recursive_count;
}

unsigned short intern_type(TypeRecord *entry, unsigned short type)
{
    RecursiveTypeEntry *candidate;
    unsigned long bucket;
    unsigned short flags;
    int matched;
    bucket = hash_recursive_type(entry);
    candidate = (RecursiveTypeEntry *)name_hash_buckets[bucket];
    while (candidate) {
        matched = 0;
        if (candidate->index >= first_user_type) {
            flags = entry->flags;
            if (flags & 1) matched = entry->index == candidate->index;
            else if (flags & 2) matched = 1;
            else {
                flags &= 4;
                if (flags) matched = 1;
                /* Retain the original repeated flag test on this branch. */
                else if (entry->index == candidate->index && flags) matched = 1;
                else matched = compare_recursive_type(entry, type, candidate->data, candidate->index);
            }
        }
        if (matched) return candidate->index;
        candidate = candidate->next;
    }
    return commit_recursive_type(type);
}

unsigned long __fastcall hash_recursive_type(TypeRecord *entry)
{
    unsigned char *record = entry->data, *name;
    unsigned short hash, kind, number = 0;
    int size, position, next, i, count;
    unsigned char marks;
    if (entry->hash_value) return entry->hash_value;
    entry->hash_value = 0xfff;
    kind = *(unsigned short *)(record+2);
    hash = kind;
    switch (kind) {
    case 4: case 5:
        name = record+14;
        name += *(unsigned short *)name < 0x8000 ? 2 : numeric_leaf_size(name);
        break;
    case 6:
        name = record+10;
        name += *(unsigned short *)name < 0x8000 ? 2 : numeric_leaf_size(name);
        break;
    case 7:
        name = record+12;
        break;
    case 0x406:
        name = record+8;
        name += *(unsigned short *)name < 0x8000 ? 2 : numeric_leaf_size(name);
        break;
    case 0x407:
        name = record+8;
        break;
    default:
#ifdef RECON_NATIVE_TEST
        /* Inline storage has twelve bytes; expanded lists own separate arrays. */
        if (!(entry->flags & 0x100) && entry->recursive_count > 12) __asm { int 3 }
#endif
        size = *(unsigned short *)record+2;
        position = 0;
        while (position < size) {
            if (number == entry->recursive_count) next = size;
            else {
                next = entry->flags & 0x100 ? entry->recursion.large.offsets[number] : entry->recursion.small[number];
                ++number;
            }
            while (position < next) {
                hash = (unsigned short)((hash>>14) ^ (hash<<2) ^ record[position]);
                ++position;
            }
            position += 2;
        }
        count = entry->recursive_count;
        if (count > 3) count = 3;
        for (i = count-1; i >= 0; --i) {
            marks = entry->flags & 0x100 ? entry->recursion.large.marks[i>>3] : entry->recursion.small[8];
            if (!(marks & (0x80>>(i&7)))) hash ^= hash_referenced_type(entry, (unsigned short)i);
        }
        entry->hash_value = hash&0xfff;
        return entry->hash_value;
    }
    /* The original predecrements the Pascal length, omitting its last byte. */
    count = *name++;
    while (--count > 0) hash = (unsigned short)((hash>>14) ^ (hash<<2) ^ *name++);
    entry->hash_value = hash&0xfff;
    return entry->hash_value;
}

unsigned short __fastcall commit_recursive_type(unsigned short type)
{
    TypeRecord *entry;
    TypeCacheEntry *named;
    unsigned short relative, unused, unused_second, i, offset, *field;
    unsigned char *original, *record, marks;
    int result;
    relative = type-first_user_type;
    if (!type_signature && relative == (unsigned short)(0xffff-first_user_type))
        entry = empty_type;
    else {
#ifdef RECON_NATIVE_TEST
        if (relative >= local_type_capacity) __asm { int 3 }
#endif
        entry = &local_type_records[relative];
        if (entry->flags & 0x1000)
            entry = get_type_record((unsigned short)(entry->forward_index-0x1000), &unused);
    }
    if (entry->flags & 0x1000) {
        relative = entry->forward_index-first_user_type;
        if (!type_signature && relative == (unsigned short)(0xffff-first_user_type))
            entry = empty_type;
        else {
#ifdef RECON_NATIVE_TEST
            if (relative >= local_type_capacity) __asm { int 3 }
#endif
            entry = &local_type_records[relative];
            if (entry->flags & 0x1000)
                entry = get_type_record((unsigned short)(entry->forward_index-0x1000), &unused_second);
        }
    }
    if (entry->flags & 0x806) return entry->index;
    result = find_named_type(entry, &named, 0);
    if (result == 3) {
        original = entry->data;
        entry->index = named->index;
        entry->data = named->type;
    } else {
        save_type(entry);
        original = entry->data;
    }
    entry->flags |= 4;
    publish_recursive_type(entry);
    for (i = 0; i < entry->recursive_count; ++i) {
#ifdef RECON_NATIVE_TEST
        /* Expanded records use the separately owned offset/mark arrays. */
        if (!(entry->flags & 0x100) && i >= 12) __asm { int 3 }
#endif
        marks = entry->flags & 0x100 ? entry->recursion.large.marks[i>>3] : entry->recursion.small[8];
        if (marks & (0x80>>(i&7))) continue;
        offset = entry->flags & 0x100 ? entry->recursion.large.offsets[i] : entry->recursion.small[i];
        field = (unsigned short *)(original+offset);
        *field = commit_recursive_type(*field);
        if (entry->flags & 0x100) entry->recursion.large.marks[i>>3] |= 0x80>>(i&7);
        else entry->recursion.small[8] |= 0x80>>(i&7);
    }
    if (result == 3) memcpy(named->type, original, *(unsigned short *)original+2);
    record = entry->data;
    publish_packed_type(record, entry->index);
    if (*(unsigned short *)(record+2) >= 4 && *(unsigned short *)(record+2) <= 5)
        record_derived_types(entry->index, *(unsigned short *)(record+6));
    return entry->index;
}

unsigned long recursive_type_slots;
RecursiveTypeEntry *recursive_type_cursor;

void __fastcall publish_recursive_type(TypeRecord *entry)
{
    RecursiveTypeEntry *block, *node;
    unsigned long hash;
    if (!recursive_type_slots) {
        block = checked_alloc(0x3000);
        recursive_type_cursor = block;
        recursive_type_slots = 0x400;
    }
    hash = hash_recursive_type(entry);
    --recursive_type_slots;
    node = recursive_type_cursor++;
    node->data = entry->data;
    node->index = entry->index;
    node->next = (RecursiveTypeEntry *)name_hash_buckets[hash];
    name_hash_buckets[hash] = node;
    ++name_hash_counts[hash];
}

unsigned short __fastcall hash_referenced_type(TypeRecord *entry, unsigned short number)
{
    unsigned short offset, type, relative, unused;
    unsigned char *record = entry->data;
    TypeRecord *referenced;
#ifdef RECON_NATIVE_TEST
    /* The selected offset must belong to this record in either representation. */
    if (number >= entry->recursive_count) __asm { int 3 }
#endif
    offset = entry->flags & 0x100 ? entry->recursion.large.offsets[number] : entry->recursion.small[number];
    type = *(unsigned short *)(record+offset);
    if (type < first_user_type) return type;
    relative = type-first_user_type;
    if (!type_signature && relative == (unsigned short)(0xffff-first_user_type))
        referenced = empty_type;
    else {
#ifdef RECON_NATIVE_TEST
        if (relative >= local_type_capacity) __asm { int 3 }
#endif
        referenced = local_type_records+relative;
        if (referenced->flags & 0x1000)
            referenced = get_type_record((unsigned short)(referenced->forward_index-0x1000), &unused);
    }
    if (referenced->flags & 4) return referenced->hash_value;
    return (unsigned short)hash_recursive_type(referenced);
}

unsigned long packed_type_slots;
TypeHashEntry *packed_type_cursor;

void __fastcall publish_packed_type(unsigned char *record, unsigned short type)
{
    TypeHashEntry *block, *node;
    unsigned short bucket;
    if (!packed_type_slots) {
        block = checked_alloc(0x3000);
        packed_type_cursor = block;
        packed_type_slots = 0x400;
    }
    --packed_type_slots;
    node = packed_type_cursor++;
    bucket = hash_type(record);
    node->data = record;
    node->index = type;
    node->next = (TypeHashEntry *)type_hash_buckets[bucket];
    type_hash_buckets[bucket] = node;
    ++type_hash_counts[bucket];
}

void convert_procedure32(void)
{
    unsigned char *type;
    ++symbol_scope_depth;
    if (!address_in_module(current_module, *(unsigned short *)(symbol_input+0x20),
                           *(unsigned long *)(symbol_input+0x1c))) {
        ++suppressed_symbol_depth;
        *(unsigned short *)(symbol_input+2) = 8;
        return;
    }
    type = symbol_input+0x22;
    *(unsigned short *)type = convert_new_type(*(unsigned short *)type);
}

void convert_bp_relative32(void)
{
    unsigned char *type;
    if (suppressed_symbol_depth) {
        *(unsigned short *)(symbol_input+2) = 8;
        /* Retain the original reload after marking the suppressed record. */
        if (suppressed_symbol_depth) return;
    }
    type = symbol_input+8;
    *(unsigned short *)type = convert_new_type(*(unsigned short *)type);
}

void convert_register_symbol(void)
{
    unsigned char *type;
    if (suppressed_symbol_depth) {
        *(unsigned short *)(symbol_input+2) = 8;
        if (suppressed_symbol_depth) return;
    }
    type = symbol_input+4;
    *(unsigned short *)type = convert_new_type(*(unsigned short *)type);
}

void end_symbol_block(void)
{
    --symbol_scope_depth;
    if (suppressed_symbol_depth) {
        *(unsigned short *)(symbol_input+2) = 8;
        --suppressed_symbol_depth;
    }
}

/* CV symbol scope links are offsets from the start of the symbol stream.
   S_END closes the active scope. Search records then chain top-level procedures. */
int link_symbol_scopes(unsigned char *symbols, unsigned long size)
{
    unsigned char *cursor, *end, *search, *first, *last;
    unsigned long parent;
    unsigned short kind;

    parent = 0;
    search = symbols+4;
    end = symbols+size;
    for (cursor = search; cursor < end; cursor += *(unsigned short *)cursor+2) {
        kind = *(unsigned short *)(cursor+2);
        if (kind == 6) {
            unsigned char *scope = symbols+parent;
            *(unsigned long *)(scope+8) = cursor-symbols;
            parent = *(unsigned long *)(scope+4);
        } else if ((kind >= 0x104 && kind <= 0x108) ||
                   (kind >= 0x204 && kind <= 0x208)) {
            *(unsigned long *)(cursor+4) = parent;
            parent = cursor-symbols;
        }
    }
    first = search;
    while (first < end && *(unsigned short *)(first+2) == 5)
        first += *(unsigned short *)first+2;
    while (search < end && *(unsigned short *)(search+2) == 5) {
        last = search;
        for (cursor = first; cursor < end; cursor += *(unsigned short *)cursor+2) {
            kind = *(unsigned short *)(cursor+2);
            if ((kind >= 0x104 && kind <= 0x106) ||
                (kind >= 0x204 && kind <= 0x206)) {
                if (!*(unsigned long *)(cursor+8)) return 0;
                if (*(unsigned short *)(last+2) == 5)
                    *(unsigned long *)(last+4) = cursor-symbols;
                else *(unsigned long *)(last+12) = cursor-symbols;
                last = cursor;
            }
        }
        search += *(unsigned short *)search+2;
    }
    return 1;
}

unsigned long local_symbol_references, local_symbol_reference_bytes;

void record_symbol_offsets(unsigned char *symbols, unsigned long size,
                           unsigned short module)
{
    unsigned char *record;
    unsigned long offset;
    unsigned short kind;

    offset = 4;
    record = symbols+4;
    while (offset < size) {
        kind = *(unsigned short *)(record+2);
        if ((kind >= 0x101 && kind <= 0x102) ||
            (kind >= 0x201 && kind <= 0x202) ||
            (kind >= 0x20d && kind <= 0x20e)) {
            create_symbol_reference(record, type_hash_extra_c, module, offset, 0x401);
            local_symbol_reference_bytes += 16;
            ++local_symbol_references;
        } else if (kind == 0x104 || kind == 0x204 || kind == 0x300) {
            create_symbol_reference(record, type_hash_extra_c, module, offset, 0x400);
            offset = *(unsigned long *)(record+8);
            record = symbols+offset;
            local_symbol_reference_bytes += 16;
            ++local_symbol_references;
        } else if (kind == 0x105 || kind == 0x205 || kind == 0x301) {
            create_symbol_reference(record, global_symbol_hash, module, offset, 0x400);
            offset = *(unsigned long *)(record+8);
            record = symbols+offset;
            global_symbols_size += 16;
            ++global_symbols_count;
        }
        offset += *(unsigned short *)record+2;
        record += *(unsigned short *)record+2;
    }
}

/* S_PROCREF/S_DATAREF point back into a module symbol stream rather than
   carrying an inline name. The final alignment word is not initialized. */
typedef struct SymbolReferenceRecord {
    unsigned short length, kind;
    unsigned long checksum, offset;
    unsigned short module;
} SymbolReferenceRecord;
typedef char SymbolReferenceRecordSize[(sizeof(SymbolReferenceRecord)==16)?1:-1];

void create_symbol_reference(unsigned char *record, void **buckets,
                             unsigned short module, unsigned long offset,
                             unsigned short kind)
{
    unsigned short segment = 0, name_offset, bucket;
    unsigned long hash, address = 0;
    PublicSymbol *node;
    SymbolReferenceRecord *reference;

    bucket = hash_public(record, &hash, &name_offset, &segment, &address, 0x1000);
    node = allocate_symbol(36);
    node->hash = hash;
    node->name_offset = name_offset;
    node->segment = segment;
    node->offset = address;
    reference = (SymbolReferenceRecord *)node->record;
    reference->length = 14;
    reference->kind = kind;
    reference->checksum = hash;
    reference->module = module;
    reference->offset = offset;
    node->next = buckets[bucket];
    buckets[bucket] = node;
}

void begin_symbol_block(void)
{
    ++symbol_scope_depth;
    if (suppressed_symbol_depth) {
        *(unsigned short *)(symbol_input+2) = 8;
        ++suppressed_symbol_depth;
    }
}

/* Resolve derived-type leaves before releasing the construction hash indices. */
unsigned short derived_type_count;

void finalize_types(void)
{
    finalize_derived_types();
    free(derived_types);
    free(name_hash_buckets);
    free(type_hash_buckets);
    free(type_hash_extra_d);
}

void finalize_derived_types(void)
{
    TypeDerivation *derived;
    TypeHashEntry *node, *created;
    unsigned char *scratch, *owner;
    unsigned short i, count, j, bucket;
    unsigned long k, size;
    int relative;

    if (type_recursion_error) return;
    for (i = 0; i < derived_type_count; ++i) {
        derived = &derived_types[i];
        count = derived->count;
        scratch = type_scratch(count*2+6);
        *(unsigned short *)scratch = (unsigned short)(count*2+4);
        *(unsigned short *)(scratch+2) = 0x205;
        *(unsigned short *)(scratch+4) = count;
        for (j = 0; count; ++j, --count)
            ((unsigned short *)(scratch+6))[j] = derived->list[count-1];
        free(derived->list);
        bucket = hash_type(scratch);
        node = (TypeHashEntry *)type_hash_buckets[bucket];
        while (node) {
            size = *(unsigned short *)node->data+2;
            for (k = 0; k < size && node->data[k] == scratch[k]; ++k) {}
            if (k == size) break;
            node = node->next;
        }
        relative = (int)derived->type-0x1000;
        owner = type_pages[relative/512][relative%512]+2;
        if (!node) {
            created = checked_alloc(sizeof(*created));
            created->index = next_type_index;
            created->data = append_buffer(&type_symbol_storage, scratch,
                                         *(unsigned short *)scratch+2);
            created->next = (TypeHashEntry *)type_hash_buckets[bucket];
            type_hash_buckets[bucket] = created;
            ++type_hash_counts[bucket];
            if ((int)next_type_index-(int)type_page_capacity == 0x1000) {
                type_page_capacity += 512;
                type_pages[((int)next_type_index-0x1000)/512] = checked_alloc(2048);
            }
            relative = (int)next_type_index-0x1000;
            type_pages[relative/512][relative%512] = created->data;
            *(unsigned short *)(owner+8) = next_type_index;
            if (!++next_type_index) {
                warning(9, module_name(current_module), NULL);
                type_recursion_error = 1;
            }
        } else *(unsigned short *)(owner+8) = node->index;
    }
}

/* Original [0001e780,0001eab5): compare bytes outside unresolved type links,
   then recursively compare those links with their published global records. */
int __fastcall compare_recursive_type(TypeRecord *entry, unsigned short type,
                                      unsigned char *record, unsigned short index)
{
    unsigned char *original, marks;
    unsigned short length, position, number, next, count, i;
    unsigned short offset, relative, unused, global_type, flags;
    unsigned short *field;
    TypeRecord *referenced;
    unsigned char *global_record;
    int page, slot, global_relative, matched;

    entry->index = index;
    original = entry->data;
    entry->flags |= 1;
    length = (unsigned short)(*(unsigned short *)original + 2);
    number = position = 0;
#ifdef RECON_NATIVE_TEST
    /* Expanded records use the separately owned offset/mark arrays. */
    if (!(entry->flags & 0x100) && entry->recursive_count > 12) __asm { int 3 }
#endif
    while (position < length) {
        while (number != entry->recursive_count) {
            marks = entry->flags & 0x100 ? entry->recursion.large.marks[number>>3] :
                entry->recursion.small[8];
            if (!(marks & (0x80 >> ((entry->flags & 0x100) ? (number&7) : number))))
                break;
            ++number;
        }
        if (number == entry->recursive_count) next = length;
        else {
            next = entry->flags & 0x100 ? entry->recursion.large.offsets[number] :
                entry->recursion.small[number];
            ++number;
        }
        count = (unsigned short)(next-position);
        if (count) {
            /* The original intrinsic compares this same byte interval. */
            for (i = 0; i < count; ++i)
                if (original[position+i] != record[position+i]) goto different;
            position = next;
        }
        position += 2;
    }
    for (number = 0; number < entry->recursive_count; ++number) {
        marks = entry->flags & 0x100 ? entry->recursion.large.marks[number>>3] :
            entry->recursion.small[8];
        if (marks & (0x80 >> ((entry->flags & 0x100) ? (number&7) : number)))
            continue;
        offset = entry->flags & 0x100 ? entry->recursion.large.offsets[number] :
            entry->recursion.small[number];
        field = (unsigned short *)(original+offset);
        relative = *field-first_user_type;
        if (!type_signature && relative == (unsigned short)(0xffff-first_user_type))
            referenced = empty_type;
        else {
#ifdef RECON_NATIVE_TEST
            if (relative >= local_type_capacity) __asm { int 3 }
#endif
            referenced = &local_type_records[relative];
            if (referenced->flags & 0x1000)
                referenced = get_type_record((unsigned short)(referenced->forward_index-0x1000), &unused);
        }
        global_type = *(unsigned short *)(record+offset);
        if (global_type < first_user_type) goto different;
        global_relative = (int)global_type-0x1000;
        page = global_relative/512;
        slot = global_relative%512;
#ifdef RECON_NATIVE_TEST
        if (page < 0 || page >= 120 || slot < 0) __asm { int 3 }
#endif
        global_record = type_pages[page][slot];
        if (global_type < first_user_type) matched = 0;
        else {
            flags = referenced->flags;
            if (flags & 1) matched = referenced->index == global_type;
            else if (flags & 2) matched = 1;
            else {
                flags &= 4;
                if (flags) matched = 1;
                /* Keep the original repeated, now-zero flag test. */
                else if (referenced->index == global_type && flags) matched = 1;
                else matched = compare_recursive_type(referenced, *field, global_record, global_type);
            }
        }
        if (!matched) goto different;
    }
    entry->flags &= ~1;
    entry->flags |= 2;
    return 1;
different:
    entry->flags &= ~1;
    return 0;
}

/* Original [0001c500,0001c63b): accumulate derived classes by base type. */
void add_derived_type(unsigned short type, unsigned short base)
{
    unsigned short i;
    TypeDerivation *entry;
    for (i = 0; i < derived_type_count; ++i)
        if (derived_types[i].type == base) break;
    if (i == derived_type_count) {
        if (derived_type_count == derived_type_capacity) {
            derived_type_capacity += 128;
            derived_types = checked_realloc(derived_types,
                (unsigned long)derived_type_capacity*sizeof(*derived_types));
        }
        entry = &derived_types[i];
        entry->count = 0;
        entry->capacity = 20;
        entry->type = base;
        entry->list = checked_alloc(40);
        ++derived_type_count;
    }
    entry = &derived_types[i];
    if (entry->count == entry->capacity) {
        entry->capacity += 20;
        entry->list = checked_realloc(entry->list, (unsigned long)entry->capacity*2);
    }
    entry->list[entry->count] = type;
    ++entry->count;
}

/* Original [0001f1d0,0001f21f): encoded numeric leaf length. */
unsigned short numeric_leaf_size(const unsigned char *leaf)
{
    unsigned short kind = *(const unsigned short *)leaf;
    switch (kind) {
    case 0x8000: return 3;             /* LF_CHAR */
    case 0x8001:                      /* LF_SHORT */
    case 0x8002: return 4;             /* LF_USHORT */
    case 0x8003:                      /* LF_LONG */
    case 0x8004:                      /* LF_ULONG */
    case 0x8005: return 6;             /* LF_REAL32 */
    case 0x8006: return 10;            /* LF_REAL64 */
    case 0x8007: return 12;            /* LF_REAL80 */
    case 0x8008: return 18;            /* LF_REAL128 */
    default:
        if (kind < 0x8000) return 2;
        /* The original leaves its switch index in AX for unknown leaves. */
        return (unsigned short)(kind-0x8000);
    }
}

/* Original [00025570,000255be): remap a constant's type and intern globals. */
void convert_constant_symbol(void)
{
    unsigned short *type;
    if (suppressed_symbol_depth) {
        *(unsigned short *)(symbol_input+2) = 8;
        if (suppressed_symbol_depth) return;
    }
    type = (unsigned short *)(symbol_input+4);
    *type = convert_new_type(*type);
    if (!symbol_scope_depth) intern_global(symbol_input, hash_public);
}

/* Original [000256b0,000256e5): local 32-bit data symbol type. */
void convert_local_data32(void)
{
    unsigned short *type;
    if (suppressed_symbol_depth) {
        *(unsigned short *)(symbol_input+2) = 8;
        if (suppressed_symbol_depth) return;
    }
    type = (unsigned short *)(symbol_input+10);
    *type = convert_new_type(*type);
}

/* Reused temporary storage for synthesized type records. */
void *type_scratch_buffer;
unsigned short type_scratch_capacity;

/* Original [0001efb0,0001f021). The stored capacity truncates to 16 bits. */
void *type_scratch(unsigned long size)
{
    void *buffer = type_scratch_buffer;
    if (type_scratch_capacity < size) {
        if (buffer) free(buffer);
        if (size <= 256) size = 256;
        type_scratch_capacity = (unsigned short)size;
        buffer = malloc(type_scratch_capacity);
        type_scratch_buffer = buffer;
        if (!buffer) {
            fatal_error(13, NULL, NULL);
            buffer = type_scratch_buffer;
        }
    }
    type_scratch_buffer = buffer;
    return buffer;
}

/* Original [0001e530,0001e6f9): append an old/precompiled type slot. */
#pragma intrinsic(memset)
void __fastcall add_old_type(const unsigned char *record, int is_new,
                            int flag, int extra, unsigned short skip)
{
    unsigned short old_count = local_type_count, forward = 0, kind;
    TypeRecord *entry;
    if (previous_type_count) {
        memset(local_type_records, 0, (unsigned long)previous_type_count*sizeof(*local_type_records));
        previous_type_count = 0;
    }
    if (skip) local_type_count += skip;
    else ++local_type_count;
    if (local_type_count >= local_type_capacity) {
        local_type_capacity += skip+6000;
        local_type_records = realloc(local_type_records,
            (unsigned long)local_type_capacity*sizeof(*local_type_records));
        if (!local_type_records) fatal_error(13, NULL, NULL);
        memset(&local_type_records[old_count], 0,
            ((unsigned long)skip+6000)*sizeof(*local_type_records));
    }
    entry = &local_type_records[old_count];
    entry->data = (unsigned char *)record;
    entry->index = (unsigned short)extra;
    entry->flags = (entry->flags & ~0x400) | ((is_new & 1)<<10);
    entry->flags = (entry->flags & ~0x800) | ((flag & 1)<<11);
    if (record) {
        kind = *(const unsigned short *)(record+2);
        switch (kind) {
        case 4: case 5: case 6:
            forward = (*(const unsigned short *)(record+8) & 0x80)>>7;
            break;
        case 7:
            forward = (*(const unsigned short *)(record+10) & 0x80)>>7;
            break;
        }
    }
    entry->flags = (entry->flags & ~0x4000) | (forward<<14);
}

/* Original [000260c0,00026277): save mappings before the dependent module. */
void save_precompiled(ModuleInfo *module)
{
    PrecompiledTypes *saved;
    TypeRecord *entry;
    unsigned short i, unused, kind, index;
    module->precompiled_bytes = (unsigned short)(module_flags*2-0x1ff4);
    module->precompiled = malloc(module->precompiled_bytes);
    if (!module->precompiled) fatal_error(14, NULL, NULL);
    saved = module->precompiled;
    /* Keep the original second check after reloading the published pointer. */
    if (!saved) fatal_error(14, NULL, NULL);
    memset(saved, 0, module->precompiled_bytes);
    saved->signature = module->precompiled_signature;
    saved->count = (unsigned short)(module_flags-0x1000);
    saved->first = 0x1000;
    for (i = 0; (int)module_flags-0x1000 > i; ++i) {
        if (!type_signature && first_user_type == (unsigned short)(0xffff-i))
            entry = empty_type;
        else {
            entry = &local_type_records[i];
            if (entry->flags & 0x1000)
                entry = get_type_record((unsigned short)(entry->forward_index-0x1000), &unused);
        }
        if (entry->flags & 6) index = entry->index;
        else {
            kind = *(unsigned short *)(entry->data+2);
            if ((kind >= 1 && kind <= 15) || kind == 17 || kind == 0x202 ||
                kind == 0x206 || (kind >= 0x208 && kind <= 0x20c))
                index = convert_new_type((unsigned short)(i+0x1000));
            else if (kind == 0x201)
                index = compact_type_list((unsigned short)(i+0x1000), *(unsigned short *)(entry->data+4));
            else continue;
        }
        saved->indices[i] = index;
    }
    module_flags = 0;
}

/* Original [0001f140,0001f16b): application allocation-failure wrapper. */
void *checked_realloc(void *block, unsigned long size)
{
    void *result = realloc(block, size);
    if (!result) fatal_error(13, NULL, NULL);
    return result;
}

/* Consecutive modules may reuse the same saved precompiled translation. */
unsigned short precompiled_generation, precompiled_type_count;
ModuleInfo *previous_precompiled_module;

/* Original [0001a2e0,0001a4d0): import a saved type-index translation. */
void load_precompiled_types(const unsigned char *record)
{
    ModuleInfo *module = find_precompiled_module(record+10);
    PrecompiledTypes *saved = module->precompiled;
    unsigned short i, count = *(const unsigned short *)(record+4);
    if (!saved) fatal_error(14, NULL, NULL);
    if (module->precompiled_signature != *(const unsigned long *)(record+6))
        fatal_error(33, module_name(current_module), module_name(module));
    if (saved->count != count ||
        (int)*(const unsigned short *)(record+2)-(int)local_type_count != 0x1000)
        fatal_error(30, module_name(current_module), module_name(module));
    if ((int)precompiled_generation-(int)type_module_count == -1 &&
        previous_precompiled_module == module) {
        precompiled_generation = type_module_count;
        local_type_count = precompiled_type_count;
        if (previous_type_count > precompiled_type_count)
            memset(&local_type_records[precompiled_type_count], 0,
                (unsigned long)(previous_type_count-precompiled_type_count)*sizeof(*local_type_records));
        previous_type_count = 0;
        return;
    }
    if (previous_type_count) {
        memset(local_type_records, 0, (unsigned long)previous_type_count*sizeof(*local_type_records));
        previous_type_count = 0;
    }
    for (i = 0; i < *(const unsigned short *)(record+4); ++i) {
#ifdef RECON_NATIVE_TEST
        if (local_type_count >= local_type_capacity) __asm { int 3 }
#endif
        local_type_records[local_type_count].index = saved->indices[i];
        local_type_records[local_type_count].flags |= 0x400;
        local_type_records[local_type_count].flags |= 0x800;
        ++local_type_count;
    }
    precompiled_generation = type_module_count;
    precompiled_type_count = local_type_count;
    previous_precompiled_module = module;
}

/* Original [0001abf0,0001ac77): match the counted PCH object name. */
ModuleInfo *find_precompiled_module(const unsigned char *name)
{
    ModuleInfo *module;
    char filename[260];
    for (module = module_list; module; module = module->next) {
        if (module->object_name && *module->object_name == *name) {
#ifdef RECON_NATIVE_TEST
            /* The original sign-extends the count; unsupported negative sizes. */
            if ((signed char)*name < 0) __asm { int 3 }
#endif
            if (!_strnicmp((const char *)name+1, (const char *)module->object_name+1,
                           (signed char)*name)) return module;
        }
    }
#ifdef RECON_NATIVE_TEST
    if ((signed char)*name < 0) __asm { int 3 }
#endif
    strncpy(filename, (const char *)name+1, (signed char)*name);
    filename[(signed char)*name] = 0;
    fatal_error(29, filename, NULL);
}

void convert_global_data16(void)
{
    unsigned short *type;
    if (suppressed_symbol_depth) {
        *(unsigned short *)(symbol_input+2) = 8;
        if (suppressed_symbol_depth) return;
    }
    type = (unsigned short *)(symbol_input+8);
    *type = convert_new_type(*type);
    intern_global(symbol_input, hash_public);
}

void convert_local_data16(void)
{
    unsigned short *type;
    if (suppressed_symbol_depth) {
        *(unsigned short *)(symbol_input+2) = 8;
        if (suppressed_symbol_depth) return;
    }
    type = (unsigned short *)(symbol_input+8);
    *type = convert_new_type(*type);
}

void convert_procedure16(void)
{
    unsigned char *type;
    ++symbol_scope_depth;
    if (!address_in_module(current_module, *(unsigned short *)(symbol_input+0x18),
                           *(unsigned short *)(symbol_input+0x16))) {
        ++suppressed_symbol_depth;
        *(unsigned short *)(symbol_input+2) = 8;
        return;
    }
    type = symbol_input+0x1a;
    *(unsigned short *)type = convert_new_type(*(unsigned short *)type);
}

void convert_bp_relative16(void)
{
    unsigned char *type;
    if (suppressed_symbol_depth) {
        *(unsigned short *)(symbol_input+2) = 8;
        if (suppressed_symbol_depth) return;
    }
    type = symbol_input+6;
    *(unsigned short *)type = convert_new_type(*(unsigned short *)type);
}

unsigned char *small_type_free_list;
unsigned short small_type_free_count;

unsigned char *medium_type_free_list;
unsigned short medium_type_free_count;

/* Splice the next LF_FIELDLIST payload over the current LF_INDEX. */
#pragma intrinsic(memcpy)
void merge_field_list(unsigned char *record, TypeRecord *entry)
{
    unsigned short type = *(unsigned short *)(record+2)-first_user_type;
    unsigned short unused, payload;
    unsigned long prefix, size;
    TypeRecord *next;
    unsigned char *source, *buffer;
    if (!type_signature && type == (unsigned short)(0xffff-first_user_type))
        next = empty_type;
    else {
#ifdef RECON_NATIVE_TEST
        if (type >= local_type_capacity) __asm { int 3 }
#endif
        next = &local_type_records[type];
        if (next->flags & 0x1000)
            next = get_type_record((unsigned short)(next->forward_index-0x1000), &unused);
    }
    source = next->data;
    prefix = (unsigned short)(record-entry->data);
    payload = *(unsigned short *)source-2;
    size = prefix+payload;
    if (size > 900) buffer = checked_alloc(size);
    else if (size > 32) buffer = allocate_medium_type();
    else buffer = allocate_small_type();
    memcpy(buffer, entry->data, prefix);
    if (entry->flags & 0x38) release_type_data(entry);
    entry->data = buffer;
    if (size > 900) entry->flags |= 8;
    else if (size > 32) entry->flags |= 0x20;
    else entry->flags |= 0x10;
    memcpy(buffer+prefix, source+4, payload);
    *(unsigned short *)entry->data += (unsigned short)(payload-4);
}

void *allocate_small_type(void)
{
    unsigned char *buffer;
    if (!small_type_free_list) return checked_alloc(32);
    buffer = small_type_free_list;
    small_type_free_list = *(unsigned char **)buffer;
    --small_type_free_count;
    return buffer;
}

void recycle_small_type(void *buffer)
{
    *(unsigned char **)buffer = small_type_free_list;
    small_type_free_list = buffer;
    ++small_type_free_count;
}

void *allocate_medium_type(void)
{
    unsigned char *buffer;
    if (!medium_type_free_list) return checked_alloc(900);
    buffer = medium_type_free_list;
    medium_type_free_list = *(unsigned char **)buffer;
    --medium_type_free_count;
    return buffer;
}

void release_type_data(TypeRecord *entry)
{
    if (entry->flags & 8) {
        free(entry->data);
        entry->flags &= ~8;
        return;
    }
    if (entry->flags & 0x20) {
        recycle_medium_type(entry->data);
        entry->flags &= ~0x20;
        return;
    }
    if (entry->flags & 0x10) {
        recycle_small_type(entry->data);
        entry->flags &= ~0x10;
    }
}

void recycle_medium_type(void *buffer)
{
    *(unsigned char **)buffer = medium_type_free_list;
    medium_type_free_list = buffer;
    ++medium_type_free_count;
}
