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
#include "compiler.h"
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
