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

/* CarveCreate: actual1994 CF1eca6..1ecf3. Later fe_misc/carve.c
 * preserves size rounding/count arithmetic but reorders the control and
 * uses a one-link free node. The shipped free nodes require8bytes. */
#include "compiler.h"
#include "carve.h"
#include <recon.h>

carve_t CarveCreate(unsigned size, unsigned count)
{
    carve_t cv;

    size = (size + 3) & ~3u;
    if (size < sizeof(CARVE_FREE)) size = sizeof(CARVE_FREE);
    cv = (carve_t)CMemAlloc(sizeof(*cv));
    cv->elm_size = size;
    cv->elm_count = count;
    cv->blk_top = size * count;
    cv->blk_count = 0;
    cv->blk_list = 0;
    cv->cursor = 0;
    cv->free_ring = 0;
    cv->index = 0;
    return cv;
}

/* The 1994 allocator carves unused elements backwards from a block's end,
 * and keeps returned elements in a doubly-linked free ring. The later
 * released version populates a singly-linked free list eagerly instead. */
void *CarveAlloc(carve_t cv)
{
    CARVE_FREE *node, *next, *previous;
    char *cursor;

    node = cv->free_ring;
    if (node) {
        next = node->next;
        previous = node->previous;
        next->previous = previous;
        previous->next = next;
        cv->free_ring = previous == node ? 0 : previous;
        return node;
    }
    cursor = (char *)cv->cursor;
    if (!cursor || cursor == cv->blk_list->data) cursor = (char *)newBlk(cv);
    cursor -= cv->elm_size;
    cv->cursor = cursor;
    return cursor;
}

void *newBlk(carve_t cv)
{
    CARVE_BLOCK *block;
    char *end;

    block = (CARVE_BLOCK *)CMemAlloc(sizeof(block->next) + cv->blk_top);
    block->next = cv->blk_list;
    cv->blk_list = block;
    end = block->data + cv->blk_top;
    ++cv->blk_count;
    cv->cursor = end;
    return end;
}
