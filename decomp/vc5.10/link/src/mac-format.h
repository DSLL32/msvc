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

#endif
