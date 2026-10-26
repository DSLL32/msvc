/* Reusable import-object conversion state and genuine lazy buffer lifetime. */
#include <windows.h>
#include <stdlib.h>
#include <string.h>
#include <new.h>
#include "librarian.h"
#include "librarian-construct.h"
#include "definition-buffer-construct.h"
#include "definition-buffer-set.h"
#include "definition-buffer-cleanup.h"
#include "files.h"
#include "input.h"
#include "memory.h"
#include "full-link-functions.h"
#include <recon.h>

extern "C" {
extern BYTE file_access_statistics;
void __cdecl free_import_payload(void);
}
/* Original47f390 and4803ac: lazily initialized20-byte owner plus guard.
 * Explicit storage keeps the real registered cleanup as its own App entry. */
static union { void *alignment; BYTE bytes[sizeof(DefinitionBuffer)]; } import_payload_storage;
static BYTE import_payload_flags;
/* Original4803c4 and47f248; the file value is consumed only after its writer. */
static BYTE conversion_file_initialized;
static int conversion_file;

extern "C" void __cdecl free_import_payload(void)
{
    DefinitionBuffer &payload=*(DefinitionBuffer *)import_payload_storage.bytes;
    if(payload.data) {
        operator delete(payload.data);
        payload.capacity=0;
    }
}

#pragma intrinsic(strlen)
extern "C" CoffLibrarianContext *__stdcall get_import_conversion(LinkContext *context,
    OutputLibrary *library,int file,CoffImportHeader *import,IMAGE_FILE_HEADER *header,
    const char *member,DWORD is_import)
{
    DefinitionBuffer &payload=*(DefinitionBuffer *)import_payload_storage.bytes;
    DWORD position,error,timestamp,ignored_size,ignored_error;
    void *ignored_view;
    WORD machine;
    FileRecord *record;
    char *name,*extension,*dot;
    char basename[256],suffix[256],temp_path[260],temp_name[260];
    CoffLibrarianContext *conversion;
    if(!(import_payload_flags&1)) {
        import_payload_flags|=1;
        new(import_payload_storage.bytes) DefinitionBuffer;
        atexit(free_import_payload);
    }
    if(file_access_statistics)record_file_access();
    record=file_slots[file];
    position=(record->flags&8)?record->file_size:((DWORD *)record->base)[3];
    if((BYTE)is_import) {
        if(position&1) {
            error=0;
            seek_file(file,(position&~1U)+2,FILE_BEGIN,&error);
        }
        int bytes=(int)import->data_size;
        if(bytes>=0 && (DWORD)payload.end+(DWORD)bytes>(DWORD)payload.data+payload.capacity)
            payload.Grow(bytes);
        if(file_access_statistics)record_file_access();
        record=file_slots[file];
        position=(record->flags&8)?record->file_size:((DWORD *)record->base)[3];
        name=(char *)payload.data;
        if(file_access_statistics)record_file_access();
        record=file_slots[file];
        if(record->flags&8)mapped_read(record,name,bytes);
        else buffered_read(record,name,bytes);
        name+=strlen(name)+1;
        dot=strrchr(name,'.');
        extension=duplicate_link_string(dot);
        *dot=0;
        machine=import->machine;
        timestamp=import->timestamp;
    } else {
        _splitpath(member,NULL,NULL,basename,suffix);
        name=basename;extension=suffix;
        machine=header->Machine;
        timestamp=header->TimeDateStamp;
    }
    if(!find_import_conversion(name,extension,library,&conversion)) {
        if(!conversion_file_initialized) {
            if(!GetTempPathA(260,temp_path) ||
               !GetTempFileNameA(temp_path,name,0,temp_name))link_fatal(NULL,1104,"TEMPFILE");
            ignored_view=NULL;ignored_size=0;
            conversion_file=open_file_view(temp_name,0x8042,0,&ignored_view,&ignored_size,&ignored_error);
            if(conversion_file==-1)link_fatal(NULL,1104,"TEMPFILE");
            error=0;
            seek_file(conversion_file,0,FILE_END,&error);
            conversion_file_initialized=1;
        }
        conversion=new CoffLibrarianContext(conversion_file,name,extension,2,machine,timestamp);
        if(conversion)conversion->image=context;
        library->import_conversions[library->conversion_count]=conversion;
        ++library->conversion_count;
        error=0;
        seek_file(conversion_file,0,FILE_END,&error);
    }
    error=0;
    seek_file(file,position,FILE_BEGIN,&error);
    return conversion;
}

#pragma intrinsic(strcmp)
extern "C" BYTE __stdcall find_import_conversion(const char *base,const char *extension,
    OutputLibrary *library,CoffLibrarianContext **result)
{
    LONG index;
    for(index=0;index<library->conversion_count;++index) {
        CoffLibrarianContext *candidate=library->import_conversions[index];
        if(!strcmp(base,candidate->owned3c) && !strcmp(extension,candidate->owned44)) {
            *result=candidate;
            return 1;
        }
    }
    if(!library->import_conversions) {
        library->conversion_capacity=5;
        library->import_conversions=(CoffLibrarianContext **)allocate_memory(20);
    } else if(library->conversion_count>(LONG)(library->conversion_capacity-1)) {
        library->conversion_capacity*=2;
        library->import_conversions=(CoffLibrarianContext **)reallocate_memory(
            library->import_conversions,library->conversion_capacity*4);
    }
    return 0;
}

/* Separate original47f378/4803b0 owner, not the payload used by lookup. */
static union { void *alignment; BYTE bytes[sizeof(DefinitionBuffer)]; } short_payload_storage;
static BYTE short_payload_flags;
extern "C" void __cdecl free_short_import_payload(void)
{
    DefinitionBuffer &payload=*(DefinitionBuffer *)short_payload_storage.bytes;
    if(payload.data) {
        operator delete(payload.data);
        payload.capacity=0;
    }
}

void RECON_THISCALL CoffLibrarianContext::ConvertShortMember(int file,CoffImportHeader *import,
    DWORD option1,DWORD option2,IMAGE_FILE_HEADER *header,IMAGE_SECTION_HEADER *sections,
    BYTE *section_flags,LibrarianImportInfo *info)
{
    DefinitionBuffer &payload=*(DefinitionBuffer *)short_payload_storage.bytes;
    FileRecord *record;
    int bytes=(int)import->data_size;
    if(!(short_payload_flags&1)) {
        short_payload_flags|=1;
        new(short_payload_storage.bytes) DefinitionBuffer;
        atexit(free_short_import_payload);
    }
    if(bytes>=0 && (DWORD)payload.end+(DWORD)bytes>(DWORD)payload.data+payload.capacity)
        payload.Grow(bytes);
    if(file_access_statistics)record_file_access();
    record=file_slots[file];
    if(record->flags&8)mapped_read(record,payload.data,bytes);
    else buffered_read(record,payload.data,bytes);
    LibrarianContext::AddSymbol(import,(const char *)payload.data,option1,
        (IMAGE_ARCHIVE_MEMBER_HEADER *)option2,header,sections,section_flags,info);
}
