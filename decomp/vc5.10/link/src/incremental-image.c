/* The old incremental database is validated before it can be reused. */
#include <windows.h>
#include <sys/stat.h>
#include <string.h>
#include <stdlib.h>
#include <io.h>
#include "full-link-functions.h"
#include "link.h"
#include "options.h"
#include "memory.h"
#include "pass-one.h"
#include <recon.h>

extern DWORD maximum_physical_memory;
extern BYTE file_access_statistics,incremental_enabled,incremental_failed;
int incremental_heap_file;
BYTE __stdcall valid_incremental_pointer(const void *base,DWORD size,
    const void *pointer,DWORD bytes)
{ __asm { int 3 } return 0; }

#pragma intrinsic(strcmp)
BYTE __stdcall read_old_image(const char *name,BYTE warn,LinkContext *image,
                              struct _stat *information)
{
    FileRecord *record;
    unsigned bytes;
    if(_stat(name,information))return 0;
    image->options.option142=0;
    if((DWORD)information->st_size<sizeof(*image) ||
       (DWORD)information->st_size>maximum_physical_memory) {
        if(warn) {
            if(information->st_size>0) {
                link_warning(NULL,4076,name);
                incremental_enabled=0;
            } else incremental_enabled=1;
        }
        return 0;
    }
    if((information->st_mode & 0x180)!=0x180) {
        if(warn) {
            link_warning(NULL,4079,name);
            incremental_enabled=0;
        }
        return 0;
    }
    incremental_heap_file=open_argument_file(name,0x8002,0);
    if(file_access_statistics)record_file_access();
    record=file_slots[incremental_heap_file];
    bytes=(record->flags & 8) ? mapped_read(record,image,sizeof(*image)) :
        buffered_read(record,image,sizeof(*image));
    close_argument_file(incremental_heap_file,1);
    if(bytes!=sizeof(*image)) {
        if(warn) {
            link_warning(NULL,4076,name);
            incremental_enabled=0;
        }
        return 0;
    }
    incremental_heap_file=0;
    if(strcmp(image->database_signature,"Microsoft Linker Database\n\a\x1a")) {
        if(warn) {
            link_warning(NULL,4076,name);
            incremental_enabled=0;
        }
        return 0;
    }
    if(!warn)return 1;
    if(image->database_type!=6) {
        if(incremental_failed)link_error(NULL,6020);
        return 0;
    }
    if(!valid_incremental_pointer(image->self,information->st_size,image->sections.first,72) ||
       !valid_incremental_pointer(image->self,information->st_size,image->pointer_list.first,68) ||
       !valid_incremental_pointer(image->self,information->st_size,image->command_line_library,68) ||
       !valid_incremental_pointer(image->self,information->st_size,image->symbols,40)) {
        if(incremental_failed)link_error(NULL,6002);
        return 0;
    }
    return 1;
}

/* Actual typed call boundaries; unreconstructed children remain named traps. */
void __stdcall initialize_incremental_database(LinkContext **);
BYTE __stdcall incremental_output_unchanged(const char *name,const LinkContext *old)
{ __asm { int 3 } return 0; }
void __stdcall map_incremental_database(LinkContext **context,void *base,DWORD bytes)
{ __asm { int 3 } }
void __stdcall restore_incremental_tables(LinkContext **context)
{ __asm { int 3 } }
extern char *old_image_name;
extern const char *output_file_name,*pdb_file_name;
extern BYTE incremental_reuse;
void __cdecl link_system_error(const char *file,unsigned message,...)
{ __asm { int 3 } }
void __stdcall load_incremental_state(LinkContext **context)
{
    LinkContext old;
    struct _stat information;
    HANDLE file;
    char *name;
    old_image_name=derive_old_image_name(*context);
    if(!old_image_name) {
        incremental_enabled=0;
        return;
    }
    if(!read_old_image(old_image_name,1,&old,&information)) {
        if(incremental_enabled)initialize_incremental_database(context);
        return;
    }
    if(!incremental_output_unchanged(output_file_name,&old)) {
        link_error(NULL,6004,output_file_name);
        (*context)->options.option142=0;
        initialize_incremental_database(context);
        return;
    }
    file=CreateFileA(output_file_name,GENERIC_READ|GENERIC_WRITE,0,NULL,
        OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
    if(file==INVALID_HANDLE_VALUE) {
        link_system_error(NULL,1168,output_file_name);
        exit(1168);
    } else CloseHandle(file);
    if(old.options.debug_level) {
        name=derive_pdb_name(output_file_name,pdb_file_name);
        if(_access(name,0)==-1) {
            link_error(NULL,6006,name);
            release_memory(name);
            initialize_incremental_database(context);
            return;
        }
        if(_access(name,2)==-1) {
            link_system_error(NULL,1168,name);
            exit(1168);
        }
        pdb_file_name=name;
    }
    map_incremental_database(context,old.self,(DWORD)information.st_size);
    if(incremental_reuse)restore_incremental_tables(context);
}

int __cdecl incremental_mapping_may_grow(void)
{
    DWORD version=GetVersion();
    return (version&0x80000000) && (BYTE)version>=4;
}
void *__stdcall next_legacy_mapping_address(void *previous)
{ __asm { int 3 } return NULL; }
void *__stdcall next_nt_mapping_address(void *previous)
{
    if(!previous)return (void *)0x3fff0000;
    if((DWORD)previous<0x7fffffff)
        return (void *)((DWORD)previous+maximum_physical_memory);
    return (void *)-1;
}
void *__stdcall next_incremental_mapping_address(BYTE legacy,void *previous)
{
    if(legacy)return next_legacy_mapping_address(previous);
    return next_nt_mapping_address(previous);
}
extern BYTE *incremental_allocation_cursor;
extern unsigned incremental_allocation_remaining;
/* Original4944ac is the mapping base, distinct from the4944a8 cursor. */
BYTE *incremental_heap_base;
BYTE __stdcall reserve_incremental_address(void *base,DWORD bytes,DWORD *error)
{ __asm { int 3 } return 0; }
void __stdcall close_incremental_mapping_file(int file)
{ __asm { int 3 } }
/* Preferred-address mapping differs between the reference and native host.
 * The whole draft is retained in scratch; this owner remains unaccepted. */
void *__stdcall open_incremental_heap(void *base,DWORD bytes,BYTE create,DWORD *error)
{ __asm { int 3 } return (void *)-1; }
BYTE __stdcall incremental_disk_space(DWORD bytes)
{ __asm { int 3 } return 0; }
void __stdcall copy_link_configuration(LinkContext *destination,const LinkContext *source)
{ __asm { int 3 } }
void __stdcall release_link_context(LinkContext **context,BYTE incremental)
{ __asm { int 3 } }
