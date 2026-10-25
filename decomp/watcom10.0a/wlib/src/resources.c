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


/* Original OpenResFile, 0x418ac8..0x418ad7. WRes resource support is part
   of this application's function graph, rather than a CRT exemption. */
#include <fcntl.h>
#include <io.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "wlib.h"
#include <recon.h>

#if defined(__WATCOMC__)
/* The original comparison is the compiler's inline memcmp intrinsic. */
#pragma intrinsic(memcmp,memcpy)
#endif

/* Encountered prefix of the resource callback table at 0x42044c. */
struct WResRoutines WResRtns = { open, close, write, read, res_seek,
                                 tell, malloc, free };
long FileShift;
WResDir MainDir;

/* Original 0x414be4..0x414bfe. Same callback as res_seek in the initial
   bld/nwlib/c/error.c: resource-relative SEEK_SET, ordinary other origins. */
long res_seek( int handle, long position, int where )
{
    if( where == SEEK_SET ) {
        return( lseek( handle, position + FileShift, where ) - FileShift );
    } else {
        return( lseek( handle, position, where ) );
    }
}

int OpenResFile( HANDLE_INFO *instance )
{
    return( instance->handle = ResOpenFileRO( instance->filename ) );
}

/* Original 0x418ad8..0x418b00 calls the directory routines directly;
   the newer InitResources2 wrapper is absent from the original graph. */
int InitResources( HANDLE_INFO *instance )
{
    MainDir = WResInitDir();
    if( MainDir == NULL ) {
        return( -1 );
    }
    return( WResReadDir( instance->handle, MainDir, NULL ) );
}

/* Original 0x419c70..0x419cb8. Reused from initial-commit
   bld/sdk/rc/wres/c/wrinitdi.c; the packed directory header is 14 bytes. */
WResDir WResInitDir( void )
{
    WResDirHead *newdir;

    newdir = (*WResRtns.alloc)( sizeof(WResDirHead) );
    if( newdir == NULL ) {
        WresRecordError( WRS_MALLOC_FAILED );
    } else {
        newdir->NumResources = 0;
        newdir->NumTypes = 0;
        newdir->TargetOS = 1;       /* WRES_OS_WIN16 */
        newdir->Head = NULL;
        newdir->Tail = NULL;
    }
    return( newdir );
}

/* Original 0x41a244..0x41a2b1. The 1994 entry contains the dispatch
   itself, with neither the later WResReadDir2 wrapper nor Win32-resource
   variant/fileinfo arguments. Source algorithm: initial wrreaddi.c. */
int WResReadDir( int handle, WResDir dir, int *dup_discarded )
{
    int error;

    error = 0;
    if( dup_discarded != NULL ) {
        *dup_discarded = 0;
    }
    if( dir->Head != NULL ) {
        __FreeTypeList( dir );
    }
    if( (*WResRtns.seek)( handle, 0L, SEEK_SET ) == -1L ) {
        error = 1;
        WresRecordError( WRS_SEEK_FAILED );
    }
    if( !error ) {
        if( WResIsWResFile( handle ) ) {
            error = readWResDir( handle, dir );
        } else {
            error = readMResDir( handle, dir, dup_discarded );
        }
    }
    return( error );
}

/* Original 0x41ba00..0x41ba85. Initial wriswrf.c algorithm, before
   the later multiformat WResFindResType entry was introduced. */
int WResIsWResFile( int handle )
{
    long savepos;
    unsigned long magic[2];
    int error;

    savepos = (*WResRtns.seek)( handle, 0L, SEEK_SET );
    if( savepos == -1L ) {
        WresRecordError( WRS_SEEK_FAILED );
        return( 0 );
    }
    error = ResReadUint32( magic, handle );
    if( !error ) {
        error = ResReadUint32( magic + 1, handle );
    }
    if( (*WResRtns.seek)( handle, savepos, SEEK_SET ) == -1L ) {
        WresRecordError( WRS_SEEK_FAILED );
    }
    return( !error && magic[0] == 0xc3d4c1d7UL &&
                     magic[1] == 0xc3d2cdcfUL );
}

/* Original 0x41c090..0x41c0cd. Read the little-endian DWORD in one
   callback, preserving distinct I/O-error and short-read status values. */
int ResReadUint32( unsigned long *value, int handle )
{
    int count;

    count = (*WResRtns.read)( handle, value, sizeof(*value) );
    if( count != sizeof(*value) ) {
        WresRecordError( count == -1 ? WRS_READ_FAILED : WRS_READ_INCOMPLETE );
        return( 1 );
    }
    return( 0 );
}

/* Original 0x41a018..0x41a11b. Initial wrreaddi.c algorithm adapted
   to the actual pre-version3 graph; two magic DWORDs and version<=2. */
