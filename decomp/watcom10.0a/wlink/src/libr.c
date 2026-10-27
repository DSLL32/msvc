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
#include "cmdline.h"
#include <recon.h>

#if defined(__WATCOMC__)
#pragma intrinsic(strlen,memcmp)
#endif



void BurnLibs( void )
{
    file_list *temp;
    omf_dict_entry *dict;

    for( temp = ObjLibFiles; temp != 0; temp = temp->next_file ) {
        dict = temp->u.dict;
        if( dict != 0 ) {
            if( dict->cache != 0 ) {
                FreeDictCache( dict->cache, (dict->pages / 64) + 1 );
            }
            _LnkFree( dict );
            temp->u.dict = 0;
        }
    }
}

mod_entry *SearchLib( file_list *lib, char *name )
{
    mod_entry *mod;
    omf_compare compare;

    if( lib->u.dict == 0 ) {
        if( !CacheOpen( lib ) ) return 0;
        lib->u.dict = ChkLAlloc( sizeof( omf_dict_entry ) );
        lib->u.dict->cache = 0;
        if( !CheckLibraryType( lib ) ) return 0;
    } else {
        if( !CacheOpen( lib ) ) return 0;
    }
    if( LinkFlags & 0x2000 ) compare = OMFCompName;
    else compare = OMFCompIName;
    if( !OMFSearchExtLib( lib, name, compare ) ) return 0;
    mod = NewModEntry();
    mod->location = lib->u.dict->location;
    mod->f.source = lib;
    mod->modinfo = lib->status & 0x1f;
    return mod;
}

#pragma pack(1)
typedef struct omf_lib_header {
    unsigned char command;
    unsigned short length;
    unsigned long dict_start;
    unsigned short dict_pages;
} omf_lib_header;
#pragma pack()

unsigned char CheckLibraryType( file_list *lib )
{
    omf_lib_header *head;
    omf_dict_entry *dict;

    /* The old request is 4 bytes; the underlying cache returns a whole block. */
    head = CacheRead( lib, 0, 4 );
    if( head == 0 || head->command != 0xf0 || head->dict_start == 0
        || head->dict_pages == 0 ) {
        lib->file->flags |= 4;
        Locator( lib->file->name, 0, 0 );
        LnkMsg( 0x330c, 0 );
        return 0;
    }
    dict = lib->u.dict;
    dict->pages = head->dict_pages;
    dict->start = head->dict_start;
    dict->rec_length = head->length + 3;
    return 1;
}

unsigned char OMFSearchExtLib( file_list *lib, char *name, omf_compare compare )
{
    unsigned short minor_class, major_count, major_class, major_inc;
    unsigned short minor_count, sector;
    omf_hash hash;
    omf_dict_entry *dict;

    dict = lib->u.dict;
    major_count = dict->pages;
    HashSymbol( &hash, name );
    major_class = hash.major_class % major_count;
    major_inc = hash.major_inc % major_count;
    if( major_inc == 0 ) major_inc = 1;
    minor_class = hash.minor_class;
    do {
        SetDict( lib, major_class );
        minor_count = 37;
        do {
            if( dict->buffer[minor_class] == 0 ) {
                if( dict->buffer[37] != 0xff ) return 0;
                break;
            }
            sector = compare( name, (char *)dict->buffer, minor_class );
            if( sector != 0 ) {
                dict->location = (unsigned long)dict->rec_length * sector;
                return 1;
            }
            minor_class += hash.minor_inc;
            if( minor_class >= 37 ) minor_class -= 37;
        } while( --minor_count != 0 );
        major_class += major_inc;
        if( major_class >= dict->pages ) major_class -= dict->pages;
    } while( --major_count != 0 );
    return 0;
}

/* These 16-bit rotations were compiler intrinsics in the released source. */
#define ROTL16(v) ((unsigned short)(((unsigned short)(v) << 2) | ((unsigned short)(v) >> 14)))
#define ROTR16(v) ((unsigned short)(((unsigned short)(v) >> 2) | ((unsigned short)(v) << 14)))

