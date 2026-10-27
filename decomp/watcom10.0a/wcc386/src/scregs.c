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


#include "score.h"
#include "stubs.h"
#include <recon.h>

unsigned char RegsEqual(score *sc,int left,int right)
{
    return sc[left].list==sc[right].list;
}

void ScoreFreeList(score *sc);
void RegInsert(score *sc,int dst_idx,int src_idx)
{
    score *src,*dst,*next;
    unsigned char generation;
    list_head **free_heads;
    src=&sc[src_idx];
    dst=&sc[dst_idx];
    ScoreFreeList(dst);
    free_heads=(list_head **)&sc[ScoreCount];
    *dst->list=(list_head)*free_heads;
    *free_heads=dst->list;
    next=dst;
    do {
        next->list=src->list;
        next=next->next_reg;
    } while (next!=dst);
    next=src;
    generation=0;
    do {
        next=next->next_reg;
        ++generation;
    } while (next!=src);
    dst->generation=generation;
    src->prev_reg->next_reg=dst;
    dst->prev_reg->next_reg=src;
    next=src->prev_reg;
    src->prev_reg=dst->prev_reg;
    dst->prev_reg=next;
}

extern score_reg **ScoreList;
void FreeScListEntry(score_list *entry);
void RegKill( score *scoreboard, hw_reg_set regs ) {
/*************************************************************/

    score_reg   *entry;
    score_list  *curr_list;
    score_list  **owner;
    int         i;
    list_head   **free_heads;

    if( regs!=0 ) {
        entry = *ScoreList;
        i = ScoreCount;
        free_heads = (list_head **)&scoreboard[ ScoreCount ];
        while( -- i >= 0 ) {
            if( (entry->reg&regs) ) {
                if( scoreboard->list != NULL ) {
                    if( scoreboard->next_reg == scoreboard ) {
                        ScoreFreeList( scoreboard );
                    } else {
                        scoreboard->list = *free_heads;
                        *free_heads = (list_head *)**free_heads;
                        *scoreboard->list = NULL;
                    }
                }
                scoreboard->prev_reg->next_reg = scoreboard->next_reg;
                scoreboard->next_reg->prev_reg = scoreboard->prev_reg;
                scoreboard->next_reg = scoreboard;
                scoreboard->prev_reg = scoreboard;
                scoreboard->generation = 0;
            } else {
                owner = scoreboard->list;
                for(;;) {
                    curr_list = *owner;
                    if( curr_list == NULL ) break;
                    if( curr_list->info.index_reg != NO_INDEX
                     && (ScoreList[curr_list->info.index_reg]->reg&regs) ) {
                        *owner = curr_list->next;
                        FreeScListEntry( curr_list );
                    } else {
                        owner = &curr_list->next;
                    }
                }
            }
            ++ entry;
            ++ scoreboard;
        }
    }
}

unsigned char ScAddOk(hw_reg_set left,hw_reg_set right);
void MergeDown(score *sc,int dst,int src)
{
    int dst_hi,src_hi,dst_lo,src_lo;
    for (;;) {
        dst_hi=ScoreList[dst]->high;
        src_hi=ScoreList[src]->high;
        if (dst_hi!=NO_INDEX && src_hi!=NO_INDEX && !RegsEqual(sc,src_hi,dst_hi)) {
            RegInsert(sc,dst_hi,src_hi);
            MergeDown(sc,dst_hi,src_hi);
        }
        src_lo=ScoreList[src]->low;
        dst_lo=ScoreList[dst]->low;
        if (dst_lo==NO_INDEX || src_lo==NO_INDEX || RegsEqual(sc,src_lo,dst_lo)) return;
        RegInsert(sc,dst_lo,src_lo);
        /* The low-half recursion is the actual tail loop. */
        dst=dst_lo;
        src=src_lo;
    }
}
void MergeUp(score *sc,int dst,int src)
{
    int dst_hi,src_hi,dst_lo,src_lo;
    for (;;) {
        dst_hi=ScoreList[dst]->high_of;
        src_hi=ScoreList[src]->high_of;
        dst_lo=ScoreList[dst]->low_of;
        src_lo=ScoreList[src]->low_of;
        if (dst_hi!=NO_INDEX && src_hi!=NO_INDEX) {
            dst_lo=ScoreList[dst_hi]->low;
            src_lo=ScoreList[src_hi]->low;
            if (dst_lo==NO_INDEX || src_lo==NO_INDEX || !RegsEqual(sc,dst_lo,src_lo)
              || RegsEqual(sc,dst_hi,src_hi)) return;
            RegInsert(sc,dst_hi,src_hi);
            dst=dst_hi;
            src=src_hi;
        } else if (dst_lo!=NO_INDEX && src_lo!=NO_INDEX) {
            dst_hi=ScoreList[dst_lo]->high;
            src_hi=ScoreList[src_lo]->high;
            if (dst_hi==NO_INDEX || src_hi==NO_INDEX || !RegsEqual(sc,dst_hi,src_hi)
              || RegsEqual(sc,dst_lo,src_lo)) return;
            RegInsert(sc,dst_lo,src_lo);
            dst=dst_lo;
            src=src_lo;
        } else return;
    }
}
void RegAdd(score *sc,int dst,int src)
{
    /* A move has just made dst equal to src, not conversely. */
    if (ScoreList[dst]->size!=ScoreList[src]->size) return;
    if (!ScAddOk(ScoreList[dst]->reg,ScoreList[src]->reg)) return;
    if (RegsEqual(sc,dst,src)) return;
    MergeDown(sc,dst,src);
    MergeUp(sc,dst,src);
    RegInsert(sc,dst,src);
}
