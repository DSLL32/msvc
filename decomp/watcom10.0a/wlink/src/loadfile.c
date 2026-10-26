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

void DefBSSSyms( void )
{
    DefABSSSym( "_edata" );
    DefABSSSym( "__edata" );
    DefABSSSym( "_end" );
    DefABSSSym( "__end" );
}

void DefABSSSym( char *name )
{
    symbol *sym;

    sym = RefISymbol( name );
    if( !(sym->info & 0x1000) ) {
        sym->info |= 0x1000;
        if( FmtData.type & 0x0080 ) sym->u.value |= 0x30000000UL;
        sym->addr.seg = 0xffff;
        sym->addr.off = 0;
    }
}

seg_leader *StackSegPtr;       /* original450194 */

void SetStkSize( void )
{
    StackSegPtr = StackSegment();
    if( StackSize < 0x200 ) StackSize = 0x200;
    if( StackSegPtr != 0 ) {
        if( LinkFlags & 2 ) {
            if( !(FmtData.type & 0x0100) ) StackSegPtr->saved_size = StackSize;
        } else if( StackSegPtr->saved_size >= 0x200 ) {
            StackSize = StackSegPtr->saved_size;
        } else {
            StackSegPtr->saved_size = StackSize;
        }
    }
}

seg_leader *StackSegment( void )
{
    seg_leader *seg;

    seg = FindStack( Root->classlist );
    if( seg == 0 && (FmtData.type & 0x0080) ) {
        seg = FindStack( NonSect->classlist );
    }
    return seg;
}

seg_leader *FindStack( class_entry *class )
{
    seg_leader *seg;

    for( ; class != 0; class = class->next_class ) {
        for( seg = class->segs; seg != 0; seg = seg->next_seg ) {
            if( (seg->info & 7) == 5 ) return seg;
        }
    }
    return 0;
}

extern class_entry *LocateBSSClass( void );
extern void DefBSSStartSize( char *, seg_leader * );
extern void DefBSSEndSize( char *, seg_leader * );

void GetBSSSize( void )
{
    class_entry *class;
    seg_leader *seg;

    class = LocateBSSClass();
    if( class != 0 ) {
        seg = class->segs;
        DefBSSStartSize( "_edata", seg );
        DefBSSStartSize( "__edata", seg );
        DefBSSEndSize( "_end", seg );
        DefBSSEndSize( "__end", seg );
    }
}

class_entry *LocateBSSClass( void )
{
    class_entry *class;

    class = (Root->areas == 0 ? Root : NonSect)->classlist;
    for( ; class != 0; class = class->next_class ) {
        if( stricmp( class->name, "BSS" ) == 0 ) return class;
    }
    return 0;
}

extern symbol *FindISymbol( char * );
extern void ConvertToFrame( address *, unsigned short );
extern void CheckBSSInStart( symbol *, char * );

void DefBSSStartSize( char *name, seg_leader *seg )
{
    symbol *sym;

    sym = FindISymbol( name );
    if( sym->addr.seg == 0xffff ) {
        sym->p.seg = seg;
        sym->addr.off = seg->seg_addr.off;
        sym->addr.seg = seg->seg_addr.seg;
        ConvertToFrame( (address *)&sym->addr, seg->group->grp_addr.seg );
    } else if( LinkFlags & 0x10 ) CheckBSSInStart( sym, name );
}

void DefBSSEndSize( char *name, seg_leader *seg )
{
    symbol *sym;

    sym = FindISymbol( name );
    if( sym->addr.seg == 0xffff ) {
        while( seg->next_seg != 0 ) seg = seg->next_seg;
        sym->p.seg = seg;
        sym->addr.seg = seg->seg_addr.seg;
        sym->addr.off = seg->seg_addr.off + seg->saved_size;
        ConvertToFrame( (address *)&sym->addr, seg->group->grp_addr.seg );
    } else if( LinkFlags & 0x10 ) CheckBSSInStart( sym, name );
}

address StackAddr;                  /* original450180 */
extern void PhoneyStack( void );

void GetStkAddr( void )
{
    if( !(FmtData.type & 0x0100) && !(FmtData.flags & 1) ) {
        if( StackSegPtr != 0 ) {
            StackAddr.seg = StackSegPtr->seg_addr.seg;
            StackAddr.off = StackSegPtr->seg_addr.off + StackSegPtr->saved_size;
        } else if( (FmtData.type & 8) && (LinkFlags & 2) ) PhoneyStack();
        else if( !(FmtData.type & 0x4250) ) {
            LnkMsg( 0x320e, 0 );
            StackAddr.seg = 0;
            StackAddr.off = 0;
        }
    }
}

void InitLoadFile( void )
{
    LnkMsg( 0x1006, 0 );
}

extern int ExeCreate( char *name );
extern void FiniDOSLoadFile( void );
extern void FiniOS2FlatLoadFile( void );
extern void FiniPELoadFile( void );
extern void FiniOS2LoadFile( void );
extern void FiniPharLapLoadFile( void );
extern void FiniNovellLoadFile( void );
extern void FiniQNXLoadFile( void );
extern void FiniELFLoadFile( void );
extern void MapSizes( void );

