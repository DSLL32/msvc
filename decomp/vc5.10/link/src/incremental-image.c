/* The old incremental database is validated before it can be reused. */
#include <windows.h>
#include <sys/stat.h>
#include <string.h>
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
