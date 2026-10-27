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
#include <malloc.h>
#include <stdio.h>
#include <string.h>
#include "link.h"
#include "resource.h"
#include <recon.h>
#if defined(__WATCOMC__)
/* Target compares the fixed trailer prefix inline, as STRING.H permits. */
#pragma intrinsic(memcmp,memcpy,strlen)
#endif


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

int WResReadHeaderRecord( WResHeader *header, int file )
{
    long position;
    int count;
    int error;

    error = 1;
    position = (*WResRtns.seek)( file, 0L, SEEK_SET );
    if( position == -1L ) {
        WresRecordError( 9 );
    } else {
        count = (*WResRtns.read)( file, header, sizeof( *header ) );
        if( count != sizeof( *header ) ) {
            WresRecordError( count == -1 ? 8 : 14 );
        } else {
            if( (*WResRtns.seek)( file, position, SEEK_SET ) == -1L ) {
                WresRecordError( 9 );
            } else {
                error = 0;
            }
        }
    }
    return error;
}

int WResReadExtHeader( WResExtHeader *header, int file )
{
    int count;

    count = (*WResRtns.read)( file, header, sizeof( *header ) );
    if( count != sizeof( *header ) ) {
        WresRecordError( count == -1 ? 8 : 14 );
        return 1;
    }
    return 0;
}

/* Version0..2 has one-byte Pascal lengths, unlike the released version3. */
#pragma pack(1)




#pragma pack()
typedef char WResID_is_3[(sizeof(WResID)==3)?1:-1];
typedef char WResTypeInfo_is_5[(sizeof(WResTypeInfo)==5)?1:-1];
typedef char WResTypeNode_is_21[(sizeof(WResTypeNode)==21)?1:-1];

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
                WresRecordError( 13 );
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

int WResReadFixedTypeRecord( WResTypeInfo *info, int file )
{
    int count;

    count = (*WResRtns.read)( file, info, sizeof( *info ) );
    if( count == sizeof( *info ) ) {
        return 0;
    }
    WresRecordError( count == -1 ? 8 : 14 );
    return 1;
}

int WResIDExtraBytes( const WResID *id )
{
    if( id->IsName ) {
        return id->ID.Name.NumChars - 1;
    }
    return 0;
}

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

#pragma pack(1)




#pragma pack()
typedef char WResResInfo_is_5[(sizeof(WResResInfo)==5)?1:-1];
typedef char WResResInfo1_is_13[(sizeof(WResResInfo1)==13)?1:-1];
typedef char WResResNode_is_21[(sizeof(WResResNode)==21)?1:-1];
typedef char WResLangNode_is_25[(sizeof(WResLangNode)==25)?1:-1];

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
                WresRecordError( 13 );
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
                    WresRecordError( 13 );
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

int WResReadFixedResRecord1( WResResInfo1 *info, int file )
{
    int count;

    count = (*WResRtns.read)( file, info, sizeof( *info ) );
    if( count != sizeof( *info ) ) {
        WresRecordError( count == -1 ? 8 : 14 );
        return 1;
    }
    return 0;
}

/* The original uses Watcom's register ABI with a six-byte far buffer;
 * the final capacity remains on the stack and is callee-popped. */
extern int GetString( WResLangInfo *, HANDLE_INFO *, unsigned, char __far *, int );

int WResLoadString( HANDLE_INFO *instance, unsigned id, char __far *buffer, int capacity )
{
    WResID string_type;
    WResID block_id;
    WResDirWindow window;
    WResLangType language;
    WResLangInfo *resource;

    language.lang = 0;
    language.sublang = 0;
    WResInitIDFromNum( (id >> 4) + 1, &block_id );
    WResInitIDFromNum( 6, &string_type );
    window = WResFindResource( &string_type, &block_id, MainDir, &language );
    if( window.CurrType == 0 || window.CurrRes == 0 || window.CurrLang == 0 ) {
        return -1;
    }
    resource = WResGetLangInfo( window );
    return GetString( resource, instance, id, buffer, capacity );
}

void WResInitIDFromNum( long number, WResID *id )
{
    id->IsName = 0;
    id->ID.Num = number;
}

WResDirWindow WResFindResource( const WResID *type, const WResID *name,
                             WResDir dir, WResLangType *language )
{
    WResDirWindow window;

    window.CurrType = __FindType( type, dir );
    if( window.CurrType != 0 ) {
        window.CurrRes = __FindRes( name, window.CurrType );
        if( window.CurrRes != 0 ) {
            if( language == 0 ) window.CurrLang = window.CurrRes->Head;
            else window.CurrLang = __FindLang( language, window.CurrRes );
        } else {
            window.CurrLang = 0;
            window.CurrType = 0;
        }
    } else {
        window.CurrRes = 0;
        window.CurrLang = 0;
    }
    return window;
}

