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

/* Adapted from bld/plusplus/c/memmgr.c at initial commit
 * 6b9cb4438959d845a2b4dd4bf19f7908ecd608e9, actual1994 body authority.
 * cmemInit falls through into the separately callable addPerm entry41122.
 * The registry keeps that tail edge. addPerm and CMemAlloc share the
 * original POP EDX, POP EBX, RET suffix under an exact ownership declaration. */
#include "compiler.h"
#include "memory.h"
#include <recon.h>

typedef struct cleanup CLEANUP;
typedef struct perm_blk PERM_BLK;
struct cleanup {
    CLEANUP *next;
    void (*rtn)(void);
};
struct perm_blk {
    PERM_BLK *next;
    unsigned amt_left, size;
    char mem[1];
};
CLEANUP *cleanupList;
PERM_BLK *permList;

void cmemInit(INITFINI *entry)
{
    (void)entry;
    cleanupList = 0;
    permList = 0;
    addPerm(0);
}

void addPerm(unsigned size)
{
    PERM_BLK *block;
    unsigned amount;

    if (size > 1024) {
        block = (PERM_BLK *)CMemAlloc(size + 12);
        linkPerm(block, size);
        return;
    }
    amount = 1024;
    for (;;) {
        block = (PERM_BLK *)malloc_segment_wrapper(amount + 12);
        if (block) {
            linkPerm(block, amount);
            return;
        }
        block = (PERM_BLK *)alloc_from_cleanup(amount + 12);
        if (block) {
            linkPerm(block, amount);
            return;
        }
        if (amount == 128) break;
        amount >>= 1;
    }
}

void linkPerm(PERM_BLK *block, unsigned amount)
{
    block->size = amount;
    block->amt_left = amount;
    RingPush((void **)&permList, block);
}

void *CPermAlloc(unsigned size)
{
    PERM_BLK *find, *last;
    void *ptr;

    size = (size + 3) & ~3u;
    last = permList;
    if (last) {
        find = last;
        do {
            find = find->next;
            ptr = cutPerm(find, size);
            if (ptr) return ptr;
        } while (find != last);
    }
    addPerm(size);
    last = permList;
    if (last) {
        find = last;
        do {
            find = find->next;
            ptr = cutPerm(find, size);
            if (ptr) return ptr;
        } while (find != last);
    }
    CErr1(63);
    CSuicide();
    return 0;
}

void *cutPerm(PERM_BLK *block, unsigned size)
{
    char *ptr;

    if (size > block->amt_left) return 0;
    ptr = block->mem + block->size - block->amt_left;
    block->amt_left -= size;
    return ptr;
}

void *CMemAlloc(unsigned size)
{
    void *ptr;

    if (!size) return 0;
    ptr = malloc_segment_wrapper(size);
    if (!ptr) {
        ptr = alloc_from_cleanup(size);
        if (!ptr) {
            CErr1(63);
            CSuicide();
        }
    }
    return ptr;
}

void CMemRegisterCleanup(void (*cleanup)(void))
{
    CLEANUP *new_cleanup;

    new_cleanup = (CLEANUP *)RingAlloc((void **)&cleanupList,
                                       sizeof(*new_cleanup));
    new_cleanup->rtn = cleanup;
}

void CMemFree(void *ptr)
{
    if (ptr) free_segment_wrapper(ptr);
}
