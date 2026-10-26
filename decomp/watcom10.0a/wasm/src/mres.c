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
* Description:  Older Win16 resource directory readers and typed owners.
*
****************************************************************************/
/* Initial Open Watcom6b9cb443, bld/sdk/rc/wres/c/{wrreaddi,mrrdrh,
 * wridfnoo,wridfrnu,wridfrst,wraddres,wrfree,rrnamoor,rrdstr,rrint,varstr}.c.
 * Adapted to actual1994 WASM packed IDs/nodes and Win16-only reader graph.
 * CF20aac..20bd2 and its finite source-owned resource closure. */
#include <string.h>
#include "wasm.h"
#include <recon.h>
#if defined(__WATCOMC__)
#pragma intrinsic(strlen,memcpy)
#endif

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
