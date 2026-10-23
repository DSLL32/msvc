#ifndef CV510_IMAGE_H
#define CV510_IMAGE_H
#include "types.h"
#include <recon.h>
extern IMAGE_FILE_HEADER image_file_header;
extern unsigned image_read_mode, optional_header_position, section_table_position;
extern unsigned debug_section_position, original_image_checksum, original_file_length;
extern unsigned debug_directory_offset, codeview_file_offset;
extern unsigned char image_linker_major, image_linker_minor;
extern unsigned short codeview_version;
extern unsigned char codeview_signature[4];
typedef struct CodeViewDirectoryHeader {
    unsigned short header_size, entry_size;
    unsigned count, next_directory, flags;
} CodeViewDirectoryHeader;
typedef struct CodeViewDirectoryEntry {
    unsigned short subsection, module;
    unsigned offset, size;
} CodeViewDirectoryEntry;
extern CodeViewDirectoryHeader codeview_directory_header;
extern CodeViewDirectoryEntry *directory_entries;
extern unsigned directory_relative_offset, directory_count;
typedef struct ModuleInfo {
    unsigned short module, reserved;
    unsigned fields[10];
    struct ModuleInfo *next;
} ModuleInfo;
typedef struct ModuleIndex {
    unsigned short module, reserved;
    unsigned directory_index;
    ModuleInfo *info;
} ModuleIndex;
extern unsigned char *libraries, *segment_map, *segment_names;
extern unsigned libraries_size, segment_map_size, segment_names_size;
extern ModuleIndex *module_index;
extern ModuleInfo *module_list;
extern unsigned pack_buffer_size, pack_fixup_count;
extern unsigned char *pack_buffer;
extern void *pack_fixups;
void load_debug_subsection(CodeViewDirectoryEntry *entry, unsigned char **data, unsigned *size);
ModuleInfo *find_module_info(unsigned short module);
int __cdecl compare_directory_entries(const void *first, const void *second);
extern unsigned source_symbol_bytes, source_line_bytes, source_type_bytes, source_module_bytes;
extern unsigned symbol_capacity, line_capacity, type_capacity, module_capacity;
extern unsigned type_segment_bytes;
extern unsigned short type_segment_count, type_segment_index;
extern void **type_segments;
int seek_cached_file(int file, int offset, int origin);
unsigned read_cached_file(int file, void *buffer, unsigned size);
unsigned tell_cached_file(int file);
unsigned short read_codeview_signature(void);
void read_codeview_directory(void);
void classify_debug_directory(void);
#endif
