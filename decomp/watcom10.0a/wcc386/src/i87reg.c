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
* Description:  Track 8087 instruction effects and stackable names.
*
****************************************************************************/


#include "block.h"
#include "genopt.h"
#include "stubs.h"
#include <recon.h>

void FPSetStack(name *value)
{
    if (value->n.class==2 && !(value->v.usage&2) &&
        (value->n.name_class==FS || value->n.name_class==FD))
        value->t.temp_flags|=0x10; /* CAN_STACK */
}

unsigned char FPSideEffect(instruction *ins)
{
    int i;
    unsigned char has_fp_reg;
    if ((CGProcessorVersion&0x70)<0x10) return 0; /* _FPULevel(FPU_87) */
    if (ins->head.opcode==0x36 || ins->head.opcode==0x29) return 1;
    if (ins->type_class!=FS && ins->type_class!=FD &&
        ins->base_type_class!=FS && ins->base_type_class!=FD) return 0;
    i=ins->num_operands;
    has_fp_reg=0;
    while (--i>=0) {
        if (ins->operands[i]->n.class==3 && (ins->operands[i]->r.reg&0x00ff0000U))
            has_fp_reg=1;
    }
    if (ins->result!=NULL && ins->result->n.class==3 &&
        (ins->result->r.reg&0x00ff0000U)) has_fp_reg=1;
    if (has_fp_reg) {
        if (ins->ins_flags&8) return 1; /* INS_PARAMETER */
        if (ins->stk_entry!=ins->stk_exit) return 1;
    }
    return 0;
}

unsigned char FPIsConvert(instruction *ins)
{
    unsigned char op_class,res_class;
    if ((CGProcessorVersion&0x70)<0x10) return 0;
    if (ins->operands[0]->n.class==0) return 0;
    op_class=ins->operands[0]->n.name_class;
    res_class=ins->result->n.name_class;
    if (op_class==res_class) return 0;
    if (ins->type_class==FS || ins->type_class==FD ||
        ins->base_type_class==FS || ins->base_type_class==FD) return 1;
    return 0;
}

unsigned char FPIsStack(name *op);
unsigned char FPStackOp(name *op)
{
    if (((CGProcessorVersion&0xf0)&0x7f)<0x10) return 0;
    return FPIsStack(op);
}

unsigned char FPInsClass(instruction *ins)
{
    if ((CGProcessorVersion&0x70)<0x10) return 10;
    if (ins->type_class!=FS && ins->type_class!=FD &&
        ins->base_type_class!=FS && ins->base_type_class!=FD) return 10;
    return FD;
}

unsigned char FPIsStack(name *opnd)
{
    return opnd->n.class==2 && (opnd->t.temp_flags&0x10)!=0;
}

