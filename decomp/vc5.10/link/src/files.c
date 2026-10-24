#include <windows.h>
#include <stdlib.h>
#include <string.h>
#include "files.h"
#include "link.h"
#include "options.h"
#include <recon.h>

FileRecord **file_slots, *file_newest;
unsigned file_record_count, file_window_limit;
FileRecord *mapped_files[16];
unsigned char file_mapping_enabled;

extern BYTE file_access_statistics;
void __cdecl record_file_access(void);

#pragma intrinsic(memcpy)
LONG __stdcall seek_file(int file,LONG offset,int origin,DWORD *error)
{
    FileRecord *record;
    if (file_access_statistics) record_file_access();
    record=file_slots[file];
    if (record->flags & 8) return seek_mapped_file(record,offset,origin,error);
    return seek_buffered_file(record,offset,origin);
}
LONG __stdcall seek_buffered_file(FileRecord *record,LONG offset,int origin)
{
    __asm { int 3 }
    return -1;
}
LONG __stdcall get_file_length(int file)
{
    LONG position,length;
    DWORD current_error=0,end_error;
    if (file_access_statistics) record_file_access();
    position=seek_file(file,0,FILE_CURRENT,&current_error);
    end_error=0;
    length=seek_file(file,0,FILE_END,&end_error);
    if (length!=position) {
        end_error=0;
        seek_file(file,position,FILE_BEGIN,&end_error);
    }
    return length;
}
unsigned __stdcall mapped_read(FileRecord *record,void *buffer,unsigned bytes)
{
    DWORD error=0;
    LONG end=(LONG)((DWORD)record->file_size+bytes);
    if (record->mapping_size<end) {
        if (!(record->flags & 2)) return 0;
        if (!resize_file_mapping(record,end,&error))
            link_fatal(record->name,1106,(DWORD)record->file_size+bytes);
    }
    memcpy(buffer,(char *)record->view+record->file_size,bytes);
    record->file_size=(LONG)((DWORD)record->file_size+bytes);
    return bytes;
}
BYTE __stdcall resize_file_mapping(FileRecord *record,LONG minimum,DWORD *error)
{
    __asm { int 3 }
    return 0;
}

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
    for (i = 1; i < 16; ++i) mapped_files[i] = NULL;
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
        record->size=0;
        record->file=NULL;
        record->view=NULL;
        record->mapping_size=0;
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
    FileRecord **destination_last,unsigned char append)
{
    FileRecord *previous=*source_first,*current;
    if (record==previous) {
        *source_first=record->previous;
        if (source_last && *source_last==record) *source_last=NULL;
    } else {
        current=previous->previous;
        while (current) {
            if (current==record) {
                previous->previous=current->previous;
                break;
            }
            previous=current;
            current=current->previous;
        }
        if (source_last && *source_last==record) *source_last=previous;
    }
    if (!*destination_first) {
        *destination_first=record;
        record->previous=NULL;
        if (destination_last) {
            *destination_last=record;
            return;
        }
    } else if (append) {
        record->previous=NULL;
        (*destination_last)->previous=record;
        *destination_last=record;
        return;
    } else {
        record->previous=*destination_first;
        *destination_first=record;
    }
}
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
long __stdcall seek_mapped_file(FileRecord *record,long offset,int origin,DWORD *error) { __asm { int 3 } return -1; }
unsigned char __stdcall initialize_file_mapping(FileRecord *record,unsigned char resize,DWORD *error)
{
    HANDLE mapping;
    if ((record->flags & 2) && resize) {
        if (SetFilePointer(record->file,record->mapping_size,NULL,FILE_BEGIN)!=
            (DWORD)record->mapping_size || !SetEndOfFile(record->file)) goto failed;
    }
    mapping=CreateFileMappingA(record->file,NULL,
        (record->flags & 2)?PAGE_READWRITE:PAGE_READONLY,0,record->mapping_size,NULL);
    if (!mapping) {
failed:
        *error=GetLastError();
        CloseHandle(record->file);
        record->file=NULL;
        return 0;
    }
    if (record->flags & 0x20) record->base=NULL;
    record->view=MapViewOfFileEx(mapping,(record->flags & 2)?FILE_MAP_ALL_ACCESS:FILE_MAP_READ,
        0,0,record->mapping_size,record->base);
    if (!record->view) {
        *error=GetLastError();
        CloseHandle(mapping);
        CloseHandle(record->file);
        record->file=NULL;
        return 0;
    }
    if (!CloseHandle(mapping)) {
        *error=GetLastError();
        return 0;
    }
    return 1;
}

int __stdcall open_mapped_file(FileRecord *record,int flags,unsigned char created,DWORD *error)
{
    unsigned i;
    unsigned char writable=(flags & 3)!=0,truncate=(flags & 0x100)!=0;
    DWORD attributes=(flags & 0x40)?(FILE_FLAG_DELETE_ON_CLOSE|FILE_ATTRIBUTE_TEMPORARY):FILE_ATTRIBUTE_NORMAL;
    long size;
    if (!created) {
        seek_mapped_file(record,0,0,error);
        return 0;
    }
    if (writable) {
        for (i=0;i<16;++i) {
            if (!mapped_files[i]) {
                mapped_files[i]=record;
                break;
            }
        }
        record->file=CreateFileA(record->name,GENERIC_READ|GENERIC_WRITE,0,NULL,
            truncate?CREATE_ALWAYS:OPEN_EXISTING,attributes,NULL);
    } else {
        record->file=CreateFileA(record->name,GENERIC_READ,FILE_SHARE_READ,NULL,
            OPEN_EXISTING,0,NULL);
    }
    if (record->file==INVALID_HANDLE_VALUE) {
        *error=GetLastError();
        record->file=NULL;
        if (writable || (*error!=ERROR_PATH_NOT_FOUND && *error!=ERROR_FILE_NOT_FOUND))
            return -1;
        move_file_record(record,&pending_files,NULL,&file_newest,NULL,0);
        link_fatal(NULL,1181,record->name);
    }
    size=truncate?0:GetFileSize(record->file,NULL);
    if (size==-1) link_fatal(NULL,1104,record->name);
    record->size=size;
    if (!writable) record->mapping_size=size;
    else if (!record->mapping_size) record->mapping_size=size<0x80000?0x80000:size;
    if (!initialize_file_mapping(record,0,error)) return -1;
    record->flags|=8;
    return 0;
}
void __cdecl initialize_file_buffers(void) { __asm { int 3 } }
int __stdcall open_buffered_file(FileRecord *record,int flags,int mode,unsigned char created) { __asm { int 3 } return -1; }
