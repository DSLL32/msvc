/* Per-module subsection dispatch and type/symbol/line conversion. */
#include "image.h"
#include "types.h"
#include <recon.h>

unsigned char current_module_pe;
unsigned short module_type_base;
unsigned short module_type_count;
unsigned short module_symbol_count;
UDTSymbol *module_udt_symbols;
unsigned short emit_udt_symbols;
int types_compacted;
unsigned long type_signature;
unsigned long type_directory;
unsigned long symbol_directory;
unsigned long public_directory;
unsigned long line_directory;
unsigned long old_line_directory;
int current_precompiled;
unsigned short module_flags;
ModuleInfo * current_module;
void * current_type_map;
unsigned char *module_record_buffer;
unsigned long total_new_type_bytes;
unsigned long total_public_bytes;

int pack_module(unsigned short module)
{
    CvDirectoryEntry *entry;
    ModuleIndex *item = &module_index[module];
    unsigned short number;
    unsigned long directory;
    char progress[80];
    unsigned count;

    current_module_pe = is_pe_image;
    module_type_base = module_type_count = module_symbol_count = emit_udt_symbols = 0;
    module_udt_symbols = 0;
    types_compacted = 0;
    type_signature = 0x0fffffff;
    type_directory = symbol_directory = public_directory = line_directory = old_line_directory = 0;
    current_precompiled = 0;
    module_flags = 0;
    directory = item->directory;
    current_module = item->info;
    current_type_map = NULL;
    number = item->module;
    entry = &directory_entries[directory];
    if (entry->subsection == 0x101) read_old_module(directory, current_module);
    else if (entry->subsection == 0x120) read_module(entry, current_module);
    if (feedback) {
        _snprintf(progress, 80, "@I4%s\n", module_name(current_module));
        for (count = 0; progress[count]; ++count) {}
        _write(2, progress, count);
    }
    for (++directory; directory < directory_entry_count; ++directory) {
        entry = &directory_entries[directory];
        if (entry->module != number) break;
        if (!entry->size) continue;
        switch (entry->subsection) {
        case 0x102: case 0x122:
            read_symbols(directory, entry);
            break;
        case 0x103: case 0x121:
            read_global_types(entry, 0);
            break;
        case 0x104: case 0x124:
            read_types(directory, entry);
            break;
        case 0x109: case 0x126:
            read_lines(directory, entry);
            break;
        case 0x123:
            read_publics(directory, entry);
            break;
        case 0x127:
            read_source_module(directory, entry);
            break;
        case 0x12f:
            current_precompiled = 1;
            read_global_types(entry, 1);
            save_precompiled(current_module);
            break;
        }
    }
    if (symbol_directory) {
        prepare_symbols(symbol_data, directory_entries[symbol_directory].size);
        pack_new_symbols(symbol_data, &directory_entries[symbol_directory]);
    } else if (public_directory && *(unsigned long *)symbol_data == 1) {
        pack_public_symbols(symbol_data, &directory_entries[public_directory]);
        total_public_bytes += directory_entries[public_directory].size;
    }
    if (type_directory) {
        if (minimum_types == 1) directory_entries[type_directory].size = 0;
        else if (*(unsigned long *)type_data != 1) {
            prepare_old_types(type_data, directory_entries[type_directory].size);
            pack_old_types(type_data, &directory_entries[type_directory], module_record_buffer, current_module);
        } else {
            total_new_type_bytes += directory_entries[type_directory].size;
            index_new_types(type_data, directory_entries[type_directory].size, &module_type_base, &module_type_count);
            pack_new_types(type_data, &directory_entries[type_directory], current_module, &module_type_base, &module_type_count);
        }
    }
    if (line_directory || old_line_directory) statistics_enabled = 1;
    if (line_directory) pack_new_lines(module_data, &directory_entries[line_directory], current_module);
    else if (old_line_directory) pack_old_lines(module_data, &directory_entries[old_line_directory], current_module);
    return 1;
}

unsigned short subsection_reads;

void read_module(CvDirectoryEntry *entry, ModuleInfo *module)
{
    unsigned char *buffer = malloc(entry->size);
    if (!buffer) fatal_error(14, NULL, NULL);
    if (cv_read_offset != (unsigned long)entry->location.offset) {
        cv_read_offset = entry->location.offset;
        ++subsection_reads;
        seek_file(input_file, cv_file_offset+cv_read_offset, 0);
    }
    module_record_buffer = buffer;
    if (!buffer) fatal_error(14, NULL, NULL);
    if (read_file(input_file, module_record_buffer, entry->size) != entry->size)
        fatal_error(17, NULL, NULL);
    cv_read_offset += entry->size;
    entry->location.data = buffer;
    module->record_size = entry->size;
    module->record = buffer;
}

