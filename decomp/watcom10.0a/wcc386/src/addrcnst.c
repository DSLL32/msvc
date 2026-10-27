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
* Description:  Promote local addresses into temporary address constants.
*
****************************************************************************/


#include "block.h"
#include "genopt.h"
#include "stubs.h"
#include <recon.h>

void MakeMovAddrConsts(void)
{
    block *blk;
    instruction *ins;
    name *op;
    for (blk=HeadBlock;blk!=NULL;blk=blk->next_block) {
        ins=blk->ins.hd.next;
        while (ins->head.opcode!=0x4b) {
            if (ins->head.opcode==0x28) {
                op=ins->operands[0];
                if (op->n.class==2) {
                    op->v.usage|=2;
                    op=AllocAddrConst(op,0,4,ins->type_class);
                    ins->head.opcode=0x26;
                    ins->operands[0]=op;
                }
            }
            ins=ins->head.next;
        }
    }
}
void KillMovAddrConsts(void)
{
    block *blk;
    instruction *ins,*new_ins;
    name *op,*new_op;
    unsigned char class;
    int i;
    blk=HeadBlock;
    while (blk!=NULL) {
        ins=blk->ins.hd.next;
        while (ins->head.opcode!=0x4b) {
            i=ins->num_operands;
            while (--i>=0) {
                op=ins->operands[i];
                if (op->n.class==0 && op->c.const_type==4) {
                    class=(ins->head.opcode==0x24 || ins->head.opcode==0x25)
                        ?ins->base_type_class:ins->type_class;
                    new_op=AllocTemp(class);
                    new_ins=MakeUnary(0x28,op->c.value,new_op,class);
                    ins->operands[i]=new_op;
                    PrefixIns(ins,new_ins);
                }
            }
            ins=ins->head.next;
        }
        blk=blk->next_block;
    }
}
