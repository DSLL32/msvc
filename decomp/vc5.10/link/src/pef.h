#ifndef LINK_PEF_H
#define LINK_PEF_H
#include <windows.h>
#include <stddef.h>
#include <recon.h>

/* On-disk Apple PEF structures. Multi-byte fields retain file byte order
 * until the dumper reverses a local copy, as in the original. */
typedef struct PefContainerHeader {
    WORD magic1,magic2;
    DWORD container_id,architecture,version,timestamp;
    DWORD old_definition_version,old_implementation_version,current_version;
    WORD section_count,loadable_sections;
    DWORD memory_address;
} PefContainerHeader;
typedef struct PefSectionHeader {
    LONG name_offset;
    DWORD address,total_size,initialized_size,raw_size,file_offset;
    BYTE kind,share_kind,alignment,reserved;
} PefSectionHeader;
typedef struct PefLoaderHeader {
    LONG main_section;
    DWORD main_offset;
    LONG init_section;
    DWORD init_offset;
    LONG term_section;
    DWORD term_offset;
    DWORD library_count,import_count,relocation_section_count;
    DWORD relocation_offset,strings_offset,hash_offset,hash_power,export_count;
} PefLoaderHeader;
typedef struct PefImportLibrary {
    DWORD name_offset,old_version,current_version,import_count,first_import;
    BYTE flags,reserved_byte;
    WORD reserved_word;
} PefImportLibrary;
typedef struct PefRelocationHeader {
    WORD section,reserved;
    DWORD instruction_count,instruction_offset;
} PefRelocationHeader;
#pragma pack(push,2)
typedef struct PefExportSymbol {
    DWORD class_and_name,value;
    SHORT section;
} PefExportSymbol;
#pragma pack(pop)
typedef char PefExportLayout[(sizeof(PefExportSymbol)==10 &&
    offsetof(PefExportSymbol,section)==8)?1:-1];
typedef char PefHeaderLayout[(sizeof(PefContainerHeader)==40 &&
    offsetof(PefContainerHeader,section_count)==32 &&
    sizeof(PefSectionHeader)==28 && offsetof(PefSectionHeader,kind)==24 &&
    sizeof(PefLoaderHeader)==56 && offsetof(PefLoaderHeader,strings_offset)==40 &&
    sizeof(PefImportLibrary)==24 && sizeof(PefRelocationHeader)==12)?1:-1];
#endif
