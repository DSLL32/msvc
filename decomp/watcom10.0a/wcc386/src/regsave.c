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
* Description:  Initialize register costs and weight savings by loop depth.
*
****************************************************************************/


/* cgen2.c's emission-state initializer. In the earlier target the next
 * imported segment starts at -1 directly, before the later SegData field. */
#include "codegen.h"
#include "regsave.h"
#include "block.h"
#include "stubs.h"
#include <recon.h>
struct savings Save;
#define COST(size_cost,time_cost) (((size_cost)*size+(time_cost)*time)>>8)

void InitWeights(unsigned size)
{
    unsigned time;
    AdjTimeSize(&time,&size);
    SetLoopCost(time);
    if ((CGProcessorVersion&15)>=4) {
        SetCost(Save.load_cost,COST(3,1));
        SetCost(Save.store_cost,COST(3,1));
        SetCost(Save.use_save,COST(1,1));
        SetCost(Save.def_save,COST(1,1));
        SetCost(Save.push_cost,COST(1,1));
        SetCost(Save.pop_cost,COST(1,1));
    } else if ((CGProcessorVersion&15)>=3) {
        SetCost(Save.load_cost,COST(3,4));
        SetCost(Save.store_cost,COST(3,2));
        SetCost(Save.use_save,COST(1,2));
        SetCost(Save.def_save,COST(1,2));
        SetCost(Save.push_cost,COST(1,2));
        SetCost(Save.pop_cost,COST(1,4));
    } else {
        SetCost(Save.load_cost,COST(3,17));
        SetCost(Save.store_cost,COST(3,18));
        SetCost(Save.use_save,COST(1,15));
        SetCost(Save.def_save,COST(1,18));
        SetCost(Save.push_cost,COST(1,11));
        SetCost(Save.pop_cost,COST(1,8));
    }
    Save.index_save=Save.load_cost[4]; /* WD in the old type-class order. */
}

void AdjTimeSize(unsigned *time,unsigned *size)
{
    if (*size>100) *size=256;
    else *size=(*size<<8)/100;
    *time=256-*size;
}

void SetLoopCost(unsigned time)
{
    unsigned weight;
    int i;
    weight=(20*time)>>8;
    if (weight==0) weight=1;
    Save.loop_weight[0]=1;
    for (i=1;i<=5;++i) Save.loop_weight[i]=Save.loop_weight[i-1]*weight;
}

void SetCost(save_def *cost,save_def value)
{
    int i;
    for (i=0;i<SAVE_CLASS_COUNT;++i) cost[i]=value;
}

unsigned Weight(unsigned value,block *blk)
{
    if (blk->depth<5) value*=Save.loop_weight[blk->depth];
    else value*=Save.loop_weight[5];
    return value;
}

void CalcLoadStore(conflict_node *conf);
name *DeAlias(name *opnd);
#define GLOBAL_OVERLAP(a,b) (((a).bits[0]&(b).bits[0])|((a).bits[1]&(b).bits[1])| \
                            ((a).bits[2]&(b).bits[2])|((a).bits[3]&(b).bits[3]))
#define SAVINGS_EQUAL(a,b) (((a)->n.class==2?DeAlias(a):(a))==(b))
/* The old compiler inlines Weight at these two sites. */
#define WEIGHTED(value,blk) ((value)*Save.loop_weight[(blk)->depth<5?(blk)->depth:5])
void CalcSavings(conflict_node *conf)
{
    save_def block_save,block_cost,cost,save;
    name *opnd;
    block *blk;
    instruction *ins,*next,*last_defn;
    data_flow_def *flow;
    unsigned char class,last,flows_in;
    int i;
    opnd=conf->name;
    if ((opnd->v.usage&2) && (opnd->v.usage&0x88)) CalcLoadStore(conf);
    if ((conf->state&4) ||
        (conf->available==0 && !(conf->state&0x240)) ||
        conf->ins_range.first==conf->ins_range.last) {
        conf->savings=0;
        return;
    }
    cost=save=block_cost=block_save=0;
    class=opnd->n.name_class;
    blk=conf->start_block;
    ins=conf->ins_range.first;
    last=0;
    flows_in=(ins->head.live.within_block&conf->id.within_block)!=0;
    for (;;) {
        flow=blk->dataflow;
        last_defn=NULL;
        while (ins->head.opcode!=0x4b) {
            next=ins->head.next;
            if (ins->head.opcode==0x36 || ins->head.opcode==0x29) {
                if (((opnd->v.usage&8) ||
                     ((opnd->v.usage&0x80) && !(conf->state&0x2000))) &&
                    !GLOBAL_OVERLAP(conf->id.out_of_block,flow->call_exempt)) {
                    if (!(ins->flags.call_flags&2) ||
                        (opnd->n.class==2 && (opnd->v.usage&8)))
                        block_cost+=Save.load_cost[class];
                    if ((last_defn!=NULL || ((opnd->v.usage&2) &&
                         GLOBAL_OVERLAP(conf->id.out_of_block,flow->in))) &&
                        (!(ins->flags.call_flags&2) || !(ins->flags.call_flags&1) ||
                         (opnd->n.class==2 && (opnd->v.usage&8)))) {
                        last_defn=NULL;
                        block_cost+=Save.store_cost[class];
                    }
                }
            }
            i=ins->num_operands;
            while (--i>=0) {
                if (SAVINGS_EQUAL(ins->operands[i],opnd)) {
                    block_save+=Save.use_save[class];
                    if (i==0 && ins->head.opcode<0x2e && ins->result!=NULL &&
                        ins->result->n.class==3) block_save+=Save.use_save[class];
                } else if (ins->operands[i]->n.class==4 &&
                           SAVINGS_EQUAL(ins->operands[i]->i.index,opnd))
                    block_save+=Save.index_save;
            }
            if (ins->result!=NULL) {
                if (SAVINGS_EQUAL(ins->result,opnd)) {
                    last_defn=ins;
                    if ((ins->head.opcode==0x26 || ins->head.opcode==0x24) &&
                        ins->result->n.class==2 && ins->operands[0]->n.class==2 &&
                        ins->result->t.location!=0x7fffffffL &&
                        ins->result->t.location==ins->operands[0]->t.location)
                        block_cost+=Save.load_cost[class];
                    else {
                        block_save+=Save.def_save[class];
                        if (ins->num_operands!=0 && ins->operands[0]->n.class==3)
                            block_save+=Save.use_save[class];
                    }
                } else if (ins->result->n.class==4 &&
                           SAVINGS_EQUAL(ins->result->i.index,opnd))
                    block_save+=Save.index_save;
            }
            ins=next;
            last=ins->head.prev==conf->ins_range.last;
            if (last) break;
        }
        if (opnd->v.usage&2) {
            /* Counting costs has identical increments for an empty block
             * and an ordinary first-use/last-definition insertion site. */
            if (GLOBAL_OVERLAP(conf->id.out_of_block,flow->need_store))
                block_cost+=Save.store_cost[class];
            if (GLOBAL_OVERLAP(conf->id.out_of_block,flow->need_load))
                block_cost+=Save.load_cost[class];
        } else if (opnd->v.usage&0x88) {
            if (last_defn!=NULL) block_cost+=Save.store_cost[class];
            if (flows_in) block_cost+=Save.load_cost[class];
        }
        save+=WEIGHTED(block_save,blk);
        cost+=WEIGHTED(block_cost,blk);
        block_save=block_cost=0;
        if (last) break;
        blk=blk->next_block;
        ins=blk->ins.hd.next;
    }
    if (save<=cost) conf->savings=0;
    else if (save-cost<0xffffffffU) conf->savings=save-cost;
    else conf->savings=0xfffffffeU;
}
#undef WEIGHTED
#undef SAVINGS_EQUAL
#undef GLOBAL_OVERLAP
void CalcLoadStore(conflict_node *conf) { STUB_TRAP(); }

