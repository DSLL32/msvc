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
#pragma intrinsic(memcmp)
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
