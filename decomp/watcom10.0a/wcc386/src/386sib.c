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
#include "memory.h"
#include "cgmemory.h"
#include "stubs.h"
#include <recon.h>

/* The older record is24B, preceding the later constant-add offset member. */
#pragma pack(4)
typedef struct sib_info {
    struct sib_info *next;
    name *reg,*index;
    int scale;
    unsigned char flags;
    instruction *ins;
} sib_info;
#pragma pack()
typedef char old_sib_info_size[(sizeof(sib_info)==24)?1:-1];
instruction *SIBPossibleIndex(instruction *ins,name *reg,name **index,
    unsigned char *is_base,hw_reg_set base_reg,hw_reg_set other_reg,
    unsigned char *dies,unsigned char *modifies);
hw_reg_set Low64Reg(hw_reg_set reg);
void ReplaceOperand(instruction *ins,name *old_op,name *new_op);
unsigned char FoldIntoIndex(instruction *ins)
{
    name *cons,*new_x;
    unsigned char is_base,dies,modifies;
    hw_reg_set base_reg,other_reg,tmp;
    sib_info sib,*sib_head,*curr_sib;
    instruction *next;

    if (ins->head.opcode==13) {
        base_reg=0;
        cons=ins->operands[1];
        if (cons->n.class!=0 || cons->c.const_type!=0 || cons->c.int_value>3)
            return 0;
    } else if (ins->head.opcode==1) {
        cons=ins->operands[1];
        if (cons->n.class!=3 || cons->n.size!=4) return 0;
        base_reg=cons->r.reg;
        if (cons==ins->operands[0]) {
            cons=AllocIntConst(1);
            base_reg=0;
        }
    } else return 0;
    other_reg=ins->operands[0]->r.reg;
    sib_head=NULL;
    dies=0;
    next=ins;
    do {
        curr_sib=NULL;
        next=SIBPossibleIndex(next,ins->result,&sib.index,&is_base,
                              base_reg,other_reg,&dies,&modifies);
        if (next==NULL) break;
        sib.ins=next;
        if (base_reg!=0 && (TargetModel&0x1000U) && sib.index->i.base!=NULL &&
            sib.index->i.base->n.class==1) break;
        if (base_reg==0) {
            sib.flags=sib.index->i.index_flags;
            if (is_base) {
                if (sib.index->i.scale!=0) break;
                sib.flags^=0x0c;
            }
            sib.scale=cons->c.int_value+sib.index->i.scale;
            if (sib.scale>3) break;
            if (ins->operands[0]==ins->result) sib.reg=sib.index->i.index;
            else {
                /* The later1996 tmp/other overlap rejection is absent. */
                tmp=(sib.index->i.index->r.reg&~ins->result->r.reg)|other_reg;
                sib.reg=AllocRegName(tmp);
                tmp=Low64Reg(tmp);
                if (sib.flags&8) {
                    if (tmp==other_reg) sib.flags^=0x0c;
                } else if (sib.flags&4) {
                    if (tmp!=other_reg) sib.flags^=0x0c;
                }
            }
        } else {
            sib.flags=sib.index->i.index_flags&~0x0c;
            if (sib.index->i.index_flags&0x0c) break;
            if (sib.index->i.base!=NULL && sib.index->i.base->n.class==2) break;
            sib.scale=sib.index->i.scale;
            if (sib.scale!=0) break;
            if (ins->operands[0]==ins->result)
                tmp=base_reg|sib.index->i.index->r.reg;
            else tmp=(sib.index->i.index->r.reg&0xc000f000U)|other_reg|base_reg;
            sib.reg=AllocRegName(tmp);
            sib.flags|=4;
        }
        curr_sib=CGAlloc(sizeof(sib_info));
        Copy((char *)&sib,(char *)curr_sib,sizeof(sib_info));
        curr_sib->next=sib_head;
        sib_head=curr_sib;
    } while (!dies && !modifies);
    if (dies && curr_sib!=NULL) {
        for (curr_sib=sib_head;curr_sib!=NULL;curr_sib=curr_sib->next) {
            new_x=ScaleIndex(curr_sib->reg,curr_sib->index->i.base,
                       curr_sib->index->i.constant,curr_sib->index->n.name_class,
                       curr_sib->index->n.size,curr_sib->scale,curr_sib->flags);
            ReplaceOperand(curr_sib->ins,curr_sib->index,new_x);
        }
        FreeIns(ins);
    } else dies=0;
    while (sib_head!=NULL) {
        curr_sib=sib_head;
        sib_head=sib_head->next;
        CGFreeSize(curr_sib,sizeof(sib_info));
    }
    return dies;
}
instruction *SIBPossibleIndex(instruction *ins,name *reg,name **index,
    unsigned char *is_base,hw_reg_set base_reg,hw_reg_set other_reg,
    unsigned char *dies,unsigned char *modifies);
hw_reg_set Low64Reg(hw_reg_set reg);
void ReplaceOperand(instruction *ins,name *old_op,name *new_op);
