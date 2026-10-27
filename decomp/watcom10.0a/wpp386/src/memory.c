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

/* Adapted from bld/cg/c/memmgr.c at Open Watcom's initial commit
 * 6b9cb4438959d845a2b4dd4bf19f7908ecd608e9. Checked against actual
 * WPP386 10.0a CF0x81ba3..0x81c00. Its private compiler allocator is
 * App code even though startup reaches it through malloc/nmalloc aliases.
 * Recovered tag/word size4, minimum allocation16 and maximum class7.
 * The surrounding allocator is introduced only as its bodies are reached. */
#include <windows.h>
#include "memory.h"
#include <recon.h>

int Initialized = 0;
unsigned AllocSize, MemorySize;
/* Encountered with GetFromFrl: eight empty circular lists share this sentinel.
 * Original CF0xb9778: tag1, class0 and sixteen self-referencing pointers. */
frl _FreeList = {
    1, 0, {
        { &_FreeList, &_FreeList }, { &_FreeList, &_FreeList },
        { &_FreeList, &_FreeList }, { &_FreeList, &_FreeList },
        { &_FreeList, &_FreeList }, { &_FreeList, &_FreeList },
        { &_FreeList, &_FreeList }, { &_FreeList, &_FreeList }
    }
};

int SizeToClass(unsigned amount)
{
    int cls;

    if (amount >= 2048) return 7;
    if (amount < 32) return 0;
    amount >>= 4;
    cls = 0;
    do {
        ++cls;
        amount >>= 1;
    } while (amount);
    return cls;
}

void *GetFromFrl(unsigned amount, int frl_class)
{
    frl *alloc, *free;
    int i, cls, amount_left;

    free = _FreeList.link[frl_class].next;
    while (free != &_FreeList) {
        amount_left = (int)(free->length - amount);
        if (amount_left >= 0) {
            if (amount_left >= 16) {
                cls = SizeToClass(amount_left);
                for (i = cls + 1; i <= free->class; ++i) {
                    free->link[i].next->link[i].prev = free->link[i].prev;
                    free->link[i].prev->link[i].next = free->link[i].next;
                }
                free->class = cls;
                free->length = amount_left;
                alloc = (frl *)((char *)free + amount_left);
                alloc->length = amount;
            } else {
                for (i = 0; i <= free->class; ++i) {
                    free->link[i].next->link[i].prev = free->link[i].prev;
                    free->link[i].prev->link[i].next = free->link[i].next;
                }
                alloc = free;
            }
            AllocSize += alloc->length;
            alloc->length |= 1;
            return alloc;
        }
        free = free->link[frl_class].next;
    }
    return 0;
}

/* The 1994 OSI variant allocates a whole64KiB multiple through AH48.
 * Actual loader dispatch translates that boundary to LocalAlloc(FIXED,size).
 * Unlike the later source, no mem_blk header or BLK_ADJUST is present here. */
void *MemFromSys(unsigned amount)
{
    char *ptr;
    unsigned size;

    size = (amount + 65535u) & ~65535u;
    ptr = (char *)LocalAlloc(LMEM_FIXED, size);
    if (ptr) {
        AddBlock(ptr, size / 16);
        return MemAlloc(amount);
    }
    return 0;
}

/* Remove one free chunk from every indexed list, first repairing the saved
 * insertion neighbours. Actual body ends at81c5b; MemBlkSize follows it. */
void UnHook(frl *unhook, frl *neighbours)
{
    int i;
    next_prev *neighbour_link, *unhook_link;

    unhook_link = unhook->link;
    neighbour_link = neighbours->link;
    for (i = 0; i <= unhook->class; ++i) {
        if (neighbour_link->next == unhook)
            neighbour_link->next = unhook_link->next;
        if (neighbour_link->prev == unhook)
            neighbour_link->prev = unhook_link->prev;
        ++neighbour_link;
        ++unhook_link;
    }
    unhook_link = unhook->link;
    for (i = 0; i <= unhook->class; ++i) {
        unhook_link->next->link[i].prev = unhook_link->prev;
        unhook_link->prev->link[i].next = unhook_link->next;
        ++unhook_link;
    }
}

