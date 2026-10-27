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
* Description:  Eliminate tail recursion and clear temporary parameter links.
*
****************************************************************************/


#include "block.h"
#include "genopt.h"
#include "stubs.h"
#include <recon.h>

unsigned char TailRecursion(void)
{
    block *blk;
    instruction *ins,*next;
    unsigned char changed;
    changed=0;
    if (!(Model&0x80000000U) && !ScaryConditions() && !BlockByBlock) {
        for (blk=HeadBlock;blk!=NULL;blk=blk->next_block) {
            ins=blk->ins.hd.next;
            while (ins->head.opcode!=0x4b) {
                if (ins->head.opcode==0x36 && OkayToTransCall(blk,ins)) {
                    DoTrans(blk,ins);
                    changed=1;
                    break;
                }
                ins=ins->head.next;
            }
        }
    }
    for (blk=HeadBlock;blk!=NULL;blk=blk->next_block) {
        ins=blk->ins.hd.next;
        while (ins->head.opcode!=0x4b) {
            ins->u.parm_list=NULL;
            ins=ins->head.next;
        }
    }
    for (ins=(instruction *)CurrProc->parms_list;ins!=NULL;ins=next) {
        next=ins->u.parm_list;
        ins->u.parm_list=NULL;
    }
    return changed;
}

unsigned char ScaryConditions(void)
{
    block *blk;
    instruction *ins;
    int i;
    for (blk=HeadBlock;blk!=NULL;blk=blk->next_block) {
        ins=blk->ins.hd.next;
        while (ins->head.opcode!=0x4b) {
            for (i=0;i<ins->num_operands;++i)
                if (ScaryOperand(ins->operands[i])) return 1;
            if (ScaryOperand(ins->result)) return 1;
            ins=ins->head.next;
        }
    }
    return 0;
}
unsigned char ScaryOperand(name *var)
{
    if (var!=NULL && var->n.class==2) return var->v.usage&8; /* USE_ADDRESS */
    return 0;
}
unsigned char OkayToTransCall(block *blk,instruction *call_ins) { STUB_TRAP(); return 0; }
void DoTrans(block *blk,instruction *call_ins) { STUB_TRAP(); }
