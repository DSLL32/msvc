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


/* SDK WRes handle adapters. Actual1994graph omits the later setmode edge. */
#include <fcntl.h>
#include <io.h>
#include <stdio.h>
#include <string.h>
#include "link.h"
#include <recon.h>
#if defined(__WATCOMC__)
/* Target compares the fixed trailer prefix inline, as STRING.H permits. */
#pragma intrinsic(memcmp)
#endif

struct WResRoutines {
    int (*open)( const char *, int, ... );
    int (*close)( int );
    int (*write)( int, const void *, unsigned );
    int (*read)( int, void *, unsigned );
    long (*seek)( int, long, int );
    long (*tell)( int );
    void *(*alloc)( unsigned );
    void (*free)( void * );
};
struct WResRoutines WResRtns = { open, close, write, read, ResSeek, QPos, ChkLAlloc, _LnkFree }; /* original441f94 */

int OpenResFile( HANDLE_INFO *instance )
{
    return instance->handle = ResOpenFileRO( instance->filename );
}

int ResOpenFileRO( const char *filename )
{
    int handle;

    handle = (*WResRtns.open)( filename, O_RDONLY | O_BINARY );
    if( handle == -1 ) {
        WresRecordError( 5 );
    }
    return handle;
}

/* The 1994 trailer record is packed14B. Its unused version bytes are kept;
 * debug_size starts at offset10, matching the actual reader. */
#pragma pack(1)
typedef struct debug_header {
    unsigned short signature;
    unsigned char exe_major_ver, exe_minor_ver;
    unsigned char obj_major_ver, obj_minor_ver;
    unsigned short lang_size, seg_size;
    unsigned long debug_size;
} debug_header;
#pragma pack()
typedef char debug_header_is_14[(sizeof(debug_header)==14)?1:-1];
#define VALID_SIGNATURE 0x8386
#define FOX_SIGNATURE1  0x8300
#define FOX_SIGNATURE2  0x8301
#define WAT_RES_SIG     0x8302
#define LEVEL           "WATCOM patch level .xxx"
#define LEVEL_HEAD_SIZE (sizeof(LEVEL)-5)
long FileShift; /* original4503c0 */

extern int FindResources( HANDLE_INFO * hInstance )
/* look for the resource information in a debugger record at the end of file */
{
    long       currpos;
    long       offset;
    debug_header   header;
    int         notfound;
    char        buffer[ sizeof( LEVEL ) ];

    #define     __handle        hInstance->handle
    notfound = 1;
    FileShift = 0;
    offset = sizeof(debug_header);
    if( WResRtns.seek( __handle, -(long)sizeof(LEVEL), SEEK_END ) != -1L ){
        if( WResRtns.read( __handle, buffer, sizeof(LEVEL) ) == sizeof(LEVEL) ) {
            if( memcmp( buffer, LEVEL, LEVEL_HEAD_SIZE ) == 0 ) {
                offset += sizeof(LEVEL);
            }
        }
    }
    currpos = WResRtns.seek( __handle, - offset, SEEK_END );
    for(;;) {
        WResRtns.read( __handle, &header, sizeof(debug_header) );
        if( header.signature == WAT_RES_SIG ) {
            notfound = 0;
            FileShift = currpos - header.debug_size + sizeof(debug_header);
            break;
        } else if( header.signature == VALID_SIGNATURE ||
                   header.signature == FOX_SIGNATURE1 ||
                   header.signature == FOX_SIGNATURE2 ) {
            currpos -= header.debug_size;
            WResRtns.seek( __handle, currpos, SEEK_SET );
        } else {        /* did not find the resource information */
            break;
        }
    }
    return( notfound );
    #undef __handle
}

/* 1994 ResSeek applies the resource offset for every absolute seek;
 * the later source additionally checks the self-file handle. */
long ResSeek( int file, long position, int where )
{
    if( where == SEEK_SET ) {
        return QLSeek( file, position + FileShift, where, 0 ) - FileShift;
    }
    return QLSeek( file, position, where, 0 );
}

WResDir MainDir; /* original4503c4 */

int InitResources( HANDLE_INFO *instance )
{
    MainDir = WResInitDir();
    if( MainDir == 0 ) {
        return -1;
    }
    return WResReadDir( instance->handle, MainDir, 0 );
}

