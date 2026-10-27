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
#include "stubs.h"
#include <recon.h>

unsigned char NotByteMove(instruction *ins)
{
    return ins->head.opcode==0x26 && ins->type_class!=0 && ins->type_class!=1;
}
unsigned char IsLESDS(instruction *ins,instruction *next)
{
    if (ins->u.gen_table->generate!=0x13 && ins->u.gen_table->generate!=0x1f) return 0; /* G_RM1/G_MOVAM */
    if (ins->type_class!=4 && ins->type_class!=5) return 0; /* WD/SW */
    return next->u.gen_table->generate==0x18; /* G_SM1 */
}
void CheckLDSES(instruction *next,instruction *ins,unsigned char first) { STUB_TRAP(); }

/* This body predates the later adjacent-memory-move combining passes. */
void OptSegs(void)
{
    block *blk;
    instruction *ins,*next;
    for (blk=HeadBlock;blk!=NULL;blk=blk->next_block) {
        ins=blk->ins.hd.next;
        while (ins->head.opcode!=0x4b) {
            next=ins->head.next;
            if (NotByteMove(ins) && NotByteMove(next)) {
                if (IsLESDS(ins,next)) CheckLDSES(next,ins,0);
                else if (IsLESDS(next,ins)) CheckLDSES(ins,next,1);
            }
            ins=next;
        }
    }
}
