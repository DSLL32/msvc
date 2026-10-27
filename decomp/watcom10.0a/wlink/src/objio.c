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
#include <stdlib.h>
/* Object input buffer initialization. */
#include "cmdline.h"
#include <recon.h>

#ifdef __WATCOMC__
#pragma intrinsic(strlen,strcpy)
#endif

unsigned TokSize; /* original4500d8 */
char *TokBuff;     /* original4500e4 */

void InitTokBuff( void )
{
    TokSize = 4096;
    TokBuff = ChkLAlloc( 4096 );
}


void FreeTokBuffs( void )
{
    if( TokBuff != 0 ) {
        _LnkFree( TokBuff );
        TokBuff = 0;
    }
}

static infilelist *CachedFiles;      /* original441f10 */
static infilelist *CachedLibFiles;   /* original441f14 */

infilelist *AllocFileEntry( char *name, path_entry *path )
{
    infilelist *entry;

    entry = AllocEntry( name, path );
    entry->next = CachedFiles;
    CachedFiles = entry;
    return entry;
}

infilelist *AllocEntry( char *name, path_entry *path )
{
    infilelist *entry;

    entry = PermAlloc( 30U + strlen( name ) );
    strcpy( entry->name, name );
    entry->path_list = path;
    entry->prefix = 0;
    entry->handle = -1;
    entry->cache = 0;
    entry->flags = 0;
    /* The 1994 entry has an inline name and does not initialize len/currpos. */
    return entry;
}

infilelist *AllocUniqueFileEntry( char *name, path_entry *path )
{
    infilelist *entry;

    for( entry = CachedLibFiles; entry != 0; entry = entry->next ) {
        if( stricmp( entry->name, name ) == 0 ) return entry;
    }
    entry = AllocEntry( name, path );
    if( CachedLibFiles == 0 ) {
        CachedLibFiles = entry;
        LinkList( &CachedFiles, entry );
    } else {
        LinkList( &CachedLibFiles, entry );
    }
    return entry;
}

unsigned char DoObjOpen( infilelist *list )
{
    char *name, *path;
    char new_name[80];
    path_entry *search;
    unsigned char haspath;
    unsigned error;
    int file;

    name = list->name;
    if( list->handle != -1 ) return 1;
    haspath = QHavePath( name );
    if( list->path_list == 0 || haspath ) {
        list->path_list = 0;
        file = QObjOpen( name );
        if( file == -1 && !haspath ) {
            file = TrySearchingLib( name, new_name, list );
        }
    } else if( list->prefix != 0 ) {
        path = list->prefix;
        QMakeFileName( &path, name, new_name );
        file = QObjOpen( new_name );
    } else {
        file = -1;
        if( list->flags & 3 ) file = QObjOpen( name );
        if( file == -1 ) {
            search = list->path_list;
            for(;;) {
                file = PathObjOpen( search->name, name, new_name, list );
                if( file != -1 || !(list->flags & 3) ) break;
                search = search->next;
                if( search == 0 ) {
                    file = TrySearchingLib( name, new_name, list );
                    break;
                }
            }
        }
    }
    if( file != -1 ) {
        list->handle = file;
        return 1;
    }
    error = (list->flags & 0x10) ? 0x3208 : 0x3308;
    PrintIOError( error, "12", name );
    list->prefix = 0;
    list->handle = -1;
    return 0;
}

int TrySearchingLib( char *name, char *buffer, infilelist *list )
{
    if( list->flags & 1 ) {
        return PathObjOpen( getenv( "LIB" ), name, buffer, list );
    }
    return -1;
}

int PathObjOpen( char *path, char *name, char *buffer, infilelist *list )
{
    int file;

    file = -1;
    for( ;; ) {
        list->prefix = path;
        if( !QMakeFileName( &path, name, buffer ) ) break;
        file = QObjOpen( buffer );
        if( file != -1 ) break;
    }
    return file;
}

unsigned long CalcAlign( unsigned long loc, unsigned short align )
{
    unsigned short modulus;

    modulus = loc % align;
    if( modulus != 0 ) modulus = align - modulus;
    return modulus;
}

unsigned char CleanCachedHandles( void )
{
    infilelist *file;

    for( file = CachedFiles; file != 0; file = file->next ) {
        if( !(file->flags & 8) && file->handle != -1 ) break;
    }
    if( file == 0 ) return 0;
    QClose( file->handle, file->name );
    file->currpos = 0;
    file->handle = -1;
    return 1;
}

unsigned char DumpObjCache( void )
{
    infilelist *file;
    for( file = CachedFiles; file != 0; file = file->next ) {
        if( CurrMod == 0 || CurrMod->f.source == 0 || CurrMod->f.source->file != file ) {
            if( FreeCacheBlocks( file, 1 ) ) return 1;
        }
    }
    return 0;
}
