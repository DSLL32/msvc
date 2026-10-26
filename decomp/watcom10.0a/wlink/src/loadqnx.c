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
* Description:  Old QNX load records, group buffers, relocations and resources.
*
****************************************************************************/

#include <string.h>
#include <malloc.h>
#include "cmdline.h"
#include "exeqnx.h"
#include <recon.h>
#ifndef __linux__
#pragma intrinsic(strlen,memcpy)
void reconstruction_trap( void );
#pragma aux reconstruction_trap = 0xcc;
#define TRAP() reconstruction_trap()
#else
#define TRAP() __asm__ __volatile__( "int3" )
#endif

unsigned short ToQNXSel( unsigned short number )
{
    return ((number - 1) << 3) | 4 | FmtData.u.qnx.priv_level;
}

void SetQNXSegFlags( void )
{
    SetSegFlags( FmtData.u.qnx.seg_flags );
    FmtData.u.qnx.seg_flags = 0;
    SetQNXGroupFlags();
}

void SetQNXGroupFlags( void )
{
    group_entry *group;
    unsigned short flags;
    int index;
    for( group = Groups; group != 0; group = group->next_group ) {
        group->u.qnxflags = (group->segflags & 1) ? 1 : 3;
        for( index = 0; index < group->num; ++index ) {
            flags = group->leaders[index]->u.p1.segflags;
            if( flags >= 16 || !(flags & 1) ) {
                group->u.qnxflags &= ~1;
                break;
            }
        }
    }
}

unsigned short ToQNXIndex( unsigned short selector )
{
    return selector >> 3;
}

extern address StartInfo;
extern void WriteLoad( void *, unsigned short );
extern void WriteDBI( void );

void FiniQNXLoadFile( void )
{
    unsigned long *segments;
    unsigned short nbytes;
    unsigned adseg;
    lmf_record record;
    lmf_header header;
    lmf_eof trailer;

    CurrSect = Root;
    nbytes = NumGroups * sizeof(unsigned long);
    segments = alloca( nbytes );
    QSeek( Root->outfile->handle, sizeof(header) + nbytes + sizeof(record),
                                                   Root->outfile->fname );
    WriteQNXResource();
    WriteQNXData( segments );
    WriteQNXRelocs( Root->reloclist, 3, 0 );
    WriteQNXRelocs( FloatFixups, 4, 0 );
    record.reserved = record.spare = 0;
    record.rec_type = 5;
    record.data_nbytes = sizeof(trailer);
    WriteLoad( &record, sizeof(record) );
    memset( &trailer, 0, sizeof(trailer) );
    WriteLoad( &trailer, sizeof(trailer) );
    WriteDBI();
    QSeek( Root->outfile->handle, 0, Root->outfile->fname );
    record.rec_type = 0;
    record.data_nbytes = sizeof(header) + nbytes;
    WriteLoad( &record, sizeof(record) );
    memset( &header, 0, sizeof(header) );
    header.version = 400;
    header.cflags = (FmtData.u.qnx.flags | (FmtData.u.qnx.priv_level << 2)) & 0x7f;
    if( LinkState & 0x80 ) header.cflags |= 2;
    if( FmtData.type & 0x4000 ) header.cflags |= 0x10;
    header.fpu = 0;
    if( FmtData.cpu_type == 0 ) header.cpu = (LinkState & 0x80) ? 386 : 86;
    else header.cpu = FmtData.cpu_type * 100 + 86;
    header.code_index = StartInfo.seg >> 3;
    header.stack_index = StackAddr.seg >> 3;
    adseg = header.stack_index;
    if( DataGroup != 0 ) adseg = DataGroup->grp_addr.seg >> 3;
    header.heap_index = adseg;
    header.argv_index = adseg;
    header.code_offset = StartInfo.off;
    header.stack_nbytes = StackSize;
    header.heap_nbytes = FmtData.u.qnx.heapsize;
    header.image_base = FmtData.base;
    WriteLoad( &header, sizeof(header) );
    WriteLoad( segments, nbytes );
}

