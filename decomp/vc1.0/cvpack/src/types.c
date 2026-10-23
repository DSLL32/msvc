/* Allocation and initial state of the type compactor's tables. */
#include "types.h"
#include "image.h"
#include <recon.h>

void **name_hash_buckets, **type_hash_buckets;
unsigned short *name_hash_counts, *type_hash_counts;
void **global_symbol_hash, **public_symbol_hash, **type_hash_extra_c, **type_hash_extra_d;
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
    global_symbol_hash = checked_zero(0x4000);
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
        entry->recursive_offsets[entry->recursive_count++] = (offset); \
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
        entry->recursive_offsets[entry->recursive_count++] = 6;
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
                entry->recursive_offsets[entry->recursive_count++] = (unsigned char)offset;
            }
        } else if (kind == 8) {
            converted = convert(*(unsigned short *)(record+6));
            *(unsigned short *)(entry->data+8) = converted;
            if (recursive_type_reference) {
#ifdef RECON_NATIVE_TEST
                if (entry->recursive_count >= 12) __asm { int 3 }
#endif
                entry->recursive_offsets[entry->recursive_count++] = 8;
            }
        }
    } else if (mode == 2 || mode == 3) {
        converted = convert(*(unsigned short *)(record+6));
        *(unsigned short *)(entry->data+8) = converted;
        if (recursive_type_reference) {
#ifdef RECON_NATIVE_TEST
            if (entry->recursive_count >= 12) __asm { int 3 }
#endif
            entry->recursive_offsets[entry->recursive_count++] = 8;
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
