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
* Description:  WHEN YOU FIGURE OUT WHAT THIS FILE DOES, PLEASE
*               DESCRIBE IT HERE!
*
****************************************************************************/


#include "block.h"
#include "genopt.h"
#include "stubs.h"
#include <recon.h>

unsigned char InsIsCandidate(instruction *ins);
unsigned char FoldIntoIndex(instruction *ins);
void BuildIndex(void)
{
    block *blk;
    instruction *ins,*next;
    unsigned char change;
    blk=HeadBlock;
    while (blk!=NULL) {
        do {
            change=0;
            ins=blk->ins.hd.next;
            while (ins->head.opcode!=0x4b) {
                next=ins->head.next;
                if (InsIsCandidate(ins) && FoldIntoIndex(ins)) {
                    UpdateLive(blk->ins.hd.next,blk->ins.hd.prev);
                    change=1;
                }
                ins=next;
            }
        } while (change);
        blk=blk->next_block;
    }
}
unsigned char IsIndexReg(hw_reg_set reg,unsigned char type_class,unsigned char is_temp);
unsigned char InsIsCandidate(instruction *ins)
{
    name *reg;
    reg=ins->result;
    if (reg==NULL || reg->n.class!=3 || reg->n.size!=4) return 0;
    if (ins->operands[0]->n.class!=3) return 0;
    return IsIndexReg(reg->r.reg,U4,0)!=0;
}

typedef enum mod_info {
    MODIFIES_NOTHING=0,MODIFIES_BASE=1,MODIFIES_REG=2
} mod_info;
hw_reg_set Low64Reg(hw_reg_set reg);
hw_reg_set High64Reg(hw_reg_set reg);
unsigned char BadUse(name *reg,name *test,name **index,unsigned char *is_base)
{
    hw_reg_set idx_name,base_name;
    if (test->n.class==3 && (test->r.reg&reg->r.reg)) return 1;
    if (test->n.class==4) {
        idx_name=test->i.index->r.reg&~0xc000f000U; /* HW_SEGS */
        base_name=idx_name;
        if (test->i.index_flags&4) idx_name=Low64Reg(idx_name);
        else if (test->i.index_flags&8) idx_name=High64Reg(idx_name);
        base_name&=~idx_name;
        if (idx_name==reg->r.reg) *index=test;
        else if (base_name==reg->r.reg) {
            *is_base=1;
            *index=test;
        } else if (test->i.index->r.reg&reg->r.reg) return 1;
    }
    return 0;
}
unsigned char LivesAfterIns(instruction *ins,name *reg)
{
    hw_reg_set live_later;
    name *res;
    live_later=ins->head.next->head.live.regs&reg->r.reg;
    if (live_later==0) return 0;
    res=ins->result;
    if (res!=NULL && res->n.class==3 &&
        (res->r.reg&live_later)==live_later) return 0;
    return 1;
}
mod_info Modifies(instruction *ins,name *reg,hw_reg_set base,hw_reg_set other)
{
    mod_info modifies=MODIFIES_NOTHING;
    if (ins->zap->reg&base) modifies|=MODIFIES_BASE;
    if (ins->zap->reg&other) modifies|=MODIFIES_BASE;
    if (ins->zap->reg&reg->r.reg) modifies|=MODIFIES_REG;
    if (ins->result!=NULL && ins->result->n.class==3) {
        if (ins->result->r.reg&other) modifies|=MODIFIES_BASE;
        if (ins->result->r.reg&base) modifies|=MODIFIES_BASE;
        if (ins->result->r.reg&reg->r.reg) modifies|=MODIFIES_REG;
    }
    return modifies;
}
instruction *SIBPossibleIndex(instruction *ins,name *reg,name **index,
    unsigned char *is_base,hw_reg_set base_reg,hw_reg_set other_reg,
    unsigned char *dies,unsigned char *pmodifies)
{
    instruction *next=ins->head.next;
    int i;
    mod_info modifies;
    for (;;) {
        if (next->head.opcode==0x4b || !(next->head.live.regs&reg->r.reg)) return NULL;
        *index=NULL;
        *is_base=0;
        i=next->num_operands;
        while (--i>=0) {
            if (BadUse(reg,next->operands[i],index,is_base)) return NULL;
        }
        if (*index!=NULL) break;
        if (next->result!=NULL && BadUse(reg,next->result,index,is_base)) return NULL;
        if (*index!=NULL) break;
        if (Modifies(next,reg,base_reg,other_reg)!=MODIFIES_NOTHING) return NULL;
        next=next->head.next;
    }
    *dies=!LivesAfterIns(next,reg);
    modifies=Modifies(next,reg,base_reg,other_reg);
    if (!*dies && next->result!=NULL && next->result->n.class==3 &&
        modifies==MODIFIES_REG && next->result->r.reg==reg->r.reg) {
        *dies=1;
        modifies=MODIFIES_NOTHING;
    }
    *pmodifies=modifies!=MODIFIES_NOTHING;
    return next;
}
