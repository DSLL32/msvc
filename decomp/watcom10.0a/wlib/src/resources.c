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
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "wlib.h"
#include <recon.h>

#if defined(__WATCOMC__)
/* The original comparison is the compiler's inline memcmp intrinsic. */
#pragma intrinsic(memcmp,memcpy,strlen)
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

/* Original 0x41b0c0..0x41b0fd: old packed13B resource record. */
int WResReadFixedResRecord1( WResResInfo1 *info, int file )
{
    int count;

    count = (*WResRtns.read)( file, info, sizeof( *info ) );
    if( count != sizeof( *info ) ) {
        WresRecordError( count == -1 ? WRS_READ_FAILED : WRS_READ_INCOMPLETE );
        return 1;
    }
    return 0;
}

/* Original0x418a28..0x418ac7. The old body performs lookup directly;
   there is no later WResLoadString2 wrapper. Preserve the real far formal. */
int LoadString( HANDLE_INFO *instance, unsigned resourceid,
                char __far *buffer, int capacity )
{
    WResID type, name;
    WResLangType lang;
    WResDirWindow window;

    lang.lang = 0;
    lang.sublang = 0;
    WResInitIDFromNum( (resourceid >> 4) + 1, &name );
    WResInitIDFromNum( 6, &type );       /* String-table resource type. */
    window = WResFindResource( &type, &name, MainDir, &lang );
    if( window.CurrType == NULL || window.CurrRes == NULL ||
        window.CurrLang == NULL ) {
        return( -1 );
    }
    return( GetString( WResGetLangInfo( window ), instance, resourceid,
                       buffer, capacity ) );
}

/* Original0x419b20..0x419b28. Old numeric IDs are exactly three bytes. */
void WResInitIDFromNum( unsigned short number, WResID *id )
{
    id->IsName = 0;
    id->ID.Num = number;
}

/* Original0x419bb8..0x419c14. Reused initial wrfindrs.c lookup
   returns the full three-pointer window using the genuine struct ABI. */
WResDirWindow WResFindResource( const WResID *type, const WResID *name,
                        WResDir currdir, WResLangType *lang )
{
    WResDirWindow   newwind;

    newwind.CurrType = __FindType( type, currdir );
    if( newwind.CurrType != NULL ) {
        newwind.CurrRes = __FindRes( name, newwind.CurrType );
        if( newwind.CurrRes != NULL ) {
            if( lang == NULL ) {
                newwind.CurrLang = newwind.CurrRes->Head;
            } else {
                newwind.CurrLang = __FindLang( lang, newwind.CurrRes );
            }
        } else {
            newwind.CurrLang = NULL;
            newwind.CurrType = NULL;
        }
    } else {
        newwind.CurrRes = NULL;
        newwind.CurrLang = NULL;
    }

    return( newwind );
}

/* Original0x419b30..0x419b54; initial wrfindrs.c type list search. */
WResTypeNode *__FindType( const WResID *type, WResDir dir )
{
    WResTypeNode *node;

    for( node = dir->Head; node != NULL; node = node->Next ) {
        if( WResIDCmp( type, &node->Info.TypeName ) ) {
            break;
        }
    }
    return( node );
}

/* Original0x41b000..0x41b03a; initial wridcmp.c compares either
   two names or two numeric IDs, and rejects mixed identifier kinds. */
int WResIDCmp( const WResID *first, const WResID *second )
{
    if( first->IsName && second->IsName ) {
        return( WResIDNameCmp( &first->ID.Name, &second->ID.Name ) == 0 );
    } else if( !first->IsName && !second->IsName ) {
        return( first->ID.Num == second->ID.Num );
    }
    return( 0 );
}

/* Original0x419b54..0x419b78; initial wrfindrs.c resource-name search. */
WResResNode *__FindRes( const WResID *name, WResTypeNode *type )
{
    WResResNode *node;

    for( node = type->Head; node != NULL; node = node->Next ) {
        if( WResIDCmp( name, &node->Info.ResName ) ) {
            break;
        }
    }
    return( node );
}

