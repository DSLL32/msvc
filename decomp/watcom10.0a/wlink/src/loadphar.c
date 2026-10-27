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
* Description:  Phar Lap simple and extended executable writers.
*
****************************************************************************/

#include <string.h>
#include "cmdline.h"
#include "exephar.h"
#include <recon.h>

#define HEAD_SIZE 384
#define RTP_SIZE 128
#define NUM_GDT_DESCRIPTORS 7
#define NUM_IDT_DESCRIPTORS 1

typedef enum {
    DR_IS_CODE = 1, DR_IS_APP = 2, DR_IS_USER = 4, DR_BASE = 8, DR_TSS = 16
} desc_flags;

extern unsigned long AppendToLoadFile( char * );

void FiniPharLapLoadFile( void )
{
    unsigned long start;
    start = AppendToLoadFile( FmtData.u.phar.stub );
    if( FmtData.type & 0x2800 ) WritePharExtended( start );
    else WritePharSimple( start );
}

extern address StartInfo;
extern symbol *FindISymbol( char * );
extern unsigned long MemorySize( void );
extern void WriteLoad( void *, unsigned short );
extern void WriteDBI( void );
extern void PadLoad( unsigned long );

void WritePharExtended( unsigned long start )
{
    extended_header header;
    unsigned long   file_size;
    unsigned long   image_offset;
    unsigned long   temp;
    unsigned long   extra;

    QSeek( Root->outfile->handle, start + HEAD_SIZE, Root->outfile->fname );
    file_size = HEAD_SIZE;
    if( FmtData.type & 0x2000 ) {
        header.sit_offset = file_size;
        temp = WriteSIT();
        header.sit_size = temp;
        file_size += temp;
        header.sit_entry_size = sizeof(seg_info_table);
        header.reloc_offset = file_size;
        temp = WritePharRelocs();
        header.reloc_size = temp;
        file_size += temp;
    } else {
        header.sit_offset = 0;
        header.sit_size = 0;
        header.sit_entry_size = 0;
        header.reloc_offset = file_size;
        header.reloc_size = 0;
    }
    header.rtp_offset = file_size;
    temp = WriteRTPBlock();
    header.rtp_size = temp;
    file_size += temp;
    image_offset = file_size;
    if( FmtData.type & 0x2000 ) {
        file_size += WritePharSegData();
    } else {
        file_size += WritePharData( start + image_offset );
    }
    WriteDBI();
    header.signature = 0x3350;
    if( FmtData.type & 0x2000 ) {
        header.format_level = 2;
    } else {
        header.format_level = 1;
    }
    header.header_size = HEAD_SIZE;
    header.file_size = file_size;
    header.checksum = 0;
    header.load_offset = image_offset;
    header.load_size = file_size - image_offset;
    header.sym_offset = 0;
    header.sym_size = 0;
    if( FmtData.type & 0x2000 ) {
        temp = sizeof(TSS);
        header.gdt_offset = temp;
        extra = NUM_GDT_DESCRIPTORS * sizeof(descriptor);
        header.gdt_size = extra;
        temp += extra;
        extra = NUM_IDT_DESCRIPTORS * sizeof(descriptor);
        header.idt_offset = temp;
        header.idt_size = extra;
        temp += extra;
        extra = (NumGroups + 1) * sizeof(descriptor);
        header.ldt_offset = temp;
        header.ldt_size = extra;
        header.tss_offset = 0;
        header.tss_size = sizeof(TSS);
        header.min_extra = 0;
        header.max_extra = 0;
        header.SS = StackAddr.seg;
        header.CS = StartInfo.seg;
        header.LDT = 0x28;
        header.TSS = 0x8;
    } else {
        header.gdt_offset = 0;
        header.gdt_size = 0;
        header.idt_offset = 0;
        header.idt_size = 0;
        header.ldt_offset = 0;
        header.ldt_size = 0;
        header.tss_offset = 0;
        header.tss_size = 0;
        extra = MemorySize() - file_size + image_offset;
        temp = FmtData.u.phar.mindata + extra;
        if( temp < FmtData.u.phar.mindata ) temp = 0xffffffff;
        header.min_extra = temp;
        temp = FmtData.u.phar.maxdata + extra;
        if( temp < FmtData.u.phar.maxdata ) temp = 0xffffffff;
        header.max_extra = temp;
        header.SS = 0;
        header.CS = 0;
        header.LDT = 0;
        header.TSS = 0;
    }
    header.offset = FmtData.base;
    header.ESP = StackAddr.off;
    header.EIP = StartInfo.off;
    header.flags = 0;    // packing not yet implemented.
    header.reserved1 = 0;
    header.stack_size = StackSize;
    header.mem_req = header.load_size;
    QSeek( Root->outfile->handle, start, Root->outfile->fname );
    WriteLoad( &header, sizeof( extended_header ) );
    PadLoad( HEAD_SIZE - sizeof( extended_header ) );
}


