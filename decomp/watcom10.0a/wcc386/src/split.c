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
* Description:  Dispatch target instruction reductions.
*
****************************************************************************/


#include "block.h"
#include "stubs.h"
#include "regchoices.h"
#include <recon.h>

typedef instruction *(*reduction)(instruction *);
extern reduction ReduceTab[89];
instruction *Reduce(instruction *ins)
{
    unsigned which;
    ins->head.state=0;
    which=ins->u.gen_table->generate;
    /* Only the encountered source-backed table prefix is declared. Other
     * reduction data must trap in both builds, never default to a null slot. */
    if (which<0x68 || which>0xc0) STUB_TRAP();
    /* The four actual pointer-arithmetic holes are NULL in r.h and the
     * shipped table; unsupported holes trap before indirect dispatch. */
    if (which>=0xa6 && which<=0xa9) STUB_TRAP();
    return ReduceTab[which-0x68](ins);
}

void MoveSegOp(instruction *ins,instruction *new_ins,int operand);
void PrefixIns(instruction *ins,instruction *new_ins);
unsigned char Op1Possible(instruction *ins);
void MarkPossible(instruction *ins,name *opnd,unsigned char possible);
conflict_node *NameConflict(instruction *ins,name *opnd);
conflict_node *GiveRegister(conflict_node *conf,unsigned char needs_one);
instruction *rOP1REG(instruction *ins)
{
    instruction *new_ins;
    name *temp;
    unsigned char type_class;

    type_class=ins->head.opcode==0x24 || ins->head.opcode==0x25 ?
               ins->base_type_class : ins->type_class;
    temp=AllocTemp(type_class);
    new_ins=MakeMove(ins->operands[0],temp,type_class);
    ins->operands[0]=temp;
    MoveSegOp(ins,new_ins,0);
    PrefixIns(ins,new_ins);
    MarkPossible(ins,temp,Op1Possible(ins));
    ins->u.gen_table=NULL;
    GiveRegister(NameConflict(ins,temp),1);
    return new_ins;
}
unsigned char Op1Possible(instruction *ins)
{
    return RegList[ins->u.gen_table->reg_set].left;
}

/* Encountered type-size reductions from the original11-class universe,
 * cf7c140..7c14b, predating later U8/I8 and FL additions. */
unsigned char HalfClass[11]={0,0,U1,I1,U2,I2,U2,U2,I2,I4,U4};

instruction *rSWAPOPS(instruction *ins)
{
    name *op;
    op=ins->operands[0];
    ins->operands[0]=ins->operands[1];
    ins->operands[1]=op;
    return ins;
}
instruction *rSWAPCMP(instruction *ins) { STUB_TRAP();return NULL; }

name *LowPart(name *value,unsigned char type_class);
void HalfType(instruction *ins)
{
    ins->type_class=HalfClass[ins->type_class];
    ins->table=NULL;
    ins->head.state=0;
}
instruction *rCYPLOW(instruction *ins)
{
    HalfType(ins);
    ins->operands[0]=LowPart(ins->operands[0],ins->type_class);
    ins->operands[1]=LowPart(ins->operands[1],ins->type_class);
    if (ins->result!=NULL && ins->head.opcode<0x2e)
        ins->result=LowPart(ins->result,ins->type_class);
    return ins;
}

void InMemory(conflict_node *conf);
void ForceToMemory(name *value)
{
    while (value->v.conflict!=NULL) InMemory(value->v.conflict);
}
instruction *rFORCERESMEM(instruction *ins)
{
    ForceToMemory(ins->result);
    return ins;
}

void DupSeg(instruction *ins,instruction *new_ins);
void ReplIns(instruction *ins,instruction *new_ins);
instruction *rMOVEINDEX(instruction *ins)
{
    instruction *new_ins;
    /* This1994 near-address reduction fixes U4. Its shared replacement
     * tail starts after rMAKEMOVE's DupSeg call. */
    new_ins=MakeMove(ins->operands[0]->i.index,ins->result,U4);
    ReplIns(ins,new_ins);
    return new_ins;
}

