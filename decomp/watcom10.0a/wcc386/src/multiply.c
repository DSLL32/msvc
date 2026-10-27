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
* Description:  Replace suitable integer multiplications with shifts and additions.
*
****************************************************************************/


#include "block.h"
#include "genopt.h"
#include "address.h"
#include "stubs.h"
#include <recon.h>

instruction *CheckMul(instruction *ins);
void MulToShiftAdd(void)
{
    block *blk;
    instruction *ins,*next;
    name *op;
    for (blk=HeadBlock;blk!=NULL;blk=blk->next_block) {
        for (ins=blk->ins.hd.next;ins->head.opcode!=0x4b;ins=ins->head.next) {
            if (ins->head.opcode!=5) continue; /* OP_MUL */
            if (ins->operands[0]->n.class==0) {
                op=ins->operands[0];
                if (op->c.const_type==0) {
                    ins->operands[0]=ins->operands[1];
                    ins->operands[1]=op;
                }
            }
            op=ins->operands[1];
            switch (op->n.class) {
            case 2:
                if (!(op->t.temp_flags&0x100)) continue; /* CONST_TEMP */
                ins->operands[1]=op->v.symbol;
                /* fall through: the original keeps op for restoration. */
            case 0:
                if (ins->type_class!=U4 && ins->type_class!=I4) continue;
                if (op->c.const_type!=0) continue;
                next=CheckMul(ins);
                if (next==ins) ins->operands[1]=op;
                ins=next;
            }
        }
    }
}
enum { DO_SHL,DO_SUB,DO_ADD,DO_XFR,MAXOPS=20 };
typedef struct mul_op { unsigned char op,cnt; } mul_op;
typedef char old_mul_op_size[(sizeof(mul_op)==2)?1:-1];
/* Actual40-byte BSS operation sequence at806e4, written by Factor. */
static mul_op Ops[MAXOPS];
int Factor(unsigned long value,int *cost);
int MulCost(unsigned long value);
instruction *CheckMul(instruction *ins)
{
    long rhs;
    int i,cost;
    unsigned char negative,type_class;
    instruction *new_ins;
    name *orig,*temp;
    rhs=ins->operands[1]->c.int_value;
    negative=0;
    type_class=ins->type_class;
    if (type_class==I4 && rhs<0) {
        rhs=(long)(0UL-(unsigned long)rhs);
        negative=1;
    }
    i=Factor(rhs,&cost);
    if (MulCost(rhs)<=cost || i==MAXOPS) return ins;
    orig=AllocTemp(type_class);
    new_ins=MakeMove(ins->operands[0],orig,type_class);
    PrefixIns(ins,new_ins);
    temp=AllocTemp(type_class);
    new_ins=MakeMove(orig,temp,type_class);
    PrefixIns(ins,new_ins);
    for (;i<MAXOPS;++i) {
        switch (Ops[i].op) {
        case DO_XFR: new_ins=MakeUnary(0x26,temp,orig,type_class);break;
        case DO_ADD: new_ins=MakeBinary(1,temp,orig,temp,type_class);break;
        case DO_SUB: new_ins=MakeBinary(3,temp,orig,temp,type_class);break;
        case DO_SHL: new_ins=MakeBinary(0x0d,temp,AllocIntConst(Ops[i].cnt),temp,type_class);break;
        }
        PrefixIns(ins,new_ins);
    }
    if (negative) new_ins=MakeUnary(0x11,temp,ins->result,type_class);
    else new_ins=MakeMove(temp,ins->result,type_class);
    PrefixIns(ins,new_ins);
    FreeIns(ins);
    return new_ins;
}
unsigned CountBits(unsigned long value);
int ShiftCost(void);
int AddCost(void);
int Factor(unsigned long value,int *cost)
{
    int shlcnt,i;
    unsigned num_ops;
    unsigned long test,pow2;
    *cost=0;
    i=MAXOPS;
    if (value==0 || value==0xffffffffUL) return i;
    num_ops=0;
    test=value>>1;
    do {
        pow2=1;
        shlcnt=0;
        while (test!=0) {
            pow2<<=1;
            ++shlcnt;
            test>>=1;
        }
        while (pow2!=1) {
            if (value%(pow2-1)==0) test=pow2-1;
            else if (value%(pow2+1)==0) test=pow2+1;
            else test=0;
            if (test!=0 && CountBits(value/test)+2<=CountBits(value)) {
                value/=test;
                if (--i<0) return MAXOPS;
                Ops[i].op=test==pow2-1?DO_SUB:DO_ADD;
                if (--i<0) return MAXOPS;
                Ops[i].op=DO_SHL;
                Ops[i].cnt=shlcnt;
                if (--i<0) return MAXOPS;
                Ops[i].op=DO_XFR;
                break;
            }
            pow2>>=1;
            --shlcnt;
        }
    } while (pow2!=1);
    shlcnt=0;
    for (;;) {
        while (!(value&1)) { ++shlcnt;value>>=1; }
        if (shlcnt!=0) {
            if (--i<0) return MAXOPS;
            Ops[i].op=DO_SHL;
            Ops[i].cnt=shlcnt;
            *cost+=ShiftCost();
            ++num_ops;
        }
        if (value==1) break;
        if (--i<0) return MAXOPS;
        shlcnt=0;
        if ((value&3)==1) {
            *cost+=AddCost();
            Ops[i].op=DO_ADD;
            value>>=1;
            shlcnt=1;
        } else {
            /* Original SubCost and AddCost are the same physical
             * entry6c574; retain the single representative vertex. */
            *cost+=AddCost();
            Ops[i].op=DO_SUB;
            ++value;
        }
        ++num_ops;
    }
    *cost+=num_ops;
    return i;
}
unsigned CountBits(unsigned long value)
{
    unsigned count;
    count=0;
    /* Actual61a48 counts signed-digit operations: a low run of ones
     * can become a subtract followed by a shift. This predates the
     * released utils.c population-count implementation. */
    while (value!=0) {
        if (!(value&1)) { value>>=1;continue; }
        if (value&2) ++value;
        else value>>=1;
        ++count;
    }
    return count;
}