void read_publics(unsigned long directory, const CvDirectoryEntry *entry)
{
    if (cv_read_offset != (unsigned long)entry->location.offset) {
        cv_read_offset = entry->location.offset;
        ++subsection_reads;
        seek_file(input_file, cv_file_offset+cv_read_offset, 0);
    }
    if (read_file(input_file, symbol_data, entry->size) != entry->size)
        fatal_error(19, "Publics", module_name(current_module));
    cv_read_offset += entry->size;
    public_directory = directory;
}

unsigned long symbol_scope_depth;

void pack_public_symbols(void *symbols, CvDirectoryEntry *entry)
{
    unsigned char record[512];
    unsigned char *cursor, *end;
    unsigned short length, kind;
    unsigned long i;
    symbol_scope_depth = 0;
    if (!entry->size) return;
    end = (unsigned char *)symbols+entry->size;
    for (cursor = (unsigned char *)symbols+4; cursor < end; cursor += length) {
        length = (unsigned short)(cursor[0]+((unsigned short)cursor[1]<<8)+2);
        kind = (unsigned short)(cursor[2]+((unsigned short)cursor[3]<<8));
        for (i = 0; i < length; ++i) record[i] = cursor[i];
        switch (kind) {
        case 0x103:
            if (type_signature != 1) {
                if (minimum_types == 1) *(unsigned short *)(record+8) = 0;
                else *(unsigned short *)(record+8) = convert_old_type(*(unsigned short *)(record+8));
            } else if (minimum_types == 1) {
                if (*(unsigned short *)(record+8) >= 0x1000) *(unsigned short *)(record+8) = 0;
            } else *(unsigned short *)(record+8) = convert_new_type(*(unsigned short *)(record+8));
            break;
        case 0x203:
            if (type_signature != 1) {
                if (minimum_types == 1) *(unsigned short *)(record+8) = 0;
                else *(unsigned short *)(record+10) = convert_old_type(*(unsigned short *)(record+10));
            } else if (minimum_types == 1) {
                if (*(unsigned short *)(record+10) >= 0x1000) *(unsigned short *)(record+10) = 0;
            } else *(unsigned short *)(record+10) = convert_new_type(*(unsigned short *)(record+10));
            break;
        }
        intern_public(record, hash_public);
    }
}

unsigned long public_symbols_count, public_symbols_size, duplicate_symbols;

int intern_public(unsigned char *record, SymbolHash hash)
{
    unsigned short segment = 0, name_offset, name_length, bucket, padding;
    unsigned long symbol_offset = 0, full_hash, i, count;
    PublicSymbol *node;
    const unsigned char *name;
    int same;
    bucket = hash(record, &full_hash, &name_offset, &segment, &symbol_offset, 0x1000);
    name = record+name_offset;
    name_length = *name;
    for (node = public_symbol_hash[bucket]; node; node = node->next) {
        if (node->hash != full_hash) continue;
        same = 1;
        for (i = 0; i < (unsigned long)name_length+1; ++i)
            if (name[i] != node->record[node->name_offset+i]) { same = 0; break; }
        if (same) goto duplicate;
    }
    padding = 3-((*(unsigned short *)record+1)&3);
    node = allocate_symbol(*(unsigned short *)record+padding+0x16);
    node->hash = full_hash;
    node->name_offset = name_offset;
    node->segment = segment;
    node->offset = symbol_offset;
    count = *(unsigned short *)record+2;
    for (i = 0; i < count; ++i) node->record[i] = record[i];
    *(unsigned short *)node->record += padding;
    public_symbols_size += *(unsigned short *)record+padding+2;
    ++public_symbols_count;
    for (i = 0; i < padding; ++i) node->record[count+i] = 0;
    node->next = public_symbol_hash[bucket];
    public_symbol_hash[bucket] = node;
    *(unsigned short *)(record+2) = 8;
    return 1;
duplicate:
    count = (unsigned long)*(unsigned short *)record-node->name_offset-2;
    same = 1;
    for (i = 0; i < count; ++i)
        if (record[2+i] != node->record[2+i]) { same = 0; break; }
    if (same) {
        *(unsigned short *)(record+2) = 8;
        ++duplicate_symbols;
        return 0;
    }
    warning(2, (char *)record+11, module_name(current_module));
    return 2;
}

