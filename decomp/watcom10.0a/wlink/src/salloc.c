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

#include "cmdline.h"
#include <recon.h>

seg_leader *CurrentSeg;              /* original4500f4 */

void ResizeSegment( seg_leader *seg, unsigned long *offset, unsigned long size,
                    unsigned char align, unsigned char combine )
{
    unsigned char oldalign;
    unsigned long oldsize;

    oldalign = (seg->info & 0x38) >> 3;
    if( !(align == 5 && oldalign > 2) && align > oldalign ) {
        seg->info = (seg->info & ~0x38) | (align << 3);
    }
    CurrentSeg = seg;
    switch( combine ) {
    case 1:
    case 2:
        *offset = CAlign( seg->size - seg->data_start, align );
        seg->size = BumpUp( BumpUp( *offset, seg->data_start ), size );
        break;
    case 4:
        *offset = 0;
        seg->size = BumpUp( seg->size, size );
        break;
    case 6:
        *offset = 0;
        if( size > seg->size ) seg->size = size;
        break;
    case 5:
        oldsize = seg->size - seg->data_start;
        seg->size = BumpUp( seg->size, size );
        *offset = oldsize - size;
        seg->data_start = seg->size - oldsize;
        break;
    case 7:
        oldsize = seg->size - seg->data_start;
        if( size > seg->size ) seg->size = size;
        *offset = oldsize - size;
        seg->data_start = seg->size - oldsize;
        break;
    }
}

const unsigned short AlignMask[] = { 0, 0, 1, 15, 255, 3, 4095, 0 };

unsigned long CAlign( unsigned long value, unsigned char align )
{
    unsigned part, mask;

    if( align > 1 ) {
        mask = AlignMask[align];
        part = value & mask;
        if( part != 0 ) value = BumpUp( value, mask - part + 1 );
    }
    return value;
}

unsigned long BumpUp( unsigned long value, unsigned long size )
{
    value += size;
    if( CurrentSeg != 0 && !(CurrentSeg->info & 0x0400) && value > 0x10000UL ) {
        LnkMsg( 0x7315, "sl", CurrentSeg->segname->name, value - 0x10000UL );
    }
    return value;
}

static address CodeLoc;              /* original4501f0 */
static address DataLoc;              /* original4501f8 */

void StartMemMap( void )
{
    if( FmtData.type & 0x0100 ) {
        CodeLoc.seg = 1;
        DataLoc.seg = 2;
        CodeLoc.off = 0;
        DataLoc.off = 0;
    } else {
        CurrLoc.off = 0;
        CurrLoc.seg = (FmtData.type & 0xff1f) != 0;
        if( FmtData.type & 0x2800 ) CurrLoc.off = FmtData.base;
    }
}

void NewSegment( seg_leader *seg )
{
    group_entry *group;
    seg_leader *save;
    unsigned long align;
    unsigned char auto_group;

    align = (seg->info & 0x38) >> 3;
    if( seg->info & 0x7000 ) {
        if( !(seg->info & 0x0400) ) CurrLoc.off &= 15;
        Align( align );
        if( !(seg->info & 0x0400) ) CurrLoc.off &= 15;
        seg->seg_addr.off = CurrLoc.off;
        seg->seg_addr.seg = CurrLoc.seg;
        AddSize( seg->saved_size );
    } else if( !(FmtData.type & 0x621f) ) {
        save = CurrentSeg;
        if( FmtData.type & 0x3c00 ) CurrentSeg = 0;
        group = seg->group;
        if( FmtData.type & 0x0100 ) {
            if( group->segflags & 1 ) CurrLoc = DataLoc;
            else CurrLoc = CodeLoc;
        }
        auto_group = group->name[0] == 0 && group->leaders[0] != seg;
        if( !auto_group ) NormalizeAddr();
        Align( align );
        if( !auto_group ) NormalizeAddr();
        seg->seg_addr.off = CurrLoc.off;
        seg->seg_addr.seg = CurrLoc.seg;
        AddSize( seg->saved_size );
        if( FmtData.type & 0x0100 ) {
            if( group->segflags & 1 ) DataLoc = CurrLoc;
            else CodeLoc = CurrLoc;
        }
        CurrentSeg = save;
    } else {
        group = seg->group;
        CurrLoc.seg = group->grp_addr.seg;
        seg->seg_addr.seg = CurrLoc.seg;
        CurrLoc.off = group->totalsize;
        if( (FmtData.type & 0x0010) && seg == FmtData.u.pe.dgroupsplitseg ) {
            FmtData.u.pe.bsspad = ((CurrLoc.off + FmtData.u.pe.objalign - 1)
                & ~(FmtData.u.pe.objalign - 1)) - CurrLoc.off;
            AddSize( FmtData.u.pe.bsspad );
        }
        if( seg->saved_size == 0 && group->name[0] == 0 ) {
            seg->seg_addr.off = CurrLoc.off;
        } else {
            Align( align );
            seg->seg_addr.off = CurrLoc.off;
            AddSize( seg->saved_size );
            group->totalsize = CurrLoc.off;
        }
    }
}

void Align( unsigned char align )
{
    AddSize( CAlign( CurrLoc.off, align ) - CurrLoc.off );
}

void AddSize( unsigned long size )
{
    CurrSect->size += size;
    CurrLoc.off = BumpUp( CurrLoc.off, size );
}

void NormalizeAddr( void )
{
    unsigned long seg;

    if( FmtData.type & 0xff1f ) return;
    if( CurrSect != NonSect && (LinkFlags & 0x1000) ) return;
    seg = CurrLoc.seg + (CurrLoc.off >> 4);
    if( seg > 0xffff ) LnkMsg( 0x3378, 0 );
    CurrLoc.seg = seg;
    CurrLoc.off &= 15;
}

void MAlign( unsigned char align )
{
    CurrLoc.off = CAlign( CurrLoc.off, align );
}
