/* Executable headers and the original CodeView directory. */
#include "image.h"
#include "types.h"
#include <recon.h>

unsigned long cv_read_offset;
long optional_header_offset;
long section_header_offset;
long original_file_size;
unsigned short cv_version;
long debug_section_offset;
unsigned long debug_directory_offset;
long cv_file_offset;
CvDirectoryHeader cv_directory_header;
long cv_directory_offset;
CvDirectoryEntry *directory_entries;
unsigned long directory_entry_count;
unsigned long directory_size;
unsigned long module_size;
unsigned long type_size;
unsigned long symbol_size;
unsigned long global_symbol_size;
unsigned long directory_capacity;
unsigned long module_capacity;
unsigned long type_capacity;
unsigned long symbol_capacity;
unsigned char ** global_symbol_blocks;
unsigned short global_symbol_block;
unsigned short global_symbol_block_count;

void read_image_debug(void)
{
    IMAGE_DOS_HEADER dos;
    IMAGE_FILE_HEADER file;
    IMAGE_OPTIONAL_HEADER optional;
    IMAGE_SECTION_HEADER section;
    IMAGE_DEBUG_DIRECTORY debug;
    unsigned long signature, length, entries, sections;
    unsigned short unusual_dos = 0;
    char drive[4], directory[256], filename[256], extension[256];
    char buffer[4096], *source, *destination;
    int output, bytes;

    cv_read_offset = 0;
    if (seek_file(input_file, 0, 0) == -1 ||
        read_file(input_file, &dos, sizeof(dos)) != sizeof(dos))
        fatal_error(17, NULL, NULL);
    if (dos.e_magic == IMAGE_DOS_SIGNATURE) {
        unusual_dos = dos.e_lfarlc != 0x40;
        if (seek_file(input_file, dos.e_lfanew, 0) != -1 &&
            read_file(input_file, &signature, 4) == 4) {
            if (read_file(input_file, &file, sizeof(file)) != sizeof(file))
                fatal_error(17, NULL, NULL);
            optional_header_offset = tell_file(input_file);
            if (read_file(input_file, &optional, sizeof(optional)) != sizeof(optional))
                fatal_error(17, NULL, NULL);
            if (file.SizeOfOptionalHeader != sizeof(optional) &&
                seek_file(input_file, file.SizeOfOptionalHeader-sizeof(optional), 1) == -1)
                fatal_error(17, NULL, NULL);
            if (signature == IMAGE_NT_SIGNATURE) is_pe_image = 1;
        }
    } else if (dos.e_magic == IMAGE_NT_SIGNATURE) {
        is_pe_image = 1;
        seek_file(input_file, 0, 0);
        if (read_file(input_file, &file, sizeof(file)) != sizeof(file))
            fatal_error(17, NULL, NULL);
        optional_header_offset = tell_file(input_file);
        if (read_file(input_file, &optional, sizeof(optional)) != sizeof(optional))
            fatal_error(17, NULL, NULL);
        if (file.SizeOfOptionalHeader != sizeof(optional) &&
            seek_file(input_file, file.SizeOfOptionalHeader-sizeof(optional), 1) == -1)
            fatal_error(17, NULL, NULL);
    }
    if (is_pe_image) {
        sections = file.NumberOfSections;
        checksum_required = optional.CheckSum;
        section_header_offset = tell_file(input_file);
        while (sections) {
            if (read_file(input_file, &section, sizeof(section)) != sizeof(section))
                fatal_error(17, NULL, NULL);
            if (strncmp((char *)section.Name, ".debug", 6) == 0) {
                debug_section_offset = tell_file(input_file)-sizeof(section);
                break;
            }
            --sections;
        }
        if (seek_file(input_file, 0, 2) == -1) fatal_error(17, NULL, NULL);
        original_file_size = tell_file(input_file);
        if (seek_file(input_file, section_header_offset, 0) == -1)
            fatal_error(17, NULL, NULL);
        sections = file.NumberOfSections;
        while (sections) {
            section_header_offset = tell_file(input_file);
            if (read_file(input_file, &section, sizeof(section)) != sizeof(section))
                fatal_error(17, NULL, NULL);
            if (section.VirtualAddress <= optional.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].VirtualAddress &&
                section.VirtualAddress+section.SizeOfRawData > optional.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].VirtualAddress) {
                debug_directory_offset = optional.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].VirtualAddress-section.VirtualAddress;
                break;
            }
            --sections;
        }
        if (!sections) fatal_error(24, NULL, NULL);
        entries = optional.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].Size/sizeof(debug);
        if (!entries) fatal_error(17, NULL, NULL);
        while (entries) {
            if (seek_file(input_file, debug_directory_offset+section.PointerToRawData, 0) == -1)
                fatal_error(17, NULL, NULL);
            if (read_file(input_file, &debug, sizeof(debug)) != sizeof(debug))
                fatal_error(17, NULL, NULL);
            if (debug.Type == IMAGE_DEBUG_TYPE_CODEVIEW) break;
            debug_directory_offset += sizeof(debug);
            --entries;
        }
        if (debug.Type != IMAGE_DEBUG_TYPE_CODEVIEW) fatal_error(24, NULL, NULL);
        cv_file_offset = debug.PointerToRawData;
        if (seek_file(input_file, cv_file_offset, 0) == -1) fatal_error(17, NULL, NULL);
        cv_version = read_cv_signature();
    } else {
        if (seek_file(input_file, -8, 2) == -1) fatal_error(17, NULL, NULL);
        cv_version = read_cv_signature();
        if (read_file(input_file, &length, 4) != 4 ||
            seek_file(input_file, -length, 2) == -1) fatal_error(24, NULL, NULL);
        cv_file_offset = tell_file(input_file);
        if (read_cv_signature() != cv_version) fatal_error(17, NULL, NULL);
    }
    if (strip_symbols) {
        if (debug_filename) {
            source = debug_filename;
            destination = executable_name;
            do { *destination++ = *source; } while (*source++);
            _splitpath(executable_name, drive, directory, filename, extension);
            if (!extension[0]) {
                source = ".dbg";
                destination = extension;
                do { *destination++ = *source; } while (*source++);
            }
            _makepath(executable_name, drive, directory, filename, extension);
            debug_filename = executable_name;
            output = open_file(executable_name, 0x8101, 0x80);
            if (output == -1) {
                output = open_file(debug_filename, 0x8000);
                fatal_error(output == -1 ? 15 : 16, debug_filename, NULL);
            }
            seek_file(input_file, cv_file_offset, 0);
            while ((bytes = read_file(input_file, buffer, sizeof(buffer))) > 0)
                if (write_file(output, buffer, bytes) != bytes) fatal_error(20, NULL, NULL);
            if (bytes < 0) fatal_error(17, NULL, NULL);
            close_file(output, 1);
        }
        if (truncate_file(input_file, cv_file_offset) == -1) fatal_error(20, NULL, NULL);
        exit(0);
    }
    switch (cv_version) {
    case 0:
        if (unusual_dos) fatal_error(25, NULL, NULL);
        read_old_directory();
        initialize_modules();
        break;
    case 1:
        read_new_directory(1);
        if (cv_directory_header.next) fatal_error(17, NULL, NULL);
        initialize_modules();
        break;
    case 3:
        read_new_directory(0);
        if (cv_directory_header.next) fatal_error(17, NULL, NULL);
        read_packed_modules();
        already_packed = 1;
        break;
    }
    symbol_capacity = symbol_size;
    type_capacity = type_size;
    module_capacity = module_size;
    directory_capacity = directory_size;
    if (directory_size) {
        directory_data = malloc(directory_size);
        if (!directory_data) fatal_error(13, NULL, NULL);
    }
    symbol_data = malloc(symbol_capacity);
    if (!symbol_data) fatal_error(13, NULL, NULL);
    type_data = malloc(type_capacity);
    if (!type_data) fatal_error(13, NULL, NULL);
    module_data = malloc(module_capacity);
    if (!module_data) fatal_error(13, NULL, NULL);
    global_symbol_size += 4;
    global_symbol_block_count = (unsigned short)(global_symbol_size/0xffe8+2);
    global_symbol_blocks = checked_zero(global_symbol_block_count*sizeof(*global_symbol_blocks));
    if (!global_symbol_blocks) fatal_error(13, NULL, NULL);
    length = global_symbol_size < 0xffe8 ? global_symbol_size : 0xffe8;
    global_symbol_blocks[global_symbol_block] = malloc(length);
    if (!global_symbol_blocks[global_symbol_block]) fatal_error(13, NULL, NULL);
}