unsigned long WriteSIT( void )
{
    group_entry *group;
    seg_info_table sit;
    unsigned long size;

    size = 0;
    sit.flags = 0;
    sit.base = 0;
    sit.extra = 0;
    sit.base = FmtData.base;
    for( group = Groups; group != 0; group = group->next_group ) {
        sit.selector = group->grp_addr.seg;
        sit.extra = group->totalsize - group->size;
        WriteLoad( &sit, sizeof(sit) );
        size += sizeof(sit);
    }
    return size;
}

extern unsigned long DumpMaxRelocList( reloc_info **, unsigned long );

unsigned long WritePharRelocs( void )
{
    reloc_info *temp;
    temp = Root->reloclist;
    return DumpMaxRelocList( &temp, 0 );
}

unsigned long WriteRTPBlock( void )
{
    symbol *sym;
    FmtData.u.phar.params->signature = 0x5844;
    if( FmtData.u.phar.breaksym != 0 ) {
        sym = FindISymbol( FmtData.u.phar.breaksym );
        if( sym == 0 ) {
            LnkMsg( 0x3241, "s", FmtData.u.phar.breaksym );
            FmtData.u.phar.params->realbreak = 0;
        } else {
            FmtData.u.phar.params->realbreak = sym->addr.off;
        }
    }
    WriteLoad( FmtData.u.phar.params, sizeof(rtpblock) );
    PadLoad( RTP_SIZE - sizeof(rtpblock) );
    return RTP_SIZE;
}

unsigned long WritePharSegData( void )
{
    TSS tss;
    group_entry *group;
    unsigned pos, size, flags;

    OrderGroups( CompareProtSegments );
    memset( &tss, 0, sizeof(tss) );
    tss.eip = StartInfo.off;
    tss.esp = StackAddr.off;
    tss.cs = StartInfo.seg;
    tss.ss = StackAddr.seg;
    tss.ds = DataGroup->grp_addr.seg;
    tss.ldt = 0x28;
    WriteLoad( &tss, sizeof(tss) );
    WriteDescriptor( 0, 0, 0 );
    WriteDescriptor( 0, sizeof(tss), DR_TSS );
    WriteDescriptor( 0, sizeof(tss), DR_BASE | DR_IS_APP );
    pos = sizeof(tss);
    size = NUM_GDT_DESCRIPTORS * sizeof(descriptor);
    WriteDescriptor( pos, size, DR_BASE | DR_IS_APP );
    pos += size;
    size = NUM_IDT_DESCRIPTORS * sizeof(descriptor);
    WriteDescriptor( pos, size, DR_BASE | DR_IS_APP );
    pos += size;
    size = (NumGroups + 1) * sizeof(descriptor);
    WriteDescriptor( pos, size, DR_BASE );
    WriteDescriptor( pos, size, DR_BASE | DR_IS_APP );
    WriteDescriptor( 0, 0, 0 );
    WriteDescriptor( 0, 0, 0 );
    pos += size;
    for( group = Groups; group != 0; group = group->next_group ) {
        flags = DR_BASE | DR_IS_APP | DR_IS_USER;
        if( !(group->segflags & 1) ) flags |= DR_IS_CODE;
        WriteDescriptor( pos, group->totalsize, flags );
        pos += group->totalsize;
    }
    for( group = Groups; group != 0; group = group->next_group ) {
        QWrite( CurrSect->outfile->handle, (void *)group->data,
                                 group->size, CurrSect->outfile->fname );
        if( group->totalsize > group->size ) {
            PadLoad( group->totalsize - group->size );
        }
    }
    return pos;
}