void WriteQNXResource( void )
{
    lmf_record record;
    lmf_resource resource;
    unsigned long len;
    int handle;

    if( FmtData.u.qnx.resource == 0 ) return;
    record.rec_type = 6;
    record.reserved = record.spare = 0;
    memset( &resource, 0, sizeof(resource) );
    if( FmtData.u.qnx.flags & 0x80 ) {
        handle = QObjOpen( FmtData.u.qnx.resource );
        if( handle == -1 ) {
            PrintIOError( 0x325d, "s", FmtData.u.qnx.resource );
            return;
        }
        record.data_nbytes = TokSize + sizeof(resource);
        len = QFileSize( handle );
        while( len > TokSize ) {
            WriteLoad( &record, sizeof(record) );
            WriteLoad( &resource, sizeof(resource) );
            len -= TokSize;
            QRead( handle, TokBuff, TokSize, FmtData.u.qnx.resource );
            WriteLoad( TokBuff, TokSize );
        }
        if( len != 0 ) {
            record.data_nbytes = len + sizeof(resource);
            WriteLoad( &record, sizeof(record) );
            WriteLoad( &resource, sizeof(resource) );
            QRead( handle, TokBuff, len, FmtData.u.qnx.resource );
            WriteLoad( TokBuff, len );
        }
        QClose( handle, FmtData.u.qnx.resource );
    } else {
        len = strlen( FmtData.u.qnx.resource );
        FmtData.u.qnx.resource[len++] = '\n';
        record.data_nbytes = len + sizeof(resource);
        WriteLoad( &record, sizeof(record) );
        WriteLoad( &resource, sizeof(resource) );
        WriteLoad( FmtData.u.qnx.resource, len );
    }
}

void WriteQNXData( unsigned long *segments )
{
    group_entry *group;
    lmf_record record;
    lmf_rw_end rw_end;
    unsigned char first;

    first = 1;
    rw_end.signature = 0;
    for( group = Groups; group != 0; group = group->next_group ) {
        if( group->u.qnxflags == 0 && group->totalsize != 0 ) {
            if( first && group->size >= 38 ) {
                memcpy( &rw_end.verify, (char *)group->data + 36, 2 );
                first = 0;
            }
            WriteQNXGroup( group, segments );
        }
        rw_end.signature += group->totalsize;
        rw_end.signature <<= 3;
        rw_end.signature ^= 0x32476235;
    }
    record.reserved = record.spare = 0;
    record.rec_type = 7;
    record.data_nbytes = sizeof(rw_end);
    WriteLoad( &record, sizeof(record) );
    /* The old writer leaves verify uninitialized if no suitable data group
     * exists. That output domain has not been qualified. */
    if( first ) TRAP();
    rw_end.verify ^= 0xffff;
    WriteLoad( &rw_end, sizeof(rw_end) );
    for( group = Groups; group != 0; group = group->next_group ) {
        if( group->u.qnxflags != 0 && group->totalsize != 0 ) {
            WriteQNXGroup( group, segments );
        }
        WriteQNXRelocs( group->grp_relocs, 8, group->grp_addr.seg >> 3 );
    }
}

extern seg_leader *StackSegPtr;

void WriteQNXGroup( group_entry *group, unsigned long *segments )
{
    lmf_record record;
    lmf_data data;
    unsigned long left;

    record.rec_type = 2;
    record.reserved = record.spare = 0;
    data.segment = group->grp_addr.seg >> 3;
    data.offset = 0;
    segments[data.segment] = group->totalsize
                           | ((unsigned long)group->u.qnxflags << 28);
    if( StackSegPtr != 0 && group == StackSegPtr->group ) {
        segments[data.segment] -= StackSize;
    }
    record.data_nbytes = 0x7e00;
    left = group->size;
    while( left > 0x7dfa ) {
        WriteQNXLoad( group->data, &data, &record );
        data.offset += 0x7dfa;
        left -= 0x7dfa;
    }
    if( left != 0 ) {
        record.data_nbytes = left + sizeof(data);
        WriteQNXLoad( group->data, &data, &record );
    }
}

void WriteQNXLoad( unsigned long data, lmf_data *load, lmf_record *record )
{
    WriteLoad( record, sizeof(*record) );
    WriteLoad( load, sizeof(*load) );
    QWrite( CurrSect->outfile->handle, (char *)data + load->offset,
                    record->data_nbytes - sizeof(*load), CurrSect->outfile->fname );
}

extern unsigned long DumpMaxRelocList( reloc_info **, unsigned long );

void WriteQNXRelocs( reloc_info *head, unsigned type, unsigned short segment )
{
    lmf_record record;
    unsigned long pos, size;
    unsigned adjust;
    unsigned char islinear;

    adjust = 0;
    record.rec_type = type;
    record.reserved = record.spare = 0;
    islinear = type == 8;
    if( islinear ) adjust = sizeof(segment);
    pos = QFileSize( Root->outfile->handle );
    while( head != 0 ) {
        QSeek( Root->outfile->handle, pos + sizeof(record), Root->outfile->fname );
        if( islinear ) WriteLoad( &segment, sizeof(segment) );
        size = DumpMaxRelocList( &head, 0x7e00 - adjust ) + adjust;
        QSeek( Root->outfile->handle, pos, Root->outfile->fname );
        record.data_nbytes = size;
        WriteLoad( &record, sizeof(record) );
        pos += size + sizeof(record);
    }
    QSeek( Root->outfile->handle, pos, Root->outfile->fname );
}