/* Original0x419b78..0x419bb5. A missing language selects neutral0/0;
   compare the packed language and sublanguage fields independently. */
WResLangNode *__FindLang( WResLangType *lang, WResResNode *resource )
{
    WResLangType neutral;
    WResLangNode *node;

    if( lang == NULL ) {
        neutral.lang = 0;
        neutral.sublang = 0;
        lang = &neutral;
    }
    node = resource->Head;
    while( node != NULL ) {
        if( node->Info.lang.lang == lang->lang &&
            node->Info.lang.sublang == lang->sublang ) {
            break;
        }
        node = node->Next;
    }
    return( node );
}

/* Original0x419c20..0x419c36, initial wrgetli.c. The12-byte window
   is passed by value and the period callee removes those12 stack bytes. */
WResLangInfo *WResGetLangInfo( WResDirWindow window )
{
    if( window.CurrLang == NULL ) {
        return( NULL );
    }
    return( &window.CurrLang->Info );
}

/* Original0x4188e0..0x418a28. Reused initial loadstr.c string-table
   reader. char __far* preserves period pointer/RET12 ABI; the native flat
   projection is limited to Usage's live128B stack object: MsgGet loads DS
   at414c73, forwards the same buffer and pushes capacity128. Other owners
   are unqualified. The four-site original GS patch is unchanged. */