#pragma pack(1)
struct WResDirHead {
    unsigned short NumResources, NumTypes;
    struct WResTypeNode *Head, *Tail;
    unsigned short TargetOS;
};
#pragma pack()
typedef char WResDirHead_is_14[(sizeof(struct WResDirHead)==14)?1:-1];

WResDir WResInitDir( void )
{
    WResDir newdir;

    newdir = (*WResRtns.alloc)( sizeof( *newdir ) );
    if( newdir == 0 ) {
        WresRecordError( 13 );
    } else {
        newdir->NumResources = 0;
        newdir->NumTypes = 0;
        newdir->TargetOS = 1;       /* WRES_OS_WIN16 */
        newdir->Head = 0;
        newdir->Tail = 0;
    }
    return newdir;
}

int WResReadDir( int file, WResDir dir, int *duplicates )
{
    int error;

    error = 0;
    if( duplicates != 0 ) {
        *duplicates = 0;
    }
    if( dir->Head != 0 ) {
        __FreeTypeList( dir );
    }
    if( (*WResRtns.seek)( file, 0L, SEEK_SET ) == -1L ) {
        error = 1;
        WresRecordError( 9 );
    }
    if( !error ) {
        if( WResIsWResFile( file ) ) {
            error = readWResDir( file, dir );
        } else {
            error = readMResDir( file, dir, duplicates );
        }
    }
    return error;
}

int WResIsWResFile( int file )
{
    long savepos;
    unsigned long magic[2];
    int error;

    savepos = (*WResRtns.seek)( file, 0L, SEEK_SET );
    if( savepos == -1L ) {
        WresRecordError( 9 );
        return 0;
    }
    error = ResReadUint32( magic, file );
    if( !error ) {
        error = ResReadUint32( magic + 1, file );
    }
    if( (*WResRtns.seek)( file, savepos, SEEK_SET ) == -1L ) {
        WresRecordError( 9 );
    }
    return !error && magic[0] == 0xc3d4c1d7UL && magic[1] == 0xc3d2cdcfUL;
}

int ResReadUint32( unsigned long *value, int file )
{
    int count;

    count = (*WResRtns.read)( file, value, sizeof( *value ) );
    if( count != sizeof( *value ) ) {
        WresRecordError( count == -1 ? 8 : 14 );
        return 1;
    }
    return 0;
}

#pragma pack(1)
struct WResHeader {
    unsigned long Magic[2], DirOffset;
    unsigned short NumResources, NumTypes, WResVer;
};
struct WResExtHeader {
    unsigned short TargetOS, reserved[4];
};
#pragma pack()
typedef char WResHeader_is_18[(sizeof(WResHeader)==18)?1:-1];
typedef char WResExtHeader_is_10[(sizeof(WResExtHeader)==10)?1:-1];

int readWResDir( int file, WResDir dir )
{
    WResHeader head;
    WResExtHeader ext;
    int error;

    error = WResReadHeaderRecord( &head, file );
    if( !error ) {
        if( head.Magic[0] != 0xc3d4c1d7UL || head.Magic[1] != 0xc3d2cdcfUL ) {
            error = 1;
            WresRecordError( 3 );
        }
    }
    if( !error && head.WResVer > 2 ) {
        error = 1;
        WresRecordError( 4 );
    }
    if( !error ) {
        if( head.WResVer >= 1 ) {
            error = (*WResRtns.seek)( file, sizeof(head), SEEK_CUR ) == -1L;
            if( error ) {
                WresRecordError( 9 );
            } else {
                error = WResReadExtHeader( &ext, file );
            }
        } else {
            ext.TargetOS = 1;
        }
    }
    if( !error ) {
        dir->NumResources = head.NumResources;
        dir->NumTypes = head.NumTypes;
        dir->TargetOS = ext.TargetOS;
        if( (*WResRtns.seek)( file, head.DirOffset, SEEK_SET ) == -1L ) {
            error = 1;
            WresRecordError( 9 );
        }
    }
    if( !error ) {
        error = readTypeList( file, dir, head.WResVer );
    }
    return error;
}
