/* Image defaults copied by create_image (00017000).
 * Layouts are the COFF header, PE32 optional header, and LINK driver options.
 */
typedef unsigned short WORD;
typedef unsigned long DWORD;
typedef struct FileHeader {
    WORD machine, section_count;
    DWORD timestamp, symbol_offset, symbol_count;
    WORD optional_header_size, characteristics;
} FileHeader;
typedef struct DataDirectory { DWORD address, size; } DataDirectory;
typedef struct OptionalHeader {
    WORD magic;
    unsigned char linker_major, linker_minor;
    DWORD code_size, initialized_size, uninitialized_size;
    DWORD entry, code_base, data_base, image_base;
    DWORD section_alignment, file_alignment;
    WORD os_major, os_minor, image_major, image_minor;
    WORD subsystem_major, subsystem_minor;
    DWORD version, image_size, header_size, checksum;
    WORD subsystem, dll_flags;
    DWORD stack_reserve, stack_commit, heap_reserve, heap_commit;
    DWORD loader_flags, directory_count;
    DataDirectory directories[16];
} OptionalHeader;
typedef struct ImageOptions {
    WORD format;
    unsigned char fixed_image, fixed_alignment;
    unsigned char flags[16];
    DWORD resource_format, debug_format, debug_flags;
    DWORD profile, working_set;
    char *dll_extension;
    DWORD import_mode, export_mode, optimize;
    DWORD reserved[4];
} ImageOptions;
char dll_extension[] = ".dll";
ImageOptions image_option_defaults = {1,0,0,{0},0,0,6,0,0,dll_extension,0,0,1,{0}};
FileHeader file_header_defaults = {0x14c,0,0,0,0,224,0x100};
OptionalHeader optional_header_defaults = {
    0x10b,2,23,0,0,0,0,0,0,0x10000,0x1000,0x200,
    1,0,0,0,0,0,0,0,0,0,0,0,
    0x100000,0x1000,0x100000,0x1000,0,16,{{0,0}}
};
#include "image-asm.inc"
