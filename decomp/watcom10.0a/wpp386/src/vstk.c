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

/* bld/fe_misc/c/vstk.c, initial commit6b9cb443. The observed16-byte
 * control and all five initialization stores match the shipped body. */
#include <stddef.h>
#include "vstk.h"
#include "pending.h"
#include "compiler.h"
#include <recon.h>

void VstkOpen(VSTK_CTL *stack, unsigned size, unsigned count)
{
    stack->current = 0;
    stack->top = 0;
    stack->freed = 0;
    stack->per_block = count;
    stack->size = size;
}

void *VstkTop(VSTK_CTL *stack)
{
    return stack->current;
}
unsigned vstkDataSize(const VSTK_CTL *stack);
int vstkInBlk(const VSTK_BLK *block, void *entry, unsigned size)
{
    if (!entry || (char *)entry<block->data ||
        (char *)entry>=block->data+size) return 0;
    return 1;
}
void *VstkNext(VSTK_CTL *stack, void *entry)
{
    VSTK_BLK *block;
    unsigned size=vstkDataSize(stack);
    for (block=stack->top;;block=block->last) {
        if (!block) {
            block=stack->top;
            entry=block ? block->data : NULL;
            break;
        }
        if (vstkInBlk(block,entry,size)) {
            entry=(char *)entry+stack->size;
            if ((char *)entry>=block->data+size) {
                block=block->last;
                entry=block ? block->data : NULL;
            }
            break;
        }
    }
    return entry;
}

unsigned vstkDataSize(const VSTK_CTL *stack)
{
    return stack->per_block*stack->size;
}
void *vstkPushBlk(VSTK_CTL *stack)
{
    VSTK_BLK *block=stack->freed;
    unsigned size=vstkDataSize(stack);
    if (!block) block=(VSTK_BLK *)CMemAlloc(size+sizeof(block->last));
    else stack->freed=block->last;
    block->last=stack->top;
    stack->top=block;
    return block->data+size;
}
void *VstkPush(VSTK_CTL *stack)
{
    char *current;
    if (!stack->top || !stack->current || stack->current==stack->top->data)
        current=(char *)vstkPushBlk(stack);
    else current=(char *)stack->current;
    current-=stack->size;
    stack->current=current;
    return current;
}

VSTK_BLK *vstkPopBlk(VSTK_CTL *stack)
{
    VSTK_BLK *block=stack->top;
    VSTK_BLK *next=block->last;
    block->last=stack->freed;
    stack->freed=block;
    stack->top=next;
    return next;
}
void *VstkPop(VSTK_CTL *stack)
{
    void *old=stack->current;
    VSTK_BLK *block;
    if (old) {
        if ((char *)old==stack->top->data+vstkDataSize(stack)-stack->size) {
            block=vstkPopBlk(stack);
            stack->current=block ? block->data : NULL;
        } else stack->current=(char *)old+stack->size;
    }
    return old;
}

void freeVstkBlocks(VSTK_BLK *block)
{
    VSTK_BLK *next;
    while (block) {
        next=block->last;
        CMemFree(block);
        block=next;
    }
}
void VstkClose(VSTK_CTL *stack)
{
    freeVstkBlocks(stack->top);
    freeVstkBlocks(stack->freed);
}

/* Reached1994 indexed access grows the real block chain when needed, and
 * indexing a new top slot advances current just as a pushed slot would. */
void *VstkIndex(VSTK_CTL *stack,int index)
{
    unsigned blocks=0,required,per;
    VSTK_BLK *block;
    int pushed=0,on_top=1;
    char *result;
    if (index<0) return NULL;
    for (block=stack->top;block;block=block->last) ++blocks;
    per=stack->per_block;
    required=(unsigned)index/per+1;
    while (blocks<required) {
        pushed=1;
        vstkPushBlk(stack);
        ++blocks;
    }
    index=blocks*per-(unsigned)index;
    block=stack->top;
    while ((unsigned)index>per) {
        index-=per;
        block=block->last;
        on_top=0;
    }
    result=block->data+(index-1)*stack->size;
    if (pushed || !stack->current ||
        (on_top && result<(char *)stack->current)) stack->current=result;
    return result;
}
int VstkDimension(const VSTK_CTL *stack)
{
    VSTK_BLK *block=stack->top;
    int dimension=0;
    if (block) {
        dimension=-((char *)stack->current-block->data)/stack->size;
        while (block) {
            dimension+=stack->per_block;
            block=block->last;
        }
    }
    return dimension-1;
}
