#include <windows.h>
#include <stdlib.h>
#include <string.h>
#include "files.h"
#include "link.h"
#include <recon.h>

FileRecord **file_slots, *file_newest;
unsigned file_record_count, file_window_limit, file_statistics[15];
unsigned char file_mapping_enabled;

void __stdcall initialize_file_cache(unsigned mapped_slots, unsigned mapped_limit,
                                    unsigned ordinary_slots, unsigned ordinary_limit)
{
    HMODULE kernel;
    unsigned count, limit, i;
    FileRecord *record;
    kernel = GetModuleHandleA("kernel32.dll");
    file_mapping_enabled = !(kernel && GetProcAddress(kernel, "IsTNT"));
    if (get_debug_flag(44)) file_mapping_enabled = 0;
    if (file_mapping_enabled) {
        count = mapped_slots;
        limit = mapped_limit;
    } else {
        count = ordinary_slots;
        limit = ordinary_limit;
    }
    file_slots = (FileRecord **)zero_allocate(count * sizeof(FileRecord *));
    for (i = 0; i < count; ++i) {
        record = new_file_record();
        record->index = i;
        if (file_newest) record->previous = file_newest;
        file_newest = record;
        file_slots[i] = record;
    }
    file_window_limit = limit;
    for (i = 0; i < 15; ++i) file_statistics[i] = 0;
}
FileRecord *__cdecl new_file_record(void)
{
    FileRecord *record;
    record = (FileRecord *)zero_allocate(sizeof(FileRecord));
    ++file_record_count;
    return record;
}
void *__stdcall zero_allocate(unsigned bytes)
{
    void *allocation;
    allocation = calloc(1, bytes);
    if (!allocation) out_of_memory();
    return allocation;
}
void __cdecl out_of_memory(void) { __asm { int 3 } }

extern unsigned char file_access_statistics;
void __cdecl record_file_access(void);
FileRecord *__stdcall acquire_file_record(const char *,int,int,unsigned char *);
int __stdcall open_mapped_file(FileRecord *,int,unsigned char,DWORD *);
void __cdecl initialize_file_buffers(void);
int __stdcall open_buffered_file(FileRecord *,int,int,unsigned char);

int __stdcall open_argument_file(const char *name,int flags,int mode)
{
    unsigned char created=0;
    DWORD size=0;
    int mapped=-1;
    FileRecord *record;
    if (file_access_statistics) record_file_access();
    record=acquire_file_record(name,flags,mode,&created);
    if (file_mapping_enabled) mapped=open_mapped_file(record,flags,created,&size);
    if (mapped==-1) {
        initialize_file_buffers();
        open_buffered_file(record,flags,mode,created);
    }
    return record->index;
}

FileRecord *__stdcall find_cached_file(const char *name);
FileRecord *__cdecl get_available_file_record(void);
char *__stdcall duplicate_argument(const char *text);

FileRecord *__stdcall acquire_file_record(const char *name,int flags,int mode,unsigned char *created)
{
    FileRecord *record=find_cached_file(name);
    if (!record) {
        record=get_available_file_record();
        *created=1;
        record->name=name?duplicate_argument(name):NULL;
        record->flags=0;
        record->position=0;
        record->file=NULL;
        record->mapping=NULL;
        record->mapped_size=0;
        record->file_size=0;
        record->base=NULL;
        if (!(flags & 15) || (flags & 2)) record->flags=1;
        if (flags & 3) record->flags|=2;
        if (flags & 0x100) record->flags|=0x10;
        if (flags & 0x40) record->flags|=0x20;
    }
    return record;
}
FileRecord *cached_files,*cached_file_tail,*pending_files;
unsigned cached_file_count;
void __stdcall move_file_record(FileRecord *,FileRecord **,FileRecord **,
                                FileRecord **,FileRecord **,unsigned char);

#pragma intrinsic(strcmp)
FileRecord *__stdcall find_cached_file(const char *name)
{
    FileRecord *record=cached_files;
    if (!name || !record) return NULL;
    do {
        if (!strcmp(record->name,name)) {
            --cached_file_count;
            move_file_record(record,&cached_files,&cached_file_tail,&pending_files,NULL,0);
            record->flags&=~4u;
            return record;
        }
        record=record->previous;
    } while (record);
    return NULL;
}
void __stdcall move_file_record(FileRecord *record,FileRecord **source_first,
    FileRecord **source_last,FileRecord **destination_first,
    FileRecord **destination_last,unsigned char append) { __asm { int 3 } }
int __stdcall close_buffered_file(FileRecord *record);
BOOL __stdcall close_mapped_file(FileRecord *record);

FileRecord *__cdecl get_available_file_record(void)
{
    FileRecord *record;
    if (!file_newest) {
        record=cached_files;
        --cached_file_count;
        move_file_record(record,&cached_files,&cached_file_tail,&file_newest,NULL,0);
        if (record->flags & 8) close_mapped_file(record);
        else close_buffered_file(record);
    }
    record=file_newest;
    move_file_record(record,&file_newest,NULL,&pending_files,NULL,0);
    return record;
}
int __stdcall close_buffered_file(FileRecord *record) { __asm { int 3 } return -1; }
BOOL __stdcall close_mapped_file(FileRecord *record) { __asm { int 3 } return 0; }
int __stdcall open_mapped_file(FileRecord *record,int flags,unsigned char created,DWORD *size) { __asm { int 3 } return -1; }
void __cdecl initialize_file_buffers(void) { __asm { int 3 } }
int __stdcall open_buffered_file(FileRecord *record,int flags,int mode,unsigned char created) { __asm { int 3 } return -1; }