void CheckCC(instruction *ins,instruction *new_ins);
unsigned char Op2Possible(instruction *ins)
{
    return RegList[ins->u.gen_table->reg_set].right;
}
instruction *rLOADOP2(instruction *ins)
{
    instruction *new_ins;
    name *temp;
    temp=AllocTemp(ins->type_class);
    new_ins=MakeMove(ins->operands[1],temp,ins->type_class);
    CheckCC(ins,new_ins);
    ins->operands[1]=temp;
    MoveSegOp(ins,new_ins,0);
    PrefixIns(ins,new_ins);
    MarkPossible(ins,temp,Op2Possible(ins));
    ins->u.gen_table=NULL;
    GiveRegister(NameConflict(ins,temp),1);
    return new_ins;
}

unsigned char CanUseOp1(instruction *ins,name *opnd)
{
    name *index;
    if (opnd->n.class!=3) return 0;
    if (opnd->r.reg&ins->head.next->head.live.regs) return 0;
    if (ins->result->n.class==4) {
        index=ins->result->i.index;
        if (index->n.class==3 && (index->r.reg&opnd->r.reg)) return 0;
    }
    return 1;
}
unsigned char ResultPossible(instruction *ins)
{
    return RegList[ins->u.gen_table->reg_set].result;
}
void MoveSegRes(instruction *ins,instruction *new_ins);
void SuffixIns(instruction *ins,instruction *new_ins);
instruction *rUSEREGISTER(instruction *ins)
{
    instruction *new_ins,*after;
    name *temp;
    temp=ins->operands[0];
    if (CanUseOp1(ins,temp)) {
        new_ins=MakeMove(temp,ins->result,ins->type_class);
        ins->result=temp;
        MoveSegRes(ins,new_ins);
        SuffixIns(ins,new_ins);
        new_ins=ins;
    } else {
        temp=AllocTemp(ins->type_class);
        new_ins=MakeMove(ins->operands[0],temp,ins->type_class);
        CheckCC(ins,new_ins);
        ins->operands[0]=temp;
        MoveSegOp(ins,new_ins,0);
        PrefixIns(ins,new_ins);
        after=MakeMove(temp,ins->result,ins->type_class);
        ins->result=temp;
        MoveSegRes(ins,after);
        SuffixIns(ins,after);
        MarkPossible(ins,temp,ResultPossible(ins));
        ins->u.gen_table=NULL;
        GiveRegister(NameConflict(ins,temp),1);
    }
    return new_ins;
}

void NegOp2(instruction *ins)
{
    ins->operands[1]=AllocConst(CFCnvI32F((long)(0UL-(unsigned long)ins->operands[1]->c.int_value)));
}
instruction *rMAKESUB(instruction *ins)
{
    ins->head.opcode=3; /* OP_SUB */
    NegOp2(ins);
    ins->u.gen_table=NULL;
    return ins;
}

void DupSegRes(instruction *ins,instruction *new_ins);
instruction *rMOVOP1RES(instruction *ins)
{
    instruction *new_ins;
    new_ins=MakeMove(ins->operands[0],ins->result,ins->type_class);
    ins->operands[0]=ins->result;
    MoveSegOp(ins,new_ins,0);
    DupSegRes(ins,new_ins);
    PrefixIns(ins,new_ins);
    return new_ins;
}

void MoveSegRes(instruction *ins,instruction *new_ins);
void SuffixIns(instruction *ins,instruction *new_ins);
instruction *rOP1RESTEMP(instruction *ins)
{
    instruction *first,*last;
    name *temp;
    temp=AllocTemp(ins->type_class);
    first=MakeMove(ins->operands[0],temp,ins->type_class);
    ins->operands[0]=temp;
    MoveSegOp(ins,first,0);
    PrefixIns(ins,first);
    last=MakeMove(temp,ins->result,ins->type_class);
    ins->result=temp;
    MoveSegRes(ins,last);
    SuffixIns(ins,last);
    return first;
}

void ChangeType(instruction *ins,unsigned char type_class)
{
    ins->table=NULL;
    ins->head.state=0;
    ins->type_class=type_class;
}

/* split.c's actual XOR-self replacement retains the result name and class. */
instruction *rMAKEXORRR(instruction *ins)
{
    instruction *new_ins;
    new_ins=MakeBinary(11,ins->result,ins->result,ins->result,ins->type_class);
    ReplIns(ins,new_ins);
    return new_ins;
}

instruction *rMOVOP2TEMP(instruction *ins)
{
    instruction *new_ins;
    name *temp;
    temp=AllocTemp(ins->type_class);
    new_ins=MakeMove(ins->operands[1],temp,ins->type_class);
    ins->operands[1]=temp;
    MoveSegOp(ins,new_ins,0);
    PrefixIns(ins,new_ins);
    return new_ins;
}
