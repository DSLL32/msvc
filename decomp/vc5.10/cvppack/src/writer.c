/* Packed CodeView directory and PE image writer [004069e0,00407602). */
#include "writer.h"
#include <recon.h>

#pragma intrinsic(memcpy, memset)
extern int verbose;
PackedSignature packed_signature = { { 'N', 'B', '1', '1' }, 0 };
unsigned output_capacity, output_used;
unsigned char *output_buffer;
int output_file;
unsigned total_packed_source_bytes, packed_file_length;
unsigned output_padding_size;
unsigned char output_padding[4] = { 0 };

/* Output-buffer setup [004100e0,00410135). */
void begin_buffered_output(int file)
{
    output_capacity = 65512;
    do {
        output_buffer = malloc(output_capacity);
        if (output_buffer) break;
        output_capacity >>= 1;
    } while (output_capacity >= 4096);
    if (!output_buffer) output_capacity = 0;
    output_used = 0;
    output_file = file;
}

/* The original expands this buffer shortcut at each write site. Keep it in
 * the writer's function body rather than adding a call vertex. */
#define BUFFER_WRITE(source_, bytes_, result_) do { \
    unsigned count_ = (bytes_); \
    const void *input_ = (source_); \
    if (output_capacity - output_used >= count_) { \
        memcpy(output_buffer + output_used, input_, count_); \
        output_used += count_; \
        (result_) = 1; \
    } else (result_) = buffered_write(input_, count_); \
} while (0)

/* Copy one packed subsection, then align it to a DWORD boundary.
 * [004068a0,004069da), ordinary cdecl directory-index argument. */
void write_subsection(unsigned index)
{
    unsigned char *source;
    unsigned short written;
    if (directory_entries[index].size) {
        source = directory_entries[index].data;
        if (!source) fatal_error(1001, NULL, NULL);
        BUFFER_WRITE(source, directory_entries[index].size, written);
        if (!written) fatal_error(1008, NULL, NULL);
    }
    directory_entries[index].offset = image_read_mode - codeview_file_offset;
    output_padding_size = 4 - (directory_entries[index].size & 3);
    if (output_padding_size != 4) {
        BUFFER_WRITE(output_padding, output_padding_size, written);
        if (!written) fatal_error(1008, NULL, NULL);
    }
}

/* Global-public records and their name/address hashes [0040d600,0040d998). */
/* Insert S_ALIGN records so symbol streams respect their 4K pages.
 * [0040f960,0040fad2), cdecl; the final reserve requests an end marker. */
unsigned insert_symbol_page_padding(PublicSymbol **buckets, unsigned reserve)
{
    PublicSymbol *node, *previous, *padding;
    unsigned bucket, position = 0, added = 0, size;
    unsigned short overhead = 0;
    for (bucket = 0; bucket < 4096; ++bucket) {
        previous = NULL;
        node = buckets[bucket];
        if (node) overhead = (unsigned short)(reserve + 4);
        while (node) {
            if ((position & 0xffff) + *(unsigned short *)node->record + overhead + 2 > 4096) {
                size = 4096 - (position & 0xffff);
                padding = allocate_public_block(16 + size);
                padding->hash = 0xffff;
                padding->name_offset = 0xffff;
                padding->segment = 0;
                padding->offset = 0;
                memset(padding->record, 0, size);
                *(unsigned short *)padding->record = (unsigned short)(size - 2);
                *(unsigned short *)(padding->record + 2) = 0x402;
                padding->next = node;
                if (previous) previous->next = padding;
                else buckets[bucket] = padding;
                added += size;
                position = 0;
            }
            position += (unsigned short)(*(unsigned short *)node->record + 2);
            previous = node;
            node = node->next;
        }
        if (bucket == 4095 && reserve >= 4) {
            padding = allocate_public_block(24);
            padding->hash = 0xffff;
            padding->name_offset = 0xffff;
            padding->segment = 0;
            padding->offset = 0;
            memset(padding->record, 0xff, 8);
            *(unsigned short *)padding->record = 6;
            *(unsigned short *)(padding->record + 2) = 0x402;
            padding->next = node;
            if (previous) previous->next = padding;
            else buckets[bucket] = padding;
            added += 8;
        }
    }
    return added;
}

/* CodeView name hash, method 10 [0040e920,0040edbb).
 * Buckets retain twelve entries inline and chain five-entry overflow nodes. */