unsigned short __cdecl hash_public(const unsigned char *record, unsigned long *hash,
    unsigned short *name_offset, unsigned short *segment, unsigned long *offset,
    unsigned short buckets)
{
    const unsigned char *name;
    unsigned short kind = *(const unsigned short *)(record+2), section = 0;
    unsigned long address = 0, combined = 0, tail = 0, length;
    switch (kind) {
    case 3:
        name = record+6;
        name += *(const unsigned short *)name < 0x8000 ? 2 : numeric_leaf_size(name);
        break;
    case 4: name = record+6; break;
    case 0x101: case 0x102: case 0x103:
        name = record+10;
        section = *(const unsigned short *)(record+6);
        address = *(const unsigned short *)(record+4);
        break;
    case 0x104: case 0x105:
        name = record+29;
        section = *(const unsigned short *)(record+24);
        address = *(const unsigned short *)(record+22);
        break;
    case 0x201: case 0x202: case 0x203: case 0x20d: case 0x20e:
        name = record+12;
        section = *(const unsigned short *)(record+8);
        address = *(const unsigned long *)(record+4);
        break;
    case 0x204: case 0x205:
        name = record+37;
        section = *(const unsigned short *)(record+32);
        address = *(const unsigned long *)(record+28);
        break;
    case 0x300: case 0x301:
        name = record+54;
        section = *(const unsigned short *)(record+48);
        address = *(const unsigned long *)(record+44);
        break;
    default:
        *segment = 0; *offset = 0; *hash = 0;
        return 0;
    }
    *name_offset = (unsigned short)(name-record);
    length = *name++;
    while (length&3) {
        tail |= name[length-1]&0xdf;
        tail <<= 8;
        --length;
    }
    for (length >>= 2; length; --length, name += 4) {
        combined ^= *(const unsigned long *)name&0xdfdfdfdf;
        combined = (combined<<4)|(combined>>28);
    }
    *segment = section;
    *offset = address;
    *hash = combined^tail;
    return (unsigned short)(*hash%buckets);
}

unsigned long public_symbol_available;
unsigned char *public_symbol_cursor;

void *allocate_symbol(unsigned long size)
{
    unsigned char *result;
    size &= 0xffff;
    if (public_symbol_available < size) {
        result = checked_alloc(0x1fa4);
        public_symbol_available = 0x1fa4;
    } else result = public_symbol_cursor;
    public_symbol_available -= size;
    public_symbol_cursor = result+size;
    return result;
}

unsigned long module_type_bytes;
int module_types_indexed;
unsigned short first_user_type, local_type_count, previous_type_count;

void read_global_types(const CvDirectoryEntry *entry, int precompiled)
{
    unsigned long count;
    unsigned char *data = global_symbol_blocks[0];
    global_type_data = data;
    if (cv_read_offset != (unsigned long)entry->location.offset) {
        cv_read_offset = entry->location.offset;
        ++subsection_reads;
        seek_file(input_file, cv_file_offset+cv_read_offset, 0);
    }
    if (read_file(input_file, global_type_data, 4) != 4) fatal_error(17, NULL, NULL);
    module_types_indexed = 0;
    module_type_bytes = 0;
    type_signature = *(unsigned long *)global_type_data;
    if (type_signature != 1) {
        count = entry->size-4;
        if (*data != 1 && (*data != 0 || (data[3] != 0xa6 && data[3] != 0xa7)))
            fatal_error(19, "Types", module_name(current_module));
        type_signature = 0;
        first_user_type = 0x200;
        if (read_file(input_file, data+4, count) != count) fatal_error(17, NULL, NULL);
        cv_read_offset += entry->size;
        read_old_global_types(global_type_data, entry->size);
    } else {
        first_user_type = 0x1000;
        read_new_global_types(entry->size-4, precompiled);
        cv_read_offset += entry->size;
    }
    if (!previous_type_count) previous_type_count = local_type_count;
    if (!module_types_indexed) reset_type_hash();
}

void *module_type_buffer;

void read_types(unsigned long directory, const CvDirectoryEntry *entry)
{
    if (cv_read_offset != (unsigned long)entry->location.offset) {
        cv_read_offset = entry->location.offset;
        ++subsection_reads;
        seek_file(input_file, cv_file_offset+cv_read_offset, 0);
    }
    if (read_file(input_file, type_data, entry->size) != entry->size)
        fatal_error(19, "Types", module_name(current_module));
    cv_read_offset += entry->size;
    module_type_buffer = type_data;
    type_directory = directory;
}

unsigned long global_symbols_count, global_symbols_size;

