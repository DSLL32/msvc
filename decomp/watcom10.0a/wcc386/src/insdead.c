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
* Description:  Remove dead backend instructions and unused temporaries.
*
****************************************************************************/


#include "block.h"
#include "address.h"
#include "genopt.h"
#include "stubs.h"
#include <recon.h>

unsigned char InsDead(void)
{
    return DoInsDead(0,0);
}
unsigned char DoInsDead(unsigned char just_the_loop,unsigned char in_regalloc)
{
    if (BlockByBlock && !BreakExists()) return 0;
    return RemoveUselessStuff(just_the_loop,in_regalloc);
}
unsigned char RemoveUselessStuff(unsigned char just_the_loop,unsigned char in_regalloc)
{
    block *tail;
    unsigned char change;
    InitVisitedTemps();
    FindUsefulIns(BlockList,just_the_loop,in_regalloc);
    tail=TailBlocks();
    if (tail!=NULL) FindUsefulIns(tail,just_the_loop,in_regalloc);
    change=FreeUselessIns(BlockList,just_the_loop,in_regalloc);
    if (tail!=NULL) change|=FreeUselessIns(tail,just_the_loop,in_regalloc);
    if (!in_regalloc) FreeUnVisitedTemps();
    return change;
}
void InitVisitedTemps(void)
{
    name *op,*alias;
    for (op=Names[2];op!=NULL;op=op->n.next_name) op->t.temp_flags&=~4U;
    for (op=Names[2];op!=NULL;op=op->n.next_name) {
        if (op->v.usage&0x28) { /* USE_ADDRESS | VAR_VOLATILE */
            alias=op;
            do {
                alias->t.temp_flags|=4; /* VISITED */
                alias=(name *)alias->t.alias;
            } while (alias!=op);
        }
    }
    if (Model&0x80000000U) {
        for (op=Names[2];op!=NULL;op=op->n.next_name) {
            /* Old _FrontEndTmp: real FE symbol, not a stack parameter. */
            if (!(op->t.temp_flags&0x100) && op->v.symbol!=NULL)
                op->t.temp_flags|=4;
        }
    }
    if (BlockByBlock || (Model&0x80000000U)) {
        for (op=Names[2];op!=NULL;op=op->n.next_name)
            if (op->v.usage&2) op->t.temp_flags|=4;
    }
}
void FindUsefulIns(block *head,unsigned char just_the_loop,unsigned char in_regalloc)
{
    block *blk;
    instruction *ins;
    unsigned char change;
    if (just_the_loop) {
        for (blk=head;blk!=NULL;blk=blk->prev_block) {
            if (!(blk->class&0x200)) { /* IN_LOOP */
                ins=blk->ins.hd.prev;
                while (ins->head.opcode!=0x4b) {
                    MarkOpsUseful(ins);
                    if (ins->result!=NULL) MarkUseful(ins->result);
                    ins=ins->head.prev;
                }
            }
        }
    }
    do {
        change=0;
        for (blk=head;blk!=NULL;blk=blk->prev_block) {
            if (!just_the_loop || (blk->class&0x200)) {
                ins=blk->ins.hd.prev;
                while (ins->head.opcode!=0x4b) {
                    if (!in_regalloc || DoesSomething(ins)) change|=CheckUseful(ins);
                    ins=ins->head.prev;
                }
            }
        }
    } while (change);
}
unsigned char MarkOpsUseful(instruction *ins)
{
    int i;
    unsigned char change;
    change=0;
    ins->ins_flags|=1; /* INS_VISITED */
    i=ins->num_operands;
    while (--i>=0) change|=MarkUseful(ins->operands[i]);
    return change;
}
unsigned char MarkUseful(name *op)
{
    unsigned char change;
    name *alias;
    change=0;
    if (op->n.class==2) {
        if (!(op->t.temp_flags&4)) {
            change=1;
            alias=op;
            do {
                alias->t.temp_flags|=4;
                alias=(name *)alias->t.alias;
            } while (alias!=op);
        }
    } else if (op->n.class==4) {
        change|=MarkUseful(op->i.index);
        /* Unlike usage propagation, even a fake index base is useful. */
        if (op->i.base!=NULL) change|=MarkUseful(op->i.base);
    } else if (op->n.class==0 && op->c.const_type==4) {
        change|=MarkUseful((name *)op->c.value);
    }
    return change;
}
unsigned char CheckUseful(instruction *ins)
{
    name *res;
    unsigned char opcode,change;
    opcode=ins->head.opcode;
    change=0;
    res=ins->result;
    /* Old _OpIsJump includes SELECT2d and conditional/unconditional2e..35. */
    if (!((opcode>=0x2e && opcode<=0x35) || opcode==0x2d) &&
        opcode!=0x2a && opcode!=0x2b && opcode!=0 &&
        opcode!=0x29 && opcode!=0x36) {
        if (SideEffect(ins)) {
            if (res!=NULL) change|=MarkUseful(res);
            change|=MarkOpsUseful(ins);
            return change;
        }
        if (res!=NULL) {
            if (res->n.class==1 || res->n.class==3) {
                change|=MarkOpsUseful(ins);
                return change;
            }
            if (res->n.class==4) {
                change|=MarkUseful(res);
                change|=MarkOpsUseful(ins);
                return change;
            }
            if (res->n.class==2 && (res->t.temp_flags&4)) {
                change|=MarkOpsUseful(ins);
                return change;
            }
        }
    } else {
        if (res!=NULL) change|=MarkUseful(res);
        change|=MarkOpsUseful(ins);
    }
    return change;
}
unsigned char SideEffect(instruction *ins)
{
    if (ins->head.opcode==0x2a || ins->head.opcode==0x2b) return 1;
    if ((ins->ins_flags&0x20) && ins->head.opcode!=0x26) return 1;
    if (FPSideEffect(ins)) return 1;
    return VolatileIns(ins);
}
unsigned char VolatileIns(instruction *ins)
{
    int i;
    i=ins->num_operands;
    while (--i>=0) if (IsVolatile(ins->operands[i])) return 1;
    if (ins->result!=NULL && IsVolatile(ins->result)) return 1;
    return 0;
}
unsigned char FreeUselessIns(block *head,unsigned char just_the_loop,unsigned char in_regalloc)
{
    block *blk;
    instruction *ins,*prev;
    unsigned char change;
    change=0;
    for (blk=head;blk!=NULL;blk=blk->prev_block) {
        ins=blk->ins.hd.prev;
        if (just_the_loop && !(blk->class&0x200)) {
            while (ins->head.opcode!=0x4b) {
                ins->ins_flags&=~1U;
                ins=ins->head.prev;
            }
        } else {
            while (ins->head.opcode!=0x4b) {
                prev=ins->head.prev;
                if (!(ins->ins_flags&1)) {
                    change=1;
                    if (in_regalloc) DoNothing(ins);
                    else FreeIns(ins);
                } else ins->ins_flags&=~1U;
                ins=prev;
            }
        }
    }
    return change;
}
void FreeUnVisitedTemps(void)
{
    name *op,**owner;
    owner=&Names[2];
    for (;;) {
        op=*owner;
        if (op==NULL) break;
        if (op->t.temp_flags&4) {
            op->t.temp_flags&=~4U;
            owner=&op->n.next_name;
        } else {
            *owner=op->n.next_name;
            FreeAName(op);
        }
    }
}

unsigned char LoopInsDead(void)
{
    return DoInsDead(1,0);
}

unsigned char RegInsDead(void)
{
    return DoInsDead(0,1);
}
