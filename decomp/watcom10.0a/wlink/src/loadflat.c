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


/*
   LOADFLAT : utilities for creation of OS2 FLAT (V2.X) executable files.
*/

#include <string.h>
#include "cmdline.h"
#include "exeflat.h"
#include <recon.h>

extern unsigned long Write_Stub_File( void );
extern unsigned long WriteObjectTables( os2_flat_header *, unsigned long );
extern unsigned long ResNonResNameTable( unsigned char );
extern unsigned long DumpFlatEntryTable( void );
extern unsigned long WriteFixupTables( os2_flat_header *, unsigned long );
extern unsigned long ImportModTable( unsigned long * );
extern unsigned long ImportProcTable( unsigned long * );
/* File-local loadflat WriteDataPages has a different ABI from loadpe's. */
extern unsigned long WriteFlatDataPages( void );
extern unsigned long NullAlign( unsigned long );
extern void PadLoad( unsigned long );
extern void WriteLoad( void *, unsigned );
extern void WriteDBI( void );

void FiniOS2FlatLoadFile( void )
{
    os2_flat_header header;
    unsigned long stub_len, curr, count, last;

    memset( &header, 0, sizeof(header) );
    CurrSect = Root;
    QSeek( Root->outfile->handle, 0, Root->outfile->fname );
    stub_len = Write_Stub_File();
    curr = sizeof(header);
    QSeek( Root->outfile->handle, stub_len + sizeof(header), Root->outfile->fname );
    curr += WriteObjectTables( &header, curr );
    header.resname_off = curr;
    curr += ResNonResNameTable( 1 );
    header.num_rsrcs = 0;
    header.entry_off = curr;
    header.rsrc_off = header.resname_off;
    curr += DumpFlatEntryTable();
    header.moddir_off = 0;
    curr += WriteFixupTables( &header, curr );
    header.impmod_off = curr;
    curr += ImportModTable( &count );
    header.num_impmods = count;
    PadLoad( 1 );
    header.impproc_off = curr;
    ++curr;
    curr += ImportProcTable( &count );
    header.fixup_size = curr - header.fixpage_off;
    header.loader_size = curr - header.objtab_off;
    header.page_off = NullAlign( 512 );
    last = WriteFlatDataPages();
    header.l.last_page = (FmtData.type & 2) ? last : FmtData.u.os2.segment_shift;
    header.nonres_off = NullAlign( 1 );
    header.nonres_size = ResNonResNameTable( 0 );
    if( header.nonres_size == 0 ) header.nonres_off = 0;
    WriteDBI();
    header.signature = (FmtData.type & 2) ? 0x454c : 0x584c;
    header.byte_order = 0;
    header.word_order = 0;
    header.level = 0;
    header.cpu_type = FmtData.cpu_type > 3 ? 3 : 2;
    header.os_type = 1;
    if( FmtData.minor < 10 ) FmtData.minor *= 10;
    header.version = FmtData.major * 100 + FmtData.minor;
    if( FmtData.flags & 1 ) {
        header.flags |= 0x8000;
        if( FmtData.u.os2.flags & 4 ) header.flags |= 4;
        if( FmtData.u.os2.flags & 0x8000 ) header.flags |= 0x40000000UL;
    } else if( FmtData.u.os2.flags & 0x0400 ) {
        header.flags |= 0x020000UL;
    } else if( FmtData.u.os2.flags & 0x0800 ) {
        header.flags |= 0x028000UL;
    }
    if( FmtData.u.os2.flags & 0x4000 ) header.flags |= 0x0100;
    else if( FmtData.u.os2.flags & 0x20 ) header.flags |= 0x0300;
    else header.flags |= 0x0200;
    if( LinkState & 8 ) header.flags |= 0x2000;
    if( (FmtData.type & 4) && ((!(LinkFlags & 0x08000000UL)) ^ ((LinkFlags & 1) != 0)) )
        header.flags |= 0x10;
    header.page_size = 4096;
    header.heapsize = FmtData.u.os2.heapsize;
    header.num_preload = 0;
    header.num_inst_preload = 0;
    header.num_inst_demand = 0;
    QSeek( Root->outfile->handle, stub_len, Root->outfile->fname );
    WriteLoad( &header, sizeof(header) );
}

