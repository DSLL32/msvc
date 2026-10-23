#ifndef LINK_FILES_H
#define LINK_FILES_H
#include <windows.h>
#include <stddef.h>
#include <recon.h>
#ifdef __cplusplus
extern "C" {
#endif


typedef struct FileRecord {
    char *name;
    unsigned flags;
    unsigned index;
    long position;
    HANDLE file, mapping;
    long mapped_size, file_size;
    void *base;
    struct FileRecord *previous;
} FileRecord;
typedef char FileRecordLayoutCheck[(sizeof(FileRecord)==40 &&
    offsetof(FileRecord,flags)==4 && offsetof(FileRecord,index)==8 &&
    offsetof(FileRecord,previous)==36)?1:-1];
extern FileRecord **file_slots, *file_newest;
extern unsigned file_record_count, file_window_limit, file_statistics[15];
extern unsigned char file_mapping_enabled;
FileRecord *__cdecl new_file_record(void);
void *__stdcall zero_allocate(unsigned bytes);
void __cdecl out_of_memory(void);
#ifdef __cplusplus
}
#endif
#endif