void FSinCos(void);
void StackShortLivedTemps(void);
void NoStackAcrossCalls(void);
void FPAlloc(void);
void FPRegAlloc(void)
{
    if ((CGProcessorVersion&0x70)>=0x10) {
        if ((CGProcessorVersion&0x70)>=0x20) FSinCos();
        StackShortLivedTemps();
        NoStackAcrossCalls();
        FPAlloc();
    }
}
void FindSinCos(instruction *ins,unsigned char next_op);
void FSinCos(void)
{
    block *blk;
    instruction *ins;
    for (blk=HeadBlock;blk!=NULL;blk=blk->next_block) {
        ins=blk->ins.hd.next;
        while (ins->head.opcode!=0x4b) {
            if (ins->head.opcode==0x15) FindSinCos(ins,0x14);
            else if (ins->head.opcode==0x14) FindSinCos(ins,0x15);
            ins=ins->head.next;
        }
    }
}
void FindSinCos(instruction *ins,unsigned char next_op) { STUB_TRAP(); }
void StackShortLivedTemps(void)
{
    conflict_node *conf;
    instruction *ins1,*ins2;
    name *temp;
    for (conf=ConfList;conf!=NULL;conf=conf->next_conflict) {
        ins1=conf->ins_range.first;
        ins2=conf->ins_range.last;
        if (ins1==NULL || ins2==NULL) continue;
        if (ins1->type_class!=FS && ins1->type_class!=FD &&
            ins1->base_type_class!=FS && ins1->base_type_class!=FD) continue;
        if (ins2->type_class!=FS && ins2->type_class!=FD &&
            ins2->base_type_class!=FS && ins2->base_type_class!=FD) continue;
        if ((ins1->ins_flags&8) || (ins2->ins_flags&8)) continue;
        if (ins1->head.next!=ins2) continue;
        temp=conf->name;
        if (temp->n.class!=2 || (temp->t.temp_flags&0x80) ||
            (name *)temp->t.alias!=temp) continue;
        FPSetStack(temp);
    }
}
void CheckForStack(name *temp);
void NoStackAcrossCalls(void)
{
    block *blk;
    instruction *ins;
    int i;
    for (blk=HeadBlock;blk!=NULL;blk=blk->next_block) {
        ins=blk->ins.hd.next;
        while (ins->head.opcode!=0x4b) {
            i=ins->num_operands;
            while (--i>=0) CheckForStack(ins->operands[i]);
            if (ins->result!=NULL) CheckForStack(ins->result);
            ins=ins->head.next;
        }
    }
}
unsigned char CanStack(name *temp);
void CheckForStack(name *temp)
{
    if (temp->n.class!=2) return;
    if (CanStack(temp)) return;
    temp->t.temp_flags&=~0x10;
}
unsigned char CanStack(name *temp)
{
    instruction *first,*last;
    conflict_node *conf;
    if (temp->v.usage&2) return 0;
    conf=temp->v.conflict;
    while (conf!=NULL) {
        if (conf->start_block==NULL || conf->ins_range.first==NULL ||
            conf->ins_range.last==NULL ||
            conf->ins_range.first==conf->ins_range.last) return 0;
        first=conf->ins_range.first->head.next;
        last=conf->ins_range.last;
        for (;;) {
            if (first==last) break;
            if (first->head.opcode==0x36 || first->head.opcode==0x29) return 0;
            first=first->head.next;
        }
        conf=conf->next_for_name;
    }
    return 1;
}
name *FPStatWord;
void SetStackLevel(instruction *ins,int *stk_level);
unsigned char AssignFPResult(block *blk,instruction *ins,int *stk_level);
void AssignFPOps(instruction *ins,int *stk_level);
void LiveInfoUpdate(void);
void FPConvert(void);
void MoveSegOp(instruction *ins,instruction *new_ins,int duplicate);
void FPAlloc(void)
{
    block *blk;
    instruction *ins,*new_ins;
    int stk_level,sequence;
    unsigned char need_live_update;
    name *temp;
    CurrProc->state.unalterable|=0x00ff0000U;
    FPStatWord=NULL;
    stk_level=0;
    need_live_update=0;
    for (blk=HeadBlock;blk!=NULL;blk=blk->next_block) {
        ins=blk->ins.hd.next;
        if (ins->head.opcode==0x4b) continue;
        ins->stk_entry=stk_level;
        SetStackLevel(ins,&stk_level);
        sequence=0;
        for (;;) {
            do {
                if (ins->head.opcode!=0x24) break;
                if (!((ins->type_class==FS && ins->base_type_class==FD) ||
                      (ins->type_class==FD && ins->base_type_class==FS))) break;
                if (ins->operands[0]->n.class==3) break;
                if (ins->operands[0]->n.class==2 &&
                    (ins->operands[0]->t.temp_flags&0x10)) break;
                temp=AllocTemp(ins->base_type_class);
                temp->t.temp_flags|=0x10;
                new_ins=MakeMove(ins->operands[0],temp,ins->base_type_class);
                ins->operands[0]=temp;
                MoveSegOp(ins,new_ins,0);
                PrefixIns(ins,new_ins);
                ins=new_ins;
            } while (0);
            if (AssignFPResult(blk,ins,&stk_level)) need_live_update=1;
            ins->sequence=sequence;
            ins->stk_exit=stk_level;
            if ((CGProcessorVersion&0x70)>=0x30 && stk_level==0) ++sequence;
            ins=ins->head.next;
            if (ins->head.opcode==0x4b) break;
            ins->stk_entry=stk_level;
            SetStackLevel(ins,&stk_level);
            AssignFPOps(ins,&stk_level);
        }
        if ((blk->class&1) && stk_level==1) stk_level=0;
        if (stk_level!=0) Zoiks(74);
    }
    if (need_live_update) LiveInfoUpdate();
    FPConvert();
}
void SetStackLevel(instruction *ins,int *stk_level)
{
    if (ins->head.opcode!=0x36 && ins->head.opcode!=0x29) return;
    if (ins->result->r.reg!=0x00010000U) return; /* HW_ST0 */
    if (!(ins->flags.call_flags&4)) return; /* CALL_IGNORES_RETURN */
    --*stk_level;
}
/* i86enc.c owns the single physical ST register-encoding table. */
extern hw_reg_set FPRegs[8];
conflict_node *FindConflictNode(name *opnd,block *blk,instruction *ins);
unsigned char MathOpsBlowStack(conflict_node *conf,int stk_level);
unsigned char AssignARegister(conflict_node *conf,hw_reg_set reg);
unsigned char AssignFPResult(block *blk,instruction *ins,int *stk_level)
{
    name *op;
    conflict_node *conf;
    op=ins->result;
    if (op==NULL) return 0;
    if (op->n.class==3) {
        if (op->r.reg&0x00ff0000U) ++*stk_level;
        return 0;
    }
    if (op->n.class!=2 || !(op->t.temp_flags&0x10)) return 0;
    if (op->v.usage&0xa) return 0;
    if (op->n.name_class!=FS && op->n.name_class!=FD) return 0;
    if (*stk_level<0 || *stk_level>=Max87Stk-1) return 0;
    conf=FindConflictNode(op,blk,ins);
    if (conf==NULL || MathOpsBlowStack(conf,*stk_level)) return 0;
    ++*stk_level;
    return AssignARegister(conf,FPRegs[*stk_level]);
}
int FPStkReq(instruction *ins);
int NumOperands(instruction *ins);
unsigned char MathOpsBlowStack(conflict_node *conf,int stk_level)
{
    instruction *ins;
    ins=conf->ins_range.first;
    if (ins==conf->ins_range.last) return 1;
    for (;;) {
        ins=ins->head.next;
        if (ins==conf->ins_range.last) break;
        if (FPStkReq(ins)+stk_level+NumOperands(ins)>=Max87Stk) return 1;
    }
    return 0;
}
int Count87Regs(hw_reg_set regs);
void AssignFPOps(instruction *ins,int *stk_level)
{
    int old_level,i;
    name *op;
    i=ins->num_operands;
    if (ins->head.opcode==0x36 || ins->head.opcode==0x29)
        *stk_level-=Count87Regs(ins->operands[0]->r.reg);
    else {
        old_level=*stk_level;
        while (--i>=0) {
            op=ins->operands[i];
            if (op->n.class==3 && (op->r.reg&0x00ff0000U)) --*stk_level;
        }
        if (ins->num_operands<2) return;
        if (ins->operands[0]!=ins->operands[1]) return;
        if (old_level==*stk_level) return;
        ++*stk_level;
    }
}
int Count87Regs(hw_reg_set regs)
{
    int i,count;
    count=0;
    for (i=0;i<8;++i) {
        if (FPRegs[i]&regs) ++count;
    }
    return count;
}
void CnvOperand(instruction *ins);
void CnvResult(instruction *ins);
void FPConvert(void)
{
    block *blk;
    instruction *ins,*next;
    for (blk=HeadBlock;blk!=NULL;blk=blk->next_block) {
        ins=blk->ins.hd.next;
        while (ins->head.opcode!=0x4b) {
            next=ins->head.next;
            if (ins->head.opcode==0x24 &&
                (ins->type_class==FS || ins->type_class==FD ||
                 ins->base_type_class==FS || ins->base_type_class==FD)) {
                CnvOperand(ins);
                CnvResult(ins);
            }
            ins=next;
        }
    }
}
conflict_node *NameConflict(instruction *ins,name *op);
conflict_node *InMemory(conflict_node *conf);
void ToMemory(instruction *ins,name *value)
{
    conflict_node *conf;
    if (value->n.class==2 || value->n.class==1) {
        conf=NameConflict(ins,value);
        if (conf!=NULL) InMemory(conf);
        else value->v.usage|=0x90; /* NEEDS_MEMORY | USE_MEMORY */
    }
}
void CnvOperand(instruction *ins)
{
    name *temp;
    instruction *new_ins;
    unsigned char class;
    class=ins->base_type_class;
    switch (class) {
    case U1:case I1:
        temp=AllocTemp(I2);
        new_ins=MakeConvert(ins->operands[0],temp,I2,class);
        ins->base_type_class=I2;
        ins->operands[0]=temp;
        MoveSegOp(ins,new_ins,0);
        PrefixIns(ins,new_ins);
        InMemory(NameConflict(ins,temp));
        break;
    case U2:
        temp=AllocTemp(I4);
        new_ins=MakeConvert(ins->operands[0],temp,I4,class);
        ins->base_type_class=I4;
        ins->operands[0]=temp;
        MoveSegOp(ins,new_ins,0);
        PrefixIns(ins,new_ins);
        InMemory(NameConflict(ins,temp));
        break;
    case I2:case I4:
        ToMemory(ins,ins->operands[0]);
        break;
    case U4:
        /* Before U8 existed, the original made an eight-byte XX temporary
         * and explicitly zero-extended the upper DWORD with a second move. */
        temp=AllocTemp(XX);
        temp->n.size=8;
        new_ins=MakeMove(ins->operands[0],TempOffset(temp,0,U4),U4);
        ins->operands[0]=temp;
        MoveSegOp(ins,new_ins,0);
        PrefixIns(ins,new_ins);
        InMemory(NameConflict(ins,temp));
        SuffixIns(new_ins,MakeMove(AllocIntConst(0),TempOffset(temp,4,U4),U4));
        break;
    }
}
void CnvResult(instruction *ins)
{
    name *temp;
    instruction *new_ins;
    unsigned char class;
    class=ins->type_class;
    switch (class) {
    case U1:case I1:
        temp=AllocTemp(I2);
        temp->v.usage|=0x90;
        new_ins=MakeConvert(temp,ins->result,class,I2);
        ins->result=temp;
        MoveSegRes(ins,new_ins);
        SuffixIns(ins,new_ins);
        break;
    case U2:
        temp=AllocTemp(I4);
        temp->v.usage|=0x90;
        new_ins=MakeConvert(temp,ins->result,class,I4);
        ins->result=temp;
        MoveSegRes(ins,new_ins);
        SuffixIns(ins,new_ins);
        break;
    case I2:case I4:
        ToMemory(ins,ins->result);
        break;
    case U4:
        temp=AllocTemp(XX);
        temp->n.size=8;
        temp->v.usage|=0x90;
        new_ins=MakeMove(TempOffset(temp,0,U4),ins->result,U4);
        ins->result=temp;
        MoveSegRes(ins,new_ins);
        SuffixIns(ins,new_ins);
        break;
    }
}


unsigned char FPStackIns(instruction *ins)
{
    if ((CGProcessorVersion&0x70U)<0x10U) return 0;
    if (ins->head.opcode==0x36 || ins->head.opcode==0x29) return 1;
    return ins->type_class==FS || ins->type_class==FD
        || ins->base_type_class==FS || ins->base_type_class==FD;
}

void FPNotStack(name *value)
{
    if (value->n.class==2) value->t.temp_flags&=~0x10U; /* CAN_STACK. */
}
