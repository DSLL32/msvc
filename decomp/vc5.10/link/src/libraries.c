#include <windows.h>
#include <stdlib.h>
#include <string.h>
#include <mbstring.h>
#include "options.h"
#include "pass-one.h"
#include "sections.h"
#include "full-link-functions.h"
#include <recon.h>

#pragma intrinsic(strlen,strcpy,strcat)
void *__stdcall find_library(const unsigned char *name,void *list,BYTE before)
{
    LibraryEnumerator enumerator;
    OutputLibrary *found=NULL;
    const unsigned char *candidate;
    char basename[260],filename[256],suffix[256];
    initialize_library_enumerator(&enumerator,(OutputLibrary *)list);
    while (next_library(&enumerator)) {
        candidate=(const unsigned char *)enumerator.current->name;
        if (before && candidate) {
            _splitpath((const char *)candidate,NULL,NULL,filename,suffix);
            strcpy(basename,filename);
            strcat(basename,suffix);
            candidate=(const unsigned char *)basename;
        }
        if ((!name && !enumerator.current->name) ||
            (enumerator.current->name && name && !_mbsicmp(candidate,name))) {
            found=enumerator.current;
            end_section_enumerator((SectionEnumerator *)&enumerator);
            break;
        }
    }
    return found;
}

void __stdcall initialize_library_enumerator(LibraryEnumerator *enumerator,OutputLibrary *first)
{
    enumerator->next=next_library;
    enumerator->end=(void (__stdcall *)(LibraryEnumerator *))end_section_enumerator;
    enumerator->current=NULL;
    enumerator->first=first;
}

BYTE __stdcall next_library(LibraryEnumerator *enumerator)
{
    if (!enumerator->current) enumerator->current=enumerator->first;
    else enumerator->current=enumerator->current->next;
    return enumerator->current!=NULL;
}
