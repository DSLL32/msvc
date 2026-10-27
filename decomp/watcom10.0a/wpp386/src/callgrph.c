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


/* Actual1994 callgrph.c callback table and graph initialization. */
#include "callgrph.h"
#include "pending.h"
#include <recon.h>
void DgrfConstruct(DIRGRAPH_CTL *ctl,DIRGRAPH_VFT *vft);
void cgrfInit(DIRGRAPH_CTL *base)
{
    CALLGRAPH *ctl=(CALLGRAPH *)base;
    ctl->carve_nodes=CarveCreate(sizeof(CALLNODE),64);
    ctl->carve_edges=CarveCreate(sizeof(CALLEDGE),64);
    ctl->pruned=0;
}
void cgrfFini(DIRGRAPH_CTL *base)
{
    CALLGRAPH *ctl=(CALLGRAPH *)base;
    CarveDestroy(ctl->carve_nodes);
    CarveDestroy(ctl->carve_edges);
}
DIRGRAPH_NODE *cgrfAllocNode(DIRGRAPH_CTL *ctl)
{ return CarveAlloc(((CALLGRAPH *)ctl)->carve_nodes); }
DIRGRAPH_EDGE *cgrfAllocEdge(DIRGRAPH_CTL *ctl)
{ return CarveAlloc(((CALLGRAPH *)ctl)->carve_edges); }
void cgrfFreeNode(DIRGRAPH_CTL *ctl,DIRGRAPH_NODE *node)
{ CarveFree(((CALLGRAPH *)ctl)->carve_nodes,node); }
void cgrfFreeEdge(DIRGRAPH_CTL *ctl,DIRGRAPH_EDGE *edge)
{ CarveFree(((CALLGRAPH *)ctl)->carve_edges,edge); }
DIRGRAPH_NODE *cgrfInitNode(DIRGRAPH_CTL *ctl,DIRGRAPH_NODE *base)
{
    CALLNODE *node=(CALLNODE *)base;
    (void)ctl;
    node->refs=0;
    node->depth=0;
    node->opcodes=0;
    node->addrs=0;
    node->cond_flags=0;
    node->cgfile=NULL;
    node->state_table=0;
    node->calls_done=0;
    node->is_vft=0;
    node->inline_fun=0;
    node->inlineable=0;
    node->inlineable_oe=0;
    return base;
}
void changeNodeReference(int refs,int addrs,CALLEDGE *edge)
{
    CALLNODE *node=(CALLNODE *)edge->base.target;
    edge->refs+=refs;
    edge->addrs+=addrs;
    node->refs+=refs;
    node->addrs+=addrs;
}
void changeCtlCounts(CALLGRAPH *ctl,CALLEDGE *edge)
{ changeNodeReference(ctl->incr_refs,ctl->incr_addrs,edge); }
DIRGRAPH_EDGE *cgrfInitEdge(DIRGRAPH_CTL *ctl,DIRGRAPH_EDGE *base)
{
    CALLEDGE *edge=(CALLEDGE *)base;
    edge->refs=0;
    edge->addrs=0;
    changeCtlCounts((CALLGRAPH *)ctl,edge);
    return base;
}
DIRGRAPH_EDGE *cgrfDupEdge(DIRGRAPH_CTL *ctl,DIRGRAPH_EDGE *edge)
{
    changeCtlCounts((CALLGRAPH *)ctl,(CALLEDGE *)edge);
    return edge;
}
/* Known registered empty callback. Its actual C3 is also the preceding
 * two edge callbacks' return instruction; seeded, never an observed entry. */
void cgrfPruneNode(DIRGRAPH_CTL *ctl,DIRGRAPH_NODE *node)
{ (void)ctl; (void)node; }
void cgrfPruneEdge(DIRGRAPH_CTL *ctl,DIRGRAPH_EDGE *base)
{
    CALLEDGE *edge=(CALLEDGE *)base;
    (void)ctl;
    changeNodeReference(-(int)edge->refs,-(int)edge->addrs,edge);
}
static DIRGRAPH_VFT cgrfVft={
    cgrfInit,cgrfFini,cgrfAllocNode,cgrfAllocEdge,cgrfFreeNode,cgrfFreeEdge,
    cgrfInitNode,cgrfInitEdge,cgrfDupEdge,cgrfPruneNode,cgrfPruneEdge
};
void CgrfInit(CALLGRAPH *ctl)
{ DgrfConstruct(&ctl->base,&cgrfVft); }

DIRGRAPH_NODE *DgrfAddNode(DIRGRAPH_CTL *ctl,void *object);
CALLNODE *CgrfAddFunction(CALLGRAPH *ctl,struct symbol *sym)
{ return (CALLNODE *)DgrfAddNode(&ctl->base,sym); }

DIRGRAPH_EDGE *DgrfAddEdge(DIRGRAPH_CTL *ctl,DIRGRAPH_NODE *source,
                           DIRGRAPH_NODE *target);
void CgrfAddAddrOf(CALLGRAPH *ctl,CALLNODE *source,CALLNODE *target)
{
    ctl->incr_refs=0;
    ctl->incr_addrs=1;
    DgrfAddEdge(&ctl->base,&source->base,&target->base);
}

void CgrfAddCall(CALLGRAPH *ctl,CALLNODE *source,CALLNODE *target)
{
    ctl->incr_refs=1;
    ctl->incr_addrs=(unsigned)-1;
    DgrfAddEdge(&ctl->base,&source->base,&target->base);
}

int DgrfWalkObjects(DIRGRAPH_CTL *ctl,int (*walker)(DIRGRAPH_CTL *,DIRGRAPH_NODE *));
void CgrfWalkFunctions(CALLGRAPH *ctl,int (*walker)(CALLGRAPH *,CALLNODE *))
{ DgrfWalkObjects(&ctl->base,(int (*)(DIRGRAPH_CTL *,DIRGRAPH_NODE *))walker); }

void DgrfPruneNode(DIRGRAPH_CTL *ctl,DIRGRAPH_NODE *node);
void CgrfPruneFunction(CALLGRAPH *ctl,CALLNODE *node)
{
    if (!node->refs && !node->addrs) {
        ctl->pruned=1;
        DgrfPruneNode(&ctl->base,&node->base);
    }
}

void DgrfDestruct(DIRGRAPH_CTL *ctl);
void CgrfFini(CALLGRAPH *ctl) { DgrfDestruct(&ctl->base); }
