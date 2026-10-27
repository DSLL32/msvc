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

#ifdef __WATCOMC__
#pragma intrinsic(memcpy)
#endif

reloc_info *FloatFixups;       /* original4501e8 */

void FreeRelocInfo( void )
{
    group_entry *group;
    unsigned long pages, blocks;
    void **table;

    if( !(LinkState & 1) ) return;
    if( FmtData.type & 0x0016 ) {
        for( group = Groups; group != 0; group = group->next_group ) {
            TraverseOS2RelocList( group, FreeRelocList );
            table = group->grp_relocs;
            if( table != 0 ) {
                pages = (group->totalsize + 4095) >> 12;
                blocks = (pages >> 10) + ((pages & 1023) != 0);
                while( blocks != 0 ) {
                    _LnkFree( *table++ );
                    --blocks;
                }
            }
        }
    } else if( FmtData.type & 0x4209 ) {
        for( group = Groups; group != 0; group = group->next_group ) {
            FreeRelocList( group->grp_relocs );
        }
    } else if( Root != 0 ) {
        FreeRelocSect( Root );
        ProcAllOvl( FreeRelocSect );
    }
    if( FmtData.type & 0x4200 ) {
        FreeRelocList( FloatFixups );
        FreeRelocSect( Root );
    }
}

#pragma pack(1)
struct reloc_info {
    struct reloc_info *next;
    unsigned short sizeleft;
    union { void *addr; unsigned long spill; } loc;
};
#pragma pack()

unsigned char FreeRelocList( reloc_info *list )
{
    while( list != 0 ) {
        if( !(list->sizeleft & 0x8000) ) {
            _LnkFree( list->loc.addr );
        }
        list = list->next;
    }
    return 0;
}

void FreeRelocSect( section *sec )
{
    FreeRelocList( sec->reloclist );
}

unsigned FmtRelocSize;          /* original4501ec */

void SetRelocSize( void )
{
    if( FmtData.type & 0x000f ) FmtRelocSize = 8;
    else if( FmtData.type & 0x0010 ) FmtRelocSize = 2;
    else if( FmtData.type & 0x0100 ) FmtRelocSize = 4;
    else if( FmtData.type & 0x1000 ) FmtRelocSize = 4;
    else if( FmtData.type & 0x4200 ) FmtRelocSize = 6;
    else if( FmtData.type & 0x8000 ) FmtRelocSize = 8;
    else if( FmtData.type & 0x2000 ) FmtRelocSize = 6;
    else FmtRelocSize = 4;
}

typedef struct os2_reloc_header {
    reloc_info *externals, *internals;
} os2_reloc_header;
extern reloc_info ***PERelocInit( unsigned long );
extern os2_reloc_header **OS2FlatRelocInit( unsigned long );
extern void DoWriteReloc( reloc_info **, void *, unsigned short );

void WriteReloc( group_entry *group, unsigned long off, void *reloc, unsigned short size )
{
    os2_reloc_header **pages;
    reloc_info ***table;
    reloc_info **head;
    unsigned long idx;
    unsigned char *item;

    if( FmtData.type & 0x0010 ) {
        table = group->grp_relocs;
        if( table == 0 ) {
            table = PERelocInit( group->totalsize );
            group->grp_relocs = table;
        }
        idx = (off - group->grp_addr.off) >> 12;
        head = &table[idx >> 10][idx & 1023];
    } else if( FmtData.type & 6 ) {
        pages = group->grp_relocs;
        if( pages == 0 ) {
            pages = OS2FlatRelocInit( group->totalsize );
            group->grp_relocs = pages;
        }
        idx = (off - group->grp_addr.off) >> 12;
        head = &pages[idx >> 10][idx & 1023].externals;
        item = reloc;
        if( !(item[1] & 3) && (item[0] == 5 || item[0] == 7 || item[0] == 8) )
            head = &pages[idx >> 10][idx & 1023].internals;
    } else if( FmtData.type & 9 ) {
        head = (reloc_info **)&group->grp_relocs;
    } else {
        head = (reloc_info **)&group->section->reloclist;
    }
    DoWriteReloc( head, reloc, size );
    ++group->section->relocs;
}

extern void **OS2PagedRelocInit( unsigned long, unsigned );

reloc_info ***PERelocInit( unsigned long size )
{
    return (reloc_info ***)OS2PagedRelocInit( size, sizeof(reloc_info *) );
}

