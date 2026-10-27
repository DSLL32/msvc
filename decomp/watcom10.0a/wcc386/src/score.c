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
* Description:  Basic type initialization and type-node allocation.
*
****************************************************************************/


/* cgen2.c's emission-state initializer. In the earlier target the next
 * imported segment starts at -1 directly, before the later SegData field. */
#include "block.h"
#include "cgmemory.h"
#include "memory.h"
#include "genopt.h"
#include "score.h"
#include "stubs.h"
#include <recon.h>
void *ScListFrl;

void ScoreInit(void)
{
    InitFrl(&ScListFrl);
}

struct score_info;
struct score_reg;
struct score_info *ScZero;
struct score_reg **ScoreList;
void ConstSizes(void);
void ScoreRoutine(void);
void ScoreCleanUp(void);
int Spawn(void (*callback)(void));
void ProcMessage(int message);
void Score(void)
{
    unsigned char old_memout;
    block *blk;
    ConstSizes();
    old_memout=SetMemOut(MO_SUICIDE);
    ScZero=NULL;
    ScoreList=NULL;
    for (blk=HeadBlock;blk!=NULL;blk=blk->next_block) blk->cc=NULL;
    if (Spawn(ScoreRoutine)!=0) ProcMessage(9);
    ScoreCleanUp();
    SetMemOut(old_memout);
}
extern name *Names[5];
void ConstSizes(void)
{
    name *cons;
    for (cons=Names[0];cons!=NULL;cons=cons->n.next_name) {
        if (cons->c.const_type==0)
            cons->n.size=CFIsU16(cons->c.value)?2:4;
    }
}
int ScoreCount;
void ScoreCalcList(void);
void InitZero(void);
void *ScAlloc(int size);
void ScFree(void *owner,int size);
void ScoreClear(score *sc);
void FreeScoreBoard(score *sc);
void ScInitRegs(score *sc);
unsigned char DoScore(block *blk);
void ScoreDescendants(block *blk);
void ScoreRoutine(void)
{
    block *blk;
    ScoreCalcList();
    if (ScoreCount!=0) {
        InitZero();
        for (blk=HeadBlock;blk!=NULL;blk=blk->next_block) blk->class&=~0x800U;
        MakeLiveInfo();
        for (blk=HeadBlock;blk!=NULL;blk=blk->next_block) {
            if (!(blk->class&0x800U)) {
                blk->cc=ScAlloc(ScoreCount*(sizeof(score)+sizeof(list_head))+sizeof(list_head));
                ScoreClear(blk->cc);
                for (;;) {
                    FreeScoreBoard(blk->cc);
                    ScInitRegs(blk->cc);
                    if (!DoScore(blk)) break;
                    UpdateLive(blk->ins.hd.next,blk->ins.hd.prev);
                }
                blk->class|=0x800U;
                ScoreDescendants(blk);
                FreeScoreBoard(blk->cc);
                ScFree(blk->cc,ScoreCount*(sizeof(score)+sizeof(list_head))+sizeof(list_head));
                blk->cc=NULL;
            }
        }
    }
}
void InitZero(void)
{
    ScZero=ScAlloc(sizeof(score_info));
    ScZero->class=0;
    ScZero->offset=0;
    ScZero->symbol.p=NULL;
    ScZero->index_reg=NO_INDEX;
    ScZero->base=NULL;
}

