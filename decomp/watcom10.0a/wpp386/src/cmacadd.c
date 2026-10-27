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

/* MacroStorageInit from the actual1994 body. Later cmacadd.c factors
 * these calls into storageInitialize and adds PCH/counter state. */
#include <string.h>
#include <stddef.h>
#include "compiler.h"
#include "macro.h"
#include <recon.h>

#define MACRO_HASH_SIZE 4093
static MEPTR *macroHashTable;
static unsigned macroHashBucket;
typedef struct macro_seg_list {
    struct macro_seg_list *next;
    char segment[0x8000 - sizeof(char *)];
} MACRO_SEG_LIST;
static MACRO_SEG_LIST *macroSegmentList, *macroSegment;
static char *macroLimit;
char *MacroOffset;

void MacroStorageInit(void)
{
    unsigned amount;

    amount = MACRO_HASH_SIZE * sizeof(*macroHashTable);
    macroAllocSegment(amount);
    macroHashTable = (MEPTR *)macroAllocateInSeg(amount);
    memset(macroHashTable, 0, amount);
}

void macroAllocSegment(unsigned minimum)
{
    if (minimum > sizeof(macroSegment->segment) - 2) {
        CErr1(100);
        CSuicide();
    }
    macroSegment = (MACRO_SEG_LIST *)RingAlloc((void **)&macroSegmentList,
                                               sizeof(*macroSegment));
    MacroOffset = macroSegment->segment;
    macroLimit = MacroOffset + sizeof(macroSegment->segment) - 2;
}

void *macroAllocateInSeg(unsigned size)
{
    void *result;

    result = MacroOffset;
    MacroOffset += (size + 3) & ~3u;
    return result;
}

/* The1994 outer definition routine copies a temporary record/payload into
 * segment storage, completes it via macroAdd, and frees the temporary.
 * The released2002 MacroDefine works directly in the segment instead. */
void MacroDefine(MEPTR entry, char *definition, unsigned length)
{
    unsigned name_size, total;

    name_size = strlen(entry->macro_name) + offsetof(MEDEFN, macro_name) + 1;
    if (length) entry->macro_defn = name_size;
    total = name_size + length;
    entry->macro_len = total;
    MacroOverflow(total, 0);
    macroCopy(entry, MacroOffset, name_size);
    if (length) macroCopy(definition, MacroOffset + name_size, length);
    macroAdd(entry, total);
    CMemFree(entry);
}

void MacroOverflow(unsigned amount_needed, unsigned amount_used)
{
    char *old_offset;

    amount_needed = (amount_needed + 3) & ~3u;
    if (MacroOffset + amount_needed > macroLimit) {
        old_offset = MacroOffset;
        macroAllocSegment(amount_needed);
        if (amount_used) macroCopy(old_offset, MacroOffset, amount_used);
    }
}

/* Original helper has source first; its CRT memcpy call has dest first. */
void macroCopy(const void *source, void *dest, unsigned length)
{
    memcpy(dest, source, length);
}

/* This completion step corresponds to the core of the released MacroDefine.
 * Actual1994 has no override/unlink/browse branches. The six-byte epilogue
 * atdb70 is reached by a tail jump from this distinct logical entry. */
MEPTR macroAdd(MEPTR entry, unsigned length)
{
    MEPTR current, old_entry, result;
    char *name;

    macroCopy(entry, MacroOffset, offsetof(MEDEFN, macro_name) + 1);
    current = (MEPTR)MacroOffset;
    name = current->macro_name;
    result = 0;
    if (magicPredefined(name)) {
        CErr2p(648, name);
    } else {
        old_entry = MacroLookup(name);
        if (!old_entry) {
            current->macro_flags = InitialMacroFlag;
            result = (MEPTR)macroAllocateInSeg(length);
            RingAppend((void **)&macroHashTable[macroHashBucket], result);
        } else if (macroCompare(current, old_entry)) {
            if (!old_entry->macro_defn) CErr2p(648, name);
            else CErr2p(86, name);
        }
    }
    return result;
}

/* A macro in the released source, but a separate callable1994 entry. */
int magicPredefined(char *name)
{
    return strcmp("defined", name) == 0;
}

MEPTR MacroLookup(char *name)
{
    MEPTR last, curr;

    macroHash(name);
    last = macroHashTable[macroHashBucket];
    if (last) {
        curr = last;
        do {
            curr = curr->next_macro;
            if (curr->macro_name[0] == name[0] &&
                !strcmp(curr->macro_name, name)) return curr;
        } while (curr != last);
    }
    return 0;
}

unsigned macroHash(const char *name)
{
    unsigned hash, fold;
    unsigned char c;

    hash = 0;
    while ((c = (unsigned char)*name++) != 0) {
        hash = (hash << 4) + c;
        fold = hash & ~0xfffu;
        hash ^= fold;
        hash ^= fold >> 12;
    }
    hash &= 0xfff;
    if (hash >= MACRO_HASH_SIZE) hash -= MACRO_HASH_SIZE;
    macroHashBucket = hash;
    return hash;
}

static unsigned undefCount;
void MacroStateGet(MACRO_STATE *state)
{
    state->curr_offset=MacroOffset;
    state->undef_count=undefCount;
}

int MacroStateMatchesCurrent(MACRO_STATE *state)
{
    if (state->curr_offset!=MacroOffset) return 0;
    if (state->undef_count!=undefCount) return 0;
    return 1;
}

int MacroExists(char *name)
{
    MEPTR entry=MacroLookup(name);
    if (!entry) return 0;
    entry->macro_flags |= 8;
    return 1;
}

MEPTR MacroSearch(void)
{
    return MacroLookup(Buffer);
}

void RingFree(void **head);
void MacroStorageFini(void)
{ RingFree((void **)&macroSegmentList); }