WResTypeNode *__FindType( const WResID *type, WResDir dir )
{
    WResTypeNode *node;

    for( node = dir->Head; node != 0; node = node->Next ) {
        if( WResIDCmp( type, &node->Info.TypeName ) ) break;
    }
    return node;
}

int WResIDCmp( const WResID *first, const WResID *second )
{
    if( first->IsName && second->IsName ) {
        return WResIDNameCmp( &first->ID.Name, &second->ID.Name ) == 0;
    } else if( !first->IsName && !second->IsName ) {
        return first->ID.Num == second->ID.Num;
    }
    return 0;
}

WResResNode *__FindRes( const WResID *name, WResTypeNode *type )
{
    WResResNode *node;

    for( node = type->Head; node != 0; node = node->Next ) {
        if( WResIDCmp( name, &node->Info.ResName ) ) break;
    }
    return node;
}

WResLangNode *__FindLang( WResLangType *language, WResResNode *resource )
{
    WResLangType neutral;
    WResLangNode *node;

    if( language == 0 ) {
        neutral.lang = 0;
        neutral.sublang = 0;
        language = &neutral;
    }
    node = resource->Head;
    while( node != 0 ) {
        if( node->Info.lang.lang == language->lang &&
            node->Info.lang.sublang == language->sublang ) break;
        node = node->Next;
    }
    return node;
}

WResLangInfo *WResGetLangInfo( WResDirWindow window )
{
    if( window.CurrLang == 0 ) return 0;
    return &window.CurrLang->Info;
}

/* The current WriteHelp->Msg_Get path owns a128B stack buffer. Msg_Get
 * supplies DS and capacity128; WResLoadString forwards the same far pointer.
 * The qualified original GS patch only changes selector transport/stores.
 * Keep the genuine six-byte far ABI in the period build (RET12); the native
 * ABI uses the corresponding flat pointer. Other owners remain unqualified. */
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

int CloseResFile( HANDLE_INFO *instance )
{
    WResFreeDir( MainDir );
    return ResCloseFile( instance->handle );
}

void WResFreeDir( WResDir dir )
{
    if( dir != 0 ) {
        __FreeTypeList( dir );
        (*WResRtns.free)( dir );
    }
}

void __FreeTypeList( WResDir dir )
{
    WResTypeNode *node, *next;

    for( node = dir->Head; node != 0; node = next ) {
        next = node->Next;
        __FreeResList( node );
        (*WResRtns.free)( node );
    }
    dir->Head = 0;
    dir->Tail = 0;
}

void __FreeResList( WResTypeNode *type )
{
    WResResNode *node, *next;

    for( node = type->Head; node != 0; node = next ) {
        next = node->Next;
        __FreeLangList( node );
        (*WResRtns.free)( node );
    }
    type->Head = 0;
    type->Tail = 0;
}

void __FreeLangList( WResResNode *resource )
{
    WResLangNode *node, *next;

    for( node = resource->Head; node != 0; node = next ) {
        next = node->Next;
        (*WResRtns.free)( node );
    }
    resource->Head = 0;
    resource->Tail = 0;
}

int ResCloseFile( int file )
{
    int result;

    result = (*WResRtns.close)( file );
    if( result == -1 ) WresRecordError( 6 );
    return result;
}

/* Initial WRes source, checked against the old WLINK fixed records and nodes. */
void WresRecordError(int status) { (void)status; }

/* WLINK438230..43826d, initial wrrfrr.c: version2 five-byte fixed record. */
int WResReadFixedResRecord(WResResInfo *info, int handle)
{
    int count;
    count = (*WResRtns.read)(handle, info, sizeof(*info));
    if (count != sizeof(*info)) {
        WresRecordError(count == -1 ? 8 : 14);
        return 1;
    }
    return 0;
}

/* WLINK4382d0..438322, initial wrrewrid.c. First name byte is in the fixed ID. */
int WResReadExtraWResID(WResID *id, int handle)
{
    int length, count;
    if (id->IsName) {
        length = (int)id->ID.Name.NumChars - 1;
        if (length > 0) {
            count = (*WResRtns.read)(handle, id->ID.Name.Name + 1, length);
            if (count == -1) {
                WresRecordError(8);
                return 1;
            }
            if (count != length) {
                WresRecordError(14);
                return 1;
            }
        }
    }
    return 0;
}

