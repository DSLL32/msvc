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
* Description:  Initialize variable usage for register allocation.
*
****************************************************************************/


#include "block.h"
#include "genopt.h"
#include "stubs.h"
#include "regchoices.h"
#include "options.h"
#include <recon.h>

void NullConflicts(unsigned short off)
{
    name *temp;
    for (temp=Names[2];temp!=NULL;temp=temp->n.next_name) {
        temp->v.conflict=NULL;
        /* Retain only inter-block and memory/address attributes. The old
         * usage universe ends at NEEDS_MEMORY, before later alignment bits. */
        temp->v.usage&=0xfa;
        if (!(temp->v.usage&0xb8)) temp->v.usage&=~off;
        temp->v.block_usage=0;
    }
}

hw_reg_set GivenRegisters;
enum allocation_state { ALLOC_DONE,ALLOC_BITS,ALLOC_CONST_TEMP };
void InitChoices(void);
int ExpandOps(unsigned char keep_on_truckin);
unsigned char SplitConflicts(void);
void DeadInstructions(void);
void FixChoices(void);
unsigned char RegInsDead(void);
enum allocation_state AssignConflicts(void);
void LiveInfoUpdate(void);
unsigned char MoreConflicts(void);
unsigned char RegAlloc(unsigned char keep_on_truckin)
{
    int unknowns;
    enum allocation_state last;
    name *temp;
    conflict_node *conf;
    GivenRegisters=0;
    if (!BlockByBlock) {
        InitChoices();
        unknowns=ExpandOps(keep_on_truckin);
        if (unknowns<=0) return unknowns==0;
        if (SplitConflicts()) {
            FreeConflicts();
            /* NullConflicts(0) was inlined in the original build. */
            for (temp=Names[2];temp!=NULL;temp=temp->n.next_name) {
                temp->v.conflict=NULL;
                temp->v.usage&=0xfa;
                temp->v.block_usage=0;
            }
            HaveLiveInfo=0;
            if (!(Model&0x80000000U)) {
                DeadInstructions();
                FindReferences();
                PropagateMoves();
                PropRegsOne();
                /* The later released ReConstFold pass is absent here. */
            }
            FindReferences();
            MakeConflicts();
            MakeLiveInfo();
            HaveLiveInfo=1;
            AxeDeadCode();
        }
    }
    last=ALLOC_DONE;
    for (;;) {
        InitChoices();
        unknowns=ExpandOps(keep_on_truckin);
        if (unknowns<=0) break;
        FixChoices();
        if (last==ALLOC_CONST_TEMP) RegInsDead();
        last=AssignConflicts();
        if (last==ALLOC_BITS) {
            /* Original inlines the AssignMoreBits state transition. */
            for (conf=ConfList;conf!=NULL;conf=conf->next_conflict) conf->state&=~4;
            if (MoreConflicts()) {
                MakeLiveInfo();
                AxeDeadCode();
            } else LiveInfoUpdate();
        }
    }
    return unknowns==0;
}
void InitChoices(void)
{
    conflict_node *conf;
    name *opnd,*alias;
    block *blk;
    instruction *ins;
    int i;
    for (conf=ConfList;conf!=NULL;conf=conf->next_conflict) conf->possible=26;
    if (BlockByBlock) {
        for (blk=HeadBlock;blk!=NULL;blk=blk->next_block) {
            ins=blk->ins.hd.next;
            while (ins->head.opcode!=0x4b) {
                i=ins->num_operands;
                while (--i>=0) {
                    opnd=ins->operands[i];
                    /* Actual build inlines InitAChoice at both sites. */
                    if (opnd->n.class==2 && !(opnd->t.temp_flags&2)) {
                        alias=opnd;
                        do {
                            alias->t.possible=26;
                            alias=(name *)alias->t.alias;
                        } while (alias!=opnd);
                    }
                }
                opnd=ins->result;
                if (opnd!=NULL && opnd->n.class==2 && !(opnd->t.temp_flags&2)) {
                    alias=opnd;
                    do {
                        alias->t.possible=26;
                        alias=(name *)alias->t.alias;
                    } while (alias!=opnd);
                }
                ins=ins->head.next;
            }
        }
    } else {
        for (opnd=Names[2];opnd!=NULL;opnd=opnd->n.next_name) opnd->t.possible=26;
    }
}

