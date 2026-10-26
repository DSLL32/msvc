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

/* RingPush: actual WPP386 CF0x46c22..0x46c42. Intrusive circular
 * rings retain a last-node header; pushing inserts before the first node. */
#include <stddef.h>
#include "compiler.h"
#include "carve.h"
#include "pending.h"
#include <recon.h>

typedef struct ring_node RING_NODE;
struct ring_node { RING_NODE *next; };

void *RingPush(void **head, void *entry)
{
    RING_NODE *last, *node;

    last = (RING_NODE *)*head;
    node = (RING_NODE *)entry;
    if (!last) {
        node->next = node;
        *head = node;
    } else {
        node->next = last->next;
        last->next = node;
    }
    return node;
}

/* bld/fe_misc/c/ringfns.c, initial source reference: allocate then append. */
void *RingAlloc(void **head, unsigned size)
{
    void *node;

    node = CMemAlloc(size);
    RingAppend(head, node);
    return node;
}

void RingAppend(void **head, void *entry)
{
    RING_NODE *last, *node;

    last = (RING_NODE *)*head;
    node = (RING_NODE *)entry;
    if (!last) {
        node->next = node;
    } else {
        node->next = last->next;
        last->next = node;
    }
    *head = node;
}

void RingInsert(void **head, void *entry, void *insert)
{
    RING_NODE *last, *node, *after;

    last = (RING_NODE *)*head;
    node = (RING_NODE *)entry;
    after = (RING_NODE *)insert;
    if (!last || last == after) {
        RingAppend(head, entry);
    } else if (!after) {
        node->next = last->next;
        last->next = node;
    } else {
        node->next = after->next;
        after->next = node;
    }
}

void *RingPop(void **head)
{
    RING_NODE *last, *first;
    last=(RING_NODE *)*head;
    first=0;
    if (last) {
        first=last->next;
        if (first==last) *head=0;
        else last->next=first->next;
    }
    return first;
}

void RingFree(void **head)
{
    void *entry;
    while ((entry=RingPop(head))!=0) CMemFree(entry);
}

/* Move an existing member to the first traversal position without changing
 * its identity. Actual1994 CF46b45..46b70. */
void *RingPromote(void **head,void *item,void *previous)
{
    RING_NODE *last,*node,*prev;
    last=(RING_NODE *)*head;
    node=(RING_NODE *)item;
    prev=(RING_NODE *)previous;
    if (prev!=0 && last!=prev) {
        if (last!=node) {
            prev->next=node->next;
            node->next=last->next;
            last->next=node;
        } else last=prev;
        *head=last;
    }
    return node;
}

int RingCount(void *head)
{
    RING_NODE *ptr;
    int count;
    ptr=(RING_NODE *)head;
    count=0;
    if (ptr!=0) {
        do { ptr=ptr->next; ++count; } while (ptr!=head);
    }
    return count;
}

void *RingPred(void *head,void *entry)
{
    RING_NODE *last=head,*previous=last;
    if(last) {
        do {
            if(previous->next==entry) return previous;
            previous=previous->next;
        } while(previous!=last);
    }
    return NULL;
}
void *RingPruneWithPrev(void **head,void *entry,void *previous)
{
    RING_NODE *node=entry,*prev=previous;
    if(!prev) prev=*head;
    prev->next=node->next;
    if(prev==node) *head=NULL;
    else if(*head==node) *head=prev;
    node->next=NULL;
    return node;
}
void *RingPrune(void **head,void *entry)
{
    void *previous=RingPred(*head,entry);
    return RingPruneWithPrev(head,entry,previous);
}

void *RingLookup(void *head,int (*compare)(void *,const void *),const void *comparand)
{
    RING_NODE *first,*ptr;
    first=(RING_NODE *)head;
    if (first!=0) {
        ptr=first->next;
        do {
            if (compare(ptr,comparand)) return ptr;
            if (ptr==first) break;
            ptr=ptr->next;
        } while (1);
    }
    return 0;
}

/* Old ring.c46cee..46d03 allocates then appends the real carve owner. */
void *RingCarveAlloc(carve_t cv,void **head)
{
    void *element;
    element=CarveAlloc(cv);
    RingAppend(head,element);
    return element;
}

/* Intrusive ordinary stack, unlike the circular RingPush above. */
void *StackPush(void **head,void *entry)
{
    *(void **)entry=*head;
    *head=entry;
    return entry;
}

void *StackPop(void **head)
{
    void *top=*head;
    if (!top) return 0;
    *head=*(void **)top;
    return top;
}

/* bld/fe_misc/c/ringfns.c: safe iteration saves next before a callback may
 * release the current node. The ring header value is not dereferenced again. */
void RingWalk(void *head,void (*routine)(void *))
{
    RING_NODE *last=head,*node,*next;
    if (!last) return;
    next=last->next;
    do {
        node=next;
        next=node->next;
        routine(node);
    } while (node!=last);
}
