/* Ordinary object archives use the context's symbol table and real input
 * members, separately from the specialized DEF import-library interface. */
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include "librarian.h"
#include "link.h"
#include "files.h"
#include "input.h"
#include "pass-one.h"
#include "full-link-functions.h"
#include <recon.h>

extern "C" {
extern const char *default_phase,*output_file_name;
extern BYTE reproducible_build,file_access_statistics;
extern int output_file,current_input_file;
extern DWORD *librarian_ordinal_symbols;
char *__stdcall format_module_name(OutputModule *,char *);
void __stdcall load_object_module(LinkContext *,void *,OutputModule *,DWORD);
DWORD __stdcall intern_symbol_string(StringTable *,const char *);
DWORD __stdcall build_archive_linker_members(LinkContext *,DWORD,DWORD);
void __stdcall write_converted_archive_member(LinkContext *,OutputModule *,int,DWORD,DWORD,
    const char *,DWORD,DWORD *);
void __stdcall destroy_string_table(StringTable **);
void __stdcall write_archive_header(int,const char *,DWORD,DWORD,DWORD,DWORD,IMAGE_ARCHIVE_MEMBER_HEADER *);
void __stdcall write_archive_member_offsets(int,StringTable *,DWORD);
void __stdcall release_memory(void *);
}

typedef struct ArchiveBuildPosition { DWORD reserved[3],position; } ArchiveBuildPosition;
#define ARCHIVE_WRITE(data,bytes) do { \
 if(file_access_statistics)record_file_access(); \
 file=file_slots[output_file]; \
 if(file->flags&8)mapped_write(file,(data),(bytes)); \
 else buffered_write(file,(data),(bytes)); \
} while(0)
#define ARCHIVE_READ(data,bytes) do { \
 if(file_access_statistics)record_file_access(); \
 file=file_slots[current_input_file]; \
 if(file->flags&8)mapped_read(file,(data),(bytes)); \
 else buffered_read(file,(data),(bytes)); \
} while(0)
#define ARCHIVE_TELL(result) do { \
 if(file_access_statistics)record_file_access(); \
 file=file_slots[output_file]; \
 (result)=(file->flags&8)?file->file_size:((ArchiveBuildPosition *)file->base)->position; \
} while(0)
#define ARCHIVE_SEEK(where,origin) do { error=0;seek_file(output_file,(where),(origin),&error); } while(0)

