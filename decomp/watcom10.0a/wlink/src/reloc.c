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

#include "cmdline.h"
#include <recon.h>

static reloc_info *FloatFixups;       /* original4501e8 */

void FreeRelocInfo( void )
{
    group_entry *group;
    unsigned long pages, blocks;
    void **table;

    if( !(LinkState & 1) ) return;
    if( FmtData.type & 0x0016 ) {
        for( group = Groups; group != 0; group = group->next_group ) {
            TraverseOS2RelocList( group, FreeRelocList );
            table = group->grp_relocs;
            if( table != 0 ) {
                pages = (group->totalsize + 4095) >> 12;
                blocks = (pages >> 10) + ((pages & 1023) != 0);
                while( blocks != 0 ) {
                    _LnkFree( *table++ );
                    --blocks;
                }
            }
        }
    } else if( FmtData.type & 0x4209 ) {
        for( group = Groups; group != 0; group = group->next_group ) {
            FreeRelocList( group->grp_relocs );
        }
    } else if( Root != 0 ) {
        FreeRelocSect( Root );
        ProcAllOvl( FreeRelocSect );
    }
    if( FmtData.type & 0x4200 ) {
        FreeRelocList( FloatFixups );
        FreeRelocSect( Root );
    }
}

#pragma pack(1)
struct reloc_info {
    struct reloc_info *next;
    unsigned short sizeleft;
    union { void *addr; unsigned long spill; } loc;
};
#pragma pack()

unsigned char FreeRelocList( reloc_info *list )
{
    while( list != 0 ) {
        if( !(list->sizeleft & 0x8000) ) {
            _LnkFree( list->loc.addr );
        }
        list = list->next;
    }
    return 0;
}

void FreeRelocSect( section *sec )
{
    FreeRelocList( sec->reloclist );
}
