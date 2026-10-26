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


/* Adapted from the initial Open Watcom memmgr.c (2002), checked against
 * WCC3861994 CF47c77..47cd4. This private compiler heap is App code;
 * the malloc/nmalloc wrapper does not turn it into CRT support. */
#include <windows.h>
#include "memory.h"
#include <recon.h>

int Initialized;
unsigned AllocSize,MemorySize;
/* One allocated sentinel, with eight circular list heads. These shared
 * pointers have observable identity in the allocator's list operations. */
FreeList _FreeList = {
    1, 0, {
        {&_FreeList,&_FreeList}, {&_FreeList,&_FreeList},
        {&_FreeList,&_FreeList}, {&_FreeList,&_FreeList},
        {&_FreeList,&_FreeList}, {&_FreeList,&_FreeList},
        {&_FreeList,&_FreeList}, {&_FreeList,&_FreeList}
    }
};

void *MemAlloc(unsigned amount)
{
    char *chunk;
    unsigned cls;

    if (!Initialized) MemInit();
    if (!amount) return 0;
    amount=(amount+7)&~3u;
    if (amount<16) amount=16;
    cls=SizeToClass(amount);
    for (;;) {
        chunk=(char *)GetFromFrl(amount,cls);
        if (chunk) return chunk+4;
        if (cls==7) break;
        ++cls;
    }
    return MemFromSys(amount);
}

/* Actual1994 MemInit shares the former CalcMemSize tail, without a call
 * vertex. Its default is4MiB (2002's16MiB differs); its free-list sentinel is
 * statically initialized elsewhere, not filled here as in later MemInit. */
void MemInit(void)
{
    unsigned requested;
    int query_max,query_size;
    char buff[80];

    if (Initialized) return;
    Initialized=1;
    AllocSize=0;
    Initialized=2;
    requested=0;
    query_max=0;
    query_size=0;
    if (GetEnvVar("WCGMEMORY",buff,9)) {
        if (buff[0]=='?' && !buff[1]) query_max=1;
        else if (buff[0]=='#' && !buff[1]) query_size=1;
        else requested=(unsigned)myatoi(buff)*1024;
    }
    MemorySize=requested ? requested : 4u*1024*1024;
    if (query_max || query_size) {
        myitoa(4096,CopyStr("Maximum WCGMEMORY=",buff));
        write(1,buff,Length(buff));
        write(1,"\r\n",2);
    }
}

unsigned SizeToClass(unsigned amount)
{
    unsigned cls;

    if (amount>=2048) return 7;
    if (amount<32) return 0;
    amount>>=4;
    cls=0;
    do {
        ++cls;
        amount>>=1;
    } while (amount);
    return cls;
}

void *GetFromFrl(unsigned amount,int frl_class)
{
    FreeList *alloc;
    FreeList *free;
    int i;
    int cls;
    int amount_left;

    free=_FreeList.link[frl_class].next;
    while (free!=&_FreeList) {
        amount_left=(int)(free->length-amount);
        if (amount_left>=0) {
            if (amount_left>=16) {
                cls=SizeToClass((unsigned)amount_left);
                for (i=cls+1;i<=free->class;++i) {
                    free->link[i].next->link[i].prev=free->link[i].prev;
                    free->link[i].prev->link[i].next=free->link[i].next;
                }
                free->class=cls;
                free->length=(unsigned)amount_left;
                alloc=(FreeList *)((char *)free+free->length);
                alloc->length=amount;
            } else {
                for (i=0;i<=free->class;++i) {
                    free->link[i].next->link[i].prev=free->link[i].prev;
                    free->link[i].prev->link[i].next=free->link[i].next;
                }
                alloc=free;
            }
            AllocSize+=alloc->length;
            alloc->length|=1;
            return alloc;
        }
        free=free->link[frl_class].next;
    }
    return 0;
}

/* The original OSI AH48 service is LocalAlloc(LMEM_FIXED,bytes) in the
 * actual Win32 loader. Call that API normally at the authorized service
 * boundary, retaining the original private-heap graph on either side. */
void *MemFromSys(unsigned amount)
{
    char *ptr;
    unsigned size;

    size=(amount+65535u)&~65535u;
    ptr=(char *)LocalAlloc(LMEM_FIXED,size);
    if (ptr) {
        AddBlock(ptr,(int)(size>>4));
        return MemAlloc(amount);
    }
    return 0;
}

unsigned AddBlock(char *block,int paragraphs)
{
    unsigned length;
    FreeList *free;

    if (!paragraphs) return 0;
    length=(unsigned)paragraphs*16u-4u;
    ((FreeList *)(block+length))->length=1;
    free=(FreeList *)block;
    free->length=length;
    AllocSize+=length;
    MemFree(block+4);
    return length;
}

void MemFree(char *block)
{
    int i;
    FreeList *free;
    FreeList *prev;
    FreeList *next;
    FreeList *search;
    FreeList neighbours;
    FreeLinks *neighbour_link;
    FreeLinks *free_link;

    free=(FreeList *)(block-4);
    free->length&=~1u;
    AllocSize-=free->length;
    search=&_FreeList;
    neighbour_link=&neighbours.link[7];
    for (i=7;i>=0;--i) {
        for (;;) {
            prev=search;
            search=search->link[i].next;
            if (search>free) break;
            if (search->length==1) break;
        }
        search=prev;
        neighbour_link->prev=prev;
        neighbour_link->next=prev->link[i].next;
        --neighbour_link;
    }
    next=prev->link[0].next;
    if ((FreeList *)((char *)prev+prev->length)==free) {
        UnHook(prev,&neighbours);
        prev->length+=free->length;
        free=prev;
    }
    if ((FreeList *)((char *)free+free->length)==next) {
        UnHook(next,&neighbours);
        free->length+=next->length;
    }
    free->class=SizeToClass(free->length);
    neighbour_link=neighbours.link;
    free_link=free->link;
    for (i=0;i<=free->class;++i) {
        neighbour_link->prev->link[i].next=free;
        neighbour_link->next->link[i].prev=free;
        free_link->next=neighbour_link->next;
        free_link->prev=neighbour_link->prev;
        ++free_link;
        ++neighbour_link;
    }
}

void UnHook(FreeList *unhook,FreeList *neighbours)
{
    int i;
    FreeLinks *neighbour_link,*unhook_link;
    neighbour_link=neighbours->link;
    unhook_link=unhook->link;
    for (i=0;i<=unhook->class;++i) {
        if (neighbour_link->next==unhook) neighbour_link->next=unhook_link->next;
        if (neighbour_link->prev==unhook) neighbour_link->prev=unhook_link->prev;
        ++neighbour_link;
        ++unhook_link;
    }
    unhook_link=unhook->link;
    for (i=0;i<=unhook->class;++i) {
        unhook_link->next->link[i].prev=unhook_link->prev;
        unhook_link->prev->link[i].next=unhook_link->next;
        ++unhook_link;
    }
}