void ScoreCopy(score *source,score *dest);
void ScoreSeed(block *blk,block *son,unsigned index);
void ScoreDescendants(block *blk)
{
    int i;
    block *son;
    hw_reg_set regs;
    i=blk->targets;
    while (--i>=0) {
        son=blk->edge[i].destination;
        if (son->inputs==1 && !(son->class&0x800U)) {
            son->cc=ScAlloc(ScoreCount*(sizeof(score)+sizeof(list_head))+sizeof(list_head));
            ScoreClear(son->cc);
            for (;;) {
                ScoreCopy(blk->cc,son->cc);
                ScoreSeed(blk,son,i);
                if (!DoScore(son)) break;
                UpdateLive(son->ins.hd.next,son->ins.hd.prev);
            }
            son->class|=0x4800U;
            /* Historical callback boundary transports one block pointer.
             * This void callback's EAX result is discarded, not a defined
             * pointer result. Native SafeRecurse is normal-path only. */
            SafeRecurse((void *(*)(void *))ScoreDescendants,son);
            FreeScoreBoard(son->cc);
            ScFree(son->cc,ScoreCount*(sizeof(score)+sizeof(list_head))+sizeof(list_head));
            son->cc=NULL;
        }
    }
    regs=0;
    i=blk->targets;
    while (--i>=0) {
        son=blk->edge[i].destination;
        if (son->class&0x4000U) {
            regs|=son->ins.hd.next->head.live.regs;
            son->class&=~0x4000U;
        }
    }
    blk->ins.hd.live.regs|=regs;
    UpdateLive(blk->ins.hd.next,blk->ins.hd.prev);
}
void CopyList(score *source,score *dest,list_head **heads,int index);
void ScoreCopy(score *source,score *dest)
{
    list_head **heads;
    int i;
    FreeScoreBoard(dest);
    heads=(list_head **)&dest[ScoreCount];
    i=ScoreCount;
    do {
        --i;
        dest[i].next_reg=&dest[source[i].next_reg->index];
        dest[i].prev_reg=&dest[source[i].prev_reg->index];
        dest[i].generation=source[i].generation;
        dest[i].list=NULL;
        *heads=(list_head *)heads+1;
        ++heads;
    } while (i!=0);
    *heads=NULL;
    i=ScoreCount;
    heads=(list_head **)&dest[ScoreCount];
    do {
        --i;
        CopyList(source,dest,heads,i);
    } while (i!=0);
}
score_list *NewScListEntry(void);
void CopyList(score *source,score *dest,list_head **heads,int index)
{
    score_list *first,*entry;
    score *next;
    if (dest[index].list==NULL) {
        dest[index].list=*heads;
        *heads=(list_head *)**heads;
        *dest[index].list=NULL;
        next=dest[index].next_reg;
        while (next->list==NULL) {
            next->list=next->prev_reg->list;
            next=next->next_reg;
        }
    }
    if (*dest[index].list==NULL) {
        next=dest[index].next_reg;
        first=*source[index].list;
        while (first!=NULL) {
            entry=NewScListEntry();
            Copy((char *)&first->info,(char *)&entry->info,sizeof(score_info));
            entry->next=*next->list;
            *next->list=entry;
            first=first->next;
        }
    }
}

void ScoreMakeEqual(score *sc,name *left,name *right);
void ScoreSeed(block *blk,block *son,unsigned index)
{
    instruction *cmp;
    unsigned which;
    if (!(blk->class&4) || (blk->class&0x8000U)) return;
    cmp=blk->ins.hd.prev;
    while (cmp->head.opcode==0) cmp=cmp->head.prev;
    switch (cmp->head.opcode) {
    case 0x30: which=cmp->flags.bool&15;break;
    case 0x31: which=(cmp->flags.bool>>4)&15;break;
    default: return;
    }
    if (which!=index) return;
    ScoreMakeEqual(son->cc,cmp->operands[0],cmp->operands[1]);
}

/* scmain.c's file-local CleanUp is distinct from the existing reference
 * optimizer CleanUp; retain its actual separate call identity. */
void ScoreCleanUp(void)
{
    block *blk;
    blk=HeadBlock;
    while (blk!=NULL) {
        FreeScoreBoard(blk->cc);
        ScFree(blk->cc,ScoreCount*(sizeof(score)+sizeof(list_head))+sizeof(list_head));
        blk->cc=NULL;
        blk=blk->next_block;
    }
    ScFree(ScoreList,ScoreCount*(sizeof(void *)+sizeof(score_reg)));
    ScFree(ScZero,sizeof(score_info));
}
void ProcMessage(int message) { STUB_TRAP(); }

void ScoreFini(void) { STUB_TRAP(); }
