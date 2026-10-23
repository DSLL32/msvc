/* Original image/debug reader [00405d00,004063e3). */
#include "image.h"
#include <recon.h>

IMAGE_FILE_HEADER image_file_header;
unsigned image_read_mode, optional_header_position, section_table_position;
unsigned debug_section_position, original_image_checksum, original_file_length;
unsigned debug_directory_offset, codeview_file_offset;
unsigned char image_linker_major, image_linker_minor;
unsigned short codeview_version;
CodeViewDirectoryHeader codeview_directory_header;
CodeViewDirectoryEntry *directory_entries;
unsigned directory_relative_offset, directory_count;
unsigned source_symbol_bytes, source_line_bytes, source_type_bytes, source_module_bytes;
unsigned symbol_capacity, line_capacity, type_capacity, module_capacity;
unsigned type_segment_bytes;
unsigned short type_segment_count, type_segment_index;
void **type_segments;

void read_image_debug(void)
{
    IMAGE_DOS_HEADER dos;
    IMAGE_OPTIONAL_HEADER optional;
    IMAGE_SECTION_HEADER section;
    IMAGE_DEBUG_DIRECTORY directory;
    DWORD signature;
    unsigned count, remaining, amount;
    int valid = 0;
    image_read_mode = 0;
    if (seek_cached_file(input_file, 0, 0) == -1) fatal_error(1005, NULL, NULL);
    count = read_cached_file(input_file, &dos, sizeof(dos));
    if (count >= 26) {
        if (dos.e_magic != IMAGE_DOS_SIGNATURE) {
            if (seek_cached_file(input_file, 0, 0) != -1) {
                read_cached_file(input_file, &signature, sizeof(signature));
                if (signature == IMAGE_NT_SIGNATURE || seek_cached_file(input_file, 0, 0) != -1)
                    valid = 1;
            }
        } else if (count >= sizeof(dos) && dos.e_lfanew &&
                   seek_cached_file(input_file, dos.e_lfanew, 0) != -1 &&
                   read_cached_file(input_file, &signature, sizeof(signature)) >= sizeof(signature) &&
                   signature == IMAGE_NT_SIGNATURE) valid = 1;
    }
    if (!valid) fatal_error(1005, NULL, NULL);
    if (read_cached_file(input_file, &image_file_header, sizeof(image_file_header)) != sizeof(image_file_header))
        fatal_error(1005, NULL, NULL);
    if (image_file_header.Characteristics & IMAGE_FILE_DEBUG_STRIPPED) fatal_error(1012, NULL, NULL);
    optional_header_position = tell_cached_file(input_file);
    if (read_cached_file(input_file, &optional, image_file_header.SizeOfOptionalHeader) != image_file_header.SizeOfOptionalHeader)
        fatal_error(1005, NULL, NULL);
    image_linker_major = optional.MajorLinkerVersion;
    image_linker_minor = optional.MinorLinkerVersion;
    remaining = image_file_header.NumberOfSections;
    if (image_file_header.SizeOfOptionalHeader == 56) {
        while (remaining) {
            section_table_position = tell_cached_file(input_file);
            if (read_cached_file(input_file, &section, sizeof(section)) != sizeof(section)) fatal_error(1005, NULL, NULL);
            if (!strncmp((const char *)section.Name, ".rdata", 5)) break;
            --remaining;
        }
        if (!remaining) fatal_error(1012, NULL, NULL);
        if (seek_cached_file(input_file, section.PointerToRawData, 0) == -1) fatal_error(1005, NULL, NULL);
        do {
            if (read_cached_file(input_file, &directory, sizeof(directory)) != sizeof(directory)) fatal_error(1005, NULL, NULL);
            if (directory.Type == IMAGE_DEBUG_TYPE_CODEVIEW) break;
            debug_directory_offset += sizeof(directory);
        } while (directory.Type);
    } else {
        original_image_checksum = optional.CheckSum;
        count = optional.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].Size / sizeof(directory);
        if (!count) fatal_error(1005, NULL, NULL);
        section_table_position = tell_cached_file(input_file);
        while (remaining) {
            if (read_cached_file(input_file, &section, sizeof(section)) != sizeof(section)) fatal_error(1005, NULL, NULL);
            if (!strncmp((const char *)section.Name, ".debug", 6)) {
                debug_section_position = tell_cached_file(input_file) - sizeof(section);
                break;
            }
            --remaining;
        }
        if (seek_cached_file(input_file, 0, 2) == -1) fatal_error(1005, NULL, NULL);
        original_file_length = tell_cached_file(input_file);
        if (seek_cached_file(input_file, section_table_position, 0) == -1) fatal_error(1005, NULL, NULL);
        remaining = image_file_header.NumberOfSections;
        while (remaining) {
            section_table_position = tell_cached_file(input_file);
            if (read_cached_file(input_file, &section, sizeof(section)) != sizeof(section)) fatal_error(1005, NULL, NULL);
            signature = optional.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].VirtualAddress;
            if (signature >= section.VirtualAddress && signature < section.VirtualAddress + section.SizeOfRawData) {
                debug_directory_offset = signature - section.VirtualAddress;
                break;
            }
            --remaining;
        }
        if (!remaining) fatal_error(1012, NULL, NULL);
        if (seek_cached_file(input_file, section.PointerToRawData + debug_directory_offset, 0) == -1) fatal_error(1005, NULL, NULL);
        while (count) {
            if (read_cached_file(input_file, &directory, sizeof(directory)) != sizeof(directory)) fatal_error(1005, NULL, NULL);
            if (directory.Type == IMAGE_DEBUG_TYPE_CODEVIEW) break;
            debug_directory_offset += sizeof(directory);
            --count;
        }
    }
    if (directory.Type != IMAGE_DEBUG_TYPE_CODEVIEW) fatal_error(1012, NULL, NULL);
    codeview_file_offset = directory.PointerToRawData;
    if (seek_cached_file(input_file, codeview_file_offset, 0) == -1) fatal_error(1005, NULL, NULL);
    codeview_version = read_codeview_signature();
    read_codeview_directory();
    if (codeview_directory_header.next_directory) fatal_error(1005, NULL, NULL);
    classify_debug_directory();
    symbol_capacity = source_symbol_bytes;
    line_capacity = source_line_bytes;
    type_capacity = source_type_bytes;
    module_capacity = source_module_bytes;
    if (source_module_bytes) {
        debug_directory = malloc(source_module_bytes);
        if (!debug_directory) fatal_error(1001, NULL, NULL);
    }
    symbol_data = malloc(symbol_capacity);
    if (!symbol_data) fatal_error(1001, NULL, NULL);
    type_data = malloc(line_capacity);
    if (!type_data) fatal_error(1001, NULL, NULL);
    module_data = malloc(type_capacity);
    if (!module_data) fatal_error(1001, NULL, NULL);
    type_segment_bytes += 4;
    type_segment_count = (type_segment_bytes * 2) / 0xffe8 + 2;
    type_segments = allocate_cleared(type_segment_count * sizeof(void *));
    if (!type_segments) fatal_error(1001, NULL, NULL);
    amount = type_segment_bytes < 0xffe8 ? type_segment_bytes : 0xffe8;
    type_segments[type_segment_index] = malloc(amount);
    if (!type_segments[type_segment_index]) fatal_error(1001, NULL, NULL);
}