unsigned char cv_signature[4];

unsigned short read_cv_signature(void)
{
    if (read_file(input_file, cv_signature, 4) == 4) {
        if (cv_signature[0] != 'N' || cv_signature[1] != 'B' || cv_signature[2] != '0' ||
            cv_signature[3] < '2' || cv_signature[3] == '3' ||
            cv_signature[3] == '4' || cv_signature[3] == '7')
            fatal_error(25, NULL, NULL);
        if (cv_signature[3] == '9') {
            if (strip_symbols) return 0;
            warning(1, NULL, NULL);
            exit(0);
        }
        if (cv_signature[3] == '8') return strip_symbols ? 0 : 3;
        if (cv_signature[3] == '2') return 0;
        if (cv_signature[3] == '5') return 1;
        fatal_error(25, NULL, NULL);
    }
    fatal_error(17, NULL, NULL);
    return 0;
}

void read_new_directory(int unpacked)
{
    unsigned short module = 0;
    unsigned long i;
    if (read_file(input_file, &cv_directory_offset, 4) != 4 ||
        seek_file(input_file, cv_file_offset+cv_directory_offset, 0) == -1 ||
        read_file(input_file, &cv_directory_header, sizeof(cv_directory_header)) != sizeof(cv_directory_header))
        fatal_error(17, NULL, NULL);
    directory_entry_count = cv_directory_header.count;
    directory_entries = malloc((directory_entry_count+6)*sizeof(*directory_entries));
    if (!directory_entries) fatal_error(13, NULL, NULL);
    if (read_file(input_file, directory_entries, directory_entry_count*sizeof(*directory_entries)) != directory_entry_count*sizeof(*directory_entries))
        fatal_error(17, NULL, NULL);
    if (unpacked) {
        for (i = 0; i < directory_entry_count; ++i) {
            if (directory_entries[i].module && directory_entries[i].module != 0xffff &&
                directory_entries[i].module != module) {
                if (directory_entries[i].subsection != 0x120) break;
                module = directory_entries[i].module;
            }
        }
        if (i != directory_entry_count)
            qsort(directory_entries, directory_entry_count, sizeof(*directory_entries), compare_directory);
    }
}