void MemFree(char *block)
{
    int i;
    frl *free, *prev, *next, *search;
    frl neighbours;
    next_prev *neighbour_link, *free_link;

    free = (frl *)(block - 4);
    free->length &= ~1u;
    AllocSize -= free->length;
    search = &_FreeList;
    neighbour_link = &neighbours.link[7];
    for (i = 7; i >= 0; --i) {
        for (;;) {
            prev = search;
            search = search->link[i].next;
            if ((unsigned)search > (unsigned)free) break;
            if (search->length == 1) break;
        }
        search = prev;
        neighbour_link->prev = prev;
        neighbour_link->next = prev->link[i].next;
        --neighbour_link;
    }
    next = prev->link[0].next;
    if ((frl *)((char *)prev + prev->length) == free) {
        UnHook(prev, &neighbours);
        prev->length += free->length;
        free = prev;
    }
    if ((frl *)((char *)free + free->length) == next) {
        UnHook(next, &neighbours);
        free->length += next->length;
    }
    free->class = SizeToClass(free->length);
    neighbour_link = neighbours.link;
    free_link = free->link;
    for (i = 0; i <= free->class; ++i) {
        neighbour_link->prev->link[i].next = free;
        neighbour_link->next->link[i].prev = free;
        free_link->next = neighbour_link->next;
        free_link->prev = neighbour_link->prev;
        ++free_link;
        ++neighbour_link;
    }
}

/* OSI blocks end in an allocated tag. The usable part joins the private heap. */
unsigned AddBlock(char *block, unsigned paragraphs)
{
    unsigned length;
    frl *free;

    if (!paragraphs) return 0;
    length = paragraphs * 16 - 4;
    ((frl *)(block + length))->length = 1;
    free = (frl *)block;
    free->length = length;
    AllocSize += length;
    MemFree(block + 4);
    return length;
}

void *MemAlloc(unsigned amount)
{
    char *chunk;
    int cls;

    if (!Initialized) MemInit();
    if (!amount) return 0;
    amount = (amount + 7) & ~3u;
    if (amount < 16) amount = 16;
    cls = SizeToClass(amount);
    for (;;) {
        chunk = (char *)GetFromFrl(amount, cls);
        if (chunk) return chunk + 4;
        if (cls == 7) break;
        ++cls;
    }
    return MemFromSys(amount);
}

/* Actual1994 MemInit contains the CalcMemSize tail in the same function.
 * Its default is4MiB; the later source uses16MiB and initializes a free-list
 * sentinel here that was already static data in the shipped1994 program. */
void MemInit(void)
{
    unsigned requested;
    unsigned char query_max, query_size;
    char buff[80];

    if (Initialized) return;
    Initialized = 1;
    AllocSize = 0;
    Initialized = 2;
    requested = 0;
    query_max = 0;
    query_size = 0;
    if (GetEnvVar("WCGMEMORY", buff, 9)) {
        if (buff[0] == '?' && !buff[1]) query_max = 1;
        else if (buff[0] == '#' && !buff[1]) query_size = 1;
        else requested = (unsigned)myatoi(buff) * 1024;
    }
    MemorySize = requested ? requested : 4u * 1024 * 1024;
    if (query_max || query_size) {
        myitoa(4096, CopyStr("Maximum WCGMEMORY=", buff));
        write(1, buff, Length(buff));
        write(1, "\r\n", 2);
    }
}

/* Original OSI allocator's empty coalescing hook shares MemFree's C3.
 * It is statically identified and case-seeded, not an observed entry. */
void MemCoalesce(void) { }
unsigned MemSize(void)
{
    if (!Initialized) MemInit();
    return MemorySize;
}
