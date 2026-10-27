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

unsigned char UnChangeable(instruction *ins);
unsigned char ThrashDown(instruction *ins);
unsigned char ThrashUp(instruction *ins);
unsigned char RegThrash(block *blk)
{
    instruction *ins,*next;
    for (ins=blk->ins.hd.next;ins->head.opcode!=0x4b;ins=next) {
        next=ins->head.next;
        if (ins->head.opcode==0x26 && !UnChangeable(ins) &&
            ins->operands[0]->n.class==3 &&
            !(ins->head.next->head.live.regs&ins->operands[0]->r.reg) &&
            ins->result->n.class==3) {
            if (ThrashDown(ins) || ThrashUp(ins)) {
                UpdateLive(blk->ins.hd.next,blk->ins.hd.prev);
                return 1;
            }
        }
    }
    return 0;
}
/* Released private Modifies is distinct from the public optimizer helper. */
unsigned char ThrashModifies(name *from,name *to,name *op)
{
    if (op==NULL || op->n.class!=3) return 0;
    if (op->r.reg&to->r.reg) return 1;
    if (op->r.reg&from->r.reg) return 1;
    return 0;
}
unsigned char UsedIn(name *op,name *reg)
{
    if (op->n.class==3) return (op->r.reg&reg->r.reg)!=0;
    if (op->n.class==4 && op->i.index->n.class==3 &&
        (op->i.index->r.reg&reg->r.reg)) return 1;
    return 0;
}
unsigned char CantChange(instruction **pins,name *from,name *to) { STUB_TRAP();return 0; }
unsigned char ChangeIns(instruction *ins,name *to,name **op,int flags);
unsigned char ThrashDown(instruction *ins)
{
    name *from,*to;
    instruction *other;
    int i;
    from=ins->operands[0];
    to=ins->result;
    other=ins->head.prev;
    for (;;) {
        if (other->head.opcode==0x4b) return 0;
        if (other->head.opcode==0x36 || other->head.opcode==0x29) return 0;
        if (other->head.next->head.live.regs&to->r.reg) return 0;
        if ((other->head.next->head.live.regs&from->r.reg)!=from->r.reg) return 0;
        if (other->zap->reg&from->r.reg) return 0;
        if (other->zap->reg&to->r.reg) return 0;
        if (other->result==from) break;
        if (ThrashModifies(from,to,other->result)) return 0;
        i=other->num_operands;
        while (--i>=0) if (UsedIn(other->operands[i],from)) return 0;
        if (other->result!=NULL && UsedIn(other->result,from)) return 0;
        other=other->head.prev;
    }
    if (other->head.live.regs&to->r.reg) {
        if (!ChangeIns(other,to,&other->result,2)) return 0;
        FreeIns(ins);
    } else {
        if (CantChange(&other,from,to)) return 0;
        ins->head.prev->head.next=ins->head.next;
        ins->head.next->head.prev=ins->head.prev;
        PrefixIns(other,ins);
    }
    return 1;
}
unsigned char ThrashUp(instruction *ins)
{
    instruction *other,*last;
    name *from,*to;
    int i;
    from=ins->operands[0];
    to=ins->result;
    other=ins->head.next;
    for (;;) {
        if (other->head.opcode==0x4b) return 0;
        if (other->head.opcode==0x36 || other->head.opcode==0x29) return 0;
        if (other->zap->reg&from->r.reg) return 0;
        if (other->zap->reg&to->r.reg) return 0;
        if (other->result==to) break;
        if (ThrashModifies(from,to,other->result)) return 0;
        i=other->num_operands;
        while (--i>=0) if (UsedIn(other->operands[i],to)) return 0;
        if (other->result!=NULL && UsedIn(other->result,to)) return 0;
        other=other->head.next;
    }
    last=other->head.next;
    for (;;) {
        if (last->head.opcode==0x4b) return 0;
        if (last->zap->reg&from->r.reg) return 0;
        if (last->zap->reg&to->r.reg) return 0;
        if (last->head.opcode==0x26 && last->operands[0]==to && last->result==from)
            break;
        /* Retain the actual old test of other->result here. */
        if (ThrashModifies(from,to,other->result)) return 0;
        last=last->head.next;
    }
    if (CantChange(&other,to,from)) return 0;
    FreeIns(last);
    ins->head.prev->head.next=ins->head.next;
    ins->head.next->head.prev=ins->head.prev;
    SuffixIns(other,ins);
    return 1;
}