void build_symbol_name_hash(SymbolKeyProc key, unsigned short *method,
    unsigned *bytes, TypeArena *stream, unsigned count, PublicSymbol **buckets)
{
    NameHashBucket **pages, *bucket;
    NameHashOverflow *overflow_pages[10] = { 0 }, **link;
    PublicSymbol *node;
    unsigned short hash_count, page_count, entries, slot;
    unsigned index, page, size, position = 0, overflow_count = 0, ordinal;
    unsigned offset;
    if (!count) return;
    index = count / 10;
    if (index >= 0xffff) index = 0xffff;
    if (index <= 6) index = 6;
    hash_count = (unsigned short)((index + 1) & 0xfffe);
    page_count = (unsigned short)(hash_count / 629 + 1);
    pages = allocate_bytes((unsigned)page_count * sizeof(*pages));
    for (page = 0; page < page_count; ++page) {
        size = page == page_count - 1 ? ((unsigned)hash_count * sizeof(NameHashBucket)) % 65416 : 65416;
        pages[page] = allocate_cleared(size);
    }
    for (index = 0; index < 4096; ++index) {
        for (node = buckets[index]; node; node = node->next) {
            size = *(unsigned short *)node->record + 2;
            if (*(unsigned short *)(node->record + 2) == 0x402) {
                position += size;
                continue;
            }
            ordinal = node->hash % hash_count;
            bucket = &pages[ordinal / 629][ordinal % 629];
            if (bucket->count < 12) {
                bucket->entries[bucket->count].offset = position;
                bucket->entries[bucket->count].hash = node->hash;
            } else {
                slot = (unsigned short)((bucket->count - 12) % 5);
                link = &bucket->overflow;
                while (*link) {
                    if (slot && !(*link)->next) break;
                    link = &(*link)->next;
                }
                if (!slot) {
                    ordinal = (unsigned short)overflow_count;
                    page = ordinal / 1488;
                    if (!overflow_pages[page]) overflow_pages[page] = allocate_cleared(65504);
                    *link = &overflow_pages[page][ordinal % 1488];
                    ++overflow_count;
                }
                (*link)->entries[slot].offset = position;
                (*link)->entries[slot].hash = node->hash;
            }
            ++bucket->count;
            position += size;
        }
    }
    initialize_arena(stream, 4096);
    copy_type_arena(stream, (const unsigned char *)&hash_count, 2);
    fill_type_arena(stream, 0, 2);
    offset = 0;
    for (index = 0; index < hash_count; ++index) {
        bucket = &pages[index / 629][index % 629];
        copy_type_arena(stream, (const unsigned char *)&offset, 4);
        offset += (unsigned)bucket->count * 8;
    }
    for (index = 0; index < hash_count; ++index) {
        bucket = &pages[index / 629][index % 629];
        copy_type_arena(stream, (const unsigned char *)&bucket->count, 2);
        fill_type_arena(stream, 0, 2);
    }
    for (index = 0; index < hash_count; ++index) {
        bucket = &pages[index / 629][index % 629];
        entries = bucket->count;
        size = entries > 12 ? 12 : entries;
        copy_type_arena(stream, (const unsigned char *)bucket->entries, size * sizeof(NameHashPair));
        entries = (unsigned short)(entries - size);
        for (link = &bucket->overflow; entries; link = &(*link)->next) {
            size = entries > 5 ? 5 : entries;
            copy_type_arena(stream, (const unsigned char *)(*link)->entries, size * sizeof(NameHashPair));
            entries = (unsigned short)(entries - size);
        }
    }
    *method = 10;
    *bytes = 4 + (count + hash_count) * 8;
    for (page = 0; page < page_count; ++page) free(pages[page]);
    free(pages);
    for (page = 0; page < 10; ++page) free(overflow_pages[page]);
}

/* Address hash rows sort by the unsigned symbol offset difference.
 * The original subtraction wraps to the signed cdecl result [0040edc0,0040edd7). */
int __cdecl compare_symbol_address(const void *first, const void *second)
{
    const AddressHashEntry *a = first, *b = second;
    return (int)(a->symbol->offset - b->symbol->offset);
}