void BuildNameTree(conflict_node *conf);
void BurnNameTree(reg_tree *tree);
void ReAlias(reg_tree *tree);
unsigned char SplitConflicts(void)
{
    conflict_node *conf;
    unsigned char change;
    change=0;
    for (conf=ConfList;conf!=NULL;conf=conf->next_conflict) {
        BuildNameTree(conf);
        if (conf->tree!=NULL && conf->tree->idx==26) { /* RL_NUMBER_OF_SETS */
            change=1;
            ReAlias(conf->tree);
            if (conf->tree->temp!=NULL) conf->tree->temp->v.usage|=0x10;
            if (conf->tree->alt!=NULL) conf->tree->alt->v.usage|=0x10;
        }
        BurnNameTree(conf->tree);
        conf->tree=NULL;
    }
    return change;
}
void ReAlias(reg_tree *tree) { STUB_TRAP(); }


void FreeAConflict(conflict_node *conf);
void CalcSavings(conflict_node *conf);
void ConstSavings(void);
unsigned char ConfBefore(conflict_node *left,conflict_node *right);
void *SortList(void *list,unsigned next_offset,unsigned char (*before)(void *,void *));
conflict_node *InMemory(conflict_node *conf);
conflict_node *GiveRegister(conflict_node *conf,unsigned char try_only);
enum allocation_state AssignConflicts(void)
{
    conflict_node *conf,*next;
    enum allocation_state state;
    name *opnd;
    unsigned char only_const_temps;
    for (conf=ConfList;conf!=NULL;conf=next) {
        next=conf->next_conflict;
        if (conf->start_block==NULL) FreeAConflict(conf);
        else {
            if (!(conf->state&8)) {
                conf->available=1;
                CalcSavings(conf);
                if (!(conf->state&4)) conf->state|=0x0a;
            }
            conf->state&=~0x0480;
        }
    }
    ConstSavings();
    /* The old compiler inlined this SortConflicts wrapper at this site. */
    ConfList=SortList(ConfList,offsetof(conflict_node,next_conflict),
                     (unsigned char (*)(void *,void *))ConfBefore);
    state=ALLOC_BITS;
    conf=ConfList;
    if (conf==NULL) return state;
    opnd=conf->name;
    only_const_temps=opnd->n.class==2 && (opnd->t.temp_flags&0x100);
    while (conf!=NULL) {
        opnd=conf->name;
        next=conf->next_conflict;
        if (!(conf->state&4)) {
            if (!only_const_temps && opnd->n.class==2 &&
                (opnd->t.temp_flags&0x100)) return ALLOC_CONST_TEMP;
            /* The 1994 body predates IsUncacheableMemory here. */
            if (conf->savings==0) next=InMemory(conf);
            else next=GiveRegister(conf,0);
            if (opnd->v.conflict==conf) state=ALLOC_DONE;
        }
        conf=next;
    }
    return state;
}
unsigned char ConfBefore(conflict_node *left,conflict_node *right)
{
    return left->savings>right->savings;
}
void PutInMemory(conflict_node *conf);
conflict_node *InMemory(conflict_node *conf)
{
    conflict_node *next,*other;
    other=conf->name->v.conflict;
    while (other!=NULL) {
        if (other!=conf) {
            PutInMemory(other);
            next=other->next_for_name;
            FreeAConflict(other);
            other=next;
        } else other=other->next_for_name;
    }
    PutInMemory(conf);
    next=conf->next_conflict;
    FreeAConflict(conf);
    return next;
}
void MemConstTemp(conflict_node *conf);
void PutInMemory(conflict_node *conf)
{
    block *blk;
    instruction *ins;
    name *opnd;

    DGBlip();
    opnd=conf->name;
    if (opnd->n.class==2) {
        if (opnd->t.temp_flags&0x100) MemConstTemp(conf);
        else {
            do {
                opnd->v.usage|=0x90;
                opnd=(name *)opnd->t.alias;
            } while (opnd!=conf->name);
        }
    } else opnd->v.usage|=0x90;
    blk=conf->start_block;
    ins=conf->ins_range.first;
    if (blk!=NULL) {
        for (;;) {
            if (ins->head.state==1) ins->head.state=0;
            if (ins==conf->ins_range.last) break;
            if (ins->head.opcode==0x4b) {
                blk=blk->next_block;
                ins=(instruction *)&blk->ins;
            }
            ins=ins->head.next;
            ins->head.live.out_of_block[0]&=~conf->id.out_of_block.bits[0];
            ins->head.live.out_of_block[1]&=~conf->id.out_of_block.bits[1];
            ins->head.live.out_of_block[2]&=~conf->id.out_of_block.bits[2];
            ins->head.live.out_of_block[3]&=~conf->id.out_of_block.bits[3];
            ins->head.live.within_block&=~conf->id.within_block;
        }
        if (conf->ins_range.first==conf->ins_range.last &&
            conf->name->n.class==2 && !(conf->name->v.usage&8) &&
            !SideEffect(conf->ins_range.first)) DoNothing(conf->ins_range.first);
    }
}