#pragma intrinsic(strlen)
extern "C" void __stdcall write_librarian_output(LinkContext *context,DWORD temporary)
{
    const char *saved_phase=default_phase;
    StringTable *symbols=context->symbols,*long_names;
    LibraryEnumerator libraries;
    ModuleEnumerator modules;
    OutputModule *module;
    OutputLibrary *library;
    IMAGE_ARCHIVE_MEMBER_HEADER header;
    struct _stat information;
    char long_name[20],copy_buffer[512];
    const char *name,*input_name;
    void *mapped;
    FileRecord *file;
    DWORD timestamp,member_time,member_mode,member_size;
    DWORD count=0,index,second_member,error,position,remaining,bytes;
    BYTE long_reference;

    default_phase="BuildLibrary";
    _tzset();
    timestamp=reproducible_build?(DWORD)-1:(DWORD)time(NULL);
    create_string_table(&long_names,1024,128);
    initialize_contribution_pools();
    initialize_library_enumerator(&libraries,(OutputLibrary *)context->pointer_list.first);
    while(next_library(&libraries)) {
        initialize_module_enumerator(&modules,libraries.current);
        index=count+1;
        while(next_module(&modules)) {
            count=(WORD)count+1;
            if(count>65535)link_fatal(NULL,1189,65535);
            module=modules.current;
            if(strlen(module->name)>15)intern_symbol_string(long_names,module->name);
            current_input_file=open_module_file(module,0x8000,0);
            if(module->library->flags&16)position=0;
            else position=(module->flags&0x40)?
                *(DWORD *)((BYTE *)module->optional_data+12):(DWORD)module->identity;
            error=0;seek_file(current_input_file,position,FILE_BEGIN,&error);
            context->ignore_directives=1;
            load_object_module(context,NULL,module,index);
            context->ignore_directives=0;
            if(!(module->flags&0x40))close_argument_file(current_input_file,0);
            ++index;
        }
    }
    count=(WORD)count;
    if(!count) {
        if((BYTE)temporary)output_file_name=NULL;
        link_fatal(NULL,1160);
    }
    current_phase[0]=0;
    ARCHIVE_WRITE("!<arch>\n",8);
    second_member=build_archive_linker_members(context,timestamp,count);
    if(long_names->hash.strings.used) {
        default_phase="BuildLibrary.EmitLongNames";
        bytes=long_names->hash.strings.used-4;
        write_archive_header(output_file,"/",0,timestamp,0,bytes,NULL);
        ARCHIVE_WRITE(long_names->hash.strings.data+4,bytes);
        if(bytes&1)ARCHIVE_WRITE("\n",1);
    }
    default_phase="BuildLibrary.EmitMember";
    librarian_ordinal_symbols=(DWORD *)zero_allocate(count*4+16);
    index=0;
    initialize_library_enumerator(&libraries,(OutputLibrary *)context->pointer_list.first);
    while(next_library(&libraries)) {
        library=libraries.current;
        initialize_module_enumerator(&modules,library);
        while(next_module(&modules)) {
            module=modules.current;
            format_module_name(module,current_phase);
            name=module->name;
            long_reference=(BYTE)(strlen(name)>15);
            if(long_reference) {
                _ultoa(intern_symbol_string(long_names,name)-4,long_name,10);
                name=long_name;
            }
            input_name=(module->library->flags&16)?
                (const char *)module->identity:module->library->name;
            current_input_file=open_argument_file(input_name,0x8000,0);
            if(!(module->library->flags&16)) {
                error=0;seek_file(current_input_file,(LONG)module->identity-60,FILE_BEGIN,&error);
                ARCHIVE_READ(&header,60);
                if(sscanf((const char *)header.Date,"%ld",&member_time)!=1)member_time=timestamp;
                if(sscanf((const char *)header.Mode,"%ho",&member_mode)!=1)member_mode=0;
                if(sscanf((const char *)header.Size,"%ld",&member_size)!=1)
                    link_fatal(library->name,1127);
            } else {
                input_name=(module->library->flags&16)?
                    (const char *)module->identity:module->library->name;
                if(_stat(input_name,&information)==-1) {
                    member_time=timestamp;member_mode=0;
                } else {
                    member_time=information.st_mtime;member_mode=information.st_mode;
                }
                member_size=get_file_length(current_input_file);
            }
            ARCHIVE_TELL(position);
            librarian_ordinal_symbols[(WORD)index+1]=position;
            if(context->options.librarian_convert && (module->flags&0x40)) {
                write_converted_archive_member(context,module,output_file,member_time,member_mode,
                    name,long_reference,&member_size);
            } else {
                write_archive_header(output_file,name,long_reference,member_time,member_mode,member_size,NULL);
                mapped=map_file_range(output_file,librarian_ordinal_symbols[(WORD)index+1]+60,member_size);
                if(mapped) {
                    ARCHIVE_READ(mapped,member_size);
                    ARCHIVE_SEEK(member_size,FILE_CURRENT);
                } else {
                    remaining=member_size;
                    while(remaining) {
                        bytes=remaining>512?512:remaining;
                        ARCHIVE_READ(copy_buffer,bytes);
                        ARCHIVE_WRITE(copy_buffer,bytes);
                        remaining-=bytes;
                    }
                }
            }
            if(member_size&1)ARCHIVE_WRITE("\n",1);
            close_argument_file(current_input_file,0);
            ++index;
        }
    }
    current_phase[0]=0;
    destroy_string_table(&long_names);
    if(count_hash_symbols(symbols->hash.state)) {
        default_phase="BuildLibrary.EmitOffsets";
        ARCHIVE_SEEK(0x48,FILE_BEGIN);
        write_archive_member_offsets(output_file,symbols,0);
        ARCHIVE_SEEK(second_member+64,FILE_BEGIN);
        ARCHIVE_WRITE(librarian_ordinal_symbols+1,count*4);
        ARCHIVE_SEEK(4,FILE_CURRENT);
        write_archive_member_offsets(output_file,symbols,1);
    }
    release_memory(librarian_ordinal_symbols);
    default_phase=saved_phase;
}

extern "C" {
DWORD __stdcall count_librarian_symbols(StringTable *,DWORD *,DWORD *,DWORD *);
DWORD __stdcall archive_big_endian_count(const DWORD *);
void __stdcall write_archive_symbol_names(int,StringTable *,DWORD);
}

