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

static hw_reg_set *RoverByte,*RoverWord,*RoverDouble;
unsigned char LoadStoreIns(instruction *ins);
unsigned char FixMem16Moves(void);
unsigned char LdStAlloc(void)
{
    block *blk;
    instruction *ins,*next;
    unsigned char changed;
    RoverByte=RoverWord=RoverDouble=NULL;
    if (OptForSize>50 || (CGProcessorVersion&15)<4) return 0;
    changed=0;
    for (blk=HeadBlock;blk!=NULL;blk=blk->next_block) {
        for (ins=blk->ins.hd.next;ins->head.opcode!=0x4b;ins=next) {
            next=ins->head.next;
            if (LoadStoreIns(ins)) {
                UpdateLive(blk->ins.hd.next,blk->ins.hd.prev);
                changed=1;
            }
        }
    }
    changed|=FixMem16Moves();
    return changed;
}
unsigned char LoadStoreIns(instruction *ins) { STUB_TRAP();return 0; }
unsigned char FixMem16Moves(void) { STUB_TRAP();return 0; }

void CompressMem16Moves(void) { STUB_TRAP(); }
void CompressIns(instruction *ins) { STUB_TRAP(); }
void DeadInstructions(void);
void LdStCompress(void)
{
    block *blk;
    instruction *ins;
    if (OptForSize>50 || (CGProcessorVersion&15U)<4U) return;
    CompressMem16Moves();
    for (blk=HeadBlock;blk!=NULL;blk=blk->next_block)
        for (ins=blk->ins.hd.next;ins->head.opcode!=0x4b;ins=ins->head.next)
            CompressIns(ins);
    DeadInstructions();
}
