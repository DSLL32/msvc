/* Buffered packed CodeView output and executable-header repair. */
#include "output.h"
#include <recon.h>

unsigned char *output_buffer;
unsigned long output_capacity, output_used;
int output_handle;
PackedSignature packed_signature = { { 'N', 'B', '0', '9' }, 0 };
long packed_file_size;
unsigned long source_line_bytes;

/* The original inlines this fast path around its buffered writer. */
#define OUTPUT_BYTES(source, count) do { \
    const unsigned char *emit_source = (const unsigned char *)(source); \
    unsigned long emit_size = (count), emit_i; \
    if (output_capacity-output_used >= emit_size) { \
        for (emit_i = 0; emit_i < emit_size; ++emit_i) \
            output_buffer[output_used+emit_i] = emit_source[emit_i]; \
        output_used += emit_size; \
    } else if (!write_output(emit_source, (unsigned short)emit_size)) \
        fatal_error(20, NULL, NULL); \
} while (0)

void write_packed_debug(void)
{
    ModuleInfo *module;
    CvDirectoryEntry *entry;
    unsigned long count = 0, i, directory, debug_size, remainder, rounded;
    long change;
    IMAGE_SECTION_HEADER section;
    IMAGE_DEBUG_DIRECTORY debug;
    IMAGE_OPTIONAL_HEADER optional;

    for (module = module_list; module; module = module->next) {
        ++count;
        if (module->symbol_size) ++count;
        if (module->line_size) ++count;
    }
    directory_entry_count = count;
    count = 0;
    for (module = module_list; module; module = module->next) {
        entry = &directory_entries[count++];
        entry->subsection = 0x120;
        entry->module = module->module;
        entry->location.data = module->record;
        entry->size = module->record_size;
        if (module->symbol_size) {
            entry = &directory_entries[count++];
            entry->subsection = 0x125;
            entry->module = module->module;
            entry->size = module->symbol_size;
            entry->location.data = module->symbols;
        }
        if (module->line_size) {
            entry = &directory_entries[count++];
            entry->subsection = 0x127;
            entry->module = module->module;
            entry->size = module->line_size;
            entry->location.data = module->lines;
            source_line_bytes += module->line_size;
        }
    }
    if (verbose == 1) printf("Line/Address size\t\t= %8.ld\n", source_line_bytes);
    qsort(directory_entries, directory_entry_count, sizeof(*directory_entries),
          compare_output_directory);
    cv_file_offset = (cv_file_offset+3)&~3L;
    if (seek_file(input_file, cv_file_offset, 0) == -1) fatal_error(20, NULL, NULL);
    initialize_output(input_file);
    OUTPUT_BYTES(&packed_signature, sizeof(packed_signature));
    cv_read_offset = tell_file(input_file)+output_used;
    for (i = 0; i < directory_entry_count; ++i) {
        cv_read_offset = tell_file(input_file)+output_used;
        entry = &directory_entries[i];
        if (entry->size > 65535) {
            for (module = module_list; module && module->module != entry->module;
                 module = module->next) {}
            if (entry->subsection == 0x125) warning(8, "symbols", module_name(module));
            else if (entry->subsection == 0x127)
                warning(8, "source lines", module_name(module));
            else warning(8, NULL, module_name(module));
            entry->size = 0;
        }
        write_subsection(i);
    }
    cv_read_offset = tell_file(input_file)+output_used;
    write_global_publics(&directory_entries[directory_entry_count],
                         cv_read_offset-cv_file_offset);
    ++directory_entry_count;
    cv_read_offset = tell_file(input_file)+output_used;
    write_global_symbols(&directory_entries[directory_entry_count],
                         cv_read_offset-cv_file_offset);
    ++directory_entry_count;
    cv_read_offset = tell_file(input_file)+output_used;
    write_static_symbols(&directory_entries[directory_entry_count],
                         cv_read_offset-cv_file_offset);
    ++directory_entry_count;
    cv_read_offset = tell_file(input_file)+output_used;
    entry = &directory_entries[directory_entry_count];
    entry->subsection = 0x128;
    entry->module = 0xffff;
    entry->location.data = libraries;
    entry->size = libraries_size;
    write_subsection(directory_entry_count);
    cv_read_offset = tell_file(input_file)+output_used;
    ++directory_entry_count;
    entry = &directory_entries[directory_entry_count];
    entry->subsection = 0x12b;
    entry->module = 0xffff;
    if (already_packed) {
        entry->size = write_old_packed_types();
        entry->location.offset = cv_read_offset-cv_file_offset;
    } else {
        entry->size = write_packed_types();
        entry->location.offset = cv_read_offset-cv_file_offset;
        if (verbose == 1)
            printf("Initial type size\t\t= %8.ld\nCompacted type size\t\t= %8.ld\n",
                   total_local_type_bytes, entry->size);
    }
    if (segment_map) {
        cv_read_offset = tell_file(input_file)+output_used;
        entry = &directory_entries[++directory_entry_count];
        entry->subsection = 0x12d;
        entry->module = 0xffff;
        entry->location.data = segment_map;
        entry->size = segment_map_size;
        write_subsection(directory_entry_count);
    }
    if (segment_names) {
        cv_read_offset = tell_file(input_file)+output_used;
        entry = &directory_entries[++directory_entry_count];
        entry->subsection = 0x12e;
        entry->module = 0xffff;
        entry->location.data = segment_names;
        entry->size = segment_names_size;
        write_subsection(directory_entry_count);
    }
    if (source_file_index) {
        cv_read_offset = tell_file(input_file)+output_used;
        entry = &directory_entries[++directory_entry_count];
        entry->subsection = 0x133;
        entry->module = 0xffff;
        entry->location.data = source_file_index;
        entry->size = source_file_index_size;
        write_subsection(directory_entry_count);
    }
    cv_read_offset = tell_file(input_file)+output_used;
    cv_directory_header.header_size = 16;
    ++directory_entry_count;
    cv_directory_header.entry_size = 12;
    cv_directory_header.count = directory_entry_count;
    cv_directory_header.next = cv_directory_header.flags = 0;
    directory = tell_file(input_file)-cv_file_offset+output_used;
    OUTPUT_BYTES(&cv_directory_header, sizeof(cv_directory_header));
    OUTPUT_BYTES(directory_entries, (unsigned short)(cv_directory_header.count*12));
    debug_size = tell_file(input_file)-cv_file_offset+output_used+8;
    OUTPUT_BYTES(packed_signature.signature, 4);
    OUTPUT_BYTES(&debug_size, 4);
    if (!flush_output(input_file)) fatal_error(20, NULL, NULL);
    packed_file_size = tell_file(input_file);
    if (is_pe_image) {
        if (seek_file(input_file, section_header_offset, 0) == -1 ||
            read_file(input_file, &section, sizeof(section)) != sizeof(section))
            fatal_error(20, NULL, NULL);
        if (seek_file(input_file, debug_directory_offset+section.PointerToRawData, 0) == -1 ||
            read_file(input_file, &debug, sizeof(debug)) != sizeof(debug))
            fatal_error(20, NULL, NULL);
        debug.SizeOfData = debug_size;
        if (seek_file(input_file, debug_directory_offset+section.PointerToRawData, 0) == -1 ||
            write_file(input_file, &debug, sizeof(debug)) != sizeof(debug))
            fatal_error(20, NULL, NULL);
        if (debug_section_offset) {
            change = packed_file_size-original_file_size;
            if (seek_file(input_file, optional_header_offset, 0) == -1 ||
                read_file(input_file, &optional, sizeof(optional)) != sizeof(optional))
                fatal_error(20, NULL, NULL);
            if (seek_file(input_file, debug_section_offset, 0) == -1 ||
                read_file(input_file, &section, sizeof(section)) != sizeof(section))
                fatal_error(20, NULL, NULL);
            section.SizeOfRawData += change;
#ifdef RECON_NATIVE_TEST
            if (!optional.FileAlignment || !optional.SectionAlignment) __asm { int 3 }
#endif
            rounded = section.SizeOfRawData;
            remainder = rounded%optional.FileAlignment;
            if (remainder) rounded += optional.FileAlignment-remainder;
            section.Misc.VirtualSize = rounded;
            if (seek_file(input_file, debug_section_offset, 0) == -1 ||
                write_file(input_file, &section, sizeof(section)) != sizeof(section))
                fatal_error(20, NULL, NULL);
            rounded = section.Misc.VirtualSize+section.VirtualAddress;
            remainder = rounded%optional.SectionAlignment;
            if (remainder) rounded += optional.SectionAlignment-remainder;
            optional.SizeOfImage = rounded;
            if (seek_file(input_file, optional_header_offset, 0) == -1 ||
                write_file(input_file, &optional, sizeof(optional)) != sizeof(optional))
                fatal_error(20, NULL, NULL);
        }
    }
    truncate_file(input_file, packed_file_size);
    if (seek_file(input_file, cv_file_offset+4, 0) == -1 ||
        write_file(input_file, &directory, 4) != 4) fatal_error(20, NULL, NULL);
}
#undef OUTPUT_BYTES

int __cdecl compare_output_directory(const void *left, const void *right)
{
    const CvDirectoryEntry *a = left, *b = right;

    if (a->subsection == 0x120 || b->subsection == 0x120) {
        if (a->subsection == 0x120 && b->subsection != 0x120) return -1;
        if (a->subsection != 0x120 && b->subsection == 0x120) return 1;
    }
    if (b->module > a->module) return -1;
    if (b->module < a->module) return 1;
    if (b->subsection > a->subsection) return -1;
    if (b->subsection < a->subsection) return 1;
    return 0;
}

void initialize_output(int handle)
{
    output_capacity = 0xffe8;
    do {
        output_buffer = malloc(output_capacity);
        if (output_buffer) break;
        output_capacity >>= 1;
    } while (output_capacity >= 0x1000);
    if (!output_buffer) output_capacity = 0;
    output_used = 0;
    output_handle = handle;
}