#pragma pack(1)
typedef struct object_record {
    unsigned long size, addr, flags, mapidx, mapsize, reserved;
} object_record;
#pragma pack()
extern address StackAddr, StartInfo;
extern unsigned NumberBuf( unsigned long *, unsigned long, void * );
unsigned long WriteObjectTables( os2_flat_header *header, unsigned long loc )
{
    unsigned long pages, objects, size, start, left;
    unsigned map_size;
    group_entry *group;
    object_record obj;

    header->objtab_off = loc;
    pages = objects = 0;
    obj.reserved = 0;
    header->autodata_obj = DataGroup != 0 ? DataGroup->grp_addr.seg : 0;
    for( group = Groups; group != 0; group = group->next_group ) {
        if( group->grp_addr.seg == StackAddr.seg ) {
            header->stack_obj = StackAddr.seg;
            header->esp = StackAddr.off - group->grp_addr.off;
        }
        if( group->grp_addr.seg == StartInfo.seg ) {
            header->start_obj = StartInfo.seg;
            header->eip = StartInfo.off - group->grp_addr.off;
        }
        obj.size = group->totalsize;
        obj.addr = group->linear;
        obj.flags = 0;
        if( group->leaders[0]->info & 0x0400 ) obj.flags |= 0x2000;
        else if( group->info & 1 ) obj.flags |= 0x1000;
        if( (group->segflags & 0x0c00) != 0x0c00 ) obj.flags |= 0x8000;
        if( group->segflags & 1 ) {
            obj.flags |= 1;
            if( !(group->segflags & 0x80) ) obj.flags |= 2;
        } else {
            obj.flags |= 4;
            if( !(group->segflags & 0x80) ) obj.flags |= 1;
        }
        if( group->segflags & 0x1000 ) obj.flags |= 0x10;
        if( group->segflags & 0x20 ) obj.flags |= 0x20;
        if( group->segflags & 0x40 ) obj.flags |= 0x40;
        if( group->segflags & 0x0200 ) obj.flags |= 0x4000;
        if( group->segflags & 0x8000 ) obj.flags |= 0x80;
        obj.flags |= (group->segflags & 6) << 7;
        obj.mapidx = pages + 1;
        obj.mapsize = (group->size + 4095) >> 12;
        WriteLoad( &obj, sizeof(obj) );
        pages += obj.mapsize;
        ++objects;
    }
    header->num_objects = objects;
    header->num_pages = pages;
    size = objects * sizeof(obj);
    header->objmap_off = loc + size;
    start = 0;
    for( group = Groups; group != 0; group = group->next_group ) {
        left = group->size;
        while( left > 0x200000UL ) {
            map_size = NumberBuf( &start, 0x200000UL, TokBuff );
            size += map_size;
            WriteLoad( TokBuff, (unsigned short)map_size );
            left -= 0x200000UL;
        }
        map_size = NumberBuf( &start, left, TokBuff );
        size += map_size;
        WriteLoad( TokBuff, (unsigned short)map_size );
    }
    return size;
}

#pragma pack(1)
typedef struct le_map_entry { unsigned char page[3], flags; } le_map_entry;
typedef struct lx_map_entry {
    unsigned long offset;
    unsigned short size, flags;
} lx_map_entry;
#pragma pack()
unsigned NumberBuf( unsigned long *start, unsigned long limit, void *buffer )
{
    unsigned long pages, mask;
    unsigned size, shift;
    le_map_entry *le;
    lx_map_entry *lx;

    pages = (limit + 4095) >> 12;
    if( FmtData.type & 2 ) {
        size = pages * sizeof(*le);
        le = buffer;
        while( pages > 0 ) {
            ++*start;
            le->page[2] = *start;
            le->page[1] = *start >> 8;
            le->page[0] = *start >> 16;
            le->flags = 0;
            ++le;
            --pages;
        }
    } else {
        shift = FmtData.u.os2.segment_shift;
        size = pages * sizeof(*lx);
        lx = buffer;
        mask = (1UL << shift) - 1;
        while( limit > 0 ) {
            lx->offset = *start >> shift;
            if( limit > 4096 ) {
                lx->size = 4096;
                limit -= 4096;
            } else {
                lx->size = (limit + mask) & ~mask;
                limit = 0;
            }
            *start += lx->size;
            lx->flags = 0;
            ++lx;
        }
    }
    return size;
}

/* The 1994 table contains only 32-bit entries; forwarder bundles were
 * introduced in the later source. Null bundles account for ordinal gaps. */