int __fastcall intern_global(unsigned char *record, SymbolHash hash)
{
    unsigned short segment = 0, name_offset, name_length, bucket, padding;
    unsigned long symbol_offset = 0, full_hash, count, i;
    const unsigned char *name;
    PublicSymbol *node;
    int same;

    bucket = hash(record, &full_hash, &name_offset, &segment, &symbol_offset, 0x1000);
    name = record+name_offset;
    name_length = *name;
    for (node = global_symbol_hash[bucket]; node; node = node->next) {
        if (node->hash != full_hash) continue;
        same = 1;
        for (i = 0; i < (unsigned long)name_length+1; ++i)
            if (name[i] != node->record[node->name_offset+i]) { same = 0; break; }
        if (same) goto duplicate;
    }
    padding = 3-((*(unsigned short *)record+1)&3);
    node = allocate_symbol(*(unsigned short *)record+padding+0x16);
    node->hash = full_hash;
    node->name_offset = name_offset;
    node->segment = segment;
    node->offset = symbol_offset;
    count = *(unsigned short *)record+2;
    for (i = 0; i < count; ++i) node->record[i] = record[i];
    *(unsigned short *)node->record += padding;
    global_symbols_size += *(unsigned short *)record+padding+2;
    ++global_symbols_count;
    for (i = 0; i < padding; ++i) node->record[count+i] = 0;
    node->next = global_symbol_hash[bucket];
    global_symbol_hash[bucket] = node;
    *(unsigned short *)(record+2) = 8;
    return 1;
duplicate:
    count = *(unsigned short *)record;
    for (i = 0; i < count; ++i)
        if (record[i+2] != node->record[i+2]) return 2;
    *(unsigned short *)(record+2) = 8;
    ++duplicate_symbols;
    return 0;
}


unsigned short address_in_module(ModuleInfo *module, unsigned short segment,
                                unsigned long offset)
{
    unsigned char *record = module->record;
    ModuleContribution *contribution;
    unsigned short count, i;
    if (!record) fatal_error(14, NULL, NULL);
#ifdef RECON_NATIVE_TEST
    if (module->record_size < 8) __asm { int 3 }
#endif
    count = *(unsigned short *)(record+4);
#ifdef RECON_NATIVE_TEST
    if (count > (module->record_size-8)/sizeof(ModuleContribution)) __asm { int 3 }
#endif
    contribution = (ModuleContribution *)(record+8);
    for (i = 0; i < count; ++i, ++contribution) {
        /* Preserve the original unsigned subtraction and test order. */
        if (offset-contribution->offset < contribution->size &&
            contribution->segment == segment) return 1;
    }
    return 0;
}

/* Temporary per-module offsets into the consolidated counted-name pool. */
typedef struct ModuleSourceNames {
    unsigned short module, count;
    struct ModuleSourceNames *next;
    unsigned long offsets[1];
} ModuleSourceNames;
SourceName *source_name_hash[50];
unsigned char *source_file_index;
unsigned long source_file_index_size;

void build_source_file_index(void)
{
    ModuleSourceNames module_head, *last_module = &module_head, *node, *next_module;
    SourceName name_head, *last_name = &name_head, *name_item, *next_name;
    ModuleInfo *module;
    unsigned char *record, *file_record, *output;
    const unsigned char *name;
    unsigned short file_count = 0, first_file = 0, number = 1;
    unsigned long name_bytes = 0, name_offset, i, j, size;
    unsigned long *file_offsets;

    module_head.module = module_head.count = 0;
    module_head.next = NULL;
    name_head.name = NULL;
    name_head.offset = 0;
    name_head.next = name_head.bucket_next = NULL;
    for (i = 0; i < 50; ++i) source_name_hash[i] = NULL;
    for (module = module_list; module; module = module->next) {
        record = module->lines;
        if (!record) continue;
        size = *(unsigned short *)record*4+8;
        node = checked_alloc(size);
        node->module = module->module;
        node->count = *(unsigned short *)record;
        node->next = NULL;
        last_module->next = node;
        last_module = node;
        for (i = 0; i < *(unsigned short *)record; ++i) {
            file_record = record+((unsigned long *)(record+4))[i];
            name = file_record+4+*(unsigned short *)file_record*12;
            name_offset = 0;
            if (find_source_name(name, &name_offset)) node->offsets[i] = name_bytes;
            else {
                last_name = add_source_name(name, last_name, name_bytes);
                node->offsets[i] = name_bytes;
                name_bytes += *name+1;
            }
            ++file_count;
        }
    }
#ifdef RECON_NATIVE_TEST
    /* Original dereferences the empty dummy if called without any source files. */
    if (!last_name->name) __asm { int 3 }
#endif
    source_file_index_size = *last_name->name+(module_count+file_count)*4
                             +last_name->offset+5;
    source_file_index_size = (source_file_index_size+3)&~3UL;
#ifdef RECON_NATIVE_TEST
    /* Original truncates allocation size to WORD, then uses full computed sizes. */
    if (source_file_index_size > 65535) __asm { int 3 }
#endif
    output = source_file_index = checked_alloc((unsigned short)source_file_index_size);
    *(unsigned short *)output = module_count;
    *(unsigned short *)(output+2) = file_count;
    file_offsets = (unsigned long *)(output+4+module_count*4);
    for (node = module_head.next; node; node = node->next) {
        while (node->module > number) {
            ((unsigned short *)(output+2))[number] = 0;
            ((unsigned short *)(output+2))[module_count+number] = 0;
            ++number;
        }
        ((unsigned short *)(output+2))[number] = first_file;
        ((unsigned short *)(output+2))[module_count+number] = node->count;
        first_file += node->count;
        for (i = 0; i < node->count; ++i) *file_offsets++ = node->offsets[i];
        ++number;
    }
    while (module_count+1 > (unsigned long)number) {
        ((unsigned short *)(output+2))[number] = 0;
        ((unsigned short *)(output+2))[module_count+number] = 0;
        ++number;
    }
    output += (module_count+file_count)*4+4;
    for (name_item = name_head.next; name_item; name_item = name_item->next) {
        for (i = 0; i < (unsigned long)*name_item->name+1; ++i)
            output[i] = name_item->name[i];
        output += *output+1;
    }
    for (node = module_head.next; node; node = next_module) {
        next_module = node->next;
        free(node);
    }
    for (name_item = name_head.next; name_item; name_item = next_name) {
        next_name = name_item->next;
        free(name_item);
    }
}