extern "C" DWORD __stdcall build_archive_linker_members(LinkContext *context,DWORD timestamp,DWORD members)
{
    const char *saved_phase=default_phase;
    StringTable *symbols=context->symbols;
    DWORD data_symbols,symbol_count,big_count,error,position,second_member;
    FileRecord *file;
    default_phase="BuildLinkerMember";
    symbol_count=count_librarian_symbols(symbols,&data_symbols,NULL,NULL);
    symbol_count-=data_symbols;
    ARCHIVE_SEEK(60,FILE_CURRENT);
    big_count=archive_big_endian_count(&symbol_count);
    ARCHIVE_WRITE(&big_count,4);
    ARCHIVE_SEEK(symbol_count*4,FILE_CURRENT);
    write_archive_symbol_names(output_file,symbols,0);
    ARCHIVE_TELL(position);
    ARCHIVE_SEEK(8,FILE_BEGIN);
    write_archive_header(output_file,"",0,timestamp,0,position-68,NULL);
    ARCHIVE_SEEK(position,FILE_BEGIN);
    if(position&1)ARCHIVE_WRITE("\n",1);
    ARCHIVE_TELL(second_member);
    ARCHIVE_SEEK(60,FILE_CURRENT);
    ARCHIVE_WRITE(&members,4);
    ARCHIVE_SEEK(members*4,FILE_CURRENT);
    ARCHIVE_WRITE(&symbol_count,4);
    ARCHIVE_SEEK(symbol_count*2,FILE_CURRENT);
    write_archive_symbol_names(output_file,symbols,1);
    ARCHIVE_TELL(position);
    ARCHIVE_SEEK(second_member,FILE_BEGIN);
    write_archive_header(output_file,"",0,timestamp,0,position-second_member-60,NULL);
    ARCHIVE_SEEK(position,FILE_BEGIN);
    if(position&1)ARCHIVE_WRITE("\n",1);
    default_phase=saved_phase;
    return second_member;
}

extern "C" void __stdcall convert_library_member(LinkContext *,OutputLibrary *,IMAGE_FILE_HEADER *,
    void *,int,DWORD,DWORD,const char *,const char *,IMAGE_SECTION_HEADER *,BYTE *,
    void *,void **,DWORD,DWORD);

#pragma intrinsic(strcpy)
extern "C" void __stdcall load_librarian_library(LinkContext *context,const char *name)
{
    const char *saved_phase=default_phase;
    OutputLibrary *library;
    IMAGE_ARCHIVE_MEMBER_HEADER member_header;
    IMAGE_FILE_HEADER header;
    IMAGE_SECTION_HEADER sections[30];
    BYTE section_flags[32],import_header[20],section_data[60],is_import,converted;
    DWORD position=8,length,member_size,error,format;
    char *member_name;
    void *conversion;
    int saved_input;
    BYTE copy_sections,restore_input;
    FileRecord *file;

    default_phase="AddArchiveArg";
    library=create_library(name,NULL,&context->pointer_list);
#ifdef __linux__
    if(strlen(library->name)>=sizeof(current_phase)) { __asm { int 3 } }
#endif
    strcpy(current_phase,library->name);
    current_input_file=open_argument_file(library->name,0x8000,0);
    library->ReadIndex(context);
    length=get_file_length(current_input_file);
    while(position<length) {
        error=0;seek_file(current_input_file,position,FILE_BEGIN,&error);
        ARCHIVE_READ(&member_header,60);
        position+=60;
        if(sscanf((const char *)member_header.Size,"%ld",&member_size)!=1)
            link_fatal(library->name,1127);
        if(strncmp((const char *)member_header.Name,"/               ",16) &&
           strncmp((const char *)member_header.Name,"//              ",16)) {
            const char *resolved=library->MemberName(&member_header);
            if(!resolved)link_fatal(name,1127);
            member_name=duplicate_argument(resolved);
            conversion=NULL;
            copy_sections=restore_input=0;
            format=20;
            is_import=0;
            saved_input=0;
            read_coff_header(current_input_file,&header,import_header,&is_import);
            if(is_import) {
                convert_library_member(context,library,&header,import_header,current_input_file,
                    0,1,NULL,NULL,sections,section_flags,section_data,&conversion,0,0);
                saved_input=current_input_file;
                restore_input=copy_sections=1;
                format=0;
                current_input_file=*(int *)((BYTE *)conversion+0x5c);
            }
            create_input_module(NULL,member_name,(const void *)position,&header,library,
                NULL,copy_sections,format,sections,section_flags,section_data,(DWORD)conversion);
            if(restore_input)current_input_file=saved_input;
        }
        position+=member_size;
        if(position&1)++position;
    }
    close_argument_file(current_input_file,0);
    default_phase=saved_phase;
}
