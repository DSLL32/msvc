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
* Description:  Apply local instruction simplifications to selected blocks.
*
****************************************************************************/


#include "block.h"
#include "address.h"
#include "genopt.h"
#include "stubs.h"
#include <recon.h>

unsigned char PeepOpt(block *head,block *(*next)(block *,block *),block *parm,unsigned char in_regalloc)
{
    block *blk;
    unsigned char change;
    change=0;
    blk=head;
    do {
        change|=PeepOptBlock(blk,in_regalloc);
        blk=next(blk,parm);
    } while (blk!=NULL);
    return change;
}
static unsigned char AfterRegAlloc;
unsigned char PeepOptBlock(block *blk,unsigned char after_reg_alloc)
{
    instruction *ins,*next;
    unsigned char change;
    change=0;
    AfterRegAlloc=after_reg_alloc;
    for (ins=blk->ins.hd.next;ins->head.opcode!=0x4b;ins=next) {
        next=ins->head.next;
        /* VolatileIns filtering was added after this 1994 body. */
        if (DoArithmeticOps(ins,&change,&next)) continue;
        if (DoConversionOps(ins,&change,&next)) continue;
        if (DoMemWrites(ins,&change,&next)) continue;
    }
    return change;
}
unsigned char DoArithmeticOps(instruction *ins,unsigned char *change,instruction **next)
{
    instruction *result;
    /* The old type universe has no later I8/U8 bypass. */
    if (!OpConst(ins)) return 0;
    switch (ins->head.opcode) {
    case 1: result=AddOpt(ins,change); break;
    case 3: result=SubOpt(ins,change); break;
    case 5: result=MulOpt(ins,change); break;
    case 9: result=AndOpt(ins,change); break;
    case 10: result=OrOpt(ins,change); break;
    case 13: result=LShiftOpt(ins,change); break;
    case 12: result=RShiftOpt(ins,change); break;
    default: return 0;
    }
    *next=result;
    return 1;
}
unsigned char OpConst(instruction *ins)
{
    name *op;
    if (ins->num_operands!=2) return 0;
    op=ins->operands[1];
    if (op->n.class!=0 || op->c.const_type!=0) return 0;
    if (ins->type_class>PT) return 0;
    return 1;
}

instruction *SubOpt(instruction *ins,unsigned char *change);

typedef unsigned char peep_operation(instruction *,instruction *);
unsigned char DoAnd(instruction *first,instruction *second) { STUB_TRAP();return 0; }
unsigned char DoOr(instruction *first,instruction *second) { STUB_TRAP();return 0; }
unsigned char AndOr(instruction *first,instruction *second) { STUB_TRAP();return 0; }
unsigned char AndOrAnd(instruction *first,instruction *second) { STUB_TRAP();return 0; }
unsigned char SameOpWithConst(instruction *ins,instruction *next)
{
    name *op;
    if (ins->result!=next->operands[0]) return 0;
    op=next->operands[1];
    return op->n.class==0 && op->c.const_type==0;
}
unsigned char InsOrderDependant(instruction *first,instruction *second);
instruction *FindInsPair(instruction *ins,instruction **stopper,unsigned char *change,
    unsigned char op,unsigned char op2,peep_operation *operation)
{
    instruction *next;
    if (stopper!=NULL) *stopper=NULL;
    next=ins->head.next;
    while (next->head.opcode!=0x4b) {
        if (next->head.opcode==op && SameOpWithConst(ins,next)) {
            if (operation(ins,next)) {
                *change=1;
                return ins;
            }
        } else if (InsOrderDependant(ins,next)) {
            if (stopper!=NULL && next->head.opcode==op2 && SameOpWithConst(ins,next))
                *stopper=next;
            return ins->head.next;
        }
        next=next->head.next;
    }
    return ins->head.next;
}
instruction *FindInsTriple(instruction *ins,unsigned char *change,
    unsigned char op1,unsigned char op2,peep_operation *operation1,
    peep_operation *operation2,peep_operation *operation12,peep_operation *operation121)
{
    unsigned char changed;
    instruction *second,*third,*next;
    changed=0;
    next=FindInsPair(ins,&second,&changed,op1,op2,operation1);
    if (changed) {
        *change=1;
        return next;
    } else if (second!=NULL) {
        FindInsPair(second,&third,&changed,op2,op1,operation2);
        if (changed) {
            *change=1;
            return ins;
        }
        if (third==NULL) {
            if (operation12(ins,second)) {
                *change=1;
                return ins;
            }
            return next;
        }
        if (operation121(ins,second)) {
            *change=1;
            return ins;
        }
        return ins->head.next;
    }
    return next;
}
instruction *AndOpt(instruction *ins,unsigned char *change)
{
    return FindInsTriple(ins,change,9,10,DoAnd,DoOr,AndOr,AndOrAnd);
}
instruction *OrOpt(instruction *ins,unsigned char *change) { STUB_TRAP(); return NULL; }
unsigned char DoAdd(instruction *first,instruction *second) { STUB_TRAP();return 0; }
unsigned char DoRShift(instruction *first,instruction *second) { STUB_TRAP();return 0; }
/* Physical6215a folds LRShift, RLShift and Nop3, each false/no effects. */
unsigned char Nop3(instruction *first,instruction *second) { return 0; }
instruction *LShiftOpt(instruction *ins,unsigned char *change)
{
    /* DoLShift is folded into the real DoAdd entry; both add constants. */
    return FindInsTriple(ins,change,13,12,DoAdd,DoRShift,Nop3,Nop3);
}
instruction *RShiftOpt(instruction *ins,unsigned char *change)
{
    return FindInsTriple(ins,change,12,13,DoRShift,DoAdd,Nop3,Nop3);
}