/* Original CodeView signature validation [00405690,0040575d). */
unsigned char codeview_signature[4];
unsigned short read_codeview_signature(void)
{
    if (read_cached_file(input_file, codeview_signature, sizeof(codeview_signature)) == sizeof(codeview_signature)) {
        if (codeview_signature[0] != 'N' || codeview_signature[1] != 'B') fatal_error(1013, NULL, NULL);
        if (codeview_signature[2] == '0' && codeview_signature[3] == '5') return 0;
        if ((codeview_signature[2] == '0' && codeview_signature[3] == '9') ||
            (codeview_signature[2] == '1' && (codeview_signature[3] == '0' || codeview_signature[3] == '1'))) {
            warning(4001, NULL, NULL);
            if (input_file != -1) {
                close_file(input_file, 1);
                input_file = -1;
            }
            exit(0);
        }
        fatal_error(1013, NULL, NULL);
    }
    fatal_error(1005, NULL, NULL);
    return 0;
}

/* Original CodeView subsection-directory read [004057b0,00405907). */
void read_codeview_directory(void)
{
    unsigned index;
    unsigned short current_module = 0;
    if (read_cached_file(input_file, &directory_relative_offset, sizeof(directory_relative_offset)) != sizeof(directory_relative_offset) ||
        seek_cached_file(input_file, codeview_file_offset + directory_relative_offset, 0) == -1 ||
        read_cached_file(input_file, &codeview_directory_header, sizeof(codeview_directory_header)) != sizeof(codeview_directory_header))
        fatal_error(1005, NULL, NULL);
    if (!codeview_directory_header.count) fatal_error(1005, NULL, NULL);
    directory_count = codeview_directory_header.count;
    directory_entries = malloc((directory_count + 6) * sizeof(CodeViewDirectoryEntry));
    if (!directory_entries) fatal_error(1001, NULL, NULL);
    if (read_cached_file(input_file, directory_entries, directory_count * sizeof(CodeViewDirectoryEntry)) != directory_count * sizeof(CodeViewDirectoryEntry))
        fatal_error(1005, NULL, NULL);
    for (index = 0; index < directory_count; ++index) {
        unsigned short module = directory_entries[index].module;
        if (module && module != 0xffff && module != current_module) {
            if (directory_entries[index].subsection != 0x120) break;
            current_module = module;
        }
    }
    if (index != directory_count)
        qsort(directory_entries, directory_count, sizeof(CodeViewDirectoryEntry), compare_directory_entries);
}

