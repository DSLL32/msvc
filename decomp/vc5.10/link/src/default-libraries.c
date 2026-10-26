#include <windows.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include "options.h"
#include "memory.h"
#include "pass-one.h"
#include "full-link-functions.h"
#include <recon.h>

extern BYTE verbose_libraries;
BYTE __stdcall library_name_matches(const char *,DefaultLibrary *);
void __stdcall add_default_library(const char *name,LinkPointerList *list)
{
    DefaultLibrary **position;
    DefaultLibrary *library;
    if (list->suppress_default_libraries) return;
    position=&list->default_libraries;
    while (*position) {
        library=*position;
        if (library_name_matches(name,library)) {
            library->flags |= 8;
            return;
        }
        position=&library->next;
    }
    *position=(DefaultLibrary *)allocate_incremental(sizeof(DefaultLibrary));
    (*position)->next=NULL;
    (*position)->name=duplicate_link_string(name);
    (*position)->flags=8;
    (*position)->owner=NULL;
    if (verbose_libraries) link_message(9017,name);
}
#pragma intrinsic(strlen,strcpy)
BYTE __stdcall library_name_matches(const char *name,DefaultLibrary *library)
{
    unsigned length=strlen(name),other_length;
    if (length>=4 && !_stricmp(name+length-4,".lib")) length-=4;
    other_length=strlen(library->name);
    if (other_length>=4 && !_stricmp(library->name+other_length-4,".lib")) other_length-=4;
    return length==other_length && !_strnicmp(name,library->name,length);
}

extern char *repro_directory;
OutputLibrary *__stdcall find_loaded_default_library(const char *name,LinkPointerList *list)
{
    char drive[4],directory[256],filename[256],extension[256],path[260];
    _splitpath(name,drive,directory,filename,extension);
    if (!extension[0]) strcpy(extension,".lib");
    _makepath(path,drive,directory,filename,extension);
    return (OutputLibrary *)find_library((const unsigned char *)path,list->first,
        !drive[0] && !directory[0]);
}
OutputLibrary *__stdcall load_next_default_library(LinkPointerList *list)
{
    DefaultLibrary *item;
    OutputLibrary *library;
    char *name;
    struct _stat information;
    if (list->suppress_default_libraries) return NULL;
    for (item=list->default_libraries;item;item=item->next) {
        if (!(item->flags & 4) && (item->flags & 8) && !(item->flags & 0x20)) {
            item->flags |= 0x20;
            if (!find_loaded_default_library(item->name,list)) break;
        }
    }
    if (!item) return NULL;
    name=canonicalize_argument(item->name,".lib");
    library=create_library(name,NULL,list);
    if (_stat(name,&information)==-1) link_fatal(NULL,1104,name);
    library->input_tag=information.st_mtime;
    library->flags |= 8;
    if (repro_directory) copy_repro_file(name,0);
    return library;
}

void __stdcall disallow_library(const char *name,LinkPointerList *list,void *owner)
{
    DefaultLibrary **position;
    if (list->suppress_default_libraries) return;
    position=&list->default_libraries;
    while (*position) {
        if (library_name_matches(name,*position)) {
            (*position)->flags |= 0x40;
            return;
        }
        position=&(*position)->next;
    }
    *position=(DefaultLibrary *)allocate_incremental(sizeof(DefaultLibrary));
    (*position)->next=NULL;
    (*position)->name=duplicate_link_string(name);
    (*position)->flags=0x40;
    (*position)->owner=owner;
    if (verbose_libraries) link_message(9025,name);
}

void __stdcall exclude_default_library(const char *name,LinkPointerList *list)
{
    DefaultLibrary **position;
    BYTE found=0;
    if(list->suppress_default_libraries)return;
    if(!name) {
        if(verbose_libraries)link_message(9015);
        list->suppress_default_libraries=1;
        return;
    }
    position=&list->default_libraries;
    while(*position) {
        if(library_name_matches(name,*position)) {
            (*position)->flags|=4;
            found=1;
        }
        position=&(*position)->next;
    }
    if(!found) {
        *position=(DefaultLibrary *)allocate_incremental(sizeof(DefaultLibrary));
        (*position)->next=NULL;
        (*position)->name=duplicate_link_string(name);
        (*position)->flags=4;
        (*position)->owner=NULL;
    }
    if(verbose_libraries)link_message(9016,name);
}