int WResIDNameCmp(const WResIDName *first, const WResIDName *second)
{
    unsigned length;
    int result;
    length = first->NumChars < second->NumChars ? first->NumChars : second->NumChars;
    result = memicmp(first->Name, second->Name, length);
    if (result == 0) {
        if (first->NumChars == second->NumChars) return 0;
        return first->NumChars > second->NumChars ? 1 : -1;
    }
    return result;
}


int readLangInfoList(int handle, WResResNode *resource)
{
    WResLangNode *node;
    unsigned index;
    int error, count;
    error = 0;
    for (index = 0; index < resource->Info.NumResources; ++index) {
        node = (*WResRtns.alloc)(sizeof(*node));
        if (node == NULL) {
            error = 1;
            WresRecordError(13);
        }
        if (error) break;
        count = (*WResRtns.read)(handle, &node->Info, sizeof(node->Info));
        if (count != sizeof(node->Info)) {
            error = 1;
            WresRecordError(count == -1 ? 8 : 14);
            (*WResRtns.free)(node);
            break;
        }
        node->data = NULL;
        ResAddLLItemAtEnd((void **)&resource->Head, (void **)&resource->Tail, node);
    }
    return error;
}


/* Initial Win16 MRes algorithms, individually checked against the 1994 WLINK
 * call graph and packed14B headers,21B resource/type nodes and25B language nodes.
 * No later Unicode/Win32-resource dispatch is introduced. */
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
                WresRecordError( 9 );
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
    if( error ) WresRecordError( 13 );
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
   eight-byte in-place WResInitIDFromNum entry at0x204b0. */
WResID *WResIDFromNum( long number )
{
    WResID *id;
    if( number < 0 || number > 65535L ) {
        WresRecordError( 1 );
        return( NULL );
    }
    id = (*WResRtns.alloc)( sizeof(WResID) );
    if( id == NULL ) WresRecordError( 13 );
    else WResInitIDFromNum( number, id );
    return( id );
}

WResID *WResIDFromStr( const char *name )
{
    WResID *id;
    unsigned length;
    length = strlen( name );
    if( length > 255 ) {
        WresRecordError( 1 );
        return( NULL );
    }
    id = (*WResRtns.alloc)( sizeof(WResID) + length - 1 );
    if( id == NULL ) WresRecordError( 13 );
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
    if( node == NULL ) WresRecordError( 13 );
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
    if( node == NULL ) WresRecordError( 13 );
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
    if( node == NULL ) WresRecordError( 13 );
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
            WresRecordError( 2 );
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
    WresRecordError( count == -1 ? 8 : 14 );
    return( 1 );
}
int ResReadUint16( unsigned short *value, int handle )
{
    int count;
    count = (*WResRtns.read)( handle, value, 2 );
    if( count == 2 ) return( 0 );
    WresRecordError( count == -1 ? 8 : 14 );
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
        if( error ) WresRecordError( 13 );
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
    if( list == NULL ) WresRecordError( 13 );
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
    if( result == NULL ) WresRecordError( 13 );
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

/* Old16-bit resource units are projected to their low byte, without
 * a host encoding conversion. Initial rrnamoor.c/rrdstr.c role; actual
 * WLINK439138..439218 and439528..439590 are authoritative. */
ResNameOrOrdinal *ResRead32NameOrOrdinal( int handle )
{
    unsigned short flag, ordinal;
    ResNameOrOrdinal *name;
    int error, length;
    char *rest;
    error = ResReadUint16( &flag, handle );
    if( !error ) {
        if( flag == 0xffff ) {
            error = ResReadUint16( &ordinal, handle ); length = 0;
        } else if( flag != 0 ) {
            rest = ResRead32String( handle, &length );
            ++length; error = (rest == NULL);
        } else length = 0;
    }
    if( error ) name = NULL;
    else {
        name = (*WResRtns.alloc)( sizeof(ResNameOrOrdinal) + length );
        error = (name == NULL);
        if( error ) WresRecordError( 13 );
    }
    if( !error ) {
        if( flag == 0xffff ) {
            name->ord.fFlag = 0xff; name->ord.wOrdinalID = ordinal;
        } else {
            name->name[0] = flag;
            if( flag != 0 ) {
                memcpy( name->name + 1, rest, length );
                (*WResRtns.free)( rest );
            }
        }
    }
    return( name );
}
char *ResRead32String( int handle, int *length )
{
    VarString *list;
    unsigned short ch;
    int error;
    char *text;
    list = VarStringStart();
    error = ResReadUint16( &ch, handle );
    while( !error && ch != 0 ) {
        VarStringAddChar( list, (unsigned char)ch );
        error = ResReadUint16( &ch, handle );
    }
    text = VarStringEnd( list, length );
    if( error && text != NULL ) {
        (*WResRtns.free)( text ); text = NULL;
    }
    return( text );
}
