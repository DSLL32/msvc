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
* Description:  i386 segmented-pointer type selection.
*
****************************************************************************/


#include "address.h"
#include "options.h"
#include "pragma.h"
#include "stubs.h"
#include <recon.h>

/* i86segs.c; old FE and back-handle code/data distinction. */
cg_type NamePtrType(name *op)
{
    void *symbol;
    if (op->n.class==1) { /* N_MEMORY */
        if (op->m.memory_type==CG_FE || op->m.memory_type==CG_BACK) {
            symbol=op->v.symbol;
            if (AskNameCode(symbol,op->m.memory_type)) {
                if (op->m.memory_type==CG_FE && (FEAttr(symbol)&1)) {
                    if (IsFarFunc(symbol)) return T_LONG_CODE_PTR;
                    return T_NEAR_CODE_PTR;
                }
                return T_HUGE_POINTER;
            }
            if (AskSegPrivate(AskSegID(symbol,op->m.memory_type)) ||
                (TargetModel&FLOATING_DS)) {
                if (op->m.memory_type==CG_FE && (FEAttr(symbol)&0x2000))
                    return T_POINTER;
                return T_HUGE_POINTER;
            }
            return T_POINTER;
        }
        if ((TargetModel&FLOATING_DS) || op->m.memory_type==CG_CLB)
            return T_HUGE_POINTER;
        return T_POINTER;
    }
    if (op->n.class==2) { /* N_TEMP */
        if (TargetModel&(0x200|FLOATING_DS)) return T_LONG_POINTER;
        return T_POINTER;
    }
    if (TargetModel&FLOATING_DS) return T_HUGE_POINTER;
    return T_POINTER;
}

hw_reg_set CalcSegment(void *symbol,unsigned char memory_type);
name *AllocAddrConst(name *value,long offset,unsigned char kind,unsigned char type_class);
name *AddrConst(name *value,int seg,unsigned char kind);
unsigned char LoadAToMove(instruction *ins)
{
    unsigned short seg;
    name *op;
    if (TargetModel&0x1000) return 0; /* INDEXED_GLOBALS */
    if (ins->head.opcode!=0x28 && ins->head.opcode!=0x27) return 0;
    op=ins->operands[0];
    if (op->n.class!=1) return 0;
    ins->head.opcode=0x26;
    seg=AskSegID(op->v.symbol,op->m.memory_type);
    if (AskSegNear(seg)) seg=AskBackSeg();
    if (ins->type_class==U4 || ins->type_class==I4)
        ins->operands[0]=AddrConst(op,seg,2); /* CONS_OFFSET */
    else ins->operands[0]=AddrConst(op,seg,1); /* CONS_ADDRESS */
    return 1;
}
name *AddrConst(name *value,int seg,unsigned char kind)
{
    hw_reg_set reg;
    switch (kind) {
    case 3: /* CONS_SEGMENT */
        reg=0;
        if (value!=NULL) reg=CalcSegment(value->v.symbol,value->m.memory_type);
        if (reg==0) return AllocAddrConst(value,seg,kind,U2);
        return AllocRegName(reg);
    case 2: /* CONS_OFFSET */
        return AllocAddrConst(value,seg,kind,U4);
    case 1: /* CONS_ADDRESS */
        return AllocAddrConst(value,seg,kind,CP);
    }
    Zoiks(78);
    return NULL;
}

name *GetSegment(name *opnd)
{
    hw_reg_set reg;
    int seg;
    reg=CalcSegment(opnd->v.symbol,opnd->m.memory_type);
    if (reg!=0) return AllocRegName(reg);
    seg=AskSegID(opnd->v.symbol,opnd->m.memory_type);
    if (AskSegNear(seg)) return AddrConst(NULL,AskBackSeg(),3);
    return AddrConst(opnd,seg,3);
}
name *NearSegment(void)
{
    if (!(TargetModel&FLOATING_DS)) return AllocRegName(HW_DS);
    if (TargetModel&0x200U) return AddrConst(NULL,AskBackSeg(),3);
    return AllocRegName(0x8000U); /* SS. */
}

