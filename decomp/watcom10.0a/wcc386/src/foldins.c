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
* Description:  Fold instruction operands after constant propagation.
*
****************************************************************************/


#include "block.h"
#include "cgtree.h"
#include "genopt.h"
#include "stubs.h"
#include <recon.h>

unsigned char ConstFold(block *root)
{
    instruction *ins,*next;
    block *blk;
    unsigned char change;
    change=0;
    blk=root;
    for (;;) {
        ins=blk->ins.hd.next;
        while (ins->head.opcode!=0x4b) {
            next=ins->head.next;
            if (FoldIns(ins)!=NULL) change=1;
            ins=next;
        }
        blk=blk->u.partition;
        if (blk==root) break;
    }
    return change;
}
instruction *FoldIns(instruction *ins)
{
    int i;
    unsigned char have_const;
    if (ins->ins_flags&0x20) return NULL;
    i=ins->num_operands;
    have_const=0;
    while (--i>=0) {
        if (ins->operands[i]->n.class==0) {
            if (ins->operands[i]->c.const_type==0) return FoldAbsolute(ins);
            have_const=1;
        }
    }
    if (have_const) {
        i=NumOperands(ins);
        if (i>1 && RelocConst(ins->operands[1])) return CmpRelocZero(ins,0,1);
        if (i!=0 && RelocConst(ins->operands[0])) return CmpRelocZero(ins,1,0);
    }
    return NULL;
}
unsigned char RelocConst(name *op)
{
    return op->n.class==0 && op->c.const_type!=0;
}
instruction *KillCompare(instruction *ins,name *result);
instruction *CmpRelocZero(instruction *ins,int c,int r)
{
    name *cons,*rel;
    unsigned char truth,index;
    if (NumOperands(ins)!=2) return NULL;
    cons=ins->operands[c];
    if (cons->n.class!=0 || cons->c.const_type!=0 ||
        CFTest(cons->c.value)!=0) return NULL;
    rel=ins->operands[r];
    if (rel->c.const_type==2 && !AskSegNear((unsigned short)rel->c.int_value))
        return NULL;
    switch (ins->head.opcode) {
    case 0x2f:case 0x30:case 0x33:case 0x34:truth=0;break;
    case 0x2e:case 0x31:case 0x32:case 0x35:truth=1;break;
    default:return NULL;
    }
    if (!ActiveCompare(ins)) return NULL;
    if (c!=1) truth=!truth;
    if (truth) index=ins->flags.bool&15;
    else index=(ins->flags.bool>>4)&15;
    ins->flags.bool=index|(index<<4);
    return KillCompare(ins,AllocIntConst(truth?FETrue():0));
}
instruction *KillCompare(instruction *ins,name *result)
{
    instruction *new_ins;
    if (ins->result!=NULL) {
        new_ins=MakeMove(result,ins->result,ins->result->n.name_class);
        DupSeg(ins,new_ins);
        SetCSEBits(ins,new_ins);
        ReplIns(ins,new_ins);
    } else {
        new_ins=ins;
        DoNothing(ins);
    }
    return new_ins;
}
instruction *FoldAbsolute(instruction *ins)
{
    instruction *new_ins;
    int i;
    name *result,*tmp;
    tree_node *fold,*left,*rite;
    type_def *tipe,*left_tipe,*rite_tipe,*fold_tipe;
    tipe=ClassType(ins->type_class);
    left_tipe=ClassType(ins->head.opcode==0x24 || ins->head.opcode==0x25 ?
                       ins->base_type_class : ins->type_class);
    i=NumOperands(ins);
    left=NULL;
    rite=NULL;
    if (i!=0) {
        left=TName(ins->operands[0],left_tipe);
        if (i>1) {
            if (ins->operands[1]->n.name_class==XX) rite_tipe=tipe;
            else rite_tipe=ClassType(ins->operands[1]->n.name_class);
            rite=TName(ins->operands[1],rite_tipe);
        }
    }
    fold=NULL;
    switch (ins->head.opcode) {
    case 1: fold=FoldPlus(left,rite,tipe); break;
    case 3: fold=FoldMinus(left,rite,tipe); break;
    case 5: fold=FoldTimes(left,rite,tipe); break;
    case 7: fold=FoldDiv(left,rite,tipe); break;
    case 8: fold=FoldMod(left,rite,tipe); break;
    case 9: fold=FoldAnd(left,rite,tipe); break;
    case 10: fold=FoldOr(left,rite,tipe); break;
    case 11: fold=FoldXor(left,rite,tipe); break;
    case 12: fold=FoldRShift(left,rite,tipe); break;
    case 13: fold=FoldLShift(left,rite,tipe); break;
    case 0x11: fold=FoldUMinus(left,tipe); break;
    case 0x12: fold=Fold1sComp(left,tipe); break;
    case 0x24: case 0x25: fold=FoldCnvRnd(ins->head.opcode,left,tipe); break;
    case 0x30: case 0x31: case 0x32: case 0x33: case 0x34: case 0x35:
        if (ActiveCompare(ins)) {
            fold=FoldCompare(ins->head.opcode,left,rite,tipe);
            if (fold!=NULL) return StraightLine(ins,fold,1);
        }
        break;
    case 0x2e:
        if (ActiveCompare(ins)) {
            fold=FoldAnd(left,rite,tipe);
            if (fold!=NULL) return StraightLine(ins,fold,1);
        }
        break;
    case 0x2f:
        if (ActiveCompare(ins)) {
            fold=FoldAnd(left,rite,tipe);
            if (fold!=NULL) return StraightLine(ins,fold,0);
        }
        break;
    }
    if (fold!=NULL) {
        fold_tipe=fold->tipe;
        result=TGetName(fold);
        /* Both predicates are evaluated by the original, even if resultNULL. */
        if ((result!=NULL) & !NeedConvert(fold_tipe,tipe)) {
            ins->table=NULL;
            if (result->n.class!=0 && result->n.size!=TypeClassSize[ins->type_class]) return NULL;
            if (result->n.class!=0 && ins->result->n.size!=TypeClassSize[ins->type_class]) return NULL;
            new_ins=MakeMove(result,ins->result,ins->type_class);
            SetCSEBits(ins,new_ins);
            DupSeg(ins,new_ins);
            ReplIns(ins,new_ins);
            return new_ins;
        }
    } else {
        if (left!=NULL) TGetName(left);
        if (rite!=NULL) TGetName(rite);
    }
    /* The later SUB-to-ADD/OkToNegate rewrite is absent in this binary. */
    switch (ins->head.opcode) {
    case 1: case 2: case 5: case 6: case 9: case 10: case 11:
        /* The old target swaps constants even for pointer-class operands. */
        tmp=ins->operands[0];
        if (tmp->n.class==0 && tmp->c.const_type==0) {
            ins->operands[0]=ins->operands[1];
            ins->operands[1]=tmp;
        }
        break;
    }
    return NULL;
}
unsigned char ActiveCompare(instruction *ins)
{
    if (((ins->flags.bool>>4)&15)==(ins->flags.bool&15)) return 0;
    if (ins->table==NULL) return 1;
    if (ins->u.gen_table==NULL) return 0;
    return DoesSomething(ins)!=0;
}
instruction *StraightLine(instruction *ins,struct tree_node *fold,unsigned char sense)
{
    name *result;
    unsigned index;
    result=TGetName(fold);
    if (result==NULL) return NULL;
    if (result->n.class!=0 || result->c.const_type!=0) return NULL;
    if (result->c.int_value==0) sense=!sense;
    if (sense) index=ins->flags.bool&15;
    else index=(ins->flags.bool>>4)&15;
    ins->flags.bool=(unsigned char)(index|(index<<4));
    return KillCompare(ins,result);
}