static save_def MaxConstSave;
conflict_node *NameConflict(instruction *ins,name *opnd);
#define CONST_TEMP_NAME(opnd) ((opnd)->n.class==2 && ((opnd)->t.temp_flags&0x100))
/* During savings propagation the otherwise-unused tree slot is the
 * original temporary chain of constant-defining conflicts. */
void ConstSavings(void)
{
    conflict_node *conf,*other;
    block *blk;
    instruction *ins;
    for (blk=HeadBlock;blk!=NULL;blk=blk->next_block) {
        for (ins=blk->ins.hd.next;ins->head.opcode!=0x4b;ins=ins->head.next) {
            if (ins->head.opcode!=0x26) continue;
            if (!CONST_TEMP_NAME(ins->operands[0])) continue;
            if (!CONST_TEMP_NAME(ins->result)) continue;
            conf=NameConflict(ins,ins->operands[0]);
            if (conf==NULL) continue;
            other=NameConflict(ins,ins->result);
            if (other==NULL || !(conf->state&2) || !(other->state&8)) continue;
            conf->tree=(reg_tree *)other;
        }
    }
    for (conf=ConfList;conf!=NULL;conf=conf->next_conflict) {
        if ((conf->state&2) && CONST_TEMP_NAME(conf->name)) {
            for (other=(conflict_node *)conf->tree;other!=NULL;
                 other=(conflict_node *)other->tree) {
                conf->savings+=other->savings;
                if (other->state&0x4000) break;
            }
            conf->state|=0x4000;
        }
    }
    for (conf=ConfList;conf!=NULL;conf=conf->next_conflict) {
        conf->tree=NULL;
        conf->state&=~0x4000;
    }
    MaxConstSave=0;
    for (conf=ConfList;conf!=NULL;conf=conf->next_conflict) {
        if (!CONST_TEMP_NAME(conf->name)) continue;
        conf->state&=~2;
        if (!(conf->state&8) || conf->savings<MaxConstSave) continue;
        MaxConstSave=conf->savings;
    }
    for (conf=ConfList;conf!=NULL;conf=conf->next_conflict) {
        if (CONST_TEMP_NAME(conf->name) || !(conf->state&2)) continue;
        conf->state&=~2;
        if (conf->savings!=0) conf->savings+=MaxConstSave;
    }
}
#undef CONST_TEMP_NAME


hw_reg_set MustSaveRegs(void);
extern hw_reg_set GivenRegisters;
unsigned char WorthProlog(conflict_node *conf,hw_reg_set reg)
{
    save_def cost,savings;
    hw_reg_set must_save;
    unsigned char class;
    name *op;
    class=conf->name->n.name_class;
    must_save=MustSaveRegs();
    if (BlockByBlock || (reg&GivenRegisters) || !(reg&must_save)) cost=0;
    else cost=Save.pop_cost[class]+Save.push_cost[class];
    op=conf->name;
    savings=conf->savings;
    if (op->n.class==2 && (op->t.temp_flags&0x100)) {
        cost+=Weight(Save.load_cost[class]+Save.def_save[class],conf->start_block);
        savings/=20;
    } else savings-=MaxConstSave;
    return savings>=cost;
}