/* Original subsection ordering [00405760,004057b0). The complete 16-element
 * semantic priority table occupies [00411108,00411148), immediately before
 * the .debug literal; it is indexed by SST kind minus 0x120. */
int __cdecl compare_directory_entries(const void *first, const void *second)
{
    static const int order[16] = {
        1, 2, 0, 4, 3, 0, 0, 5,
        0, 0, 0, 0, 0, 0, 0, 2
    };
    const CodeViewDirectoryEntry *a = first, *b = second;
    int left, right;
    if (a->module < b->module) return -1;
    if (a->module > b->module) return 1;
    left = order[a->subsection - 0x120];
    right = order[b->subsection - 0x120];
    if (left < right) return -1;
    return left > right;
}

unsigned char *libraries, *segment_map, *segment_names;
unsigned libraries_size, segment_map_size, segment_names_size;
ModuleIndex *module_index;

/* Original subsection classification [00405a10,00405cc0), with the following
 * 16-way switch table [00405cc0,00405d00). */
void classify_debug_directory(void)
{
    unsigned index, previous = 0;
    unsigned short count = 0, found;
    int precompiled = 0;
    CodeViewDirectoryEntry *entry;
    module_count = 0;
    type_segment_bytes = source_symbol_bytes = source_line_bytes = 0;
    source_type_bytes = source_module_bytes = 0;
    for (index = 0; index < directory_count; ++index) {
        entry = &directory_entries[index];
        switch (entry->subsection) {
        case 0x120:
            if (entry->module != 0xffff) ++module_count;
            break;
        case 0x12f:
            precompiled = 1;
        case 0x121:
            if (entry->size >= type_segment_bytes) type_segment_bytes = entry->size;
            break;
        case 0x123:
            if (entry->size >= source_symbol_bytes) source_symbol_bytes = entry->size;
            break;
        case 0x124:
            if (entry->size >= source_line_bytes) source_line_bytes = entry->size;
            break;
        case 0x127:
            if (entry->size >= source_type_bytes) source_type_bytes = entry->size;
            break;
        case 0x128: load_debug_subsection(entry, &libraries, &libraries_size); break;
        case 0x12d: load_debug_subsection(entry, &segment_map, &segment_map_size); break;
        case 0x12e: load_debug_subsection(entry, &segment_names, &segment_names_size); break;
        default: fatal_error(1013, NULL, NULL);
        }
    }
    module_index = allocate_cleared(module_count * sizeof(ModuleIndex));
    if (!module_index) fatal_error(1001, NULL, NULL);
    if (precompiled) {
        for (index = 0; index < directory_count; ++index) {
            entry = &directory_entries[index];
            if (entry->subsection == 0x120 && entry->module && entry->module != 0xffff) previous = index;
            else if (entry->subsection == 0x12f) {
                module_index[count].module = entry->module;
                module_index[count].directory_index = previous;
                module_index[count].info = find_module_info(entry->module);
                ++count;
            }
        }
    }
    for (index = 0; index < directory_count; ++index) {
        entry = &directory_entries[index];
        if (entry->subsection != 0x120) continue;
        for (found = 0; found < count; ++found)
            if (module_index[found].module == entry->module) break;
        if (found == count) {
            module_index[count].module = entry->module;
            module_index[count].directory_index = index;
            module_index[count].info = find_module_info(entry->module);
            ++count;
        }
    }
}

/* Original owned subsection load [00405910,00405992). */
void load_debug_subsection(CodeViewDirectoryEntry *entry, unsigned char **data, unsigned *size)
{
    unsigned char *buffer = malloc(entry->size);
    if (!buffer) fatal_error(1001, NULL, NULL);
    seek_cached_file(input_file, codeview_file_offset + entry->offset, 0);
    if (read_cached_file(input_file, buffer, entry->size) != entry->size) fatal_error(1005, NULL, NULL);
    *data = buffer;
    *size = entry->size;
}