unsigned char DoConversionOps(instruction *ins,unsigned char *change,instruction **n)
{
    instruction *next;
    if (ins->head.opcode!=0x24 && ins->head.opcode!=0x21 && ins->head.opcode!=0x22) return 0;
    next=ins->head.next;
    if (ReDefinedBy(ins,ins->operands[0])) return 0;
    while (next->head.opcode!=0x4b) {
        if (ReDefinedBy(next,ins->result)) return 0;
        if (ReDefinedBy(next,ins->operands[0])) return 0;
        if (ins->head.opcode==0x24 && next->head.opcode==0x24) {
            if (ins->result==next->operands[0] && ins->type_class==next->base_type_class) {
                if (next->type_class==PT || next->type_class==CP ||
                    next->base_type_class==PT || next->base_type_class==CP) return 0;
                if (ins->type_class==PT || ins->type_class==CP) return 0;
                if (ins->type_class<ins->base_type_class) return 0;
                next->base_type_class=ins->base_type_class;
                next->operands[0]=ins->operands[0];
                if (next->operands[0]->n.class==2) next->operands[0]->t.temp_flags&=~0x10U;
                /* The old conversion-chain case does not clear next->table. */
                *change=1;
                return 1;
            }
        }
        if ((ins->head.opcode==0x21 && next->head.opcode==0x22) ||
            (ins->head.opcode==0x22 && next->head.opcode==0x21)) {
            if (ins->result==next->operands[0]) {
                next->operands[0]=ins->operands[0];
                next->head.opcode=0x26;
                next->table=NULL;
                *change=1;
                return 1;
            }
        }
        next=next->head.next;
    }
    return 0;
}
unsigned char DoMemWrites(instruction *ins,unsigned char *change,instruction **n)
{
    instruction *next;
    if (ins->result!=NULL) {
        /* N_MEMORY was added to this scan after the 1994 version. */
        if (ins->result->n.class==4 ||
            (ins->result->n.class==2 && (ins->result->v.usage&8))) {
            if (SideEffect(ins)) return 0;
            for (next=ins->head.next;next->head.opcode!=0x4b;next=next->head.next) {
                if (ReferencedBy(next,ins->result)) break;
                if (ReDefinedBy(next,ins->result)) {
                    if (next->result==ins->result) {
                        *change=1;
                        FreeIns(ins);
                        return 1;
                    }
                    break;
                }
            }
        }
    }
    return 0;
}
unsigned char SameThing(name *left,name *right);
unsigned char ReferencedBy(instruction *ins,name *op)
{
    int i;
    name *curr;
    if ((ins->head.opcode==0x36 || ins->head.opcode==0x29) &&
        VisibleToCall(ins,op,0)) return 1;
    for (i=0;i<ins->num_operands;++i) {
        curr=ins->operands[i];
        if (curr->n.class==4 || curr->n.class==1) return 1;
        if (SameThing(curr,op)) return 1;
    }
    return 0;
}

instruction *AddOpt(instruction *ins,unsigned char *change)
{
    return FindInsPair(ins,NULL,change,1,0,DoAdd);
}

unsigned char DoMul(instruction *first,instruction *second) { STUB_TRAP();return 0; }
instruction *MulOpt(instruction *ins,unsigned char *change)
{
    return FindInsPair(ins,NULL,change,5,0,DoMul);
}

instruction *SubOpt(instruction *ins,unsigned char *change)
{
    return FindInsPair(ins,NULL,change,3,0,DoAdd);
}