void WriteDescriptor( unsigned long base, unsigned long limit, unsigned flags )
{
    descriptor desc;
    desc.baselow = base;
    desc.basemid = base >> 16;
    desc.basehigh = base >> 24;
    desc.bits1 = desc.bits2 = 0;
    if( flags & DR_BASE ) desc.bits1 |= 0x82;
    if( flags & DR_TSS ) desc.bits1 |= 0x89;
    if( flags & DR_IS_CODE ) desc.bits1 |= 8;
    if( flags & DR_IS_APP ) desc.bits1 |= 0x10;
    if( flags & DR_IS_USER ) desc.bits2 |= 0x40;
    if( limit > 0 ) --limit;
    if( limit >= 0x100000UL ) {
        limit >>= 12;
        desc.bits2 |= 0x80;
    }
    desc.limitlow = limit;
    desc.bits2 |= (limit >> 16) & 15;
    WriteLoad( &desc, sizeof(desc) );
}

extern unsigned char CompareOffsets( address *, address * );
extern unsigned char WriteDOSGroup( group_entry * );

unsigned long WritePharData( unsigned long file_pos )
{
    group_entry *group;
    outfilelist *file;
    unsigned char repos;

    OrderGroups( CompareOffsets );
    CurrSect = Root;
    file = Root->outfile;
    file->file_loc = file_pos;
    Root->u.file_loc = file_pos;
    Root->sect_addr.off = Groups->grp_addr.off;
    Root->sect_addr.seg = Groups->grp_addr.seg;
    for( group = Groups; group != 0; group = group->next_group ) {
        repos = WriteDOSGroup( group );
        if( repos ) QSeek( file->handle, file->file_loc, file->fname );
    }
    return file->file_loc - file_pos;
}

void WritePharSimple( unsigned long start )
{
    simple_header header;
    unsigned long file_size, header_size, extra, temp;

    if( FmtData.type & 0x1000 ) {
        QSeek( Root->outfile->handle, start + sizeof(header), Root->outfile->fname );
        extra = start + sizeof(header) + WritePharRelocs();
        header_size = (extra + 15) & ~15UL;
        PadLoad( header_size - extra );
    } else {
        QSeek( Root->outfile->handle, start + sizeof(header) + 2, Root->outfile->fname );
        header_size = sizeof(header) + 2;
    }
    file_size = header_size + WritePharData( start + header_size );
    WriteDBI();
    header.signature = (FmtData.type & 0x0400) ? 0x504d : 0x514d;
    header.mod_size = file_size % 512;
    header.file_size = (file_size + 511) / 512;
    header.num_relocs = Root->relocs;
    header.hdr_size = header_size / 16;
    extra = MemorySize() - file_size + header_size + 0xfff;
    temp = FmtData.u.phar.mindata + extra;
    if( temp < FmtData.u.phar.mindata ) temp = 0xffffffffUL;
    header.min_data = temp >> 12;
    temp = FmtData.u.phar.maxdata + extra;
    if( temp < FmtData.u.phar.maxdata ) temp = 0xffffffffUL;
    header.max_data = temp >> 12;
    header.ESP = StackAddr.off;
    header.checksum = 0;
    header.EIP = StartInfo.off;
    header.reloc_offset = 0x1e;
    header.overlay_num = 0;
    header.always_one = (header_size >> 20) + 1;
    QSeek( Root->outfile->handle, start, Root->outfile->fname );
    WriteLoad( &header, sizeof(header) );
    if( FmtData.type & 0x0400 ) PadLoad( 2 );
}