/* CodeView address hash, method 12 [0040ede0,0040f0c6). */
void build_symbol_address_hash(unsigned short *method, unsigned *bytes,
    TypeArena *stream, unsigned count, PublicSymbol **buckets)
{
    AddressHashGroup *groups, *group;
    PublicSymbol *node;
    unsigned short maximum_segment = 0;
    unsigned absolute_count = 0, position = 0, offset;
    unsigned bucket;
    int segment, index;
    if (!count) return;
    for (bucket = 0; bucket < 4096; ++bucket) {
        for (node = buckets[bucket]; node; node = node->next) {
            if (*(unsigned short *)(node->record + 2) != 0x402 && node->segment > maximum_segment)
                maximum_segment = node->segment;
        }
    }
    groups = allocate_cleared((unsigned)maximum_segment * sizeof(AddressHashGroup));
    for (bucket = 0; bucket < 4096; ++bucket) {
        for (node = buckets[bucket]; node; node = node->next) {
            if (*(unsigned short *)(node->record + 2) == 0x402) continue;
            if (!node->segment) ++absolute_count;
            else ++groups[node->segment - 1].count;
        }
    }
    for (segment = 0; segment < maximum_segment; ++segment) {
        if (groups[segment].count)
            groups[segment].entries = allocate_cleared((unsigned)groups[segment].count * sizeof(AddressHashEntry));
    }
    for (bucket = 0; bucket < 4096; ++bucket) {
        for (node = buckets[bucket]; node; node = node->next) {
            if (*(unsigned short *)(node->record + 2) != 0x402 && node->segment) {
                group = &groups[node->segment - 1];
                group->entries[group->written].symbol = node;
                group->entries[group->written].stream_offset = position;
                ++group->written;
            }
            position += *(unsigned short *)node->record + 2;
        }
    }
    for (segment = 0; segment < maximum_segment; ++segment)
        qsort(groups[segment].entries, groups[segment].count, sizeof(AddressHashEntry), compare_symbol_address);
    initialize_arena(stream, 4096);
    copy_type_arena(stream, (const unsigned char *)&maximum_segment, 2);
    fill_type_arena(stream, 0, 2);
    offset = 0;
    for (segment = 0; segment < maximum_segment; ++segment) {
        copy_type_arena(stream, (const unsigned char *)&offset, 4);
        offset += groups[segment].count * 8;
    }
    for (segment = 0; segment < maximum_segment; ++segment) {
        copy_type_arena(stream, (const unsigned char *)&groups[segment].count, 2);
        fill_type_arena(stream, 0, 2);
    }
    for (segment = 0; segment < maximum_segment; ++segment) {
        for (index = 0; index < groups[segment].count; ++index) {
            copy_type_arena(stream, (const unsigned char *)&groups[segment].entries[index].stream_offset, 4);
            copy_type_arena(stream, (const unsigned char *)&groups[segment].entries[index].symbol->offset, 4);
        }
    }
    *method = 12;
    *bytes = 4 + (count - absolute_count + maximum_segment) * 8;
    for (segment = 0; segment < maximum_segment; ++segment)
        if (groups[segment].entries) free(groups[segment].entries);
    free(groups);
}

void write_publics(CodeViewDirectoryEntry *entry, unsigned offset)
{
    SymbolHashHeader header;
    TypeArena names, addresses;
    TypeArenaBlock *block;
    PublicSymbol *node;
    unsigned bucket, added, padding = 0;
    unsigned short written;
    header.name_method = header.address_method = 0;
    header.symbol_bytes = header.name_bytes = header.address_bytes = 0;
    added = insert_symbol_page_padding((PublicSymbol **)type_table_global, 4);
    public_symbol_bytes += added;
    header.symbol_bytes = public_symbol_bytes;
    build_symbol_name_hash(symbol_record_key, &header.name_method, &header.name_bytes,
        &names, public_symbol_count, (PublicSymbol **)type_table_global);
    build_symbol_address_hash(&header.address_method, &header.address_bytes,
        &addresses, public_symbol_count, (PublicSymbol **)type_table_global);
    entry->offset = offset;
    entry->subsection = 0x12a;
    entry->module = 0xffff;
    entry->size = sizeof(header) + header.symbol_bytes + header.name_bytes + header.address_bytes;
    BUFFER_WRITE(&header, sizeof(header), written);
    if (!written) fatal_error(1008, NULL, NULL);
    for (bucket = 0; bucket < 4096; ++bucket) {
        for (node = ((PublicSymbol **)type_table_global)[bucket]; node; node = node->next) {
            BUFFER_WRITE(node->record, *(unsigned short *)node->record + 2, written);
            if (!written) fatal_error(1008, NULL, NULL);
        }
    }
    if (header.name_method) {
        for (block = first_hash_output_block(&names); block; block = next_hash_output_block(block)) {
            BUFFER_WRITE(block->data, block->used, written);
            if (!written) fatal_error(1008, NULL, NULL);
        }
    }
    if (header.address_method) {
        for (block = first_hash_output_block(&addresses); block; block = next_hash_output_block(block)) {
            BUFFER_WRITE(block->data, block->used, written);
            if (!written) fatal_error(1008, NULL, NULL);
        }
    }
    added = 4 - (entry->size & 3);
    if (added != 4) {
        BUFFER_WRITE(&padding, added, written);
        if (!written) fatal_error(1008, NULL, NULL);
    }
    if (verbose) printf(message_text(8013), "\t\t", header.symbol_bytes, '\n');
}

