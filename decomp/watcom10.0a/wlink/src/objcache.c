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

#include <stdio.h>
#include <string.h>
#include "cmdline.h"
#include <recon.h>

#ifdef __WATCOMC__
#pragma intrinsic(memcpy)
#endif

unsigned char CacheOpen( file_list *list )
{
    infilelist *file;
    unsigned blocks, i;
    void **cache;

    file = list->file;
    if( file->flags & 4 ) return 0;
    if( !DoObjOpen( file ) ) {
        file->flags |= 4;
        return 0;
    }
    file->flags |= 8;
    if( file->cache == 0 ) {
        file->len = QLSeek( file->handle, 0, SEEK_END, file->name );
        file->currpos = file->len;
        if( file->len == 0 ) {
            file->flags |= 4;
            LnkMsg( 0x3392, "s", file->name );
            return 0;
        }
        blocks = NumCacheBlocks( file->len );
        cache = PermAlloc( blocks * sizeof( void * ) );
        file->cache = cache;
        for( i = 0; i < blocks; ++i ) cache[i] = 0;
    }
    return 1;
}

unsigned NumCacheBlocks( unsigned long size )
{
    unsigned blocks;

    blocks = size >> 13;
    if( size & 0x1fff ) ++blocks;
    return blocks;
}

void *CacheRead( file_list *list, unsigned long pos, unsigned len )
{
    infilelist *file;
    void **cache;
    unsigned first, block, offset, available, count;
    unsigned long blockpos;
    unsigned char multi;
    char *dest;

    file = list->file;
    offset = pos & 0x1fff;
    first = pos >> 13;
    block = first;
    blockpos = (unsigned long)block << 13;
    available = 8192 - offset;
    multi = 0;
    cache = file->cache;
    for(;;) {
        if( cache[block] == 0 ) {
            cache[block] = ChkLAlloc( 8192 );
            if( file->currpos != blockpos ) {
                QSeek( file->handle, blockpos, file->name );
            }
            file->currpos = blockpos + 8192;
            QRead( file->handle, cache[block], 8192, file->name );
        }
        if( available >= len ) break;
        multi = 1;
        blockpos += 8192;
        ++block;
        available += 8192;
    }
    if( !multi ) return (char *)cache[first] + offset;
    if( len > TokSize ) {
        TokSize = (len + 511) & ~511U;
        TokBuff = LnkReAlloc( TokBuff, TokSize );
    }
    count = 8192 - offset;
    memcpy( TokBuff, (char *)cache[first] + offset, count );
    len -= count;
    dest = TokBuff + count;
    ++first;
    while( len > 8192 ) {
        memcpy( dest, cache[first], 8192 );
        len -= 8192;
        dest += 8192;
        ++first;
    }
    memcpy( dest, cache[first], len );
    return TokBuff;
}

unsigned char CacheEnd( file_list *list, unsigned long loc )
{
    return loc >= list->file->len;
}

void CacheClose( file_list *list, unsigned pass )
{
    infilelist *file;
    unsigned char all;

    file = list->file;
    if( file->handle == -1 ) return;
    file->flags &= ~8;
    switch( pass ) {
    case 1:
        FreeCacheBlocks( file, !(file->flags & 2) );
        break;
    case 2:
    case 3:
        all = 1;
        if( pass == 2 ) all = !(file->flags & 2);
        FreeCacheBlocks( file, all );
        QClose( file->handle, file->name );
        file->handle = -1;
        FreeObjCache( list );
        break;
    }
}

unsigned char FreeCacheBlocks( infilelist *file, unsigned char all )
{
    unsigned index, blocks, keep;
    unsigned char freed;
    void **cache;

    freed = 0;
    if( all ) keep = ~0U;
    else keep = file->currpos >> 13;
    if( file->cache != 0 ) {
        blocks = NumCacheBlocks( file->len );
        cache = file->cache;
        for( index = 0; index < blocks; ++index ) {
            if( index != keep && cache[index] != 0 ) {
                _LnkFree( cache[index] );
                cache[index] = 0;
                freed = 1;
            }
        }
    }
    return freed;
}

void FreeObjCache( file_list *list )
{
    FreeCacheBlocks( list->file, 1 );
    list->file->cache = 0;
}
