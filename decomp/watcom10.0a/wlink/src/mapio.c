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

#include <time.h>
#include <string.h>
#include "diagnostic.h"
#include <recon.h>

#ifdef __WATCOMC__
#pragma intrinsic(strlen,memcpy,strcpy)
#endif

time_t StartT;                   /* original4501a0 */

void StartTime( void )
{
    StartT = time( 0 );
}

/* Old map symbol-record list; newer releases split undefined/trace rings. */
typedef struct symrecinfo {
    struct symrecinfo *next;
    symbol *sym;
    mod_entry *mod;
} symrecinfo;
static symrecinfo *UndefList;    /* original441e9c */

void FreeUndefs( void )
{
    FreeList( (path_entry *)UndefList );
}

static unsigned char Absolute_Seg; /* original441e98 */
static unsigned char Buffering;    /* original441e99 */
static unsigned BufferSize;       /* original450198 */
unsigned MapCol;                   /* original45019c */

void MapInit( void )
{
    char tim[9], dat[9];
    char *ptr, *msg;
    struct tm *localt;

    Absolute_Seg = 0;
    Buffering = 0;
    if( !(MapFlags & 1) ) return;
    MapFile = QOpenRW( MapFName );
    StartMapBuffering();
    localt = localtime( &StartT );
    MapCol = 0;
    msg = "WATCOM Linker Version 10.0";
    BufWrite( msg, strlen(msg) );
    WriteMapNL( 1 );
    msg = "Copyright by WATCOM International Corp. 1985, 1994. All rights reserved.";
    BufWrite( msg, strlen(msg) );
    WriteMapNL( 1 );
    ptr = PutDec( tim, localt->tm_hour );
    *ptr++ = ':';
    ptr = PutDec( ptr, localt->tm_min );
    *ptr++ = ':';
    ptr = PutDec( ptr, localt->tm_sec );
    *ptr = '\0';
    ptr = PutDec( dat, localt->tm_year );
    *ptr++ = '/';
    ptr = PutDec( ptr, localt->tm_mon + 1 );
    *ptr++ = '/';
    ptr = PutDec( ptr, localt->tm_mday );
    *ptr = '\0';
    LnkMsg( 0x2024, "12", dat, tim );
    StopMapBuffering();
}

void StartMapBuffering( void )
{
    Buffering = 1;
    BufferSize = 0;
}

void BufWrite( char *buffer, int len )
{
    int diff;

    if( Buffering ) {
        diff = BufferSize + len - TokSize;
        if( diff >= 0 ) {
            memcpy( TokBuff + BufferSize, buffer, len - diff );
            QWrite( MapFile, TokBuff, TokSize, MapFName );
            BufferSize = diff;
            if( diff > 0 ) memcpy( TokBuff, buffer + len - diff, diff );
        } else {
            memcpy( TokBuff + BufferSize, buffer, len );
            BufferSize += len;
        }
    } else QWrite( MapFile, buffer, len, MapFName );
}

void WriteMapNL( unsigned count )
{
    unsigned len;
    char *newline;

    if( MapFlags & 1 ) {
        newline = "\r\n";
        len = strlen( newline );
        while( count != 0 ) {
            BufWrite( newline, len );
            --count;
        }
        MapCol = 0;
    }
}

char *PutDec( char *ptr, unsigned value )
{
    *ptr++ = '0' + (value / 10) % 10;
    *ptr++ = '0' + value % 10;
    return ptr;
}

void StopMapBuffering( void )
{
    if( BufferSize != 0 ) {
        QWrite( MapFile, TokBuff, BufferSize, MapFName );
    }
    Buffering = 0;
    BufferSize = 0;
}

void WriteGroups( void )
{
    group_entry *group;
    msg_address addr;

    if( Groups != 0 ) {
        WriteBox( 0x114 );
        Msg_Write_Map( 0x115 );
        Msg_Write_Map( 0x116 );
        WriteMapNL( 1 );
        for( group = Groups; group != 0; group = group->next_group ) {
            if( group->name[0] != '\0' ) {
                WriteFormat( 0, group->name );
                addr.off = group->grp_addr.off;
                addr.seg = group->grp_addr.seg;
                WriteFormat( 32, "%a", addr );
                WriteFormat( 53, "%h", group->totalsize );
                WriteMapNL( 1 );
            }
        }
    }
}

void WriteBox( unsigned msgnum )
{
    char box_buff[128];
    char msg_buff[128];
    unsigned i;

    Msg_Get( msgnum, msg_buff );
    WriteMapNL( 2 );
    box_buff[0] = '+';
    for( i = 1; i < strlen( msg_buff ) - 1; ++i ) box_buff[i] = '-';
    box_buff[i] = '+';
    box_buff[i + 1] = '\0';
    WriteMap( "%t24%s", "", box_buff );
    WriteMap( "%t24%s", "", msg_buff );
    WriteMap( "%t24%s", "", box_buff );
    WriteMapNL( 1 );
}