void BuildRegTree(conflict_node *conf);
hw_reg_set GiveBestReg(conflict_node *conf,reg_tree *tree,hw_reg_set except,unsigned char needs_one);
void BitOff(conflict_node *conf);
conflict_node *GiveRegister(conflict_node *conf,unsigned char needs_one)
{
    reg_tree *tree;
    conflict_node *next_valid;
    hw_reg_set given;
    /* GRBlip is the same physical target hook as the known DGBlip. */
    DGBlip();
    BuildRegTree(conf);
    tree=conf->tree;
    if (conf->state&0x0900) needs_one=1;
    given=GiveBestReg(conf,tree,CurrProc->state.unalterable,needs_one);
    if (tree!=NULL && (tree->hi!=NULL || tree->lo!=NULL)) LiveInfoUpdate();
    if (given!=0) {
        next_valid=conf->next_conflict;
        BitOff(conf);
        FreeAConflict(conf);
    } else {
        if (conf->state&0x40) {
            conf->state|=0x80;
            conf->state&=~0x40;
        } else if (conf->state&0x0200) {
            conf->state|=0x0400;
            conf->state&=~0x0200;
        }
        if (!(conf->state&0x0480)) next_valid=InMemory(conf);
        else next_valid=conf->next_conflict;
    }
    BurnNameTree(tree);
    return next_valid;
}
void NeighboursUse(conflict_node *conf);
unsigned char TooGreedy(conflict_node *conf,hw_reg_set reg,name *except);
int CountRegMoves(conflict_node *conf,hw_reg_set reg,reg_tree *tree,int depth);
unsigned char WorthProlog(conflict_node *conf,hw_reg_set reg);
unsigned char FixInstructions(conflict_node *conf,reg_tree *tree,hw_reg_set reg,unsigned char live_update);
hw_reg_set GiveBestReg(conflict_node *conf,reg_tree *tree,hw_reg_set except,unsigned char needs_one)
{
    hw_reg_set reg,best,given,gave_hi,gave_lo,*possible;
    int best_saves,saves;
    unsigned char greed,all_true,failed;
    NeighboursUse(conf);
    if (tree==NULL) given=0;
    else if (tree->regs==NULL) {
        failed=0;
        gave_hi=gave_lo=0;
        if (tree->hi!=NULL) {
            gave_hi=GiveBestReg(conf,tree->hi,except,needs_one);
            if (gave_hi==0) failed=1;
        }
        if (tree->lo!=NULL) {
            gave_lo=GiveBestReg(conf,tree->lo,except|gave_hi,needs_one);
            if (gave_lo==0) failed=1;
        }
        if (failed) given=0;
        else given=gave_hi|gave_lo;
        GivenRegisters|=gave_hi|gave_lo;
    } else {
        best=0;
        best_saves=-1;
        all_true=1;
        for (possible=tree->regs;*possible!=0;++possible) {
            reg=*possible;
            if (!(reg&conf->with.regs) && !(reg&except)) {
                greed=TooGreedy(conf,reg,tree->temp);
                if (greed==0) {
                    saves=CountRegMoves(conf,reg,conf->tree,3);
                    if (saves>best_saves || (saves==best_saves &&
                        (GivenRegisters&reg)==reg && (GivenRegisters&best)!=best)) {
                        best=reg;
                        best_saves=saves;
                    }
                }
                if (greed!=1) all_true=0;
            }
        }
        if (all_true) given=0;
        else if (best==0) {
            if (conf->state&0x40) {
                conf->state|=0x80;
                conf->state&=~0x40;
            }
            if (conf->state&0x0200) {
                conf->state|=0x0400;
                conf->state&=~0x0200;
            }
            given=0;
        } else if (needs_one || WorthProlog(conf,best)) {
            FixInstructions(conf,tree,best,1);
            given=best;
            GivenRegisters|=given;
        } else given=0;
    }
    if (given==0 && needs_one) Zoiks(40);
    return given;
}
conflict_node *FindConflictNode(name *opnd,block *blk,instruction *ins);
void NowAlive(name *opnd,conflict_node *conf,name_set *alive,block *blk);
void NowDead(name *opnd,conflict_node *conf,name_set *alive,block *blk);
#define GLOBAL_INTERSECTS(a,b) (((a).bits[0]&(b).bits[0])|((a).bits[1]&(b).bits[1])| \
                                ((a).bits[2]&(b).bits[2])|((a).bits[3]&(b).bits[3]))
