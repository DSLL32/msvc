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
   COMDEF -- support for Microsoft COMDEFs

*/

#include <string.h>
#include "cmdline.h"
#include <recon.h>

#ifdef __WATCOMC__
#pragma intrinsic(strlen)
#endif

/* This older two-pass writer saves segdata records directly, before the
 * later released comdat_info/comdat_piece ring representation. */
static segnode ComdatNodes[2];       /* original450008,45001c */
extern unsigned long CalcLIDataLength( void );
extern void SaveComdatData( symbol *, segdata *, unsigned long, unsigned char );

void ProcComdat( unsigned char pass )
{
    segdata *sdata, *previous;
    segnode *base;
    seg_leader *leader;
    list_of_names *symname;
    symbol *sym;
    char *name;
    unsigned namelen;
    unsigned char flags, attr, align, alloc, keep;
    unsigned long dataoff;
    unsigned short segidx;

    sdata = AllocSegData();
    keep = 0;
    flags = *ObjBuff++;
    attr = *ObjBuff++;
    align = *ObjBuff++;
    if( ObjFormat & 3 ) {
        dataoff = *(unsigned long *)ObjBuff;
        ObjBuff += 4;
    } else {
        dataoff = *(unsigned short *)ObjBuff;
        ObjBuff += 2;
    }
    SkipIdx();
    alloc = attr & 15;
    if( alloc == 0 ) {
        SkipIdx();
        segidx = GetIdx();
        if( segidx != 0 ) {
            base = FindNode( SegNodes, segidx );
        } else {
            base = 0;
            sdata->a.absolute.frame = *(unsigned short *)ObjBuff;
            ObjBuff += 2;
            sdata->flags |= 8;
        }
        if( align == 0 ) align = (base->info & 0x38) >> 3;
    }
    symname = FindName( GetIdx() );
    name = symname->name;
    namelen = strlen( name );
    if( flags & 4 ) {
        sym = SymOp( 0x17, name, namelen );
    } else if( pass == 2 ) {
        sym = SymOp( 0x1d, name, namelen );
        if( sym == 0 ) sym = SymOp( 0x16, name, namelen );
    } else {
        sym = SymOp( 0x16, name, namelen );
    }
    sdata->owner.sym = sym;
    if( (sym->info & 15) == 4 && (sym->info & 0x1000) ) {
        if( pass == 2 ) ObjFormat |= 0x20;
        goto discard;
    }
    if( flags & 2 ) sdata->length = CalcLIDataLength();
    else sdata->length = EOObjRec - ObjBuff;
    sdata->acbp = (align << 5) | 8;
    if( ObjFormat & 3 ) sdata->acbp |= 1;

    if( pass == 1 ) {
        if( (sym->info & 14) != 2 ) {
            ClearSymUnion( sym );
            sym->u.value = 0;
        }
        if( flags & 1 ) {
            if( !(sym->u.value & 0x40000000UL) && sym->p.seg != 0 ) {
                previous = sym->p.seg;
                previous->length += sdata->length;
            }
            ComdatNodes[0].entry = sym->p.seg;
        } else if( sym->u.value & 0xc0000000UL ) {
            sym->u.value |= 0x40000000UL;
            if( !(attr & 0xf0) ) ReportMultiple( sym, symname->name,
                strlen( symname->name ) );
            ComdatNodes[0].entry = 0;
        } else {
            keep = 1;
            if( alloc == 0 ) {
                if( base == 0 ) {
                    XDefSymAddr( sym, dataoff, sdata->a.absolute.frame );
                    if( LinkFlags & 0x10000UL ) CleanStripInfo( sym );
                    else sym->p.seg = 0;
                    keep = 0;
                } else {
                    leader = base->entry;
                    if( !(base->info & 0x0100) ) leader->info |= 0x8000;
                    sdata->u.leader = leader;
                    sdata->flags = (sdata->flags & ~4)
                        | ((base->info & 0x0200) != 0 ? 4 : 0);
                }
            } else {
                sdata->flags = (sdata->flags & ~4) | ((alloc & 1) << 2);
                --alloc;
                sdata->flags = (sdata->flags & ~0x60) | ((alloc & 3) << 5);
            }
            if( keep ) {
                if( (LinkFlags & 0x10000UL) && (sdata->flags & 4) ) {
                    DefStripSym( sym, sdata );
                } else {
                    sym->p.seg = sdata;
                }
                sym->info |= 0x10;
            }
            if( !(LinkFlags & 0x10000UL) && align != 0 )
                DBIAddGlobal( sym, strlen( sym->name ) );
            sym->info |= 0x1000;
            sym->u.value = 0x80000000UL;
            if( (sym->info & 15) > 1 || !(sym->info & 0x0100) ) {
                sym->publink = CurrMod->publist;
                CurrMod->publist = sym;
            }
            sym->info = (sym->info & 0xfff0) | 3;
            ComdatNodes[0].entry = sdata;
        }
        if( ComdatNodes[0].entry != 0 ) {
            previous = ComdatNodes[0].entry;
            ComdatNodes[0].info = (previous->flags & 4) ? 0x0200 : 0;
            CurrNode = &ComdatNodes[0];
        } else {
            CurrNode = 0;
        }
    } else {
        if( (sym->info & 0x10) || (sym->info & 14) != 2 ) {
            ObjFormat |= 0x20;
            goto discard;
        }
        if( (flags & 1) && !(sym->info & 0x20) ) {
            previous = sym->u.comdat;
            sdata->a.saved_addr = previous->a.saved_addr;
            SaveComdatData( sym, sdata, dataoff, flags & 2 );
            previous->length += sdata->length;
        } else if( sym->info & 0xa0 ) {
            ObjFormat |= 0x20;
            sym->info |= 0x20;
        } else {
            keep = 1;
            sdata->a.saved_addr = sym->u.value & 0x3fffffffUL;
            SaveComdatData( sym, sdata, dataoff, flags & 2 );
            if( alloc == 0 ) {
                if( base->info & 0x80 ) {
                    sdata->u.leader = base->entry;
                    sdata->flags |= 0x10;
                    if( !(base->info & 0x0800) ) {
                        base->info |= 0x0800;
                        leader = sym->p.seg;
                        base->offset = leader->seg_addr.off
                            + leader->addr_adjustment + sdata->a.saved_addr;
                    }
                } else {
                    sdata->u.leader = sym->p.seg;
                }
            } else {
                --alloc;
                sdata->flags = (sdata->flags & ~0x60) | ((alloc & 3) << 5);
            }
            sym->u.comdat = sdata;
            sym->info |= 0x80;
        }
    }
    if( keep ) {
        sdata->next = SavedSegs;
        SavedSegs = sdata;
        return;
    }
discard:
    FreeSegData( sdata );
}