/* The subsection comparator uses a 17-by-17 signed ordering matrix.  Spell
   out its rule so each row's relationship is visible, rather than copying
   an opaque block of initialized data. */
#define SUBSECTION_ORDER(a,b) ((a)<(b) ? -1 : ((a)>(b) ? 1 : 0))
#define ORDER_ROW(a) { SUBSECTION_ORDER(a,0), SUBSECTION_ORDER(a,1), \
    SUBSECTION_ORDER(a,2), SUBSECTION_ORDER(a,3), SUBSECTION_ORDER(a,4), \
    SUBSECTION_ORDER(a,5), SUBSECTION_ORDER(a,6), SUBSECTION_ORDER(a,7), \
    SUBSECTION_ORDER(a,8), SUBSECTION_ORDER(a,9), SUBSECTION_ORDER(a,10), \
    SUBSECTION_ORDER(a,11), SUBSECTION_ORDER(a,12), SUBSECTION_ORDER(a,13), \
    SUBSECTION_ORDER(a,14), SUBSECTION_ORDER(a,15), SUBSECTION_ORDER(a,16) }
int directory_order[17][17] = {
    ORDER_ROW(0), ORDER_ROW(1), ORDER_ROW(2), ORDER_ROW(3), ORDER_ROW(4),
    ORDER_ROW(5), ORDER_ROW(6), ORDER_ROW(7), ORDER_ROW(8), ORDER_ROW(9),
    ORDER_ROW(10), ORDER_ROW(11), ORDER_ROW(12), ORDER_ROW(13), ORDER_ROW(14),
    ORDER_ROW(15), ORDER_ROW(16)
};
#undef ORDER_ROW
#undef SUBSECTION_ORDER

int __cdecl compare_directory(const void *first, const void *second)
{
    const CvDirectoryEntry *a = first, *b = second;
    unsigned short left, right;
    if (b->module > a->module) return -1;
    if (b->module < a->module) return 1;
    left = a->subsection-0x120;
    right = b->subsection-0x120;
    return directory_order[left][right];
}