#define GLOBAL_EMPTY(a) (((a).bits[0]|(a).bits[1]|(a).bits[2]|(a).bits[3])==0)
#define CLEAR_NAME_SET(set) do { \
    (set).regs=0; (set).within_block=0; \
    (set).out_of_block[0]=(set).out_of_block[1]= \
    (set).out_of_block[2]=(set).out_of_block[3]=0; \
} while (0)
void NeighboursUse(conflict_node *conf)
{
    block *blk;
    instruction *ins,*last;
    name *dst,*definition;
    name_set no_conflict;
    unsigned i;
    unsigned char no_id;
    blk=conf->start_block;
    ins=conf->ins_range.first;
    last=conf->ins_range.last;
    if (ins->head.opcode==0x26 && ins->result==conf->name)
        definition=ins->operands[0];
    else definition=NULL;
    CLEAR_NAME_SET(conf->with);
    if (ins!=last) {
        CLEAR_NAME_SET(no_conflict);
        for (;;) {
            ins=ins->head.next;
            if (ins->head.opcode!=0x4b) {
                dst=ins->result;
                if (dst!=NULL)
                    NowDead(dst,FindConflictNode(dst,blk,ins),&no_conflict,blk);
                if (ins->head.opcode!=0x26) {
                    if (dst==conf->name) { CLEAR_NAME_SET(no_conflict); }
                } else if (ins->operands[0]==conf->name) {
                    NowAlive(dst,FindConflictNode(dst,blk,ins),&no_conflict,blk);
                } else if (dst==conf->name) {
                    CLEAR_NAME_SET(no_conflict);
                    definition=ins->operands[0];
                    NowAlive(definition,FindConflictNode(definition,blk,ins),&no_conflict,blk);
                }
                no_id=conf->id.within_block==0 && GLOBAL_EMPTY(conf->id.out_of_block);
                if (ins!=last) {
                    if ((conf->name->v.usage&0x88) || no_id ||
                        (conf->id.within_block&ins->head.next->head.live.within_block) ||
                        ((conf->id.out_of_block.bits[0]&ins->head.next->head.live.out_of_block[0])|
                         (conf->id.out_of_block.bits[1]&ins->head.next->head.live.out_of_block[1])|
                         (conf->id.out_of_block.bits[2]&ins->head.next->head.live.out_of_block[2])|
                         (conf->id.out_of_block.bits[3]&ins->head.next->head.live.out_of_block[3]))) {
                        conf->with.regs|=ins->zap->reg;
                        if (dst!=NULL && dst->n.class==3) conf->with.regs|=dst->r.reg;
                    }
                    /* This old body predates the later intermediate
                     * CheckIndexZap else-branch in the released source. */
                } else if (dst!=NULL && dst->n.class==4 &&
                           FindConflictNode(dst->i.index,blk,ins)==conf)
                    conf->with.regs|=ins->zap->reg;
            }
            no_id=conf->id.within_block==0 && GLOBAL_EMPTY(conf->id.out_of_block);
            if ((ins->head.live.out_of_block[0]&conf->id.out_of_block.bits[0])|
                (ins->head.live.out_of_block[1]&conf->id.out_of_block.bits[1])|
                (ins->head.live.out_of_block[2]&conf->id.out_of_block.bits[2])|
                (ins->head.live.out_of_block[3]&conf->id.out_of_block.bits[3]) ||
                (ins->head.live.within_block&conf->id.within_block) || no_id) {
                conf->with.regs|=ins->head.live.regs&~no_conflict.regs;
                for (i=0;i<4;++i)
                    conf->with.out_of_block[i]|=ins->head.live.out_of_block[i]&~no_conflict.out_of_block[i];
                conf->with.within_block|=ins->head.live.within_block&~no_conflict.within_block;
            }
            if (ins->head.opcode==0x36 || ins->head.opcode==0x29) {
                CLEAR_NAME_SET(no_conflict);
            } else if (ins->head.opcode==0x4b) {
                conf->with.within_block=0;
                blk=blk->next_block;
                ins=(instruction *)&blk->ins;
                CLEAR_NAME_SET(no_conflict);
            }
            if (ins==last) break;
        }
    }
    if (conf->name->n.class==2 && (conf->name->t.temp_flags&0x100) &&
        definition!=NULL && definition->n.class==3) {
        conf->with.regs&=~definition->r.reg;
        conf->savings=0xffffffffU;
    }
}
#undef CLEAR_NAME_SET
#undef GLOBAL_EMPTY
#undef GLOBAL_INTERSECTS