void HashSymbol( omf_hash *hash, char *name )
{
    unsigned short major_class, minor_class, major_inc, minor_inc;
    unsigned short count, curr;
    unsigned char *leftptr, *rightptr;

    count = strlen( name );
    leftptr = (unsigned char *)name;
    rightptr = leftptr + count;
    major_class = count | 0x20;
    major_inc = 0;
    minor_class = 0;
    minor_inc = count | 0x20;
    for( ;; ) {
        --rightptr;
        curr = *rightptr | 0x20;
        major_inc = curr ^ ROTL16( major_inc );
        minor_class = curr ^ ROTR16( minor_class );
        if( --count == 0 ) break;
        curr = *leftptr | 0x20;
        ++leftptr;
        major_class = curr ^ ROTL16( major_class );
        minor_inc = curr ^ ROTR16( minor_inc );
    }
    minor_class %= 37;
    minor_inc %= 37;
    if( minor_inc == 0 ) minor_inc = 1;
    hash->major_class = major_class;
    hash->major_inc = major_inc;
    hash->minor_class = minor_class;
    hash->minor_inc = minor_inc;
}

void SetDict( file_list *lib, unsigned short dict_page )
{
    unsigned short pages, num_buckets, residue, bucket;
    unsigned long off;
    omf_dict_entry *dict;

    dict = lib->u.dict;
    if( dict->cache == 0 ) {
        pages = dict->pages;
        num_buckets = pages / 64;
        residue = pages - num_buckets * 64;
        dict->cache = AllocDict( num_buckets, residue );
        if( dict->cache != 0 ) {
            QSeek( lib->file->handle, dict->start, lib->file->name );
            for( bucket = 0; bucket != num_buckets; ++bucket ) {
                QRead( lib->file->handle, dict->cache[bucket],
                    512U * 64, lib->file->name );
            }
            QRead( lib->file->handle, dict->cache[bucket],
                512U * residue, lib->file->name );
            lib->file->currpos = dict->start + 512U * residue
                + 512UL * 64 * num_buckets;
        }
    }
    if( dict->cache == 0 ) {
        off = (unsigned long)dict_page * 512;
        dict->location = dict->start + off;
        dict->buffer = (unsigned char *)TokBuff;
        QSeek( lib->file->handle, dict->location, lib->file->name );
        QRead( lib->file->handle, dict->buffer, 512, lib->file->name );
        lib->file->currpos = dict->location + 512;
    } else {
        bucket = dict_page / 64;
        residue = dict_page - bucket * 64;
        dict->buffer = (unsigned char *)dict->cache[bucket] + residue * 512U;
    }
}

void **AllocDict( unsigned short num_buckets, unsigned short residue )
{
    void **cache;
    unsigned short bucket;

    cache = LAlloc( sizeof( void * ) * (num_buckets + 1) );
    if( cache == 0 ) return 0;
    for( bucket = 0; bucket != num_buckets; ++bucket ) {
        cache[bucket] = LAlloc( 512U * 64 );
        if( cache[bucket] == 0 ) {
            FreeDictCache( cache, bucket );
            return 0;
        }
    }
    if( residue != 0 ) {
        cache[bucket] = LAlloc( residue * 512U );
        if( cache[bucket] == 0 ) {
            FreeDictCache( cache, bucket );
            return 0;
        }
    } else {
        cache[bucket] = 0;
    }
    return cache;
}

unsigned short OMFCompIName( char *name, char *buff, unsigned short index )
{
    unsigned char *entry;
    int len;

    entry = (unsigned char *)buff + (unsigned char)buff[index] * 2;
    len = *entry++;
    while( --len >= 0 ) {
        /* The 1994 hash dictionary folds every byte with OR 0x20, not tolower. */
        if( (*entry | 0x20) != ((unsigned char)*name | 0x20) ) return 0;
        ++entry;
        ++name;
    }
    if( *name == 0 ) return *(unsigned short *)entry;
    return 0;
}

void FreeDictCache( void **cache, unsigned short buckets )
{
    while( buckets != 0 ) _LnkFree( cache[--buckets] );
    _LnkFree( cache );
}

unsigned short OMFCompName( char *name, char *buff, unsigned short index )
{
    unsigned len;

    buff += (unsigned char)buff[index] * 2;
    len = (unsigned char)*buff++;
    if( memcmp( buff, name, len ) == 0 && name[len] == 0 )
        return *(unsigned short *)(buff + len);
    return 0;
}

/* This old build supports only its actual OMF dictionary owner. */
unsigned char DiscardDicts( void )
{
    omf_dict_entry *dict;
    file_list *file;
    dict = 0;
    for( file = ObjLibFiles; file != 0; file = file->next_file ) {
        if( file->u.dict != 0 && file->u.dict->cache != 0 ) dict = file->u.dict;
    }
    if( dict == 0 ) return 0;
    FreeDictCache( dict->cache, (dict->pages / 64) + 1 );
    dict->cache = 0;
    return 1;
}