int GetString( WResLangInfo *resource, HANDLE_INFO *instance, unsigned resourceid,
               char __far *buffer, int capacity )
{
    long                prevpos;
    int                 length;
    int                 stringbufflen;
    char *              stringbuff;
    int                 stringnum;
    int                 stringlen;
    int                 numread;
    int                 ix1, ix2;

    prevpos = (*WResRtns.seek)( instance->handle, resource->Offset, SEEK_SET );
    if ( prevpos == -1L ) return( -1 );
    length = resource->Length;
    stringbufflen = (length > 128) ? 128 : length;
    stringbuff = (char *) alloca( stringbufflen );
    if ( stringbuff == NULL ) return( -1 );
    stringnum = resourceid & 0x0f;

    numread = 0;
    stringlen = 0;
    ix2 = 0;
    ix1 = 0;
    do {
        if ( numread == 0 ) {
            if ( length > stringbufflen ) {
                length -= stringbufflen;
            } else {
                stringbufflen = length;
                length = 0;
            }
            numread = (*WResRtns.read)( instance->handle, stringbuff, stringbufflen );
            if( numread == 0 ) return( -1 );    // 15-sep-93 AFS
            if( numread == -1 ) return( -1 );
            ix1 = 0;
        }
        if ( ix1 < numread ) {
            if ( stringlen == 0 ) {
                stringlen = ((unsigned char *)stringbuff)[ ix1++ ];
                stringnum--;
            }
            for( ; stringlen > 0; --stringlen ) {
                if ( ix1 >= numread ) break;
                if ( stringnum < 0 ) {
                    if( ix2 < capacity - 1 ) {
                        buffer[ ix2++ ] = stringbuff[ ix1 ];
                    }
                }
                ix1++;
            }
        } else {
            numread = 0;
        }
    } while( !( (stringlen == 0) && (stringnum < 0) ) );
    buffer[ ix2 ] = '\0';


    return( 0 );
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

/* Original0x418b00..0x418b16 has no later CloseResFile2 wrapper. */
int CloseResFile( HANDLE_INFO *instance )
{
    WResFreeDir( MainDir );
    return( ResCloseFile( instance->handle ) );
}

/* Original0x41a35c..0x41a372; initial source wrfreedi.c. */
void WResFreeDir( WResDir dir )
{
    if( dir != NULL ) {
        __FreeTypeList( dir );
        (*WResRtns.free)( dir );
    }
}

/* Original0x41a324..0x41a359; initial wrfreedi.c algorithm. */
void __FreeTypeList( WResDir dir )
{
    WResTypeNode *type, *next;

    type = dir->Head;
    while( type != NULL ) {
        next = type->Next;
        __FreeResList( type );
        (*WResRtns.free)( type );
        type = next;
    }
    dir->Head = NULL;
    dir->Tail = NULL;
}

/* Original0x41a2ec..0x41a321; initial wrfreedi.c algorithm. */
void __FreeResList( WResTypeNode *type )
{
    WResResNode *resource, *next;

    resource = type->Head;
    while( resource != NULL ) {
        next = resource->Next;
        __FreeLangList( resource );
        (*WResRtns.free)( resource );
        resource = next;
    }
    type->Head = NULL;
    type->Tail = NULL;
}

/* Original0x41a2c0..0x41a2ea; initial wrfreedi.c algorithm. */
void __FreeLangList( WResResNode *resource )
{
    WResLangNode *language, *next;

    language = resource->Head;
    while( language != NULL ) {
        next = language->Next;
        (*WResRtns.free)( language );
        language = next;
    }
    resource->Head = NULL;
    resource->Tail = NULL;
}

/* Original0x41a380..0x41a39c; initial rclosef.c, including the
   SDK error-report vertex on failure. */
int ResCloseFile( int handle )
{
    int result;

    result = (*WResRtns.close)( handle );
    if( result == -1 ) {
        WresRecordError( WRS_CLOSE_FAILED );
    }
    return( result );
}

/* Original0x41b040 is an empty application error hook, not a CRT body. */
void WresRecordError( WResStatus status )
{
}

/* Original0x41b080..0x41b0bd. Initial wrrfrr.c algorithm; this older
   fixed version2 record contains a language count and a three-byte ID. */
int WResReadFixedResRecord( WResResInfo *info, int handle )
{
    int count;

    count = (*WResRtns.read)( handle, info, sizeof(*info) );
    if( count != sizeof(*info) ) {
        WresRecordError( count == -1 ? WRS_READ_FAILED : WRS_READ_INCOMPLETE );
        return( 1 );
    }
    return( 0 );
}

/* Original0x41b120..0x41b172. The first Pascal-name character was
   already read in the fixed three-byte ID. Initial wrrewrid.c algorithm. */
int WResReadExtraWResID( WResID *id, int handle )
{
    int length, count;

    if( id->IsName ) {
        length = (int)id->ID.Name.NumChars - 1;
        if( length > 0 ) {
            count = (*WResRtns.read)( handle, id->ID.Name.Name + 1, length );
            if( count == -1 ) {
                WresRecordError( WRS_READ_FAILED );
                return( 1 );
            }
            if( count != length ) {
                WresRecordError( WRS_READ_INCOMPLETE );
                return( 1 );
            }
        }
    }
    return( 0 );
}

/* Original0x41be40..0x41be8a. Initial wridnacm.c algorithm: compare
   the counted common prefix without case, then order unequal lengths. */
int WResIDNameCmp( const WResIDName *first, const WResIDName *second )
{
    unsigned length;
    int result;

    length = first->NumChars < second->NumChars ? first->NumChars : second->NumChars;
    result = memicmp( first->Name, second->Name, length );
    if( result == 0 ) {
        if( first->NumChars == second->NumChars ) return( 0 );
        return( first->NumChars > second->NumChars ? 1 : -1 );
    }
    return( result );
}

/* Original0x419cc0..0x419d76. Initial wrreaddi.c language-list loop,
   with genuine1994 thirteen-byte information and twenty-five-byte nodes. */
int readLangInfoList( int handle, WResResNode *resource )
{
    WResLangNode *node;
    unsigned index;
    int error, count;

    error = 0;
    for( index = 0; index < resource->Info.NumResources; ++index ) {
        node = (*WResRtns.alloc)( sizeof(*node) );
        if( node == NULL ) {
            error = 1;
            WresRecordError( WRS_MALLOC_FAILED );
        }
        if( error ) break;
        count = (*WResRtns.read)( handle, &node->Info, sizeof(node->Info) );
        if( count != sizeof(node->Info) ) {
            error = 1;
            WresRecordError( count == -1 ? WRS_READ_FAILED : WRS_READ_INCOMPLETE );
            (*WResRtns.free)( node );
            break;
        }
        node->data = NULL;
        ResAddLLItemAtEnd( (void **)&resource->Head, (void **)&resource->Tail, node );
    }
    return( error );
}

/* Win16 MRes reader and owning directory helpers, adapted from initial
   wrreaddi.c, mrrdrh.c, wridfnoo.c, wraddres.c and wrfree.c. The1994
   graph has no Win32 MRes path, fileinfo argument or WResAddResource2. */
int readMResDir( int handle, WResDir dir, int *dup_discarded )
{
    MResResourceHeader *header;
    WResID *name, *type;
    int error, duplicate;

    header = MResReadResourceHeader( handle );
    error = 0;
    if( dup_discarded != NULL ) *dup_discarded = 0;
    dir->TargetOS = 1;
    while( header != NULL && !error ) {
        name = WResIDFromNameOrOrd( header->Name );
        type = WResIDFromNameOrOrd( header->Type );
        error = ( name == NULL || type == NULL );
        if( !error && ( type->IsName || type->ID.Num != 15 ) ) {
            error = WResAddResource( type, name, header->MemoryFlags,
                         (*WResRtns.tell)( handle ), header->Size,
                         dir, NULL, &duplicate );
            if( error && duplicate ) {
                error = 0;
                if( dup_discarded != NULL ) *dup_discarded = 1;
            }
        }
        if( !error ) {
            if( (*WResRtns.seek)( handle, header->Size, SEEK_CUR ) == -1L ) {
                error = 1;
                WresRecordError( WRS_SEEK_FAILED );
            }
        }
        if( name != NULL ) (*WResRtns.free)( name );
        if( type != NULL ) (*WResRtns.free)( type );
        MResFreeResourceHeader( header );
        if( !error ) header = MResReadResourceHeader( handle );
    }
    return( error );
}

MResResourceHeader *MResReadResourceHeader( int handle )
{
    MResResourceHeader *header;
    int error;
    header = (*WResRtns.alloc)( sizeof(MResResourceHeader) );
    error = ( header == NULL );
    if( error ) WresRecordError( WRS_MALLOC_FAILED );
    if( !error ) {
        header->Type = ResReadNameOrOrdinal( handle );
        error = ( header->Type == NULL );
    }
    if( !error ) {
        header->Name = ResReadNameOrOrdinal( handle );
        error = ( header->Name == NULL );
    }
    if( !error ) error = ResReadUint16( &header->MemoryFlags, handle );
    if( !error ) error = ResReadUint32( &header->Size, handle );
    if( error && header != NULL ) {
        /* Original frees only the header on failure; already allocated names
           are not reclaimed here. Preserve that1994 ownership defect. */
        (*WResRtns.free)( header );
        header = NULL;
    }
    return( header );
}

void MResFreeResourceHeader( MResResourceHeader *header )
{
    if( header->Type != NULL ) (*WResRtns.free)( header->Type );
    if( header->Name != NULL ) (*WResRtns.free)( header->Name );
    (*WResRtns.free)( header );
}

WResID *WResIDFromNameOrOrd( const ResNameOrOrdinal *name )
{
    if( name->ord.fFlag == 0xff ) {
        return( WResIDFromNum( name->ord.wOrdinalID ) );
    } else {
        return( WResIDFromStr( name->name ) );
    }
}

/* Initial wridfrnu.c/wridfrst.c. Allocating IDs are distinct from the
   eight-byte in-place WResInitIDFromNum entry at0x419b20. */
WResID *WResIDFromNum( long number )
{
    WResID *id;
    if( number < 0 || number > 65535L ) {
        WresRecordError( WRS_BAD_PARAMETER );
        return( NULL );
    }
    id = (*WResRtns.alloc)( sizeof(WResID) );
    if( id == NULL ) WresRecordError( WRS_MALLOC_FAILED );
    else WResInitIDFromNum( number, id );
    return( id );
}

WResID *WResIDFromStr( const char *name )
{
    WResID *id;
    unsigned length;
    length = strlen( name );
    if( length > 255 ) {
        WresRecordError( WRS_BAD_PARAMETER );
        return( NULL );
    }
    id = (*WResRtns.alloc)( sizeof(WResID) + length - 1 );
    if( id == NULL ) WresRecordError( WRS_MALLOC_FAILED );
    else {
        id->IsName = 1;
        id->ID.Name.NumChars = length;
        memcpy( id->ID.Name.Name, name, length );
    }
    return( id );
}

WResTypeNode *newTypeNode( const WResID *type )
{
    WResTypeNode *node;
    unsigned extra;
    extra = WResIDExtraBytes( type );
    node = (*WResRtns.alloc)( sizeof(WResTypeNode) + extra );
    if( node == NULL ) WresRecordError( WRS_MALLOC_FAILED );
    else {
        node->Next = node->Prev = NULL;
        node->Head = node->Tail = NULL;
        node->Info.NumResources = 0;
        memcpy( &node->Info.TypeName, type, sizeof(WResID) + extra );
    }
    return( node );
}

WResResNode *newResNode( const WResID *name )
{
    WResResNode *node;
    unsigned extra;
    extra = WResIDExtraBytes( name );
    node = (*WResRtns.alloc)( sizeof(WResResNode) + extra );
    if( node == NULL ) WresRecordError( WRS_MALLOC_FAILED );
    else {
        node->Next = node->Prev = NULL;
        node->Head = node->Tail = NULL;
        node->Info.NumResources = 0;
        memcpy( &node->Info.ResName, name, sizeof(WResID) + extra );
    }
    return( node );
}

WResLangNode *newLangNode( unsigned short flags, unsigned long offset,
                           unsigned long length, WResLangType *lang )
{
    WResLangNode *node;
    node = (*WResRtns.alloc)( sizeof(WResLangNode) );
    if( node == NULL ) WresRecordError( WRS_MALLOC_FAILED );
    else {
        node->Next = node->Prev = NULL;
        node->data = NULL;
        node->Info.MemoryFlags = flags;
        node->Info.Offset = offset;
        node->Info.Length = length;
        if( lang == NULL ) {
            node->Info.lang.lang = 0;
            node->Info.lang.sublang = 0;
        } else node->Info.lang = *lang;
    }
    return( node );
}

int WResAddResource( const WResID *type, const WResID *name,
                     unsigned short flags, unsigned long offset,
                     unsigned long length, WResDir dir, WResLangType *lang,
                     int *duplicate )
{
    WResTypeNode *typenode;
    WResResNode *resnode;
    WResLangNode *langnode;
    if( duplicate != NULL ) *duplicate = 0;
    resnode = NULL;
    typenode = __FindType( type, dir );
    if( typenode != NULL ) {
        resnode = __FindRes( name, typenode );
        if( resnode != NULL && __FindLang( lang, resnode ) != NULL ) {
            if( duplicate != NULL ) *duplicate = 1;
            WresRecordError( WRS_DUP_ENTRY );
            return( 1 );
        }
    }
    if( typenode == NULL ) {
        typenode = newTypeNode( type );
        if( typenode == NULL ) return( 1 );
        ResAddLLItemAtEnd( (void **)&dir->Head, (void **)&dir->Tail, typenode );
        ++dir->NumTypes;
    }
    if( resnode == NULL ) {
        resnode = newResNode( name );
        if( resnode == NULL ) return( 1 );
        ResAddLLItemAtEnd( (void **)&typenode->Head, (void **)&typenode->Tail, resnode );
        ++typenode->Info.NumResources;
        ++dir->NumResources;
    }
    langnode = newLangNode( flags, offset, length, lang );
    if( langnode == NULL ) return( 1 );
    ResAddLLItemAtEnd( (void **)&resnode->Head, (void **)&resnode->Tail, langnode );
    ++resnode->Info.NumResources;
    return( 0 );
}

/* Initial rrnamoor.c, rrdstr.c, rrint.c and varstr.c algorithms. The
   linked64-byte string pieces are App resource support, not CRT bodies. */
int ResReadUint8( unsigned char *value, int handle )
{
    int count;
    count = (*WResRtns.read)( handle, value, 1 );
    if( count == 1 ) return( 0 );
    WresRecordError( count == -1 ? WRS_READ_FAILED : WRS_READ_INCOMPLETE );
    return( 1 );
}
int ResReadUint16( unsigned short *value, int handle )
{
    int count;
    count = (*WResRtns.read)( handle, value, 2 );
    if( count == 2 ) return( 0 );
    WresRecordError( count == -1 ? WRS_READ_FAILED : WRS_READ_INCOMPLETE );
    return( 1 );
}
ResNameOrOrdinal *ResReadNameOrOrdinal( int handle )
{
    ResNameOrOrdinal name, *result;
    char *rest;
    int error, length;
    rest = NULL;
    error = ResReadUint8( &name.ord.fFlag, handle );
    if( !error ) {
        if( name.ord.fFlag == 0xff ) {
            error = ResReadUint16( &name.ord.wOrdinalID, handle );
            length = 0;
        } else if( name.name[0] != '\0' ) {
            rest = ResReadString( handle, &length );
            ++length;
            error = ( rest == NULL );
        } else length = 0;
    }
    if( error ) result = NULL;
    else {
        result = (*WResRtns.alloc)( sizeof(ResNameOrOrdinal) + length );
        error = ( result == NULL );
        if( error ) WresRecordError( WRS_MALLOC_FAILED );
    }
    if( !error ) {
        result->ord.fFlag = name.ord.fFlag;
        if( name.ord.fFlag == 0xff ) result->ord.wOrdinalID = name.ord.wOrdinalID;
        else if( name.name[0] != '\0' ) {
            memcpy( result->name + 1, rest, length );
            (*WResRtns.free)( rest );
        }
    }
    return( result );
}
VarString *VarStringStart( void )
{
    VarString *list;
    list = (*WResRtns.alloc)( sizeof(VarString) );
    if( list == NULL ) WresRecordError( WRS_MALLOC_FAILED );
    else { list->lastLoc = -1; list->next = NULL; }
    return( list );
}
void VarStringAddChar( VarString *list, int ch )
{
    if( list != NULL ) {
        while( list->next != NULL ) list = list->next;
        if( list->lastLoc == 63 ) {
            list->next = VarStringStart();
            if( list->next == NULL ) return;
            list = list->next;
        }
        list->partString[++list->lastLoc] = ch;
    }
}
int ComputeVarStringLen( VarString *list )
{
    int length;
    length = 0;
    while( list != NULL ) {
        length += list->lastLoc + 1;
        list = list->next;
    }
    return( length );
}
char *VarStringEnd( VarString *list, int *retlength )
{
    VarString *old;
    char *result, *next;
    int length;
    length = ComputeVarStringLen( list );
    result = (*WResRtns.alloc)( length + 1 );
    if( result == NULL ) WresRecordError( WRS_MALLOC_FAILED );
    else {
        next = result;
        while( list != NULL ) {
            memcpy( next, list->partString, list->lastLoc + 1 );
            next += list->lastLoc + 1;
            old = list;
            list = list->next;
            (*WResRtns.free)( old );
        }
        result[length] = '\0';
        if( retlength != NULL ) *retlength = length;
    }
    return( result );
}
char *ResReadString( int handle, int *length )
{
    VarString *list;
    char *result;
    unsigned char ch;
    int error;
    list = VarStringStart();
    error = ResReadUint8( &ch, handle );
    while( !error && ch != '\0' ) {
        VarStringAddChar( list, ch );
        error = ResReadUint8( &ch, handle );
    }
    result = VarStringEnd( list, length );
    if( error && result != NULL ) {
        (*WResRtns.free)( result );
        result = NULL;
    }
    return( result );
}