unsigned char UnaryOpGetsReg(instruction *ins,hw_reg_set reg,name *op);
unsigned char CheckIndecies(instruction *ins,hw_reg_set reg,hw_reg_set except,name *op);
unsigned char TooGreedy(conflict_node *conf,hw_reg_set reg,name *op)
{
    block *blk;
    instruction *ins,*last;
    hw_reg_set *ins_needs;
    unsigned char rc,needs;
    blk=conf->start_block;
    ins=conf->ins_range.first;
    if (conf->name->n.class==2 && (conf->state&0x0900)) ins=conf->ins_range.last;
    last=conf->ins_range.last;
    rc=0;
    for (;;) {
        if (ins->u.gen_table==NULL) needs=RL_;
        else needs=ins->u.gen_table->reg_set;
        ins_needs=RegSets[RegList[needs].need];
        if (*ins_needs==0 || (conf->state&0x8000) || UnaryOpGetsReg(ins,reg,op))
            rc=CheckIndecies(ins,reg,0,op);
        else {
            rc=1;
            for (;;) {
                if (!(*ins_needs&ins->head.live.regs) && !(reg&*ins_needs)) {
                    rc=CheckIndecies(ins,reg,*ins_needs,op);
                    if (rc==0) break;
                }
                ++ins_needs;
                if (*ins_needs==0) break;
            }
        }
        if (ins==last) break;
        ins=ins->head.next;
        while (ins->head.opcode==0x4b) {
            blk=blk->next_block;
            ins=blk->ins.hd.next;
        }
        if (rc!=0) break;
    }
    return rc;
}
unsigned char IsSegReg(hw_reg_set reg);
unsigned char UnaryOpGetsReg(instruction *ins,hw_reg_set reg,name *op)
{
    return NumOperands(ins)==1 && ins->result!=NULL && !IsSegReg(reg) &&
           ins->head.opcode!=0x24 && (ins->operands[0]==op || ins->result==op);
}
unsigned char StealsIdx(instruction *ins,hw_reg_set except,name *op);
unsigned char StealsSeg(instruction *ins,hw_reg_set reg,hw_reg_set except,name *op);
unsigned char CheckIndecies(instruction *ins,hw_reg_set reg,hw_reg_set except,name *op)
{
    except|=ins->head.live.regs|reg;
    if (StealsIdx(ins,except,op)) return 2; /* MAYBE */
    if (StealsSeg(ins,reg,except,op)) return 2;
    return 0;
}
unsigned char StealsIdx(instruction *ins,hw_reg_set except,name *actual_op)
{
    hw_reg_set *index_needs;
    name *op;
    conflict_node *new_conf;
    int i;
    unsigned char is_result;
    is_result=0;
    i=ins->num_operands;
    while (--i>=0) {
        op=ins->operands[i];
        if (op->n.class==4) {
            new_conf=NameConflict(ins,op->i.index);
            if (new_conf==NULL || actual_op==op->i.index) return 0;
        }
    }
    if (ins->result!=NULL) {
        op=ins->result;
        if (op->n.class==4) {
            new_conf=NameConflict(ins,op->i.index);
            if (new_conf==NULL || actual_op==op->i.index) return 0;
            is_result=1;
        }
    }
    index_needs=RegSets[ins->t.index_needs];
    if (*index_needs==0) return 0;
    for (;;) {
        if (!(*index_needs&except)) {
            if (!is_result || !(*index_needs&ins->zap->reg)) return 0;
        }
        ++index_needs;
        if (*index_needs==0) break;
    }
    return 1;
}
unsigned char IsSegReg(hw_reg_set reg);
unsigned char SegIndex(void);
unsigned char StealsSeg(instruction *ins,hw_reg_set reg,hw_reg_set except,name *actual_op)
{
    hw_reg_set *index_needs;
    name *op;
    conflict_node *new_conf;
    int i;
    i=ins->num_operands-1;
    if (i<NumOperands(ins)) return 0;
    op=ins->operands[i];
    new_conf=NameConflict(ins,op);
    if (new_conf==NULL) return 0;
    if (op==actual_op && IsSegReg(reg)) return 0;
    index_needs=RegSets[SegIndex()];
    if (*index_needs==0) return 0;
    for (;;) {
        if (!(*index_needs&except)) return 0;
        ++index_needs;
        if (*index_needs==0) break;
    }
    return 1;
}
unsigned char IsIndexReg(hw_reg_set reg,unsigned char class,unsigned char is_temp_index);
unsigned char Idx(name *op);
int CountRegMoves(conflict_node *conf,hw_reg_set reg,reg_tree *tree,int levels)
{
    block *blk;
    instruction *ins;
    int count,half;
    name *reg_name,*op1,*op2,*res;
    unsigned char idx;
    /* The earlier body has no later SUPER_OPTIMAL recursive extension. */
    levels=levels;
    if (tree==NULL) return 0;
    reg_name=AllocRegName(reg);
    count=0;
    blk=conf->start_block;
    ins=conf->ins_range.first;
    if (tree->temp!=NULL) idx=IsIndexReg(reg,tree->temp->n.name_class,0);
    else idx=0;
    half=tree->size/2;
    for (;;) {
        if (ins->head.opcode==0x4b) {
            blk=blk->next_block;
            ins=blk->ins.hd.next;
        } else {
            if (ins->head.opcode!=0x26) {
                op1=op2=NULL;
                if (ins->num_operands!=0) {
                    op1=ins->operands[0];
                    switch (ins->head.opcode) {
                    case 1: case 2: case 5: case 9: case 10: case 11:
                        op2=ins->operands[1];
                        break;
                    }
                }
                res=ins->result;
                if (res==reg_name) {
                    if (op1==tree->temp || op1==tree->alt ||
                        op2==tree->temp || op2==tree->alt) count+=half;
                } else if (res==tree->temp || res==tree->alt) {
                    if (op1==reg_name || op2==reg_name) count+=half;
                }
            } else {
                op1=ins->operands[0];
                res=ins->result;
                if (((op1==tree->temp || op1==tree->alt) &&
                     (res==reg_name || (idx && Idx(res)))) ||
                    ((res==tree->temp || res==tree->alt) &&
                     (op1==reg_name || (idx && Idx(op1))))) count+=tree->size;
                else if (res->n.class==3 && (reg&res->r.reg)) count+=half;
            }
            if (ins==conf->ins_range.last) break;
            ins=ins->head.next;
        }
    }
    return count;
}
unsigned char Idx(name *op)
{
    return op!=NULL && op->n.class==2 && (op->t.temp_flags&8)!=0;
}
void DBAllocReg(name *reg_name,name *opnd);
unsigned char ContainedIn(name *part,name *whole);
name *FindReg(reg_tree *tree,name *opnd,name *reg);
hw_reg_set SearchTree(reg_tree *tree,name *opnd,hw_reg_set reg);
name *ReplIndex(instruction *ins,reg_tree *tree,name *index,name *reg);
void DelSegOp(instruction *ins,int operand);
void DelSegRes(instruction *ins);
void FixGenEntry(instruction *ins);
void AddSegment(instruction *ins);
#define FIX_GLOBAL_OVERLAP(a,b) (((a).bits[0]&(b).bits[0])|((a).bits[1]&(b).bits[1])| \
                                ((a).bits[2]&(b).bits[2])|((a).bits[3]&(b).bits[3]))