unsigned char SegOver(name *opnd)
{
    int id;
    /* Original compiler lowered the released tail recursion to this loop. */
    for (;;) {
    if (opnd->n.class==4) {
        if (opnd->i.index->n.class!=0 && opnd->i.index->n.size!=4) return 1;
        if (opnd->i.base!=NULL) {
            opnd=opnd->i.base;
            continue;
        }
        if (TargetSwitches&0x400) return 1;
    } else if (opnd->n.class==1) {
        switch (opnd->m.memory_type) {
        case CG_FE: case CG_BACK: case CG_LBL:
            id=AskSegID(opnd->v.symbol,opnd->m.memory_type);
            if (AskSegPrivate(id)) return 1;
            if (id==AskCodeSeg() && !(TargetSwitches&0x20)) return 1;
            if ((TargetSwitches&0x200) && (TargetSwitches&0x400)) return 1;
            return 0;
        case CG_CLB: return 0;
        }
    }
    return 0;
    }
}

name *SegmentPart(name *value);
void Zoiks(int code);
name *SegName(name *value)
{
    name *index;
    for (;;) {
        if (value->n.class==3) return AllocRegName(value->r.reg&0xf000U);
        if (value->n.class==2) return AllocRegName(0x8000U); /* SS. */
        if (value->n.class==1) return GetSegment(value);
        if (value->n.class!=4) return value;
        index=value->i.index;
        /* This earlier body always follows an explicit base. The later
         * huge-index PT exception is absent from the1994 branch. */
        if (value->i.base!=NULL) { value=value->i.base;continue; }
        if (index->n.class==3) {
            if (index->r.reg==0x800U || index->r.reg==0x400U)
                return AllocRegName(0x8000U);
            Zoiks(22);
            return value;
        }
        if (index->n.name_class==CP || index->n.name_class==PT)
            return SegmentPart(index);
        return NearSegment();
    }
}

unsigned char IsFarFunc(void *symbol)
{
    unsigned *call_class;
    call_class=FEAuxInfo(FEAuxInfo(symbol,0),2); /* AUX_LOOKUP, CALL_CLASS. */
    return (*call_class&0x1000)!=0; /* FAR. */
}

/* i86segs.c. The1994 table/virtual-table/code-label classes select CS
 * directly, predating the later flat-model DS exception for those classes. */
hw_reg_set CalcSegment(void *symbol,unsigned char memory_type)
{
    unsigned attr;
    int id;
    hw_reg_set *reg;
    if (memory_type==3 || memory_type==4 || memory_type==5) return 0x4000U;
    if (memory_type==CG_FE) attr=FEAttr(symbol);
    else attr=0;
    id=AskSegID(symbol,memory_type);
    reg=FEAuxInfo((void *)(unsigned)id,50); /* PEGGED_REGISTER */
    if (reg!=NULL && *reg!=0) {
        if (*reg&0xc000f000U) return *reg;
        FEMessage(18,AskForLblSym(CurrProc->label));
    }
    if (memory_type==CG_FE && (attr&1)) { /* FE_PROC */
        if (TargetModel&0x20U) return 0x4000U; /* FLAT_MODEL */
        if (AskCodeSeg()==id) return 0x4000U;
    } else {
        if ((TargetModel&0x20U) && !(TargetModel&FLOATING_DS)) return HW_DS;
        if (AskCodeSeg()==id) {
            if (attr&0x200U) return 0; /* FE_COMMON */
            return 0x4000U;
        }
        if (AskSegPrivate(id)) return 0;
        if (!(TargetModel&FLOATING_DS)) return HW_DS;
        if ((CurrProc->state.attr&0x2002U)!=0x2000U && !(TargetModel&0x200U))
            return 0x8000U; /* SS; actual interrupt predicate, FLOATING_SS */
    }
    return 0;
}