void ProcLinsym( unsigned char pass )
{
    list_of_names *name;
    symbol *sym;
    unsigned namelen;
    unsigned char is32;
    seg_leader *leader;
    segdata *saved;
    unsigned long adjust;

    ++ObjBuff;
    name = FindName( GetIdx() );
    namelen = strlen( name->name );
    sym = SymOp( 9, name->name, namelen );
    if( sym == 0 ) sym = SymOp( 8, name->name, namelen );
    if( sym == 0 ) {
        BadObject();
        return;
    }
    if( (sym->info & 14) != 2 ) return;
    is32 = (ObjFormat & 3) != 0;
    if( pass == 1 ) {
        if( !(sym->u.value & 0x40000000UL) )
            DBIAddLines( sym->p.seg, is32, 1 );
    } else if( !(sym->info & 0x30) ) {
        leader = sym->p.seg;
        saved = sym->u.comdat;
        adjust = leader->seg_addr.off + leader->addr_adjustment
            + saved->a.saved_addr;
        DBIGenLines( leader, saved->u.leader, adjust, is32 );
    }
}

extern seg_leader *AddToLinkerComdat( symbol * );
extern void DBIAddModLines( unsigned );
void FinishComdat( saved_seg *seg )
{
    symbol *sym;
    seg_leader *leader;
    unsigned long off;

    if( (LinkFlags & 0x10000UL) && (seg->flags & 4) ) {
        seg->flags |= 0x80;
    } else {
        sym = seg->owner.sym;
        sym->info &= ~0x10;
        if( seg->u.leader == 0 ) {
            leader = AddToLinkerComdat( sym );
        } else {
            AddToSegment( seg, &off );
            sym->addr.off = off;
            sym->u.value = off | 0x40000000UL;
            leader = seg->u.leader;
            sym->addr.seg = leader->seg_addr.seg;
        }
        sym->p.seg = leader;
        DBIAddModLines( seg->line_size );
        seg->flags &= ~0x80;
    }
}

static unsigned ComdatNodeIndex;     /* original441264 */
extern void LoadObj( segnode *, unsigned long, unsigned char );
void SaveComdatData( symbol *sym, segdata *seg, unsigned long off,
                      unsigned char iterated )
{
    segnode *node;
    seg_leader *leader;

    node = &ComdatNodes[ComdatNodeIndex];
    ComdatNodeIndex ^= 1;
    leader = sym->p.leader;
    node->entry = leader;
    node->offset = seg->a.saved_addr;
    node->addr.seg = leader->seg_addr.seg;
    node->addr.off = leader->seg_addr.off + leader->addr_adjustment + node->offset;
    node->length = seg->length + off;
    node->info = leader->info & 0x763f;
    LoadObj( node, off, iterated );
}
