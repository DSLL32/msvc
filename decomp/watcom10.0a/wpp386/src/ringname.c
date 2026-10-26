/****************************************************************************
*
*                            Open Watcom Project
*
*    Portions Copyright (c) 1983-2002 Sybase, Inc. All Rights Reserved.
*
*  ========================================================================
*
*    This file contains Original Code and/or Modifications of Original
*    Code as defined in and that are subject to the Sybase Open Watcom
*    Public License version 1.0 (the 'License'). You may not use this file
*    except in compliance with the License. BY USING THIS FILE YOU AGREE TO
*    ALL TERMS AND CONDITIONS OF THE LICENSE. A copy of the License is
*    provided with the Original Code and Modifications, and is also
*    available at www.sybase.com/developer/opensource.
*
*    The Original Code and all software distributed under the License are
*    distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
*    EXPRESS OR IMPLIED, AND SYBASE AND ALL CONTRIBUTORS HEREBY DISCLAIM
*    ALL SUCH WARRANTIES, INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF
*    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR
*    NON-INFRINGEMENT. Please see the License for the specific language
*    governing rights and limitations under the License.
*
*  ========================================================================
*
* Description:  WHEN YOU FIGURE OUT WHAT THIS FILE DOES, PLEASE
*               DESCRIBE IT HERE!
*
****************************************************************************/

/* bld/plusplus/c/ringname.c at initial commit
 *6b9cb4438959d845a2b4dd4bf19f7908ecd608e9; actual body7B. */
#include <string.h>
#include "compiler.h"
#include "utility.h"
#include <recon.h>

void RingNameInit(RINGNAMECTL *control)
{
    control->ring = 0;
}

/* The1994 allocator returns the node, unlike the later source's char*. */
RINGNAME *RingNameAllocVct(RINGNAMECTL *control, const char *vector, unsigned length)
{
    RINGNAME *entry;
    entry=(RINGNAME *)RingAlloc((void **)&control->ring,5+length);
    stvcpy(entry->name,vector,length);
    if (!control->case_sensitive) strlwr(entry->name);
    return entry;
}

RINGNAME *RingNameAllocStr(RINGNAMECTL *control, const char *name)
{
    return RingNameAllocVct(control,name,strlen(name));
}

RINGNAME *RingNameLookup(RINGNAMECTL *control, const char *name)
{
    RINGNAME *entry, *last;
    if (control->case_sensitive) {
        last=control->ring;
        if (last) {
            entry=last;
            do {
                entry=entry->next;
                if (strcmp(entry->name,name)==0) return entry;
            } while (entry!=last);
        }
    } else {
        last=control->ring;
        if (last) {
            entry=last;
            do {
                entry=entry->next;
                if (stricmp(entry->name,name)==0) return entry;
            } while (entry!=last);
        }
    }
    return 0;
}

void RingNameFree(RINGNAMECTL *control)
{
    RingFree((void **)&control->ring);
}

unsigned RingNameLookupPosition(RINGNAMECTL *control, const char *name)
{
    RINGNAME *entry, *last;
    unsigned position=1;
    last=control->ring;
    if (control->case_sensitive) {
        if (last) {
            entry=last;
            do {
                entry=entry->next;
                if (strcmp(entry->name,name)==0) return position;
                ++position;
            } while (entry!=last);
        }
    } else {
        if (last) {
            entry=last;
            do {
                entry=entry->next;
                if (stricmp(entry->name,name)==0) return position;
                ++position;
            } while (entry!=last);
        }
    }
    return 0;
}
