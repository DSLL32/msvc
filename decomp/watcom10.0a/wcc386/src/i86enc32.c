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
* Description:  32-bit x86 effective-address and ModRM encoding.
*
****************************************************************************/


#include "block.h"
#include "genopt.h"
#include "options.h"
#include "stubs.h"
#include <recon.h>

unsigned char BaseIsSP(name *operand);
long TmpLoc(name *base,name *operand);
void LayRMRegOp(name *operand);
void AddToTemp(unsigned char value);
void CheckSize(void)
{
    if (!(TargetModel&0x800)) AddToTemp(0x67);
}
extern unsigned char Inst[];
extern int ILen;
unsigned char DoMDisp(name *operand,unsigned char alt);
/* Eight architectural SIB/ModRM register encodings, in hardware order. */
static hw_reg_set IndexTab[]={0x01000003,0x04000030,0x080000c0,0x0200000c,
                            0x800,0x400,0x10000100,0x20000200};
unsigned char DoIndex(hw_reg_set reg)
{
    int i;
    for (i=0;i<8;++i) if (IndexTab[i]==reg) break;
    if (i>=8) Zoiks(33);
    return (unsigned char)i;
}
void AddByte(unsigned char value);
unsigned char DoScaleIndex(hw_reg_set base,hw_reg_set index,int scale)
{
    unsigned char sib;
    sib=(unsigned char)(scale<<6);
    sib|=DoIndex(base);
    sib|=DoIndex(index)<<3;
    AddByte(sib);
    return 4;
}
void DoSymRef(name *operand,long value,unsigned char base);
void AddByte(unsigned char value);
void Add32Displacement(long value);
unsigned char Displacement(long value,hw_reg_set base)
{
    if (value==0 && !(base&0x400)) return 0;
    if (value<=127 && value>=-128) {
        AddByte((unsigned char)value);
        return 0x40;
    }
    Add32Displacement(value);
    return 0x80;
}
void DoSegRef(unsigned short segment);
void AddByte(unsigned char value);
void DoRelocConst(name *operand,int kind)
{
    if (operand->c.const_type==2) {
        ILen+=4;
        DoSymRef(operand->c.value,((name *)operand->c.value)->v.offset,0);
    } else if (operand->c.const_type==3) {
        ILen+=2;
        if (operand->c.value==NULL) DoSegRef((unsigned short)operand->c.int_value);
        else DoSymRef(operand->c.value,0,1);
        if (kind==U4 || kind==I4) {
            AddByte(0);
            AddByte(0);
        }
    } else if (operand->c.const_type==1) Zoiks(45);
}
unsigned char DoDisp(long value,hw_reg_set base,name *operand)
{
    name *tmp;
    if (operand==NULL) return Displacement(value,base);
    if (operand->n.class==2) {
        tmp=DeAlias(operand);
        if (tmp->t.location==0x7fffffff) Zoiks(34);
        value+=TmpLoc(tmp,operand);
        return Displacement(value,base);
    }
    if (operand->n.class==1) {
        ILen+=4;
        DoSymRef(operand,value,0);
        return 0x80;
    }
    if (operand->n.class==0) {
        DoRelocConst(operand,4);
        return 0x80;
    }
    Zoiks(78); /* Older diagnostic number in this actual 1994 body. */
    return 0;
}
void DoSymRef(name *operand,long value,unsigned char base);
void Add32Displacement(long value) { STUB_TRAP(); }
void EA(hw_reg_set base,hw_reg_set index,int scale,long value,name *mem_loc,unsigned char lea)
{
    if (index==0x800) Zoiks(79);
    if (base==0) {
        if (index==0) {
            if (scale!=0 || value!=0) Zoiks(79);
            Inst[1]|=DoMDisp(mem_loc,1);
        } else if (scale!=0) {
            Inst[1]|=DoScaleIndex(0x400,index,scale);
            if (mem_loc!=NULL) {
                ILen+=4;
                DoSymRef(mem_loc,value,0);
            } else Add32Displacement(value);
        } else {
            Inst[1]|=DoIndex(index);
            Inst[1]|=DoDisp(value,index,mem_loc);
        }
    } else {
        if (index==0) {
            if (scale==0 && base!=0x800) Inst[1]|=DoIndex(base);
            else Inst[1]|=DoScaleIndex(base,0x800,scale);
        } else {
            if (scale==0 && base==0x400 && (lea || !(TargetModel&0x600))) {
                base=index;
                index=0x400;
            }
            Inst[1]|=DoScaleIndex(base,index,scale);
        }
        Inst[1]|=DoDisp(value,base,mem_loc);
    }
}
void GenSeg(hw_reg_set reg);
hw_reg_set High64Reg(hw_reg_set reg);
hw_reg_set Low64Reg(hw_reg_set reg);
hw_reg_set CalcSegment(void *symbol,unsigned char memory_type);

