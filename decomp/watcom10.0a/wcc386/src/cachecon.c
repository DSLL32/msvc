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
* Description:  Cache constants in register-allocatable temporaries.
*
****************************************************************************/


#include "block.h"
#include "genopt.h"
#include "stubs.h"
#include <recon.h>

/* Actual8074c/80748 traversal state, separate from loopopts' Head. */
static block *Head;
static block *(*Next)(block *);
unsigned char FindMaxClass(name *constant,int *references);
unsigned char ReplaceConst(name *constant,name *temp,unsigned char type_class);
void ConstToTemp(block *pre,block *head,block *(*next)(block *))
{
    name *constant,*temp;
    unsigned char type_class;
    int references;
    Head=head;
    Next=next;
    for (constant=Names[0];constant!=NULL;constant=constant->n.next_name) {
        if (constant->c.const_type==4) continue;
        type_class=FindMaxClass(constant,&references);
        if (type_class==XX) continue;
        if (head==HeadBlock && references<2) continue;
        temp=AllocTemp(type_class);
        temp->t.temp_flags|=0x100; /* CONST_TEMP */
        temp->v.symbol=constant;
        if (!ReplaceConst(constant,temp,type_class)) continue;
        temp->v.usage|=2;
        SuffixIns(pre->ins.hd.prev,MakeMove(constant,temp,type_class));
    }
}
extern unsigned char Unsigned[];
int CountOps(instruction *ins,name *constant);
unsigned char FindMaxClass(name *constant,int *references)
{
    block *blk;
    instruction *ins;
    int i,type_class,num_operands;
    type_class=-1;
    *references=0;
    for (blk=Head;blk!=NULL;blk=Next(blk)) {
        for (ins=blk->ins.hd.next;ins->head.opcode!=0x4b;ins=ins->head.next) {
            num_operands=CountOps(ins,constant);
            for (i=0;i<num_operands;++i) {
                if (ins->operands[i]==constant) {
                    ++*references;
                    if ((int)ins->type_class>=type_class)
                        type_class=(ins->head.opcode==0x24 || ins->head.opcode==0x25)?
                            ins->base_type_class:ins->type_class;
                }
            }
        }
    }
    if (type_class==-1) return XX;
    /* Actual6a96d retains the Unsigned normalization later commented
     * out in June1995; the source's pre-assignment comparison is kept. */
    return Unsigned[type_class];
}
unsigned char IsStackReg(name *op);
int CountOps(instruction *ins,name *constant)
{
    switch (ins->head.opcode) {
    case 0x0c: case 0x0d: return 1;
    case 1: case 3:
        if (IsStackReg(ins->result)) return 0;
        if (constant->c.int_value==1) return 1;
        break;
    case 0x36: case 0x29: return 0;
    }
    return ins->num_operands;
}
unsigned char ReplaceConst(name *constant,name *temp,unsigned char type_class)
{
    block *blk;
    instruction *ins;
    int i,ins_class,num_operands;
    unsigned char change;
    change=0;
    for (blk=Head;blk!=NULL;blk=Next(blk)) {
        for (ins=blk->ins.hd.next;ins->head.opcode!=0x4b;ins=ins->head.next) {
            ins_class=Unsigned[(ins->head.opcode==0x24 || ins->head.opcode==0x25)?
                ins->base_type_class:ins->type_class];
            num_operands=CountOps(ins,constant);
            for (i=0;i<num_operands;++i) {
                if (ins->operands[i]!=constant) continue;
                if (ins_class==temp->n.name_class) {
                    ins->operands[i]=temp;
                    change=1;
                } else if (ins_class>=0 && ins_class<=PT && type_class<=PT) {
                    ins->operands[i]=TempOffset(temp,0,ins_class);
                    change=1;
                }
            }
        }
    }
    return change;
}

instruction *rDOCVT(instruction *ins);
conflict_node *InMemory(conflict_node *conf);
void MemConstTemp(conflict_node *conf)
{
    block *blk;
    instruction *ins;
    int i;
    name *other,*temp;
    temp=conf->name;
    other=NULL;
    for (blk=HeadBlock;blk!=NULL;blk=blk->next_block) {
        ins=blk->ins.hd.next;
        while (ins->head.opcode!=0x4b) {
            for (i=0;i<ins->num_operands;++i) {
                if (ins->operands[i]->n.class!=2) continue;
                if (DeAlias(ins->operands[i])!=temp) continue;
                ins->head.state=0;
                ins->operands[i]=(name *)ins->operands[i]->v.symbol;
                if (ins->head.opcode==0x24) ins=rDOCVT(ins);
            }
            if (ins->result!=NULL && ins->result->n.class==2 &&
                DeAlias(ins->result)==temp) {
                other=ins->operands[0];
                ins->head.opcode=0;
                ins->num_operands=0;
                ins->result=NULL;
                DoNothing(ins);
            }
            /* The actual1994 pass has no later FoldIns call here. */
            ins=ins->head.next;
        }
    }
    if (other==NULL || other->n.class!=2 || !(other->t.temp_flags&0x100) ||
        other->v.conflict==NULL) return;
    for (blk=HeadBlock;blk!=NULL;blk=blk->next_block) {
        for (ins=blk->ins.hd.next;ins->head.opcode!=0x4b;ins=ins->head.next) {
            for (i=0;i<ins->num_operands;++i) {
                if (ins->operands[i]->n.class==2 && DeAlias(ins->operands[i])==other)
                    return;
            }
        }
    }
    /* The callback transports one conflict pointer; its pointer result is
     * deliberately discarded at this historical SafeRecurse boundary. */
    SafeRecurse((recurse_callback)InMemory,other->v.conflict);
}
