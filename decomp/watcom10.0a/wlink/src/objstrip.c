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
 *  OBJSTRIP:  Dead code elimination routines.
 *
 */

#include "link.h"
#include <recon.h>

#pragma pack(1)
struct edgelist {
    edgelist *next;
    union { symbol *sym; segdata *seg; } u;
    unsigned char flags;
};
#pragma pack()
typedef char old_edge_is_9[(sizeof(edgelist) == 9) ? 1 : -1];
extern void RefSeg( segdata * );
extern void AddEdge( segdata *, symbol * );
extern void FreeEdge( edgelist * );

void DefStripSym( symbol *sym, segdata *seg )
{
    edgelist *list, *next;
    segdata *src;

    if( sym->info & 0x4000 ) RefSeg( seg );
    for( list = sym->p.edges; list != 0; list = next ) {
        next = list->next;
        if( list->flags & 1 ) {
            AddEdge( seg, list->u.sym );
            FreeEdge( list );
        } else if( seg->flags & 1 ) {
            FreeEdge( list );
        } else {
            src = list->u.seg;
            if( src->flags & 1 ) {
                RefSeg( seg );
                FreeEdge( list );
            } else {
                list->u.seg = seg;
                list->next = src->a.refs;
                src->a.refs = list;
            }
        }
    }
    sym->p.seg = seg;
}

void DoStripReference( unsigned index, unsigned char isdata )
{
    extnode *ext;

    ext = FindNode( ExtNodes, (unsigned short)index );
    if( isdata ) {
        if( !(ext->entry->info & 0x8000) ) DataRef( ext->entry );
    } else {
        AddEdge( CurrNode->entry, ext->entry );
    }
}

void DataRef( symbol *sym )
{
    if( sym->info & 0x10 ) RefSeg( sym->p.seg );
    else sym->info |= 0x4000;
}

void RefSeg( segdata *seg )
{
    edgelist *edge, *next;

    if( seg->flags & 3 ) return;
    seg->flags |= 3;
    for( edge = seg->a.refs; edge != 0; edge = next ) {
        next = edge->next;
        RefSeg( edge->u.seg );
        FreeEdge( edge );
    }
    seg->a.refs = 0;
    seg->flags &= ~2;
}

extern edgelist *AllocEdge( void );
extern void PruneNonSymEdges( symbol * );
void AddEdge( segdata *seg, symbol *sym )
{
    edgelist *edge;
    segdata *target;

    if( sym->info & 0x1000 ) {
        if( (sym->info & 0x10) && seg != sym->p.seg ) {
            target = sym->p.seg;
            if( seg->flags & 1 ) {
                RefSeg( target );
            } else if( !(target->flags & 1) ) {
                edge = AllocEdge();
                edge->u.seg = target;
                edge->next = seg->a.refs;
                edge->flags &= ~1;
                seg->a.refs = edge;
            }
        }
    } else if( seg->flags & 1 ) {
        sym->info |= 0x4000;
        PruneNonSymEdges( sym );
    } else {
        edge = AllocEdge();
        edge->u.seg = seg;
        edge->next = sym->p.edges;
        edge->flags &= ~1;
        sym->p.edges = edge;
    }
}

static edgelist *FreedEdges;       /* original441f64 */

edgelist *AllocEdge( void )
{
    edgelist *edge;

    if( FreedEdges != 0 ) {
        edge = FreedEdges;
        FreedEdges = edge->next;
    } else {
        edge = Pass1Alloc( sizeof(edgelist) );
    }
    return edge;
}

void FreeEdge( edgelist *edge )
{
    edge->next = FreedEdges;
    FreedEdges = edge;
}

void PruneNonSymEdges( symbol *sym )
{
    edgelist *list, *next;

    list = sym->p.edges;
    sym->p.edges = 0;
    while( list != 0 ) {
        next = list->next;
        if( list->flags & 1 ) {
            list->next = sym->p.edges;
            sym->p.edges = list;
        } else {
            FreeEdge( list );
        }
        list = next;
    }
}

void FreeEdgeList( edgelist *edge )
{
    edgelist *tail;

    if( edge != 0 ) {
        tail = edge;
        while( tail->next != 0 ) tail = tail->next;
        tail->next = FreedEdges;
        FreedEdges = edge;
    }
}
void CleanStripInfo( symbol *sym )
{
    FreeEdgeList( sym->p.edges );
    sym->p.edges = 0;
}

/* Original entry is the sole RET shared with CleanStripInfo's epilogue. */
void RefGroup( group_entry *group )
{
    group = group;
}

void VectorReference( unsigned index )
{
    extnode *ext;

    ext = FindNode( ExtNodes, (unsigned short)index );
    IndirectCall( ext->entry );
}