void LayIdxModRM(name *op)
{
    hw_reg_set base_reg,idx_reg;
    name *mem_loc;
    CheckSize();
    if (op->i.index->r.reg&0xc000f000U) GenSeg(op->i.index->r.reg);
    idx_reg=op->i.index->r.reg&~0xc000f000U;
    base_reg=0;
    if (op->i.base!=NULL && !(op->i.index_flags&0x10)) {
        if (op->i.base->n.class==2) {
            if (BaseIsSP(op->i.base)) base_reg=0x800;
            else base_reg=0x400;
        } else if (op->i.base->n.class==1) {
            GenSeg(CalcSegment(op->i.base->v.symbol,op->i.base->m.memory_type));
        }
        mem_loc=op->i.base;
    } else mem_loc=NULL;
    if (op->i.index_flags&0xc) {
        if (op->i.index_flags&4) base_reg=High64Reg(idx_reg);
        else base_reg=Low64Reg(idx_reg);
    }
    idx_reg&=~base_reg;
    if (idx_reg==0x800 && base_reg==0) {
        base_reg=0x800;
        idx_reg=0;
    }
    EA(base_reg,idx_reg,op->i.scale,op->i.constant,mem_loc,0);
}

void LayModRM(name *op)
{
    name *base;
    switch (op->n.class) {
    case 1: /* N_MEMORY */
        CheckSize();
        EA(0,0,0,0,op,0);
        break;
    case 2: /* N_TEMP */
        CheckSize();
        base=DeAlias(op);
        if (base->t.location==0x7fffffff) Zoiks(30);
        if (BaseIsSP(base)) EA(0x800,0,0,TmpLoc(base,op),NULL,0);
        else EA(0x400,0,0,TmpLoc(base,op),NULL,0);
        break;
    case 4: LayIdxModRM(op); break;
    case 3: LayRMRegOp(op); break;
    }
}

void LayOpbyte(unsigned char op);
unsigned UseRepForm(unsigned size);
void OpndSizeIf(unsigned char if_32)
{
    if ((if_32 && (TargetModel&0x800)) || !(TargetModel&0x800))
        AddToTemp(0x66);
}
void TakeUpSlack(long size)
{
    while (size>=2) {
        if (TargetModel&0x800) AddByte(0x66);
        AddByte(0xa5);
        size-=2;
    }
    while (size>=1) {
        AddByte(0xa4);
        --size;
    }
}
void DoRepOp(instruction *ins)
{
    long size;
    unsigned char first;
    size=ins->result->n.size;
    first=1;
    if (ins->head.opcode==0x26 && !UseRepForm(size)) {
        while (size>=4) {
            if (first) {
                LayOpbyte(0xa5);
                OpndSizeIf(0);
                first=0;
            } else {
                if (!(TargetModel&0x800)) AddByte(0x66);
                AddByte(0xa5);
            }
            size-=4;
        }
        TakeUpSlack(size);
    } else {
        LayOpbyte(0xf3);
        if (ins->head.opcode==0x26) {
            if ((size&3)==0 || OptForSize<=50) {
                if (!(TargetModel&0x800)) AddByte(0x66);
                AddByte(0xa5);
                TakeUpSlack(size&3);
            } else AddByte(0xa4);
        } else if (ins->operands[1]->n.size&1) AddByte(0xa6);
        else {
            AddByte(0xa7);
            if (!(TargetModel&0x800)) AddByte(0x66);
        }
    }
}

void LayLeaRegOp(instruction *ins)
{
    name *left,*right;
    int shift,neg;
    instruction *next;
    long disp;
    left=ins->operands[0];
    right=ins->operands[1];
    neg=1;
    switch (ins->head.opcode) {
    case 3: /* OP_SUB */
        neg=-1;
        /* fall through */
    case 1: /* OP_ADD */
        if (right->n.class==0) {
            if (right->c.const_type==0)
                EA(0,left->r.reg,0,(long)((unsigned long)neg*right->c.int_value),NULL,1);
            else EA(0,left->r.reg,0,0,right,1);
        } else if (right->n.class==3) {
            next=ins->head.next;
            neg=1;
            disp=0;
            switch (next->head.opcode) {
            case 3:
                neg=-1;
                /* fall through */
            case 1:
                if (next->operands[0]!=ins->result || next->result!=ins->result) break;
                if (next->operands[1]->n.class!=0 || next->operands[1]->c.const_type!=0) break;
                disp=(long)((unsigned long)neg*next->operands[1]->c.int_value);
                DoNothing(next);
                break;
            }
            if (disp!=0) EA(left->r.reg,right->r.reg,0,disp,NULL,1);
            else EA(left->r.reg,right->r.reg,0,0,NULL,1);
        }
        break;
    case 5: /* OP_MUL */
        switch (right->c.int_value) {
        case 3: shift=1;break;
        case 5: shift=2;break;
        case 9: shift=3;break;
        default:
            /* Only selector-approved3/5/9 has an established shift;
             * do not reproduce an undefined incoming ESI as a value. */
            STUB_TRAP();
        }
        EA(left->r.reg,left->r.reg,shift,0,NULL,1);
        break;
    case 0x0d: /* OP_LSHIFT: older body has no later586 special case. */
        EA(0,left->r.reg,(int)right->c.int_value,0,NULL,1);
        break;
    }
}

void DoMAddr(name *operand)
{
    ILen+=4;
    if (operand->n.class==0) Zoiks(35);
    else DoSymRef(operand,operand->v.offset,0);
}

hw_reg_set CalcSegment(void *symbol,unsigned char memory_type);
void GenSeg(hw_reg_set reg);
unsigned char DoMDisp(name *operand,unsigned char alt)
{
    hw_reg_set reg;
    /* alt_encoding was already unused in this compiler generation. */
    alt=alt;
    reg=CalcSegment(operand->v.symbol,operand->m.memory_type);
    if (reg&0xc000f000U) GenSeg(reg);
    DoMAddr(operand);
    return 5;
}