int find_source_name(const unsigned char *name, unsigned long *offset)
{
    SourceName *item = source_name_hash[hash_source_name(name)];
    while (item) {
        if (*item->name == *name &&
            !_strnicmp((const char *)name+1, (const char *)item->name+1, *name)) {
            *offset = item->offset;
            return 1;
        }
        item = item->bucket_next;
    }
    return 0;
}

unsigned int hash_source_name(const unsigned char *name)
{
    unsigned short sum = 0;
    int i, length = (signed char)*name;

    /* Original includes the length byte and omits the final name byte. */
    for (i = 0; i < length; ++i) sum += (signed char)name[i];
    return sum%50;
}

SourceName *add_source_name(const unsigned char *name, SourceName *tail,
                            unsigned long offset)
{
    SourceName *item = checked_alloc(sizeof(*item));
    unsigned int bucket = hash_source_name(name);

    item->bucket_next = source_name_hash[bucket];
    item->name = name;
    item->offset = offset;
    item->next = NULL;
    tail->next = item;
    /* The original never publishes item into source_name_hash[bucket]. */
    return item;
}

void read_source_module(unsigned long directory, const CvDirectoryEntry *entry)
{
    if (cv_read_offset != (unsigned long)entry->location.offset) {
        cv_read_offset = entry->location.offset;
        ++subsection_reads;
        seek_file(input_file, cv_file_offset+cv_read_offset, 0);
    }
    if (read_file(input_file, module_data, entry->size) != entry->size)
        fatal_error(17, "Source Module", module_name(current_module));
    cv_read_offset += entry->size;
    old_line_directory = directory;
}

#pragma intrinsic(memcpy)
void pack_old_lines(void *lines, CvDirectoryEntry *entry, ModuleInfo *module)
{
    unsigned char *buffer;
#ifdef RECON_NATIVE_TEST
    /* The original truncates malloc size but copies the full DWORD size. */
    if (entry->size > 0xffff) __asm { int 3 }
#endif
    buffer = malloc((unsigned short)entry->size);
    if (!buffer) fatal_error(14, NULL, NULL);
    if (!buffer) fatal_error(14, NULL, NULL);
    memcpy(buffer, lines, entry->size);
    entry->location.data = buffer;
    module->line_size = entry->size;
    module->lines = buffer;
}

char module_name_buffer[260];

/* This original counted-byte copy is a library call, not an intrinsic. */
#pragma function(memcpy)
const char *module_name(ModuleInfo *module)
{
    unsigned char *record = module->record, *name;
    if (!record) fatal_error(14, NULL, NULL);
    name = record+8+*(unsigned short *)(record+4)*12;
#ifdef RECON_NATIVE_TEST
    /* The original sign-extends the count before memcpy and indexing. */
    if ((signed char)*name < 0) __asm { int 3 }
#endif
    memcpy(module_name_buffer, name+1, (signed char)*name);
    module_name_buffer[(signed char)*name] = 0;
    return module_name_buffer;
}

#pragma intrinsic(memcpy)
