#ifndef LINK_MAC_FORMAT_H
#define LINK_MAC_FORMAT_H
#include <windows.h>
#include <stddef.h>
#include <recon.h>

/* MPPC's .ppcshl payload is a fixed header followed by 200-byte names. */
struct MacExportBucket { LONG first; DWORD count; };
struct MacSharedLibraryHeader {
    MacExportBucket buckets[76];
    DWORD reserved, named_count;
    WORD format, reserved_version;
    DWORD section_file_offset;
    char library[52];
};
typedef char MacSharedHeaderLayout[(sizeof(MacSharedLibraryHeader)==676 &&
    offsetof(MacSharedLibraryHeader,named_count)==612 &&
    offsetof(MacSharedLibraryHeader,library)==624)?1:-1];

/* The loader writer owns exactly fourteen DWORDs at4944d8..494510. */
struct MacLoaderHeader {
    LONG main_section;
    DWORD main_offset;
    LONG initializer_section;
    DWORD initializer_offset;
    LONG terminator_section;
    DWORD terminator_offset;
    DWORD library_count,import_symbol_count,relocation_section_count;
    DWORD relocation_instruction_offset,strings_offset,export_hash_offset;
    DWORD export_hash_power,export_count;
};
typedef char MacLoaderHeaderLayout[(sizeof(MacLoaderHeader)==56 &&
    offsetof(MacLoaderHeader,relocation_instruction_offset)==36 &&
    offsetof(MacLoaderHeader,strings_offset)==40 &&
    offsetof(MacLoaderHeader,export_hash_offset)==44 &&
    offsetof(MacLoaderHeader,export_hash_power)==48 &&
    offsetof(MacLoaderHeader,export_count)==52)?1:-1];
struct MacImportedLibrary {
    DWORD name_offset,old_version,current_version,symbol_count,first_symbol;
    BYTE options,reserved[3];
};
typedef char MacImportedLibraryLayout[(sizeof(MacImportedLibrary)==24 &&
    offsetof(MacImportedLibrary,options)==20)?1:-1];
struct MacImportedSymbol {
    BYTE kind,name_offset[3];
};
typedef char MacImportedSymbolSize[(sizeof(MacImportedSymbol)==4)?1:-1];

#endif