ModuleIndex *module_index;
ModuleInfo *module_list;
unsigned char *libraries, *segment_map, *segment_names;
unsigned long libraries_size, segment_map_size, segment_names_size;

void initialize_modules(void)
{
    unsigned long i, previous;
    unsigned short count, j;
    int precompiled = 0;
    CvDirectoryEntry *entry;
    ModuleIndex *module;

    module_count = 0;
    global_symbol_size = symbol_size = type_size = module_size = directory_size = 0;
    for (i = 0; i < directory_entry_count; ++i) {
        entry = &directory_entries[i];
        switch (entry->subsection) {
        case 0x101:
            if (entry->module != 0xffff) {
                ++module_count;
                if (entry->size > directory_size) directory_size = entry->size;
            }
            break;
        case 0x120:
            if (entry->module != 0xffff) ++module_count;
            break;
        case 0x102: case 0x122: case 0x123:
            if (entry->size > symbol_size) symbol_size = entry->size;
            break;
        case 0x103: case 0x121:
            if (entry->size > global_symbol_size) global_symbol_size = entry->size;
            break;
        case 0x104: case 0x124:
            if (entry->size > type_size) type_size = entry->size;
            break;
        case 0x106: case 0x128:
            read_subsection(entry, &libraries, &libraries_size);
            break;
        case 0x109: case 0x126: case 0x127:
            if (entry->size > module_size) module_size = entry->size;
            break;
        case 0x12d:
            read_subsection(entry, &segment_map, &segment_map_size);
            break;
        case 0x12e:
            read_subsection(entry, &segment_names, &segment_names_size);
            break;
        case 0x12f:
            precompiled = 1;
            if (entry->size > global_symbol_size) global_symbol_size = entry->size;
            break;
        default:
            fatal_error(25, NULL, NULL);
        }
    }
    if (module_count > 0x1555) fatal_error(17, NULL, NULL);
    module_index = checked_zero(module_count*sizeof(*module_index));
    if (!module_index) fatal_error(13, NULL, NULL);
    count = 0;
    if (precompiled == 1) {
        for (i = 0; i < directory_entry_count; ++i) {
            entry = &directory_entries[i];
            switch (entry->subsection) {
            case 0x101:
                fatal_error(17, NULL, NULL);
            case 0x120:
                if (entry->module != 0xffff && entry->module) previous = i;
                break;
            case 0x12f:
                module = &module_index[count];
                module->module = entry->module;
                module->directory = previous;
                ++count;
                module->info = find_module(directory_entries[i].module, 1);
                break;
            }
        }
    }
    for (i = 0; i < directory_entry_count; ++i) {
        entry = &directory_entries[i];
        if (entry->subsection == 0x101 || entry->subsection == 0x120) {
            for (j = 0; j < count; ++j)
                if (module_index[j].module == entry->module) break;
            if (j == count) {
                module = &module_index[count];
                module->module = entry->module;
                module->directory = i;
                ++count;
                module->info = find_module(directory_entries[i].module, 1);
            }
        }
    }
}

void read_subsection(const CvDirectoryEntry *entry, unsigned char **data, unsigned long *size)
{
    unsigned char *buffer = malloc((unsigned short)entry->size);
    if (!buffer) {
        fatal_error(13, NULL, NULL);
        if (!buffer) fatal_error(13, NULL, NULL);
    }
    seek_file(input_file, cv_file_offset+entry->location.offset, 0);
    if (read_file(input_file, buffer, (unsigned short)entry->size) != entry->size)
        fatal_error(17, NULL, NULL);
    *data = buffer;
    *size = entry->size;
}

ModuleInfo *find_module(unsigned short number, int create)
{
    ModuleInfo *previous = NULL, *current = module_list, *module;
    unsigned long *words;
    int i;
    while (current) {
        if (number == current->module) return current;
        if (number < current->module) break;
        previous = current;
        current = current->next;
    }
    if (create == 1) {
        module = malloc(sizeof(*module));
        words = (unsigned long *)module;
        for (i = 0; i < 12; ++i) words[i] = 0;
        module->module = number;
        if (!previous) module_list = module;
        else previous->next = module;
        module->next = current;
        return module;
    }
    fatal_error(31, NULL, NULL);
    return NULL;
}
