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

unsigned char GenId(block *blk,block *next)
{
    return blk->gen_id>next->gen_id;
}
void BlocksSortedBy(unsigned char (*bigger)(block *,block *))
{
    block *blk,*next;
    unsigned char change;
    for (;;) {
        blk=HeadBlock->next_block;
        if (blk==NULL) break;
        change=0;
        for (;;) {
            next=blk->next_block;
            if (next==NULL) break;
            if (bigger(blk,next)) {
                blk->prev_block->next_block=next;
                if (next->next_block!=NULL) next->next_block->prev_block=blk;
                blk->next_block=next->next_block;
                if (blk->prev_block==HeadBlock) HeadBlock->next_block=next;
                next->prev_block=blk->prev_block;
                blk->prev_block=next;
                next->next_block=blk;
                if (BlockList==next) BlockList=blk;
                change=1;
            }
            blk=next;
        }
        if (!change) break;
    }
}

/* In 1994 this entry only loads GenId, then falls into BlocksSortedBy. */
void SortBlocks(void)
{
    BlocksSortedBy(GenId);
}

unsigned DepthAlign(unsigned depth);
void CodeLabel(struct code_lbl *label,unsigned align);
void InitZeroPage(void);
void FiniZeroPage(void);
void CodeLineNum(unsigned short line,unsigned char label);
unsigned short DumpLineNum(unsigned short line,unsigned short last,unsigned char label)
{
    if ((Model&0x40000000) && line>0 && line!=last) {
        last=line;
        CodeLineNum(line,label);
    }
    return last;
}
void InitStackDepth(block *blk);
void GenObjCode(instruction *ins);
void GenJumpLabel(struct code_lbl *label);
void GenCallLabel(struct code_lbl *label) { STUB_TRAP();  }
void GenLabelReturn(void) { STUB_TRAP();  }
void TellReachedLabel(struct code_lbl *label);
void TellCondemnedLabel(struct code_lbl *label);
void FreeBlock(void);

/* Earlier object.c: no FE_NAKED/profiling callbacks or BIG_JUMP extension. */
void GenObject(void)
{
    block *blk,*next_blk;
    instruction *ins;
    unsigned short last_line,old;
    int targets,i;
    struct code_lbl *lbl;
    unsigned align;
    old=SetOP(AskCodeSeg());
    InitZeroPage();
    last_line=0;
    blk=HeadBlock;
    while (blk!=NULL) {
        if (blk->label!=CurrProc->label && blk->label!=NULL) {
            last_line=DumpLineNum(blk->ins.hd.line_num,last_line,1);
            if ((blk->class&0x10) && blk->iterations>=10) align=DepthAlign((unsigned)-2);
            else align=DepthAlign(blk->depth);
            CodeLabel(blk->label,align);
            if ((blk->edge[0].flags&1) && BlocksUnTrimmed) GenKillLabel(blk->label);
        }
        InitStackDepth(blk);
        ins=blk->ins.hd.next;
        next_blk=blk->next_block;
        while (ins->head.opcode!=0x4b) {
            if (ins->head.opcode==0 && (ins->flags.nop_flags&4))
                last_line=DumpLineNum(ins->head.line_num,last_line,1);
            else last_line=DumpLineNum(ins->head.line_num,last_line,0);
            GenObjCode(ins);
            ins=ins->head.next;
        }
        if (blk->class&2) { /* JUMP */
            if (BlockByBlock || next_blk==NULL ||
                (struct code_lbl *)blk->edge[0].destination!=next_blk->label)
                GenJumpLabel((struct code_lbl *)blk->edge[0].destination);
        } else if (blk->class&1) { /* RETURN */
            FiniZeroPage();
            GenEpilog();
        } else if (blk->class&0x40) { /* CALL_LABEL */
            GenCallLabel((struct code_lbl *)blk->edge[0].destination);
            if (BlockByBlock) {
                if (next_blk==NULL) GenJumpLabel(blk->v.next->label);
                else GenJumpLabel(next_blk->label);
            }
        } else if (blk->class&0x80) GenLabelReturn();
        if (!(blk->class&0x80)) {
            targets=blk->targets;
            while (--targets>=0) {
                lbl=(struct code_lbl *)blk->edge[targets].destination;
                TellReachedLabel(lbl);
                if ((blk->edge[targets].flags&2) && BlocksUnTrimmed) {
                    GenKillLabel(lbl);
                    i=targets;
                    while (--i>=0) {
                        if ((struct code_lbl *)blk->edge[i].destination==lbl)
                            blk->edge[i].flags&=~2;
                    }
                }
            }
        }
        if (!BlocksUnTrimmed && blk->label!=CurrProc->label && blk->label!=NULL)
            TellCondemnedLabel(blk->label);
        CurrBlock=blk;
        FreeBlock();
        blk=next_blk;
    }
    HeadBlock=blk;
    BlockList=blk;
    SetOP(old);
}
