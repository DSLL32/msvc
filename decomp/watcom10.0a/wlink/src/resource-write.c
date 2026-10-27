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


/* Open Watcom initial6b9cb4438959d845a2b4dd4bf19f7908ecd608e9.
   Actual WLINK438770..438b76; exact bodies are bound in the writer source packet.
   Initial bld/sdk/rc/wres/c/write.c adapted to
   actual1994 bodies. ResWriteString directly owns conversion/write/free;
   the later ResWriteStringLen vertex and MRes Win32 wrapper are absent. */
#include <stdio.h>
#include <string.h>
#include "resource.h"
#include <recon.h>
#if defined(__WATCOMC__)
#pragma intrinsic(memcpy,strlen)
#endif
enum { WRS_WRITE_FAILED=7, WRS_SEEK_FAILED=9, WRS_MALLOC_FAILED=13 };
static char ConvBuffer[512];       /* Original shared conversion buffer WLINK4503e8. */
int (*ConvToUnicode)( int, const char *, char * ) = DefaultConversion;
int DefaultConversion( int length, const char *source, char *buffer )
{
    int i;
    if( buffer != NULL ) {
        for( i = 0; i < length; ++i ) {
            buffer[2*i] = source[i];
            buffer[2*i+1] = 0;
        }
    }
    return( length * 2 );
}
int ResWriteUint8( const unsigned char *value, int handle )
{
    if( (*WResRtns.write)( handle, value, 1 ) != 1 ) {
        WresRecordError( WRS_WRITE_FAILED ); return( 1 );
    }
    return( 0 );
}
int ResWriteUint16( const unsigned short *value, int handle )
{
    if( (*WResRtns.write)( handle, value, 2 ) != 2 ) {
        WresRecordError( WRS_WRITE_FAILED ); return( 1 );
    }
    return( 0 );
}
int ResWriteUint32( const unsigned long *value, int handle )
{
    if( (*WResRtns.write)( handle, value, 4 ) != 4 ) {
        WresRecordError( WRS_WRITE_FAILED ); return( 1 );
    }
    return( 0 );
}
int WResWriteWResIDNameUni( const WResIDName *name, unsigned char unicode, int handle )
{
    int length;
    unsigned char count;
    const char *buffer;
    int error;
    count = (name == NULL) ? 0 : name->NumChars;
    if( unicode ) {
        buffer = ConvBuffer;
        length = (*ConvToUnicode)( count, name->Name, ConvBuffer ) / 2;
        /* The old work value is an int; the file count is its Win32 low word. */
        error = ResWriteUint16( (const unsigned short *)&length, handle );
        length *= 2;
    } else {
        length = count;
        buffer = name->Name;
        error = ResWriteUint8( &count, handle );
    }
    if( !error && count > 0 ) {
        if( (*WResRtns.write)( handle, buffer, length ) != length ) {
            WresRecordError( WRS_WRITE_FAILED ); error = 1;
        }
    }
    return( error );
}
int WResWriteWResIDName( const WResIDName *name, int handle )
{
    return( WResWriteWResIDNameUni( name, 0, handle ) );
}
int WResWriteWResID( const WResID *id, int handle )
{
    int error;
    error = ResWriteUint8( &id->IsName, handle );
    if( !error ) {
        if( id->IsName ) error = WResWriteWResIDName( &id->ID.Name, handle );
        else error = ResWriteUint16( &id->ID.Num, handle );
    }
    return( error );
}
int WResWriteTypeRecord( const WResTypeInfo *info, int handle )
{
    unsigned length;
    length = info->TypeName.IsName ? sizeof(WResTypeInfo) + info->TypeName.ID.Name.NumChars - 1 : sizeof(WResTypeInfo);
    if( (*WResRtns.write)( handle, info, length ) != length ) {
        WresRecordError( WRS_WRITE_FAILED ); return( 1 );
    }
    return( 0 );
}
int WResWriteResRecord( const WResResInfo *info, int handle )
{
    unsigned length;
    length = info->ResName.IsName ? sizeof(WResResInfo) + info->ResName.ID.Name.NumChars - 1 : sizeof(WResResInfo);
    if( (*WResRtns.write)( handle, info, length ) != length ) {
        WresRecordError( WRS_WRITE_FAILED ); return( 1 );
    }
    return( 0 );
}
int WResWriteLangRecord( const WResLangInfo *info, int handle )
{
    if( (*WResRtns.write)( handle, info, sizeof(WResLangInfo) ) != sizeof(WResLangInfo) ) {
        WresRecordError( WRS_WRITE_FAILED ); return( 1 );
    }
    return( 0 );
}
int WResWriteHeaderRecord( const WResHeader *info, int handle )
{
    int error;
    error = ((*WResRtns.seek)( handle, 0L, SEEK_SET ) == -1L);
    if( error ) WresRecordError( WRS_SEEK_FAILED );
    if( !error && (*WResRtns.write)( handle, info, sizeof(WResHeader) ) != sizeof(WResHeader) ) {
        WresRecordError( WRS_WRITE_FAILED ); error = 1;
    }
    return( error );
}
int WResWriteExtHeader( const WResExtHeader *info, int handle )
{
    if( (*WResRtns.write)( handle, info, sizeof(WResExtHeader) ) != sizeof(WResExtHeader) ) {
        WresRecordError( WRS_WRITE_FAILED ); return( 1 );
    }
    return( 0 );
}
int ResWriteString( const char *text, unsigned char unicode, int handle )
{
    int length;
    char *buffer;
    int error;
    if( text == NULL ) text = "";
    length = strlen( text ) + 1;
    if( unicode ) {
        if( length * 2 > sizeof(ConvBuffer) ) buffer = (*WResRtns.alloc)( length * 2 );
        else buffer = ConvBuffer;
        length = (*ConvToUnicode)( length, text, buffer );
    } else buffer = (char *)text;
    if( (*WResRtns.write)( handle, buffer, length ) != length ) {
        WresRecordError( WRS_WRITE_FAILED ); error = 1;
    } else error = 0;
    if( buffer != ConvBuffer && buffer != text ) (*WResRtns.free)( buffer );
    return( error );
}
int ResWriteNameOrOrdinal( const ResNameOrOrdinal *name, unsigned char unicode, int handle )
{
    unsigned short flag;
    int error;
    if( name == NULL ) return( ResWriteString( "", unicode, handle ) );
    if( name->ord.fFlag == 0xff ) {
        if( unicode ) {
            flag = 0xffff;
            error = ResWriteUint16( &flag, handle );
        } else error = ResWriteUint8( &name->ord.fFlag, handle );
        if( !error ) ResWriteUint16( &name->ord.wOrdinalID, handle );
    } else error = ResWriteString( name->name, unicode, handle );
    return( error );
}
int MResWriteResourceHeader( const MResResourceHeader *info, int handle )
{
    int error;
    error = ResWriteNameOrOrdinal( info->Type, 0, handle );
    if( !error ) error = ResWriteNameOrOrdinal( info->Name, 0, handle );
    if( !error ) error = ResWriteUint16( &info->MemoryFlags, handle );
    if( !error ) error = ResWriteUint32( &info->Size, handle );
    return( error );
}
