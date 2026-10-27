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
* Description:  Build instruction liveness and variable conflict ranges.
*
****************************************************************************/


#include "block.h"
#include "genopt.h"
#include "stubs.h"
#include <recon.h>

void GlobalConflictsFirst(void);
void FlowConflicts(instruction *first,instruction *last,block *blk);
void ExtendConflicts(block *blk,conflict_node *first_global);
void MakeLiveInfo(void)
{
    block *blk;
    conflict_node *first_global;
    unsigned char havelive;
    GlobalConflictsFirst();
    first_global=ConfList;
    havelive=HaveLiveInfo;
    HaveLiveInfo=0;
    for (blk=HeadBlock;blk!=NULL;blk=blk->next_block)
        if (blk->ins.hd.prev->head.opcode!=0)
            PrefixIns((instruction *)&blk->ins,MakeNop());
    HaveLiveInfo=havelive;
    Renumber();
    for (blk=HeadBlock;blk!=NULL;blk=blk->next_block) {
        FlowConflicts((instruction *)&blk->ins,(instruction *)&blk->ins,blk);
        ExtendConflicts(blk,first_global);
    }
}
void GlobalConflictsFirst(void)
{
    conflict_node **conf_owner,**list_owner,*list,*conf;
    conf_owner=&ConfList;
    list_owner=&list;
    for (;;) {
        conf=*conf_owner;
        if (conf==NULL) break;
        if (!(conf->state&4) && (conf->name->v.usage&2)) {
            *conf_owner=conf->next_conflict;
            *list_owner=conf;
            list_owner=&conf->next_conflict;
        } else conf_owner=&conf->next_conflict;
    }
    if (list_owner!=&list) {
        *list_owner=ConfList;
        ConfList=list;
    }
}
extern global_bit_set MemoryBits;
conflict_node *FindConflictNode(name *opnd,block *blk,instruction *ins);
void NowAlive(name *opnd,conflict_node *conf,name_set *alive,block *blk);
void NowDead(name *opnd,conflict_node *conf,name_set *alive,block *blk);
void FlowConflicts(instruction *first,instruction *last,block *blk)
{
    instruction *ins;
    name *opnd;
    conflict_node *conf;
    int i;
    unsigned char opcode;
    name_set alive;
    alive=last->head.live;
    ins=last;
    for (;;) {
        opcode=ins->head.opcode;
        i=0;
        switch (opcode) {
        case 3: case 7: case 8: case 11:
            if (ins->operands[1]==ins->operands[0] &&
                ins->operands[1]->n.class==3) i=NumOperands(ins);
            break;
        }
        if (opcode!=0x4b) {
            while (i<ins->num_operands) {
                opnd=ins->operands[i];
                if (opnd->n.class==4) opnd=opnd->i.index;
                conf=FindConflictNode(opnd,blk,ins);
                NowAlive(opnd,conf,&alive,blk);
                ++i;
            }
        }
        if (ins->head.opcode==0x36 || ins->head.opcode==0x29) {
            alive.out_of_block[0]|=MemoryBits.bits[0];
            alive.out_of_block[1]|=MemoryBits.bits[1];
            alive.out_of_block[2]|=MemoryBits.bits[2];
            alive.out_of_block[3]|=MemoryBits.bits[3];
        }
        ins->head.live=alive;
        ins->head.live.regs|=CurrProc->state.unalterable;
        ins=ins->head.prev;
        if (ins==first) break;
        alive.regs&=~ins->zap->reg;
        opnd=ins->result;
        if (opnd!=NULL) {
            if (opnd->n.class==4) {
                conf=FindConflictNode(opnd->i.index,blk,ins);
                NowAlive(opnd->i.index,conf,&alive,blk);
            } else {
                conf=FindConflictNode(opnd,blk,ins);
                NowDead(opnd,conf,&alive,blk);
            }
        }
    }
}
void ExtendConflicts(block *blk,conflict_node *first_global)
{
    conflict_node *conf;
    instruction *last_ins,*first_ins,*new_ins;
    unsigned char havelive;
    data_flow_def *flow;
    flow=blk->dataflow;
    for (conf=first_global;conf!=NULL;conf=conf->next_conflict) {
        if (conf->state&4) break;
        if (!(conf->name->v.usage&2)) break;
        if ((conf->id.out_of_block.bits[0]&flow->out.bits[0]) ||
            (conf->id.out_of_block.bits[1]&flow->out.bits[1]) ||
            (conf->id.out_of_block.bits[2]&flow->out.bits[2]) ||
            (conf->id.out_of_block.bits[3]&flow->out.bits[3])) {
            last_ins=blk->ins.hd.prev;
            if (conf->ins_range.last==NULL ||
                (long)conf->ins_range.last->id<=(long)last_ins->id) {
                if (last_ins->head.opcode!=0) {
                    new_ins=MakeNop();
                    havelive=HaveLiveInfo;
                    HaveLiveInfo=0;
                    SuffixIns(last_ins,new_ins);
                    new_ins->head.live=blk->ins.hd.live;
                    HaveLiveInfo=havelive;
                    last_ins=new_ins;
                }
                conf->ins_range.last=last_ins;
                if (conf->ins_range.first==NULL) {
                    conf->ins_range.first=last_ins;
                    conf->start_block=blk;
                }
            }
        }
        if ((conf->id.out_of_block.bits[0]&flow->in.bits[0]) ||
            (conf->id.out_of_block.bits[1]&flow->in.bits[1]) ||
            (conf->id.out_of_block.bits[2]&flow->in.bits[2]) ||
            (conf->id.out_of_block.bits[3]&flow->in.bits[3])) {
            first_ins=blk->ins.hd.next;
            if (conf->ins_range.first==NULL ||
                (long)conf->ins_range.first->id>=(long)first_ins->id) {
                if (first_ins->head.opcode!=0) {
                    new_ins=MakeNop();
                    havelive=HaveLiveInfo;
                    HaveLiveInfo=0;
                    PrefixIns(first_ins,new_ins);
                    new_ins->head.live=first_ins->head.live;
                    new_ins->head.live.regs=blk->ins.hd.live.regs;
                    HaveLiveInfo=havelive;
                    first_ins=new_ins;
                }
                conf->ins_range.first=first_ins;
                conf->start_block=blk;
                if (conf->ins_range.last==NULL) conf->ins_range.last=first_ins;
            }
        }
    }
}
void UpdateLive(instruction *first,instruction *last)
{
    instruction *ins;
    last=last->head.next;
    ins=last;
    while (ins->head.opcode!=0x4b) ins=ins->head.next;
    if (ins->head.next==ins->head.prev)
        FlowConflicts(ins,ins,((block_ins *)ins)->blk);
    else {
        if (first->head.opcode!=0x4b) first=first->head.prev;
        FlowConflicts(first,last,((block_ins *)ins)->blk);
    }
}

