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

#pragma pack(1)
/* Only fields reached by the old cleanup have been identified. */
struct mod_entry {
    unsigned char prefix[14];
    path_entry *publist;
    unsigned short modinfo;
    struct mod_entry *next_mod;
};
#pragma pack()

void CleanLinkStruct( void )
{
    if( Root == 0 ) return;
    if( FakeModule != 0 ) {
        FreeList( FakeModule->publist );
        _LnkFree( FakeModule );
    }
    BurnLibs();
    FreeFiles( ObjLibFiles );
    FreeFiles( Root->files );
    Root->files = 0;
    ObjLibFiles = 0;
    FreeMods( LibModules );
    if( SymFileName != 0 ) _LnkFree( SymFileName );
    if( FmtData.osname != 0 ) _LnkFree( FmtData.osname );
}

void FreeFiles( file_list *list )
{
    file_list *next;

    while( list != 0 ) {
        next = list->next_file;
        CacheClose( list, 3 );
        if( (list->status & 0x80000000UL) && list->u.member != 0 ) {
            FreeList( list->u.member );
        }
        list = next;
    }
}

void FreeMods( mod_entry *head )
{
    mod_entry *next;

    while( head != 0 ) {
        next = head->next_mod;
        FreeAMod( head );
        head = next;
    }
}

void FreeObjInfo( void )
{
    FreeSavedSegs();
    FreeList( ObjInfoList );
    FreeList( AuxObjInfoList );
    FreeNodes( ExtNodes );
    FreeNodes( SegNodes );
    FreeNodes( GrpNodes );
    ObjInfoList = 0;
    AuxObjInfoList = 0;
}

/* Provisional role names: this older saved segment list predates the
 * released source's storage and carve organization. */
static saved_seg *SavedSegs;       /* original44125c */

void FreeSavedSegs( void )
{
    WalkSavedSegs( FreeSavedSeg );
    SavedSegs = 0;
}

struct saved_seg {
    struct saved_seg *next;
};

void WalkSavedSegs( void (*fn)(saved_seg *) )
{
    saved_seg *seg;
    saved_seg *next;

    for( seg = SavedSegs; seg != 0; seg = next ) {
        next = seg->next;
        (*fn)( seg );
    }
}

group_entry *AbsGroups;           /* original45011c */

void FiniLinkStruct( void )
{
    FreeRelocInfo();
    FreeGroups( Groups );
    FreeGroups( AbsGroups );
    Groups = 0;
    AbsGroups = 0;
    FreeList( FreeClassBlocks );
    FreeClassBlocks = 0;
    FreeSections( Root );
    DBICleanup();
    Root = 0;
    if( FmtData.type & 0x00e0 ) FreeOvlStruct();
    BurnNodes();
    FreeTokBuffs();
}

void FreeGroups( group_entry *head )
{
    group_entry *next;

    while( head != 0 ) {
        next = head->next_group;
        FreeAGroup( head );
        head = next;
    }
}

void FreeSections( section *sec )
{
    section *next;

    while( sec != 0 ) {
        FreeFiles( sec->files );
        FreeMods( sec->mods );
        DBISectCleanup( sec );
        FreeAreas( sec->areas );
        next = sec->next_sect;
        _LnkFree( sec );
        sec = next;
    }
}

struct ovl_area {
    struct ovl_area *next;
    section *sections;
};

void FreeAreas( ovl_area *area )
{
    ovl_area *next;

    while( area != 0 ) {
        next = area->next;
        FreeSections( area->sections );
        area = next;
    }
}