/* Sort module records first, then unsigned module/subsection order.
 * [004063f0,0040645b), ordinary cdecl comparator. */
int __cdecl compare_packed_directory_entries(const void *first, const void *second)
{
    const CodeViewDirectoryEntry *a = first, *b = second;
    if (a->subsection == 0x120 || b->subsection == 0x120) {
        if (a->subsection != 0x120) return 1;
        if (b->subsection != 0x120) return -1;
    }
    if (a->module < b->module) return -1;
    if (a->module > b->module) return 1;
    if (a->subsection < b->subsection) return -1;
    return a->subsection > b->subsection;
}

void flush_packed_file(void)
{
    ModuleInfo *info;
    CodeViewDirectoryEntry *entry;
    IMAGE_SECTION_HEADER section;
    IMAGE_DEBUG_DIRECTORY debug;
    IMAGE_OPTIONAL_HEADER optional;
    unsigned index, position, directory_offset, codeview_size, padding, size;
    unsigned short written;
    directory_count = 0;
    for (info = module_list; info; info = info->next) {
        ++directory_count;
        if (info->symbols_size) ++directory_count;
        if (info->source_lines_size) ++directory_count;
    }
    entry = directory_entries;
    for (info = module_list; info; info = info->next) {
        entry->subsection = 0x120;
        entry->module = info->module;
        entry->data = info->data;
        entry->size = info->size;
        ++entry;
        if (info->symbols_size) {
            entry->subsection = 0x125;
            entry->module = info->module;
            entry->data = info->symbols;
            entry->size = info->symbols_size;
            ++entry;
        }
        if (info->source_lines_size) {
            entry->subsection = 0x127;
            entry->module = info->module;
            entry->data = info->source_lines;
            entry->size = info->source_lines_size;
            total_packed_source_bytes += info->source_lines_size;
            ++entry;
        }
    }
    if (verbose) printf(message_text(8012), "\t\t", total_packed_source_bytes, '\n');
    qsort(directory_entries, directory_count, sizeof(CodeViewDirectoryEntry), compare_packed_directory_entries);
    codeview_file_offset = (codeview_file_offset + 3) & ~3U;
    if (seek_cached_file(input_file, codeview_file_offset, 0) == -1) fatal_error(1008, NULL, NULL);
    begin_buffered_output(input_file);
    BUFFER_WRITE(&packed_signature, sizeof(packed_signature), written);
    if (!written) fatal_error(1008, NULL, NULL);
    image_read_mode = tell_cached_file(input_file) + output_used;
    for (index = 0; index < directory_count; ++index) {
        write_subsection(index);
        image_read_mode = tell_cached_file(input_file) + output_used;
    }
    write_publics(directory_entries + directory_count, image_read_mode - codeview_file_offset);
    ++directory_count;
    image_read_mode = tell_cached_file(input_file) + output_used;
    write_globals(directory_entries + directory_count, image_read_mode - codeview_file_offset);
    ++directory_count;
    image_read_mode = tell_cached_file(input_file) + output_used;
    write_static_symbols(directory_entries + directory_count, image_read_mode - codeview_file_offset);
    ++directory_count;
    image_read_mode = tell_cached_file(input_file) + output_used;
    entry = directory_entries + directory_count;
    entry->subsection = 0x128;
    entry->module = 0xffff;
    entry->data = libraries;
    entry->size = libraries_size;
    write_subsection(directory_count);
    image_read_mode = tell_cached_file(input_file) + output_used;
    ++directory_count;
    entry = directory_entries + directory_count;
    entry->subsection = 0x12b;
    entry->module = 0xffff;
    entry->size = write_global_types();
    entry->offset = image_read_mode - codeview_file_offset;
    if (verbose) printf(message_text(8011), "\t\t", source_type_input_bytes, '\n', "\t\t", entry->size, '\n');
    if (segment_map) {
        image_read_mode = tell_cached_file(input_file) + output_used;
        entry = directory_entries + ++directory_count;
        entry->subsection = 0x12d;
        entry->module = 0xffff;
        entry->data = segment_map;
        entry->size = segment_map_size;
        write_subsection(directory_count);
    }
    if (segment_names) {
        image_read_mode = tell_cached_file(input_file) + output_used;
        entry = directory_entries + ++directory_count;
        entry->subsection = 0x12e;
        entry->module = 0xffff;
        entry->data = segment_names;
        entry->size = segment_names_size;
        write_subsection(directory_count);
    }
    if (source_file_index) {
        image_read_mode = tell_cached_file(input_file) + output_used;
        entry = directory_entries + ++directory_count;
        entry->subsection = 0x133;
        entry->module = 0xffff;
        entry->data = source_file_index;
        entry->size = source_file_index_size;
        write_subsection(directory_count);
    }
    image_read_mode = tell_cached_file(input_file) + output_used;
    ++directory_count;
    codeview_directory_header.header_size = 16;
    codeview_directory_header.entry_size = 12;
    codeview_directory_header.count = directory_count;
    codeview_directory_header.next_directory = 0;
    codeview_directory_header.flags = 0;
    directory_offset = tell_cached_file(input_file) + output_used - codeview_file_offset;
    BUFFER_WRITE(&codeview_directory_header, sizeof(codeview_directory_header), written);
    if (written) BUFFER_WRITE(directory_entries, directory_count * sizeof(CodeViewDirectoryEntry), written);
    if (!written) fatal_error(1008, NULL, NULL);
    codeview_size = tell_cached_file(input_file) + output_used - codeview_file_offset + 8;
    BUFFER_WRITE(packed_signature.magic, 4, written);
    if (written) BUFFER_WRITE(&codeview_size, 4, written);
    if (!written) fatal_error(1008, NULL, NULL);
    if (!end_buffered_output()) fatal_error(1008, NULL, NULL);
    packed_file_length = tell_cached_file(input_file);
    if (debug_section_position) {
        if (seek_cached_file(input_file, optional_header_position, 0) == -1) fatal_error(1008, NULL, NULL);
        size = image_file_header.SizeOfOptionalHeader;
        if (read_cached_file(input_file, &optional, size) != size) fatal_error(1008, NULL, NULL);
        if (packed_file_length % optional.FileAlignment) {
            packed_file_length = (packed_file_length + optional.FileAlignment - 1) & ~(optional.FileAlignment - 1);
            codeview_size = packed_file_length - codeview_file_offset;
            if (seek_cached_file(input_file, packed_file_length - 8, 0) == -1 ||
                write_cached_file(input_file, packed_signature.magic, 4) != 4 ||
                write_cached_file(input_file, &codeview_size, 4) != 4) fatal_error(1008, NULL, NULL);
        }
    }
    if (seek_cached_file(input_file, section_table_position, 0) == -1 ||
        read_cached_file(input_file, &section, sizeof(section)) != sizeof(section)) fatal_error(1008, NULL, NULL);
    position = section.PointerToRawData + debug_directory_offset;
    if (seek_cached_file(input_file, position, 0) == -1 ||
        read_cached_file(input_file, &debug, sizeof(debug)) != sizeof(debug)) fatal_error(1008, NULL, NULL);
    debug.SizeOfData = codeview_size;
    if (seek_cached_file(input_file, section.PointerToRawData + debug_directory_offset, 0) == -1 ||
        write_cached_file(input_file, &debug, sizeof(debug)) != sizeof(debug)) fatal_error(1008, NULL, NULL);
    if (debug_section_position) {
        if (seek_cached_file(input_file, debug_section_position, 0) == -1 ||
            read_cached_file(input_file, &section, sizeof(section)) != sizeof(section)) fatal_error(1008, NULL, NULL);
        position = section.PointerToRawData;
        section.SizeOfRawData = section.Misc.VirtualSize = packed_file_length - position;
        if (seek_cached_file(input_file, debug_section_position, 0) == -1 ||
            write_cached_file(input_file, &section, sizeof(section)) != sizeof(section)) fatal_error(1008, NULL, NULL);
        padding = (section.Misc.VirtualSize + section.VirtualAddress) % optional.SectionAlignment;
        optional.SizeOfImage = section.Misc.VirtualSize + section.VirtualAddress;
        if (padding) optional.SizeOfImage += optional.SectionAlignment - padding;
        if (seek_cached_file(input_file, optional_header_position, 0) == -1) fatal_error(1008, NULL, NULL);
        size = image_file_header.SizeOfOptionalHeader;
        if (write_cached_file(input_file, &optional, size) != size) fatal_error(1008, NULL, NULL);
    }
    resize_cached_file(input_file, packed_file_length);
    if (seek_cached_file(input_file, codeview_file_offset + 4, 0) == -1 ||
        write_cached_file(input_file, &directory_offset, 4) != 4) fatal_error(1008, NULL, NULL);
}
#undef BUFFER_WRITE
