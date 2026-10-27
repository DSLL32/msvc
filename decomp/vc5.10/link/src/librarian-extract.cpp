/* Extract an actual archive member, preserving mapped and buffered copy paths. */
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mbstring.h>
#include "librarian.h"
#include "pass-one.h"
#include "files.h"
#include "input.h"
#include "full-link-functions.h"
#include <recon.h>
extern "C" {
extern const char *default_phase,*output_file_name;
extern int current_input_file,output_file;
extern BYTE file_access_statistics;
char *__stdcall duplicate_argument(const char *);
void __stdcall free_librarian_inputs(LinkPointerList *);
void __cdecl link_fatal(const char *,unsigned,...);
void __cdecl link_warning(const char *,unsigned,...);
}
#define EXTRACT_READ(data,bytes) do { \
    if(file_access_statistics)record_file_access(); \
    file=file_slots[current_input_file]; \
    if(file->flags&8)mapped_read(file,(data),(bytes)); \
    else buffered_read(file,(data),(bytes)); \
} while(0)
#define EXTRACT_WRITE(data,bytes) do { \
    if(file_access_statistics)record_file_access(); \
    file=file_slots[output_file]; \
    if(file->flags&8)mapped_write(file,(data),(bytes)); \
    else buffered_write(file,(data),(bytes)); \
} while(0)
#pragma intrinsic(strcat)
extern "C" void __stdcall extract_librarian_member(LinkContext *context)
{
    const char *saved_phase=default_phase,*member;
    LibraryEnumerator libraries;
    OutputLibrary *library;
    IMAGE_ARCHIVE_MEMBER_HEADER header;
    DWORD length,offset=8,size,remaining,bytes,error;
    FileRecord *file;
    void *mapped;
    char basename[512],extension[256];
    BYTE block[512];
    default_phase="ExtractMember";
    initialize_library_enumerator(&libraries,(OutputLibrary *)context->pointer_list.first);
    next_library(&libraries);
    end_section_enumerator((SectionEnumerator *)&libraries);
    library=libraries.current;
    current_input_file=open_argument_file(library->name,0x8000,0);
    length=(DWORD)get_file_length(current_input_file);
    while(offset<length) {
        error=0;seek_file(current_input_file,offset,FILE_BEGIN,&error);
        EXTRACT_READ(&header,sizeof(header));
        offset+=sizeof(header);
        if(sscanf((const char *)header.Size,"%ld",&size)!=1)
            link_fatal(library->name,1127);
        member=library->MemberName(&header);
        if(!member)link_fatal(library->name,1127);
        if(!_mbsicmp((const unsigned char *)librarian_extract_member,
                     (const unsigned char *)member)) {
            if(!output_file_name) {
                _splitpath(member,NULL,NULL,basename,extension);
                strcat(basename,extension);
                output_file_name=duplicate_argument(basename);
            }
            output_file=open_argument_file(output_file_name,0x8302,0x180);
            mapped=map_file_range(output_file,0,size);
            if(mapped)EXTRACT_READ(mapped,size);
            else {
                remaining=size;
                while(remaining) {
                    bytes=remaining>512?512:remaining;
                    EXTRACT_READ(block,bytes);
                    EXTRACT_WRITE(block,bytes);
                    remaining-=bytes;
                }
            }
            free_librarian_inputs(&context->pointer_list);
            close_argument_file(output_file,1);
            close_argument_file(current_input_file,0);
            exit(0);
        }
        offset+=size;
        if(offset&1)++offset;
    }
    close_argument_file(current_input_file,0);
    link_warning(NULL,4014,librarian_extract_member);
    default_phase=saved_phase;
}
#undef EXTRACT_READ
#undef EXTRACT_WRITE
