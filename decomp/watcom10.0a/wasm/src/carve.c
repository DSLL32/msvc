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
* Description: Original-source attribution retained for the adapted entry.
*
****************************************************************************/

/* Adapted from bld/womp/c/carve.c, initial Open Watcom commit
 * 6b9cb4438959d845a2b4dd4bf19f7908ecd608e9. CF 1aaa8..1aadd. */
#include <stddef.h>
#include "wasm.h"
#include <recon.h>

carve_t CarveCreate(size_t elm_size, size_t blk_size)
{
    cv_t *cv;
    cv = MemAlloc(sizeof(*cv));
    cv->elm_size = elm_size;
    cv->blk_size = blk_size;
    cv->blk_top = blk_size * elm_size;
    cv->blk_list = NULL;
    cv->free_list = NULL;
    newBlk(cv);
    return cv;
}

/* CF1aa80..1aaa7: exact block header is four bytes, without tail padding. */
void newBlk(cv_t *cv)
{
    carve_block *newblk;
    newblk = MemAlloc(offsetof(carve_block, data) + cv->blk_top);
    newblk->next = cv->blk_list;
    cv->blk_list = newblk;
    cv->top_elm = newblk->data + cv->blk_top;
}

/* Initial womp/c/carve.c CarveAlloc; CF1ab1c..1ab83. The source
 * assertion filename/line values are the shipped1994 diagnostic inputs. */
void *CarveAlloc(carve_t caller_cv)
{
    cv_t *cv;
    void **new;
    if (caller_cv == NULL) InternalError("g:\\bld\\womp\\c\\carve.c",83);
    cv = (cv_t *)caller_cv;
    if (cv->free_list != NULL) {
        new = cv->free_list;
        cv->free_list = *new;
        return new;
    }
    if (cv->top_elm == NULL) InternalError("g:\\bld\\womp\\c\\carve.c",90);
    if (cv->top_elm == cv->blk_list->data) newBlk(cv);
    cv->top_elm -= cv->elm_size;
    return cv->top_elm;
}

/* CF1ab84..1aba8; return the object to this carve owner's free list. */
void CarveFree(carve_t caller_cv, void *element)
{
    cv_t *cv;
    if (caller_cv == NULL) InternalError("g:\\bld\\womp\\c\\carve.c", 102);
    cv = (cv_t *)caller_cv;
    *(void **)element = cv->free_list;
    cv->free_list = (void **)element;
}

/* CF1aae0..1ab19; release all actual blocks before their carve owner. */
void CarveDestroy(carve_t caller_cv)
{
    cv_t *cv;
    carve_block *cur, *next;
    if (caller_cv == NULL) InternalError("g:\\bld\\womp\\c\\carve.c",67);
    cv = caller_cv;
    cur = cv->blk_list;
    while (cur != NULL) {
        next = cur->next;
        MemFree(cur);
        cur = next;
    }
    MemFree(cv);
}
