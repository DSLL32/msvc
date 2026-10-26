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


/* Earlier CMemInit immediately allocates the permanent area. It predates
 * the later cmemmgr.c mem_blk list and lazy InitPermArea/AllocPermArea split. */
#define WCC_CALLBACK_CALLOC
#define WCC_CALLBACK_MALLOC
#define WCC_CALLBACK_FREE
#include <stdlib.h>
#include <string.h>
#include "compiler.h"
#include "cmemory.h"
#include "allocator.h"
#include <recon.h>

static char *PermArea;
static char *PermPtr;
static unsigned PermSize;
static unsigned PermAvail;

void CMemInit(void)
{
    PermSize=0xfff0;
    while (PermSize) {
        PermArea=(char *)calloc(1,PermSize+sizeof(int));
        if (PermArea) {
            *(int *)(PermArea+PermSize)=-1;
            break;
        }
        if (PermSize<32) PermSize=32;
        PermSize-=32;
    }
    PermPtr=PermArea;
    PermAvail=PermSize;
}

/* Permanent allocations are aligned to two bytes by the older compiler. */
void *CPermAlloc(unsigned amount)
{
    char *memory;

    amount=(amount+1)&~1U;
    if (amount>PermAvail) {
        CMemInit();
        if (amount>PermAvail) return CMemAlloc(amount);
    }
    memory=PermPtr;
    PermPtr+=amount;
    PermAvail-=amount;
    memset(memory,0,amount);
    return memory;
}

void *FEmalloc(unsigned amount)
{
    void *memory;

    memory=malloc(amount);
    if (memory==NULL) {
        PageOutQuads();
        PageOutLeafs();
        PageOutSyms();
        memory=malloc(amount);
        if (memory==NULL) {
            CErr1(0x428);
            CSuicide();
        }
    }
    return memory;
}

void *CMemAlloc(unsigned amount)
{
    void *memory;

    memory=CFastAlloc(amount);
    if (memory==NULL) {
        CMemInit();
        memory=CFastAlloc(amount);
        if (memory==NULL) {
            CErr1(0x428);
            CSuicide();
        }
    }
    return memset(memory,0,amount);
}

/* The original initialized DATA sentinel at75f90 points to itself. */
static MCB CFreeList={0,&CFreeList,&CFreeList};

void *CFastAlloc(unsigned size)
{
    unsigned amount;
    MCB *block,*prev,*next;

    amount=(size+sizeof(unsigned)+1)&~1U;
    if (amount<sizeof(MCB)) amount=sizeof(MCB);
    for (block=CFreeList.prev;block!=&CFreeList;block=block->prev) {
        Ccoalesce(block);
        if (block->len>=amount) {
            if (block->len-amount>sizeof(MCB)) {
                block->len-=amount;
                block=(MCB *)((char *)block+block->len);
                block->len=amount;
            } else {
                prev=block->prev;
                next=block->next;
                prev->next=next;
                next->prev=prev;
            }
            block->len|=1;
            return (char *)block+sizeof(int);
        }
    }
    if (amount>PermAvail) return NULL;
    PermAvail-=amount;
    block=(MCB *)(PermPtr+PermAvail);
    block->len=amount|1;
    return (char *)block+sizeof(int);
}

void FEfree(void *memory)
{
    if (memory!=NULL) free(memory);
}

char *PCH_Start,*PCH_End;

void CMemFree(void *memory)
{
    MCB *block,*prev,*next;
    unsigned length;

    if (memory==NULL || (char *)memory<PermPtr) return;
    if ((char *)memory>=PCH_Start && (char *)memory<PCH_End) return;
    block=(MCB *)((char *)memory-sizeof(int));
    length=block->len&0xfffe;
    if ((char *)block==PermPtr+PermAvail) {
        PermAvail+=length;
        if (CFreeList.next!=&CFreeList) {
            Ccoalesce(CFreeList.next);
            block=CFreeList.next;
            if ((char *)block==PermPtr+PermAvail) {
                next=block->next;
                prev=block->prev;
                prev->next=next;
                next->prev=prev;
                PermAvail+=block->len;
            }
        }
    } else {
        block->len=length;
        prev=&CFreeList;
        for (;;) {
            next=prev;
            prev=prev->prev;
            if (prev==&CFreeList || prev<block) break;
        }
        next->prev=block;
        prev->next=block;
        block->prev=prev;
        block->next=next;
        Ccoalesce(block);
    }
}

void Ccoalesce(MCB *block)
{
    MCB *next;
    for (;;) {
        next=(MCB *)((char *)block+block->len);
        if (next->len & 1) break;
        block->len+=next->len;
        next->prev->next=next->next;
        next->next->prev=next->prev;
    }
}
