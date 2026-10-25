/* COFF archive linker members and the optional long-name table. */
#include <windows.h>
#include <string.h>
#include <stdio.h>
#include "link.h"
#include "input.h"
#include "files.h"
#include "memory.h"
#include "options.h"
#include "full-link-functions.h"
#include <recon.h>

extern "C" {
extern BYTE file_access_statistics;
/* Original47f084, paired with archive_member_offset47eed8. */
DWORD archive_member_size;
IMAGE_ARCHIVE_MEMBER_HEADER *__cdecl read_archive_member_header(void);
DWORD __stdcall archive_big_endian_dword(const void *value)
{
    const BYTE *bytes=(const BYTE *)value;
    return ((DWORD)bytes[0]<<24)|((DWORD)bytes[1]<<16)|((DWORD)bytes[2]<<8)|bytes[3];
}
unsigned __stdcall buffered_read(FileRecord *,void *,unsigned);
}

/* Original file-cache macros, expanded here without another graph vertex. */
typedef struct ArchiveBufferedPosition { DWORD reserved[3],position; } ArchiveBufferedPosition;
#define READ_ARCHIVE(data,bytes) do { \
    if (file_access_statistics) record_file_access(); \
    file_record=file_slots[current_input_file]; \
    if (file_record->flags & 8) mapped_read(file_record,(data),(bytes)); \
    else buffered_read(file_record,(data),(bytes)); \
} while (0)
#define ARCHIVE_POSITION(destination) do { \
    if (file_access_statistics) record_file_access(); \
    file_record=file_slots[current_input_file]; \
    destination=(file_record->flags & 8) ? file_record->file_size : \
        ((ArchiveBufferedPosition *)file_record->base)->position; \
} while (0)
/* Original494820: one shared, writable member-header result. */
static IMAGE_ARCHIVE_MEMBER_HEADER archive_header;
extern "C" IMAGE_ARCHIVE_MEMBER_HEADER *__cdecl read_archive_member_header(void)
{
    FileRecord *file_record;
    DWORD position=archive_member_offset+archive_member_size,error=0;
    if (position & 1) ++position;
    seek_file(current_input_file,position,0,&error);
    READ_ARCHIVE(&archive_header,60);
    archive_member_offset=position+60;
    sscanf((const char *)archive_header.Size,"%ld",(long *)&archive_member_size);
    return &archive_header;
}
#pragma intrinsic(memcmp)
void RECON_THISCALL OutputLibrary::ReadIndex(LinkContext *context)
{
    IMAGE_ARCHIVE_MEMBER_HEADER *first,header;
    IMAGE_FILE_HEADER object_header;
    BYTE import_header[20],is_import;
    FileRecord *file_record;
    DWORD count,members,first_offsets_position,next_member_position;
    DWORD position,file_length,bytes,error,i;
    char *name;
    archive_member_offset=8;
    archive_member_size=0;
    first=read_archive_member_header();
    if (memcmp(first->Name,"/               ",16)) {
        if (selected_tool_kind!=2) link_warning(this->name,4003);
        return;
    }
    READ_ARCHIVE(&count,4);
    archive_symbol_count=archive_big_endian_dword(&count);
    count=archive_symbol_count;
    ARCHIVE_POSITION(first_offsets_position);
    next_member_position=archive_member_offset+archive_member_size;
    if (next_member_position & 1) ++next_member_position;
    error=0;
    seek_file(current_input_file,next_member_position,0,&error);
    READ_ARCHIVE(&header,60);
    file_length=get_file_length(current_input_file);
    if (!memcmp(header.Name,"/               ",16)) {
        read_archive_member_header();
        flags |= 1;
        READ_ARCHIVE(&members,4);
        index_data[0]=allocate_memory(members*4+4);
        ARCHIVE_POSITION(position);
        if (file_length<position+members*4) link_fatal(this->name,1127,0);
        READ_ARCHIVE((DWORD *)index_data[0]+1,members*4);
        READ_ARCHIVE(&count,4);
        index_data[1]=allocate_memory(count*2);
        ARCHIVE_POSITION(position);
        if (file_length<position+count*2) link_fatal(this->name,1127,0);
        READ_ARCHIVE(index_data[1],count*2);
        ARCHIVE_POSITION(position);
        bytes=next_member_position-position+archive_member_size+60;
        index_data[2]=allocate_memory(bytes);
        READ_ARCHIVE(index_data[2],bytes);
    } else {
        error=0;
        seek_file(current_input_file,first_offsets_position,0,&error);
        index_data[0]=allocate_memory(count*4+4);
        READ_ARCHIVE(index_data[0],count*4);
        ARCHIVE_POSITION(position);
        bytes=archive_member_size-position+68;
        index_data[2]=allocate_memory(bytes);
        READ_ARCHIVE(index_data[2],bytes);
    }
    index_data[3]=allocate_memory(archive_symbol_count*4);
    name=(char *)index_data[2];
    for (i=0;i<archive_symbol_count;++i) {
        ((char **)index_data[3])[i]=name;
        while (*name++) ;
    }
    position=archive_member_offset+archive_member_size;
    if (position & 1) ++position;
    error=0;
    seek_file(current_input_file,position,0,&error);
    READ_ARCHIVE(&header,60);
    if (!memcmp(header.Name,"//              ",16)) {
        read_archive_member_header();
        name=(char *)allocate_memory(archive_member_size);
        ARCHIVE_POSITION(position);
        if (file_length<position+archive_member_size) link_fatal(this->name,1127,0);
        READ_ARCHIVE(name,archive_member_size);
        index_data[4]=name;
    } else index_data[4]=NULL;
    if (!context || !context->file_header.Machine) return;
    position=archive_member_offset+archive_member_size;
    if (position & 1) ++position;
    error=0;
    seek_file(current_input_file,position,0,&error);
    READ_ARCHIVE(&header,60);
    read_coff_header(current_input_file,&object_header,import_header,&is_import);
    if (is_import) context->CheckMachine(this->name,*(WORD *)(import_header+6));
    else if (object_header.Machine) context->CheckMachine(this->name,object_header.Machine);
}
#undef READ_ARCHIVE
#undef ARCHIVE_POSITION
