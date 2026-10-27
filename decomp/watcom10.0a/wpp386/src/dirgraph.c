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


/* Actual1994 dirgraph.c construction; virtual callbacks retain their
 * real owner and are not replaced with trace-only calls. */
#include "callgrph.h"
#include <recon.h>
void DgrfConstruct(DIRGRAPH_CTL *ctl,DIRGRAPH_VFT *vft)
{
    ctl->objects=NULL;
    ctl->vft=vft;
    vft->init_ctl(ctl);
}

DIRGRAPH_NODE *DgrfFindNode(DIRGRAPH_CTL *ctl,void *object)
{
    DIRGRAPH_NODE *last=ctl->objects,*node;
    if (last) {
        node=last->next;
        for (;;) {
            if (node->object==object) return node;
            if (node==last) break;
            node=node->next;
        }
    }
    return NULL;
}
DIRGRAPH_NODE *DgrfAddNode(DIRGRAPH_CTL *ctl,void *object)
{
    DIRGRAPH_NODE *node=DgrfFindNode(ctl,object);
    if (!node) {
        node=ctl->vft->alloc_node(ctl);
        node->object=object;
        node->edges=NULL;
        node=ctl->vft->init_node(ctl,node);
        RingAppend((void **)&ctl->objects,node);
    }
    return node;
}

DIRGRAPH_EDGE *DgrfAddEdge(DIRGRAPH_CTL *ctl,DIRGRAPH_NODE *source,
                           DIRGRAPH_NODE *target)
{
    DIRGRAPH_EDGE *last=source->edges,*edge=NULL,*check;
    if (last) {
        check=last->next;
        for (;;) {
            if (check->target==target) { edge=check; break; }
            if (check==last) break;
            check=check->next;
        }
    }
    if (!edge) {
        edge=ctl->vft->alloc_edge(ctl);
        edge->target=target;
        RingAppend((void **)&source->edges,edge);
        edge=ctl->vft->init_edge(ctl,edge);
    } else edge=ctl->vft->dup_edge(ctl,edge);
    return edge;
}

int DgrfWalkObjects(DIRGRAPH_CTL *ctl,int (*walker)(DIRGRAPH_CTL *,DIRGRAPH_NODE *))
{
    DIRGRAPH_NODE *last=ctl->objects,*node,*next;
    int result=0;
    if (last) {
        node=last->next;
        for (;;) {
            next=node->next; /* walker may remove the current node */
            result=walker(ctl,node);
            if (result || node==last) break;
            node=next;
        }
    }
    return result;
}

int DgrfWalkEdges(DIRGRAPH_CTL *ctl,DIRGRAPH_NODE *node,
                  int (*walker)(DIRGRAPH_CTL *,DIRGRAPH_EDGE *))
{
    DIRGRAPH_EDGE *last=node->edges,*edge,*next;
    int result=0;
    if (last) {
        edge=last->next;
        for (;;) {
            next=edge->next;
            result=walker(ctl,edge);
            if (result || edge==last) break;
            edge=next;
        }
    }
    return result;
}
int pruneEdge(DIRGRAPH_CTL *ctl,DIRGRAPH_EDGE *edge)
{
    ctl->vft->prune_edge(ctl,edge);
    ctl->vft->free_edge(ctl,edge);
    return 0;
}
void *RingPrune(void **head,void *entry);
void DgrfPruneNode(DIRGRAPH_CTL *ctl,DIRGRAPH_NODE *node)
{
    DgrfWalkEdges(ctl,node,pruneEdge);
    ctl->vft->prune_node(ctl,node);
    RingPrune((void **)&ctl->objects,node);
    ctl->vft->free_node(ctl,node);
}

int destroyEdge(DIRGRAPH_CTL *ctl,DIRGRAPH_EDGE *edge)
{
    ctl->vft->free_edge(ctl,edge);
    return 0;
}
int destroyNode(DIRGRAPH_CTL *ctl,DIRGRAPH_NODE *node)
{
    DgrfWalkEdges(ctl,node,destroyEdge);
    ctl->vft->free_node(ctl,node);
    return 0;
}
void DgrfDestruct(DIRGRAPH_CTL *ctl)
{
    DgrfWalkObjects(ctl,destroyNode);
    ctl->objects=NULL;
    ctl->vft->fini_ctl(ctl);
}