void AssignBit(conflict_node *conf,block *blk);
void NowAlive(name *opnd,conflict_node *conf,name_set *alive,block *blk)
{
    if (opnd->n.class==3) alive->regs|=opnd->r.reg;
    else if (conf!=NULL) {
        if (opnd->v.usage&2) {
            alive->out_of_block[0]|=conf->id.out_of_block.bits[0];
            alive->out_of_block[1]|=conf->id.out_of_block.bits[1];
            alive->out_of_block[2]|=conf->id.out_of_block.bits[2];
            alive->out_of_block[3]|=conf->id.out_of_block.bits[3];
        } else {
            if (conf->id.within_block==0) AssignBit(conf,blk);
            alive->within_block|=conf->id.within_block;
        }
    }
}
void AssignBit(conflict_node *conf,block *blk);
void NowDead(name *opnd,conflict_node *conf,name_set *alive,block *blk)
{
    if (opnd->n.class==3) alive->regs&=~opnd->r.reg;
    else if (conf!=NULL) {
        if (opnd->v.usage&2) {
            if (opnd->n.class!=2 || (name *)opnd->t.alias==opnd) {
                alive->out_of_block[0]&=~conf->id.out_of_block.bits[0];
                alive->out_of_block[1]&=~conf->id.out_of_block.bits[1];
                alive->out_of_block[2]&=~conf->id.out_of_block.bits[2];
                alive->out_of_block[3]&=~conf->id.out_of_block.bits[3];
            }
        } else {
            if (conf->id.within_block==0) AssignBit(conf,blk);
            if (opnd->n.class!=2 || (name *)opnd->t.alias==opnd)
                alive->within_block&=~conf->id.within_block;
        }
    }
}
void AssignBit(conflict_node *conf,block *blk)
{
    unsigned bit;
    if (blk->available_bit==0) conf->state|=4;
    else if (!(conf->state&4)) {
        bit=1;
        while (!(blk->available_bit&bit)) bit<<=1;
        conf->id.within_block=bit;
        blk->available_bit&=~bit;
    }
}

void LiveInfoUpdate(void)
{
    block *blk;
    blk=HeadBlock;
    for (;;) {
        if (blk->ins.hd.next!=(instruction *)&blk->ins)
            FlowConflicts((instruction *)&blk->ins,(instruction *)&blk->ins,blk);
        blk=blk->next_block;
        if (blk==NULL) break;
    }
}
