#include <windows.h>
#include <string.h>
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
#pragma intrinsic(strlen)
BYTE __stdcall library_name_matches(const char *name,DefaultLibrary *library)
{
    unsigned length=strlen(name),other_length;
    if (length>=4 && !_stricmp(name+length-4,".lib")) length-=4;
    other_length=strlen(library->name);
    if (other_length>=4 && !_stricmp(library->name+other_length-4,".lib")) other_length-=4;
    return length==other_length && !_strnicmp(name,library->name,length);
}