void FiniLoadFile( void )
{
    outfilelist *file;

    Root->outfile->handle = ExeCreate( Root->outfile->fname );
    if( Root->outfile->handle == -1 ) {
        PrintIOError( 0x345d, "s", Root->outfile->fname );
    }
    for( file = OutFiles->next; file != 0; file = file->next ) {
        file->handle = ExeCreate( file->fname );
    }
    if( FmtData.type & 0xe0 ) FiniDOSLoadFile();
    else if( FmtData.type & 6 ) FiniOS2FlatLoadFile();
    else if( FmtData.type & 0x10 ) FiniPELoadFile();
    else if( FmtData.type & 9 ) FiniOS2LoadFile();
    else if( FmtData.type & 0x3c00 ) FiniPharLapLoadFile();
    else if( FmtData.type & 0x100 ) FiniNovellLoadFile();
    else if( FmtData.type & 0x4200 ) FiniQNXLoadFile();
    else if( FmtData.type & 0x8000 ) FiniELFLoadFile();
    MapSizes();
}

void WriteLoad( void *buffer, unsigned short size )
{
    QWrite( Root->outfile->handle, buffer, size, Root->outfile->fname );
}

extern void WriteNulls( int, unsigned long, char * );

void PadLoad( unsigned long size )
{
    WriteNulls( CurrSect->outfile->handle, size, CurrSect->outfile->fname );
}

void WriteNulls( int file, unsigned long size, char *name )
{
    static char zeros[16];

    while( size > sizeof( zeros ) ) {
        size -= sizeof( zeros );
        QWrite( file, zeros, sizeof( zeros ), name );
    }
    if( size != 0 ) QWrite( file, zeros, size, name );
}

unsigned long NullAlign( unsigned long align )
{
    unsigned long off, pad;

    --align;
    off = QPos( Root->outfile->handle );
    pad = ((off + align) & ~align) - off;
    if( pad != 0 ) PadLoad( pad );
    return off + pad;
}

char *RemovePath( char *name, int *len )
{
    char *base, *dot, *p;

    base = name;
    dot = 0;
    for( p = name; *p != 0; ++p ) {
        if( *p == '.' ) dot = p;
        if( *p == '\\' || *p == '/' || *p == ':' ) {
            base = p + 1;
            dot = 0;
        }
    }
    *len = dot != 0 ? dot - base : p - base;
    return base;
}

void OrderGroups( unsigned char (*cmp)(address *,address *) )
{
    group_entry *group, *lowest, *first, **last;
    address *low_addr, *addr;

    first = Groups;
    last = &Groups;
    while( first != 0 ) {
        low_addr = (address *)&first->grp_addr;
        lowest = 0;
        for( group = first; group->next_group != 0; group = group->next_group ) {
            addr = (address *)&group->next_group->grp_addr;
            if( cmp( addr, low_addr ) ) {
                low_addr = addr;
                lowest = group;
            }
        }
        if( lowest == 0 ) {
            *last = first;
            last = &first->next_group;
            first = *last;
        } else {
            *last = lowest->next_group;
            last = &lowest->next_group->next_group;
            lowest->next_group = *last;
        }
    }
}

unsigned char CompareDosSegments( address *left, address *right )
{
    unsigned long diff;

    diff = left->off - right->off + ((unsigned long)(left->seg - right->seg) << 4);
    return (long)diff < 0;
}

extern unsigned long OvlTabOffset;
extern void PadLoad( unsigned long );
unsigned char WriteDOSGroup( group_entry *group )
{
    unsigned long loc;
    long diff;
    section *sect;
    outfilelist *file;
    unsigned char repos;

    repos = 0;
    if( group->size != 0 ) {
        sect = group->section;
        file = sect->outfile;
        loc = group->grp_addr.off - sect->sect_addr.off
            + ((unsigned long)(group->grp_addr.seg - sect->sect_addr.seg) << 4)
            + sect->u.file_loc;
        diff = (long)(loc - file->file_loc);
        if( diff > 0 ) PadLoad( diff );
        else if( diff != 0 ) {
            QSeek( file->handle, loc, file->fname );
            repos = 1;
        }
        if( group == OvlGroup ) OvlTabOffset = loc;
        QWrite( CurrSect->outfile->handle, (void *)group->data, group->size, CurrSect->outfile->fname );
        loc += group->size;
        if( loc > file->file_loc ) file->file_loc = loc;
    }
    return repos;
}

unsigned char CompareProtSegments( address *left, address *right )
{
    if( left->seg == right->seg ) return left->off < right->off;
    return left->seg < right->seg;
}

unsigned long CopyToLoad( int handle, char *name )
{
    unsigned long amount, wrote;
    wrote = 0;
    for( ;; ) {
        amount = QRead( handle, TokBuff, TokSize, name );
        if( amount == 0 ) break;
        WriteLoad( TokBuff, amount );
        wrote += amount;
    }
    QClose( handle, name );
    return wrote;
}

unsigned long PadToAlign( unsigned long offset, unsigned long alignment )
{
    unsigned long pad;
    --alignment;
    pad = ((offset + alignment) & ~alignment) - offset;
    if( pad != 0 ) PadLoad( pad );
    return offset + pad;
}
