#ifndef LINK_FILES_H
#define LINK_FILES_H
#include <windows.h>
#include <recon.h>

typedef struct FileRecord {
    char *name;
    unsigned char flags;
    unsigned index;
    long position;
    HANDLE file, mapping;
    long mapped_size, file_size;
    void *base;
    struct FileRecord *previous;
} FileRecord;
extern FileRecord **file_slots, *file_newest;
extern unsigned file_record_count, file_window_limit, file_statistics[15];
extern unsigned char file_mapping_enabled;
FileRecord *__cdecl new_file_record(void);
void *__stdcall zero_allocate(unsigned bytes);
void __cdecl out_of_memory(void);
#endif