#pragma pack(1)
typedef struct flat_bundle_prefix {
    unsigned char count, type;
    unsigned short object;
} flat_bundle_prefix;
typedef struct flat_bundle_entry32 {
    unsigned char flags;
    unsigned long offset;
} flat_bundle_entry32;
#pragma pack()
unsigned long DumpFlatEntryTable( void )
{
    entry_export *start, *place, *prev;
    unsigned prevord, gap, entries;
    unsigned long size;
    flat_bundle_prefix prefix;
    flat_bundle_entry32 item;

    size = 0;
    place = FmtData.u.os2.exports;
    prevord = 0;
    while( place != NULL ) {
        gap = place->ordinal - prevord;
        if( gap > 1 ) {
            --gap;
            prefix.count = 255;
            prefix.type = 0;
            while( gap > 255 ) {
                WriteLoad( &prefix, 2 );
                gap -= 255;
                size += 2;
            }
            prefix.count = gap;
            WriteLoad( &prefix, 2 );
            size += 2;
        }
        entries = 1;
        prev = start = place;
        place = place->next;
        while( place != NULL ) {
            if( entries >= 255 ) break;
            if( place->addr.seg != start->addr.seg ) break;
            if( (int)place->ordinal - (int)prev->ordinal > 1 ) break;
            ++entries;
            prev = place;
            place = place->next;
        }
        prefix.count = entries;
        prefix.type = 3;
        prefix.object = start->addr.seg;
        size += entries * sizeof(item);
        prevord = prev->ordinal;
        WriteLoad( &prefix, sizeof(prefix) );
        size += sizeof(prefix);
        for( ; entries > 0; --entries, start = start->next ) {
            item.flags = start->iopl_words << 2;
            if( start->flags & 2 ) item.flags |= 1;
            item.offset = start->addr.off;
            WriteLoad( &item, sizeof(item) );
        }
    }
    PadLoad( 1 );
    return size + 1;
}

extern unsigned long WriteRelocSize( reloc_info ***, unsigned long, unsigned );
extern unsigned char DumpRelocList( reloc_info * );
unsigned long WriteFixupTables( os2_flat_header *header, unsigned long loc )
{
    unsigned long size, pages, entries, high;
    unsigned low;
    group_entry *group;
    reloc_info ***list;

    header->fixpage_off = loc;
    entries = size = 0;
    for( group = Groups; group != NULL; group = group->next_group ) {
        list = group->grp_relocs;
        pages = (group->size + 4095) >> 12;
        entries += pages;
        high = (pages & 0xfffffc00UL) >> 10;
        while( high > 0 ) {
            size = WriteRelocSize( list, size, 1024 );
            --high;
            if( list != NULL ) ++list;
        }
        low = pages & 1023;
        if( low > 0 ) size = WriteRelocSize( list, size, low );
    }
    WriteLoad( &size, sizeof(size) );
    ++entries;
    header->fixrec_off = loc + entries * sizeof(size);
    size += entries * sizeof(size);
    for( group = Groups; group != NULL; group = group->next_group ) {
        TraverseOS2RelocList( group, DumpRelocList );
    }
    return size;
}

extern unsigned long RelocSize( reloc_info * );
unsigned long WriteRelocSize( reloc_info ***list, unsigned long size, unsigned limit )
{
    reloc_info **rptr;

    rptr = list != NULL ? *list : NULL;
    while( limit > 0 ) {
        WriteLoad( &size, sizeof(size) );
        if( rptr != NULL ) {
            size += RelocSize( *rptr++ );
            size += RelocSize( *rptr++ );
        }
        --limit;
    }
    return size;
}

unsigned long WriteFlatDataPages( void )
{
    group_entry *group;
    unsigned long last, mask, padding;

    last = 0;
    for( group = Groups; group != NULL; group = group->next_group ) {
        if( group->size != 0 ) {
            if( last != 0 ) {
                if( FmtData.type & 2 ) padding = 4096 - last;
                else {
                    mask = (1UL << FmtData.u.os2.segment_shift) - 1;
                    padding = ((last + mask) & ~mask) - last;
                }
                PadLoad( padding );
            }
            QWrite( CurrSect->outfile->handle, (void *)group->data,
                    group->size, CurrSect->outfile->fname );
            last = group->size & 4095;
        }
    }
    /* This old tail tests the OS/2 option word, not FmtData.type as in
     * the later source, and does not change a zero last-page result. */
    if( !(FmtData.u.os2.flags & 2) && last != 0 ) {
        mask = (1UL << FmtData.u.os2.segment_shift) - 1;
        PadLoad( ((last + mask) & ~mask) - last );
    }
    return last;
}