#define FIX_GLOBAL_EMPTY(a) (((a).bits[0]|(a).bits[1]|(a).bits[2]|(a).bits[3])==0)
#define UPDATE_ASSIGNED_LIVE(ins) do { \
    if (((ins)->head.live.out_of_block[0]&conf->id.out_of_block.bits[0])| \
        ((ins)->head.live.out_of_block[1]&conf->id.out_of_block.bits[1])| \
        ((ins)->head.live.out_of_block[2]&conf->id.out_of_block.bits[2])| \
        ((ins)->head.live.out_of_block[3]&conf->id.out_of_block.bits[3]) || \
        ((ins)->head.live.within_block&conf->id.within_block)) \
        (ins)->head.live.regs|=reg_name->r.reg; \
} while (0)
#define PREFIX_ASSIGNED_MOVE(ins,from,to) do { \
    PrefixIns(ins,MakeMove(from,to,class)); AddSegment((ins)->head.prev); \
} while (0)
#define SUFFIX_ASSIGNED_MOVE(ins,from,to) do { \
    SuffixIns(ins,MakeMove(from,to,class)); AddSegment((ins)->head.next); \
} while (0)
unsigned char FixInstructions(conflict_node *conf,reg_tree *tree,hw_reg_set reg,unsigned char need_live_update)
{
    name *reg_name,*opnd,*index,*index_reg;
    block *blk;
    instruction *ins,*next,*temp_ins,*last_defn,*final_defn,*first_use;
    data_flow_def *flow;
    unsigned char class,last,flows_in;
    int i;
    reg_name=AllocRegName(reg);
    if ((conf->name->v.usage&2) && (conf->name->v.usage&0x88))
        conf->name->v.usage|=0x10;
    opnd=tree->temp;
    if (GenSwitches&0x02000000) DBAllocReg(reg_name,opnd);
    class=opnd->n.name_class;
    blk=conf->start_block;
    ins=conf->ins_range.first;
    last=0;
    flows_in=(ins->head.live.within_block&conf->id.within_block)!=0;
    for (;;) {
        flow=blk->dataflow;
        final_defn=last_defn=first_use=NULL;
        while (ins->head.opcode!=0x4b) {
            next=ins->head.next;
            if (ins->head.opcode==0x36 || ins->head.opcode==0x29) {
                if (((opnd->v.usage&8) ||
                     ((opnd->v.usage&0x80) && !(conf->state&0x2000))) &&
                    !FIX_GLOBAL_OVERLAP(conf->id.out_of_block,flow->call_exempt)) {
                    if (!(ins->flags.call_flags&2) ||
                        (opnd->n.class==2 && (opnd->v.usage&8)))
                        SUFFIX_ASSIGNED_MOVE(ins,opnd,reg_name);
                    if ((last_defn!=NULL || ((opnd->v.usage&2) &&
                         FIX_GLOBAL_OVERLAP(conf->id.out_of_block,flow->in))) &&
                        (!(ins->flags.call_flags&2) || !(ins->flags.call_flags&1) ||
                         (opnd->n.class==2 && (opnd->v.usage&8)))) {
                        last_defn=NULL;
                        temp_ins=ins->head.prev;
                        PREFIX_ASSIGNED_MOVE(ins,reg_name,opnd);
                        if (first_use==NULL) first_use=temp_ins->head.next;
                    }
                }
                if (first_use==NULL) first_use=ins;
            }
            i=ins->num_operands;
            while (--i>=0) {
                if (ContainedIn(ins->operands[i],opnd)) {
                    if (first_use==NULL) first_use=ins;
                    DelSegOp(ins,i);
                    ins->operands[i]=FindReg(tree,ins->operands[i],reg_name);
                    FixGenEntry(ins);
                } else if (ins->operands[i]->n.class==4 &&
                           ContainedIn(ins->operands[i]->i.index,opnd)) {
                    if (first_use==NULL) first_use=ins;
                    ins->operands[i]=ReplIndex(ins,tree,ins->operands[i],reg_name);
                    ins->head.state=0;
                }
            }
            if (ins->result!=NULL) {
                if (ContainedIn(ins->result,opnd)) {
                    DelSegRes(ins);
                    last_defn=final_defn=ins;
                    /* The period compiler inlined FindReg at this result
                     * site, preserving the actual SearchTree call edge. */
                    ins->result=AllocRegName(SearchTree(tree,ins->result,reg_name->r.reg));
                    FixGenEntry(ins);
                } else if (ins->result->n.class==4 &&
                           ContainedIn(ins->result->i.index,opnd)) {
                    /* ReplIndex is similarly expanded at the result site
                     * in the original, but retains FindReg's own vertex. */
                    index=ins->result;
                    ins->t.index_needs=0;
                    index_reg=FindReg(tree,index->i.index,reg_name);
                    ins->result=ScaleIndex(index_reg,index->i.base,index->i.constant,
                        index->n.name_class,index->n.size,index->i.scale,index->i.index_flags);
                    ins->head.state=0;
                }
            }
            UPDATE_ASSIGNED_LIVE(ins);
            ins=next;
            last=ins->head.prev==conf->ins_range.last;
            if (last) break;
        }
        UPDATE_ASSIGNED_LIVE(ins);
        if (opnd->v.usage&2) {
            if (first_use==NULL) {
                first_use=conf->ins_range.first;
                if ((long)first_use->id<(long)blk->ins.hd.next->id) first_use=blk->ins.hd.next;
            }
            if (final_defn==NULL) {
                final_defn=conf->ins_range.last;
                if ((long)final_defn->id>(long)blk->ins.hd.prev->id) {
                    final_defn=blk->ins.hd.prev;
                    if (blk->class&0x0c) { /* CONDITIONAL | SELECT */
                        final_defn=blk->ins.hd.next;
                        while (!(final_defn->head.opcode>=0x2e && final_defn->head.opcode<=0x35) &&
                               final_defn->head.opcode!=0x2d) final_defn=final_defn->head.next;
                        final_defn=final_defn->head.prev;
                    } else {
                        while (final_defn->head.opcode==0) final_defn=final_defn->head.prev;
                    }
                }
            }
            if ((instruction *)&blk->ins==blk->ins.hd.next) {
                if (FIX_GLOBAL_OVERLAP(conf->id.out_of_block,flow->need_store))
                    SUFFIX_ASSIGNED_MOVE((instruction *)&blk->ins,reg_name,opnd);
                if (FIX_GLOBAL_OVERLAP(conf->id.out_of_block,flow->need_load))
                    SUFFIX_ASSIGNED_MOVE((instruction *)&blk->ins,opnd,reg_name);
            } else {
                if (FIX_GLOBAL_OVERLAP(conf->id.out_of_block,flow->need_store))
                    SUFFIX_ASSIGNED_MOVE(final_defn,reg_name,opnd);
                if (FIX_GLOBAL_OVERLAP(conf->id.out_of_block,flow->need_load)) {
                    if (first_use->head.prev==final_defn->head.next &&
                        first_use->head.prev->head.opcode!=0x4b) first_use=first_use->head.prev;
                    PREFIX_ASSIGNED_MOVE(first_use,opnd,reg_name);
                }
            }
        } else if (opnd->v.usage&0x88) {
            if (last_defn!=NULL) SUFFIX_ASSIGNED_MOVE(last_defn,reg_name,opnd);
            if (flows_in) PREFIX_ASSIGNED_MOVE(conf->ins_range.first,opnd,reg_name);
        }
        if (last) break;
        blk=blk->next_block;
        ins=blk->ins.hd.next;
    }
    if (conf->id.within_block==0 && FIX_GLOBAL_EMPTY(conf->id.out_of_block)) {
        if (need_live_update) {
            LiveInfoUpdate();
            return 0;
        }
        return 1;
    }
    return 0;
}
#undef SUFFIX_ASSIGNED_MOVE
#undef PREFIX_ASSIGNED_MOVE
#undef UPDATE_ASSIGNED_LIVE
#undef FIX_GLOBAL_EMPTY
#undef FIX_GLOBAL_OVERLAP
unsigned char ContainedIn(name *part,name *whole)
{
    if (part->n.class!=whole->n.class) return 0;
    if (part->n.class==2) {
        if (DeAlias(part)!=DeAlias(whole)) return 0;
    } else if (part->n.class==1) {
        if (part!=whole) return 0;
    } else return 0;
    if (part->v.offset<whole->v.offset) return 0;
    if (part->v.offset+part->n.size>whole->v.offset+whole->n.size) return 0;
    return 1;
}
name *FindReg(reg_tree *tree,name *opnd,name *reg)
{
    return AllocRegName(SearchTree(tree,opnd,reg->r.reg));
}
hw_reg_set LowOffsetReg(hw_reg_set reg);
hw_reg_set HighOffsetReg(hw_reg_set reg);
hw_reg_set SearchTree(reg_tree *tree,name *opnd,hw_reg_set reg)
{
    /* The actual body loops over the source's tail-recursive descent. */
    while (tree->offset!=opnd->v.offset || tree->size!=opnd->n.size) {
        if (opnd->v.offset<tree->offset+tree->size/2) {
            tree=tree->lo;
            reg=LowOffsetReg(reg);
        } else {
            tree=tree->hi;
            reg=HighOffsetReg(reg);
        }
    }
    return reg;
}
name *ReplIndex(instruction *ins,reg_tree *tree,name *index,name *reg)
{
    ins->t.index_needs=RL_;
    /* The original expands FindReg here, retaining SearchTree and the
     * AllocRegName vertices rather than an extra FindReg call. */
    reg=AllocRegName(SearchTree(tree,index->i.index,reg->r.reg));
    return ScaleIndex(reg,index->i.base,index->i.constant,
                      index->n.name_class,index->n.size,index->i.scale,
                      index->i.index_flags);
}

void BitOff(conflict_node *conf)
{
    block *blk;
    instruction *ins;

    blk=conf->start_block;
    ins=conf->ins_range.first;
    if (blk!=NULL) {
        while (ins!=conf->ins_range.last) {
            if (ins->head.opcode==0x4b) {
                blk=blk->next_block;
                ins=(instruction *)&blk->ins;
            }
            ins=ins->head.next;
            ins->head.live.out_of_block[0]&=~conf->id.out_of_block.bits[0];
            ins->head.live.out_of_block[1]&=~conf->id.out_of_block.bits[1];
            ins->head.live.out_of_block[2]&=~conf->id.out_of_block.bits[2];
            ins->head.live.out_of_block[3]&=~conf->id.out_of_block.bits[3];
            ins->head.live.within_block&=~conf->id.within_block;
        }
    }
}




unsigned char AssignARegister(conflict_node *conf,hw_reg_set reg)
{
    unsigned char need_live_update;
    BuildRegTree(conf);
    need_live_update=FixInstructions(conf,conf->tree,reg,0);
    BitOff(conf);
    /* The older body uses BurnNameTree, before the later BurnRegTree name. */
    BurnNameTree(conf->tree);
    FreeAConflict(conf);
    return need_live_update;
}