int readWResDir( int file, WResDir dir )
{
    WResHeader head;
    WResExtHeader ext;
    int error;

    error = WResReadHeaderRecord( &head, file );
    if( !error ) {
        if( head.Magic[0] != 0xc3d4c1d7UL || head.Magic[1] != 0xc3d2cdcfUL ) {
            error = 1;
            WresRecordError( WRS_BAD_SIG );
        }
    }
    if( !error && head.WResVer > 2 ) {
        error = 1;
        WresRecordError( WRS_BAD_VERSION );
    }
    if( !error ) {
        if( head.WResVer >= 1 ) {
            error = (*WResRtns.seek)( file, sizeof(head), SEEK_CUR ) == -1L;
            if( error ) {
                WresRecordError( WRS_SEEK_FAILED );
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
            WresRecordError( WRS_SEEK_FAILED );
        }
    }
    if( !error ) {
        error = readTypeList( file, dir, head.WResVer );
    }
    return error;
}

/* Original 0x41b1c0..0x41b251. Preserve the saved resource-relative
   position after a complete 18-byte header read. */
int WResReadHeaderRecord( WResHeader *header, int file )
{
    long position;
    int count;
    int error;

    error = 1;
    position = (*WResRtns.seek)( file, 0L, SEEK_SET );
    if( position == -1L ) {
        WresRecordError( WRS_SEEK_FAILED );
    } else {
        count = (*WResRtns.read)( file, header, sizeof( *header ) );
        if( count != sizeof( *header ) ) {
            WresRecordError( count == -1 ? WRS_READ_FAILED : WRS_READ_INCOMPLETE );
        } else {
            if( (*WResRtns.seek)( file, position, SEEK_SET ) == -1L ) {
                WresRecordError( WRS_SEEK_FAILED );
            } else {
                error = 0;
            }
        }
    }
    return error;
}

/* Original 0x41b254..0x41b291. The extended header is 10 packed bytes. */
int WResReadExtHeader( WResExtHeader *header, int file )
{
    int count;

    count = (*WResRtns.read)( file, header, sizeof( *header ) );
    if( count != sizeof( *header ) ) {
        WresRecordError( count == -1 ? WRS_READ_FAILED : WRS_READ_INCOMPLETE );
        return 1;
    }
    return 0;
}

/* Original 0x419f20..0x41a018. Initial wrreaddi.c type loop with
   the actual three-byte ID and five-byte fixed type record. */
int readTypeList( int file, WResDir dir, unsigned short version )
{
    WResTypeNode *node;
    WResTypeInfo info;
    int error, typenum, extra;

    for( error = 0, typenum = 0; typenum < dir->NumTypes && !error; ++typenum ) {
        error = WResReadFixedTypeRecord( &info, file );
        if( !error ) {
            extra = WResIDExtraBytes( &info.TypeName );
            node = (*WResRtns.alloc)( sizeof( *node ) + extra );
            if( node == 0 ) {
                error = 1;
                WresRecordError( WRS_MALLOC_FAILED );
            }
        }
        if( !error ) {
            node->Head = 0;
            node->Tail = 0;
            memcpy( &node->Info, &info, sizeof(info) );
            if( extra > 0 ) {
                error = WResReadExtraWResID( &node->Info.TypeName, file );
            }
        }
        if( !error ) {
            ResAddLLItemAtEnd( (void **)&dir->Head, (void **)&dir->Tail, node );
            error = readResList( file, node, version );
        }
    }
    return error;
}

/* Original 0x41b180..0x41b1b8: packed five-byte fixed type record. */
int WResReadFixedTypeRecord( WResTypeInfo *info, int file )
{
    int count;

    count = (*WResRtns.read)( file, info, sizeof( *info ) );
    if( count == sizeof( *info ) ) {
        return 0;
    }
    WresRecordError( count == -1 ? WRS_READ_FAILED : WRS_READ_INCOMPLETE );
    return 1;
}

/* Original 0x41b100..0x41b112. A Pascal name's first character is
   already included in the old three-byte ID; numeric IDs need no tail. */
int WResIDExtraBytes( const WResID *id )
{
    if( id->IsName ) {
        return( id->ID.Name.NumChars - 1 );
    }
    return( 0 );
}

/* Original 0x41b050..0x41b075: shared doubly-linked list append. */
void ResAddLLItemAtEnd( void **head, void **tail, void *item )
{
    struct resource_link {
        struct resource_link *Next, *Prev;
    } *node;

    node = item;
    if( *head == 0 ) {
        *tail = item;
        *head = item;
        node->Next = 0;
        node->Prev = 0;
    } else {
        node->Next = 0;
        node->Prev = *tail;
        ((struct resource_link *)*tail)->Next = node;
        *tail = item;
    }
}

/* Original 0x419d78..0x419f1d. Initial wrreaddi.c resource loop,
   retaining the old13B fixed record and25B language node without fileinfo. */
int readResList( int file, WResTypeNode *type, unsigned short version )
{
    WResResInfo info;
    WResResInfo1 oldinfo;
    WResResNode *node;
    WResLangNode *lang;
    WResID *id;
    int error, resnum, extra;

    for( resnum = 0, error = 0; resnum < type->Info.NumResources && !error; ++resnum ) {
        if( version < 2 ) {
            error = WResReadFixedResRecord1( &oldinfo, file );
            id = &oldinfo.ResName;
        } else {
            error = WResReadFixedResRecord( &info, file );
            id = &info.ResName;
        }
        if( !error ) {
            extra = WResIDExtraBytes( id );
            node = (*WResRtns.alloc)( sizeof(*node) + extra );
            if( node == 0 ) {
                error = 1;
                WresRecordError( WRS_MALLOC_FAILED );
            }
        }
        if( !error ) {
            node->Head = 0;
            node->Tail = 0;
            if( version < 2 ) {
                node->Info.NumResources = 1;
                memcpy( &node->Info.ResName, &oldinfo.ResName, sizeof(WResID) );
            } else {
                memcpy( &node->Info, &info, sizeof(info) );
            }
            if( extra > 0 ) {
                error = WResReadExtraWResID( &node->Info.ResName, file );
            }
            /* The target dispatches here even after a name-read error. */
            if( version < 2 ) {
                lang = (*WResRtns.alloc)( sizeof(*lang) );
                if( lang == 0 ) {
                    error = 1;
                    WresRecordError( WRS_MALLOC_FAILED );
                }
                if( !error ) {
                    lang->data = 0;
                    lang->Info.MemoryFlags = oldinfo.MemoryFlags;
                    lang->Info.Offset = oldinfo.Offset;
                    lang->Info.Length = oldinfo.Length;
                    lang->Info.lang.lang = 0;
                    lang->Info.lang.sublang = 0;
                    ResAddLLItemAtEnd( (void **)&node->Head, (void **)&node->Tail, lang );
                }
            } else {
                error = readLangInfoList( file, node );
            }
        }
        if( !error ) {
            ResAddLLItemAtEnd( (void **)&type->Head, (void **)&type->Tail, node );
        }
    }
    return error;
}

/* Original 0x419c40..0x419c65 passes binary mode directly to open.
   The newer released source adds a setmode call absent from this binary. */
int ResOpenFileRO( const char *filename )
{
    int handle;

    handle = (*WResRtns.open)( filename, O_RDONLY | O_BINARY );
    if( handle == -1 ) {
        WresRecordError( WRS_OPEN_FAILED );
    }
    return( handle );
}

#pragma pack(1)
typedef struct dbgheader {
    unsigned short signature;
    unsigned char exe_major_ver;
    unsigned char exe_minor_ver;
    unsigned char obj_major_ver;
    unsigned char obj_minor_ver;
    unsigned short lang_size;
    unsigned short seg_size;
    unsigned long debug_size;
} dbgheader;
#pragma pack()
typedef char WlibDebugHeaderSize[(sizeof(dbgheader) == 14) ? 1 : -1];

/* Original 0x418b18..0x418c0e. This algorithm and packed header are reused
   from bld/sdk/rc/wres/c/loadstr.c at Open Watcom initial commit
   6b9cb4438959d845a2b4dd4bf19f7908ecd608e9, verified against all 1994
   branches and five indirect call sites. Callback names/types are explicit;
   the original 14-byte header and 24/19-byte patch-level checks are retained. */
int FindResources( HANDLE_INFO *instance )
{
    long currpos;
    long offset;
    dbgheader header;
    int notfound;
    char buffer[sizeof("WATCOM patch level .xxx")];

    notfound = 1;
    FileShift = 0;
    offset = sizeof(dbgheader);
    if( (*WResRtns.seek)( instance->handle, -(long)sizeof(buffer), SEEK_END ) != -1L ) {
        if( (*WResRtns.read)( instance->handle, buffer, sizeof(buffer) ) == sizeof(buffer) ) {
            if( memcmp( buffer, "WATCOM patch level .xxx", 19 ) == 0 ) {
                offset += sizeof(buffer);
            }
        }
    }
    currpos = (*WResRtns.seek)( instance->handle, -offset, SEEK_END );
    for( ;; ) {
        /* The original ignores this read's count; the valid case supplies
           the complete packed header. */
        (*WResRtns.read)( instance->handle, &header, sizeof(header) );
        if( header.signature == 0x8302 ) {
            notfound = 0;
            FileShift = currpos - header.debug_size + sizeof(header);
            break;
        } else if( header.signature == 0x8386 ||
                   header.signature == 0x8300 ||
                   header.signature == 0x8301 ) {
            currpos -= header.debug_size;
            (*WResRtns.seek)( instance->handle, currpos, SEEK_SET );
        } else {
            break;
        }
    }
    return( notfound );
}
