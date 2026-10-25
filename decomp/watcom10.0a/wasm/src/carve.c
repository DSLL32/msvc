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