void WriteMap( char *format, ... )
{
    va_list args;

    va_start( args, format );
    DoWriteMap( format, &args );
    va_end( args );
}

void DoWriteMap( char *format, va_list *args )
{
    if( MapFlags & 1 ) {
        MapPrint( format, args );
        WriteMapNL( 1 );
    }
}

unsigned MapPrint( char *format, va_list *args )
{
    char buff[256];
    unsigned len;

    len = DoFmtStr( buff, sizeof(buff), format, args );
    BufWrite( buff, len );
    return len;
}

void WriteFormat( int col, char *format, ... )
{
    va_list args;
    int num;

    if( MapFlags & 1 ) {
        num = 0;
        if( col > (int)MapCol ) num = col - MapCol;
        else if( MapCol != 0 ) num = 1;
        MapCol += num;
        BufWrite( "                                      ", num );
        va_start( args, format );
        MapCol += MapPrint( format, &args );
        va_end( args );
    }
}

extern void WriteAbsSegs( class_entry * );

void WriteSegs( class_entry *firstcl )
{
    class_entry *class;
    seg_leader *seg;
    msg_address addr;
    unsigned char abs;

    abs = 0;
    class = firstcl;
    if( class != 0 ) {
        WriteBox( 0x11a );
        Msg_Write_Map( 0x11b );
        Msg_Write_Map( 0x11c );
        WriteMapNL( 1 );
        for( ; class != 0; class = class->next_class ) {
            if( !(class->flags & 0x0c) ) {
                for( seg = class->segs; seg != 0; seg = seg->next_seg ) {
                    if( seg->info & 0x38 ) {
                        WriteFormat( 0, seg->segname->name );
                        WriteFormat( 23, class->name );
                        if( seg->group != 0 ) WriteFormat( 38, seg->group->name );
                        addr.off = seg->seg_addr.off;
                        addr.seg = seg->seg_addr.seg;
                        WriteFormat( 53, "%a", addr );
                        WriteFormat( 69, "%h", seg->saved_size );
                        WriteMapNL( 1 );
                    } else {
                        abs = 1;
                        /* This old writer clears the global on this path. */
                        Absolute_Seg = 0;
                    }
                }
            }
        }
        if( abs ) WriteAbsSegs( firstcl );
    }
}

void WritePubHead( void )
{
    WriteBox( 0x11d );
    Msg_Write_Map( 0x11e );
    Msg_Write_Map( 0x11f );
    if( MapFlags & 2 ) Msg_Write_Map( 0x13c );
    WriteMapNL( 1 );
    Msg_Write_Map( 0x120 );
    Msg_Write_Map( 0x121 );
    WriteMapNL( 1 );
}

void WritePubModHead( void )
{
    char full_name[80];
    char *path;

    path = CurrMod->f.source->file->prefix;
    if( path != 0 ) {
        QMakeFileName( &path, CurrMod->f.source->file->name, full_name );
    } else strcpy( full_name, CurrMod->f.source->file->name );
    Msg_Write_Map( 0x13d, full_name, CurrMod->name );
}

void WriteModSegHead( void )
{
    WriteBox( 0x126 );
    if( Absolute_Seg ) Msg_Write_Map( 0x127 );
    Msg_Write_Map( 0x128 );
    Msg_Write_Map( 0x13b );
    WriteMapNL( 1 );
    Msg_Write_Map( 0x129 );
    Msg_Write_Map( 0x12a );
    WriteMapNL( 1 );
}

extern unsigned GetNumNodes( nodearray * );
extern char *GetClassName( seg_leader * );
extern void WriteSeg( char *, char *, unsigned short, address, unsigned long, unsigned char );

void WriteModSegs( void )
{
    unsigned i, count;
    unsigned char first;
    segnode *seg;
    seg_leader *leader;
    address addr;

    first = 1;
    count = GetNumNodes( SegNodes );
    for( i = 1; i <= count; ++i ) {
        seg = FindNode( SegNodes, i );
        if( (seg->info & 0x7000) || (seg->info & 0x80) ) continue;
        if( first ) {
            WriteFormat( 0, CurrMod->name );
            first = 0;
        }
        leader = seg->entry;
        addr.off = seg->addr.off;
        addr.seg = seg->addr.seg;
        WriteSeg( leader->segname->name, GetClassName( leader ), seg->info, addr, seg->length, 0 );
    }
}

char *GetClassName( seg_leader *seg )
{
    class_entry *class;
    seg_leader *leader;

    for( class = seg->section->classlist; class != 0; class = class->next_class ) {
        for( leader = class->segs; leader != 0; leader = leader->next_seg ) {
            if( leader == seg ) return class->name;
        }
    }
    return 0;
}