void **OS2PagedRelocInit( unsigned long size, unsigned unit )
{
    void **mem, **start;
    unsigned long pages, high, low;
    unsigned bytes;

    pages = (size + 4095) >> 12;
    high = pages >> 10;
    mem = PermAlloc( (high + 1) * sizeof(void *) );
    start = mem;
    bytes = 1024 * unit;
    while( high > 0 ) {
        *mem = ChkLAlloc( bytes );
        memset( *mem, 0, bytes );
        ++mem;
        --high;
    }
    low = pages & 1023;
    if( low != 0 ) {
        bytes = low * unit;
        *mem = ChkLAlloc( bytes );
        memset( *mem, 0, bytes );
    }
    return start;
}

extern reloc_info *AllocRelocInfo( void );
extern void SpillWrite( unsigned long, unsigned long, void *, unsigned );

void DoWriteReloc( reloc_info **list, void *reloc, unsigned short size )
{
    reloc_info *info;
    unsigned offset;

    info = *list;
    if( info == 0 ) {
        info = AllocRelocInfo();
        info->next = 0;
        *list = info;
    }
    if( (info->sizeleft & 0x7fff) < size ) {
        info = AllocRelocInfo();
        info->next = *list;
        *list = info;
    }
    offset = 512 - (info->sizeleft & 0x7fff);
    if( info->sizeleft & 0x8000 ) SpillWrite( info->loc.spill, offset, reloc, size );
    else memcpy( (char *)info->loc.addr + offset, reloc, size );
    info->sizeleft -= size;
}

extern unsigned long SpillAlloc( unsigned );

reloc_info *AllocRelocInfo( void )
{
    reloc_info *info;

    info = PermAlloc( sizeof(*info) );
    info->sizeleft = 512;
    info->loc.addr = LAlloc( 512 );
    if( info->loc.addr == 0 ) {
        info->loc.spill = SpillAlloc( 512 );
        info->sizeleft |= 0x8000;
    }
    return info;
}

unsigned long RelocSize( reloc_info *list )
{
    unsigned long total;

    total = 0;
    for( ; list != 0; list = list->next ) {
        total += 512 - (list->sizeleft & 0x7fff);
    }
    return total;
}

extern unsigned long DumpMaxRelocList( reloc_info **, unsigned long );

unsigned char DumpRelocList( reloc_info *list )
{
    DumpMaxRelocList( &list, 0 );
    return 0;
}

extern void SpillRead( unsigned long, unsigned long, void *, unsigned );

unsigned long DumpMaxRelocList( reloc_info **head, unsigned long max )
{
    unsigned long total, size;
    reloc_info *list;

    total = 0;
    list = *head;
    while( list != 0 ) {
        size = 512 - (list->sizeleft & 0x7fff);
        if( max != 0 && total != 0 && total + size >= max ) break;
        if( size != 0 ) {
            if( list->sizeleft & 0x8000 ) {
                SpillRead( list->loc.spill, 0, TokBuff, size );
                QWrite( CurrSect->outfile->handle, TokBuff, size, CurrSect->outfile->fname );
            } else {
                QWrite( CurrSect->outfile->handle, list->loc.addr, size, CurrSect->outfile->fname );
            }
        }
        list = list->next;
        total += size;
    }
    *head = list;
    return total;
}

extern unsigned char TraverseRelocBlock( reloc_info **, unsigned,
    unsigned char (*)(reloc_info *) );

unsigned char TraverseOS2RelocList( group_entry *group,
    unsigned char (*fn)(reloc_info *) )
{
    reloc_info ***list;
    unsigned long index, high;
    unsigned low;

    list = group->grp_relocs;
    if( list != 0 ) {
        index = (group->totalsize + 4095) >> 12;
        high = index >> 10;
        while( high > 0 ) {
            if( TraverseRelocBlock( *list, 1024, fn ) ) return 1;
            --high;
            ++list;
        }
        low = index & 1023;
        if( low > 0 ) return TraverseRelocBlock( *list, low, fn );
    }
    return 0;
}

unsigned char TraverseRelocBlock( reloc_info **list, unsigned num,
    unsigned char (*fn)(reloc_info *) )
{
    while( num > 0 ) {
        if( fn( *list++ ) ) return 1;
        if( FmtData.type & 6 ) {
            if( fn( *list++ ) ) return 1;
        }
        --num;
    }
    return 0;
}

os2_reloc_header **OS2FlatRelocInit( unsigned long size )
{
    return (os2_reloc_header **)OS2PagedRelocInit( size, sizeof(os2_reloc_header) );
}

void QNXLinearReloc( group_entry *group, void *data )
{
    DoWriteReloc( (reloc_info **)&group->grp_relocs, data, 4 );
}

/* QNX floating-emulation relocations use the actual packed six-byte item. */
void FloatReloc( void *data )
{
    DoWriteReloc( &FloatFixups, data, 6 );
}
