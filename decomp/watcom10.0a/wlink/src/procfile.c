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

unsigned short ObjFormat;         /* original441f54 */

void ProcObjFiles( void )
{
    list_of_names *first;

    first = ChkLAlloc( sizeof( list_of_names ) );
    first->name[0] = '\0';
    first->next_name = 0;
    /* The old empty sentinel leaves its retention byte undefined. */
    first->flags = 0;
    SavedNames = first;
    LnkMsg( 0x1003, 0 );
    if( LinkFlags & 0x10000UL ) ObjFormat |= 0x8000;
    LoadObjFiles( Root );
    if( FmtData.type & 0x80 ) OvlPass1();
}

void LoadObjFiles( section *sect )
{
    file_list *list;

    CurrMod = 0;
    CurrSect = sect;
    for( list = sect->files; list != 0; list = list->next_file ) {
        DoPass1( list );
    }
}

void DoPass1( file_list *list )
{
    unsigned long loc;
    unsigned short reclength;
    unsigned char lastmod, library, ignoreobj;
    unsigned char *header;
    member_list *member;
    mod_entry *next;

    loc = 0;
    lastmod = 0;
    if( CacheOpen( list ) ) {
        library = 0;
        header = CacheRead( list, 0, 9 );
        if( header[0] == 0xf0 ) {
            reclength = *(unsigned short *)(header + 1) + 3;
            loc = 9 + CalcAlign( 9, reclength );
            library = 1;
        }
        for(;;) {
            ignoreobj = 0;
            member = 0;
            if( EndOfLib( list, loc ) ) {
                if( list->u.member != 0 ) {
                    LnkMsg( 0x3338, "12", list->file->name,
                            list->u.member->name );
                }
                break;
            }
            IdentifyObject( list, loc );
            if( (list->status & 0x8000) && list->u.member != 0 ) {
                member = GetObjMember( list, &loc );
                if( member == 0 ) ignoreobj = 1;
                else if( list->u.member == 0 ) lastmod = 1;
            }
            if( !ignoreobj ) {
                next = NewModEntry();
                next->next_mod = 0;
                next->f.source = list;
                if( member != 0 ) {
                    next->modinfo |= member->flags;
                    _LnkFree( member );
                }
                if( !(list->status & 0x8000) ) {
                    next->modinfo |= list->status & 0x1f;
                    if( list->status & 0x0200 ) next->modinfo |= 0x8000;
                }
                if( CurrMod == 0 ) CurrSect->mods = next;
                else CurrMod->next_mod = next;
                CurrMod = next;
                next->location = loc;
                loc = ObjPass1();
                if( list->status & 0x0400 ) TraceSymList( CurrMod->publist );
            }
            if( lastmod || CacheEnd( list, loc ) ) break;
            if( library ) loc += CalcAlign( loc, reclength );
        }
        CacheClose( list, 1 );
    }
}

unsigned char EndOfLib( file_list *list, unsigned long loc )
{
    unsigned char *id;

    id = CacheRead( list, loc, 1 );
    return id != 0 && *id == 0xf1;
}

void IdentifyObject( file_list *list, unsigned long loc )
{
    if( IsOMF( list, loc ) ) {
        ObjFormat |= 0x100;
    } else if( IsORL( list, loc ) ) {
        ObjFormat |= 0x200;
    }
}

unsigned long ObjPass1( void )
{
    unsigned long loc;

    SymModStart();
    DBIInitModule( CurrMod );
    if( ObjFormat & 0x100 ) loc = OMFPass1();
    else if( ObjFormat & 0x200 ) loc = ORLPass1();
    else {
        LnkMsg( 0x3392, "s", CurrMod->f.source->file->name );
        return 0;
    }
    SymModEnd();
    DBIP1ModuleScanned();
    if( !(ObjFormat & 0x8000) && (CurrMod->modinfo & 0x4000) ) {
        DBIGenModule( CurrMod );
    }
    ReleaseNames();
    FreeObjInfo();
    return loc;
}

void ResolveUndefined( void )
{
    file_list *lib;
    symbol *sym;
    unsigned type;
    unsigned char keepgoing;

    LnkMsg( 0x1004, 0 );
    if( LinkFlags & 4 ) {
        ObjFormat |= 0x8000;
        InitModTable();
    }
    CurrSect = Root;
    ResolveVFExtdefs();
    do {
        LinkState &= ~4;
        for( lib = ObjLibFiles; lib != 0; lib = lib->next_file ) {
            if( lib->status & 0x4000 ) lib->status |= 0x1000;
            else lib->status |= 0x4000;
        }
        for( sym = HeadSym; sym != 0; sym = sym->link ) {
            type = sym->info & 15;
            if( (!(sym->info & 0x1000) && type < 8 && type > 1)
                 || ((FmtData.type & 0x0100) && type == 6) ) {
                LibFind( sym->name, (sym->info & 0x80) != 0 );
            }
            sym->info |= 0x80;
        }
        keepgoing = ResolveVFExtdefs();
    } while( keepgoing || (LinkState & 4) );
    BurnLibs();
    PrintBadTraces();
}

unsigned char ResolveVFExtdefs( void )
{
    symbol *sym;
    unsigned char resolved;

    resolved = 0;
    for( sym = HeadSym; sym != 0; sym = sym->link ) {
        if( (sym->info & 15) >= 10 ) resolved |= CheckVFList( sym );
    }
    return resolved;
}

member_list *GetObjMember( file_list *list, unsigned long *loc )
{
    member_list *member;

    member = 0;
    if( ObjFormat & 0x100 ) member = GetOMFMember( list, loc );
    else if( ObjFormat & 0x200 ) {
        /* This old build aliases the ORL member helper to the no-input,
         * constant-zero ORLPass1 entry. It remains an unimplemented stub. */
        member = (member_list *)ORLPass1();
    }
    return member;
}