void WriteSeg( char *name, char *class, unsigned short info,
               address addr, unsigned long size, unsigned char comdat )
{
    msg_address display;
    char absolute, common, uninitialized;

    WriteFormat( 16, name );
    WriteFormat( 38, class );
    absolute = (info & 0x38) ? ' ' : '*';
    common = comdat ? 'c' : ' ';
    uninitialized = (info & 0x400) ? '!' : ' ';
    display.off = addr.off;
    display.seg = addr.seg;
    WriteFormat( 53, "%a%c%c%c", display, absolute, common, uninitialized );
    WriteFormat( 70, "%h", size );
    WriteMapNL( 1 );
}

void WriteUndefined( void )
{
    symrecinfo *info;
    mod_entry *mod;

    if( UndefList != 0 ) {
        WriteBox( 0x131 );
        Msg_Write_Map( 0x132 );
        Msg_Write_Map( 0x133 );
        WriteMapNL( 1 );
        for( info = UndefList; info != 0; info = info->next ) {
            WriteFormat( 0, "%S", info->sym );
            mod = info->mod;
            WriteFormat( 32, "%s(%s)", mod->f.source->file->name, mod->name );
            WriteMapNL( 1 );
        }
    }
}

extern void WriteImports( void );
extern void Write32( char *, unsigned long );
extern unsigned long MemorySize( void );
extern unsigned short AreaSize;
extern address StartInfo;

void MapSizes( void )
{
    char msg_buff[128];
    msg_address entry;

    if( UndefList != 0 ) WriteMapNL( 1 );
    WriteImports();
    WriteLibsUsed();
    WriteBox( 0x135 );
    Msg_Get( 0x136, msg_buff );
    Write32( msg_buff, StackSize );
    Msg_Get( 0x137, msg_buff );
    Write32( msg_buff, MemorySize() );
    if( LinkFlags & 0x400 ) {
        Msg_Get( 0x138, msg_buff );
        Write32( msg_buff, (unsigned long)AreaSize << 4 );
    }
    if( !(FmtData.type & 0x100) && (!(FmtData.flags & 1) || (FmtData.type & 0x10)) ) {
        entry.off = StartInfo.off;
        entry.seg = StartInfo.seg;
        Msg_Write_Map( 0x139, entry );
    }
}

extern void XWriteImports( void );

void WriteImports( void )
{
    if( FmtData.type & 0x11f ) {
        WriteBox( 0x12c );
        if( FmtData.type & 0x8100 ) {
            Msg_Write_Map( 0x12d );
            Msg_Write_Map( 0x12e );
        } else {
            Msg_Write_Map( 0x12f );
            Msg_Write_Map( 0x130 );
        }
        WriteMapNL( 1 );
        XWriteImports();
    }
}

void WriteLibsUsed( void )
{
    file_list *lib;
    char *name, *path;
    char full_name[80];

    if( LinkState & 0x8000 ) {
        WriteBox( 0x134 );
        for( lib = ObjLibFiles; lib != 0; lib = lib->next_file ) {
            if( lib->status & 0x2000 ) {
                name = lib->file->name;
                if( lib->file->prefix != 0 ) {
                    path = lib->file->prefix;
                    QMakeFileName( &path, name, full_name );
                    name = full_name;
                }
                WriteMap( "%s", name );
            }
        }
        LinkState &= ~0x8000;
    }
}

void Write32( char *message, unsigned long size )
{
    unsigned short low;

    if( size <= 0xffff ) {
        low = size;
        WriteMap( "%s  %x (%d.)", message, low, low );
    } else {
        WriteMap( "%s  %h (%l.)", message, size, size );
    }
}

unsigned long MemorySize( void )
{
    ovl_area *area;
    section *sect;
    unsigned long base, end, max_end;

    if( !(FmtData.type & 0xe0) ) return Root->size;
    base = ((unsigned long)Root->sect_addr.seg << 4) + Root->sect_addr.off;
    max_end = base + Root->size;
    for( area = Root->areas; area != 0; area = area->next ) {
        for( sect = area->sections; sect != 0; sect = sect->next_sect ) {
            end = ((unsigned long)sect->sect_addr.seg << 4)
                + sect->sect_addr.off + sect->size;
            if( end > max_end ) max_end = end;
        }
    }
    return max_end - base;
}

/* The 1994 routine is empty: its entry is the shared final RET of MapSizes.
 * The later released implementation that prints elapsed time is not present. */
void EndTime( void )
{
}

void WriteCommunalHead( void )
{
    WriteBox( 0x123 );
    Msg_Write_Map( 0x124 );
    Msg_Write_Map( 0x125 );
    WriteMapNL( 1 );
}
