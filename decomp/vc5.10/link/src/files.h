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
    long size;
    HANDLE file;
    void *view;
    long mapping_size, file_size; /* file_size is the mapped cursor used by TELL_FILE. */
    void *base;
    struct FileRecord *previous;
} FileRecord;
typedef char FileRecordLayoutCheck[(sizeof(FileRecord)==40 &&
    offsetof(FileRecord,flags)==4 && offsetof(FileRecord,index)==8 &&
    offsetof(FileRecord,size)==12 && offsetof(FileRecord,file)==16 &&
    offsetof(FileRecord,view)==20 && offsetof(FileRecord,mapping_size)==24 &&
    offsetof(FileRecord,file_size)==28 && offsetof(FileRecord,base)==32 &&
    offsetof(FileRecord,previous)==36)?1:-1];
/* Encountered buffered-file prefix: the cursor and flags retain one owner. */
typedef struct FileBufferHeader {
    BYTE reserved00[12];
    LONG cursor;
    DWORD reserved10,flags;
} FileBufferHeader;
typedef char FileBufferCursorAt0c[(offsetof(FileBufferHeader,cursor)==12)?1:-1];
typedef char FileBufferFlagsAt14[(offsetof(FileBufferHeader,flags)==20)?1:-1];
struct OutputModule;
int __stdcall open_file_view(const char *,int,int,void **,DWORD *,DWORD *);
int __stdcall open_module_file(struct OutputModule *,int,int);
extern FileRecord **file_slots, *file_newest;
extern unsigned file_record_count, file_window_limit;
extern FileRecord *mapped_files[16];
extern unsigned char file_mapping_enabled;
FileRecord *__cdecl new_file_record(void);
void *__stdcall zero_allocate(unsigned bytes);
void __cdecl out_of_memory(void);
LONG __stdcall seek_file(int file,long offset,int origin,DWORD *error);
LONG __stdcall seek_mapped_file(FileRecord *,LONG,int,DWORD *);
LONG __stdcall seek_buffered_file(FileRecord *,LONG,int);
LONG __stdcall get_file_length(int);
int __stdcall close_argument_file(int,int);
unsigned __stdcall mapped_read(FileRecord *,void *,unsigned);
BYTE __stdcall resize_file_mapping(FileRecord *,LONG,DWORD *);
#ifdef __cplusplus
}
#endif
#endif
