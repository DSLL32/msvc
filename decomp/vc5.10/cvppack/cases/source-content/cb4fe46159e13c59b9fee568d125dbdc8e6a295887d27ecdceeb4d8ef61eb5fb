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

void write_publics(CodeViewDirectoryEntry *entry, unsigned offset)
{
    SymbolHashHeader header;
    HashOutputStream names, addresses;
    HashOutputBlock *block;
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
