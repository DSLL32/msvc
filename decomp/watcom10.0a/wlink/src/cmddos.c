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

#include <string.h>
#include "cmdline.h"
#include <recon.h>

#ifdef __WATCOMC__
#pragma intrinsic(memcpy)
#endif
extern unsigned char OvlLevel;
extern list_of_names *OvlClasses;             /* original4500f8 */
extern parse_entry Directives[];
static parse_entry SectOptions[] = {
    { "INto", ProcInto, 0x0080 },
    { 0, 0, 0 }
};
static parse_entry Sections[] = {
    { "Section", ProcSection, 0x0080 },
    { "AUTOSection", ProcAutoSection, 0x0080 },
    { "End", ProcEnd, 0x0080 },
    { 0, 0, 0 }
};
static parse_entry DosOptions[] = {
    { "COM", ProcCom, 0x0040 },
    { 0, 0, 0 }
};

unsigned char ProcDos( void )
{
    OvlLevel = 0;
    ProcOne( DosOptions, SEP_NO, 0 );
    return 1;
}

unsigned char ProcStandard( void )
{
    LinkFlags &= ~0x0400UL;
    return 1;
}
unsigned char ProcDistribute( void )
{
    LinkFlags |= 4;
    return 1;
}
unsigned char ProcPadSections( void )
{
    LinkFlags |= 0x8000UL;
    return 1;
}
unsigned char ProcDosSeg( void )
{
    LinkFlags |= 0x10;
    return 1;
}

unsigned char AddClass( void )
{
    list_of_names *ovlclass;

    ovlclass = PermAlloc( sizeof(*ovlclass) + Token.len );
    memcpy( ovlclass->name, Token.this, Token.len );
    ovlclass->name[Token.len] = '\0';
    ovlclass->flags = IsCodeClass( ovlclass->name, Token.len );
    ovlclass->next_name = OvlClasses;
    OvlClasses = ovlclass;
    return 1;
}
unsigned char ProcOverlay( void )
{
    return ProcArgList( AddClass, 1 );
}

unsigned char ProcBegin( void )
{
    section *oldsect, *sect;
    file_list **oldflist;

    LinkState |= 0x20;
    if( OvlLevel != 0 && (LinkFlags & 0x0400) ) {
        CmdFlags &= ~0x0100;
    } else {
        oldsect = CurrSect;
        oldflist = CurrFList;
        sect = NewSection();
        if( LinkFlags & 0x4021 ) DBISectInit( sect );
        NewArea( sect );
        CurrSect = sect;
        CurrFList = &sect->files;
        sect->relocs = 1;
    }
    ++OvlLevel;
    while( ProcOne( Sections, SEP_NO, 0 ) ) {}
    if( OvlLevel == 0 || !(LinkFlags & 0x0400) ) {
        CurrFList = oldflist;
        CurrSect = oldsect;
    }
    return 1;
}

void NewArea( section *sect )
{
    ovl_area *ovl, **owner;

    ovl = PermAlloc( sizeof(*ovl) );
    ovl->next = 0;
    ovl->sections = sect;
    sect->parent = CurrSect;
    owner = (ovl_area **)&CurrSect->areas;
    while( *owner != 0 ) owner = &(*owner)->next;
    *owner = ovl;
}

unsigned char ProcSection( void )
{
    if( OvlLevel == 0 ) {
        LnkMsg( 0x7a48, 0 );
    } else {
        MakeNewSection();
        ProcOne( SectOptions, SEP_NO, 0 );
        while( ProcOne( Directives, SEP_NO, 0 ) ) RestoreParser();
    }
    return 1;
}

void MakeNewSection( void )
{
    section *sect;

    if( CurrSect->relocs != 1 ) {
        sect = NewSection();
        if( LinkFlags & 0x4021 ) DBISectInit( sect );
        if( CmdFlags & 0x0100 ) sect->outfile = CurrSect->outfile;
        sect->parent = CurrSect->parent;
        CurrSect->next_sect = sect;
        CurrSect = sect;
        CurrFList = &sect->files;
    } else {
        CurrSect->relocs = 0;
    }
}
unsigned char ProcEnd( void )
{
    if( OvlLevel != 0 ) --OvlLevel;
    if( CurrSect->relocs != 0 ) CurrSect->relocs = 0;
    return 0;
}

void CmdOvlFini( void )
{
    ovl_area *area;

    if( OvlLevel != 0 ) {
        Ignite();
        LnkMsg( 0x7c4b, 0 );
    }
    if( LinkFlags & 0x0400 ) {
        area = Root->areas;
        if( area == 0 || area->next != 0 ) {
            Ignite();
            LnkMsg( 0x7c87, 0 );
        }
    }
    SetOvlClasses();
    MakeNonArea();
}

void SetOvlClasses( void )
{
    list_of_names *ovlmgr;

    if( OvlClasses != 0 ) {
        ovlmgr = PermAlloc( sizeof(*ovlmgr) + 8 );
        memcpy( ovlmgr->name, "_OVLCODE", 9 );
        ovlmgr->next_name = OvlClasses;
        OvlClasses = ovlmgr;
    }
}

void MakeNonArea( void )
{
    NonSect = NewSection();
    NewArea( NonSect );
}
