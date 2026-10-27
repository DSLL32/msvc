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
#include "cgmemory.h"
#include "address.h"
#include "stubs.h"
#include <recon.h>

int CountIns(struct block *blk)
{
    instruction *ins;
    unsigned count;
    count=0;
    ins=blk->ins.hd.next;
    while (ins->head.opcode!=0x4b) {
        ++count;
        ins=ins->head.next;
    }
    return (int)count;
}

void BlockTrim(void)
{
    if (!(CurrProc->state.attr&0x80)) { /* ROUTINE_WANTS_DEBUGGING */
        DoBlockTrim();
        Renumber();
    }
}

void DoBlockTrim(void)
{
    block *blk,*next,*target;
    instruction *ins;
    unsigned char change;
    for (;;) {
        change=0;
        MarkReachableBlocks();
        blk=HeadBlock->next_block;
        while (blk!=NULL) {
            next=blk->next_block;
            if (!(blk->class&(0x2000|0x800))) { /* UNKNOWN_DESTINATION / VISITED */
                while (blk->input_edges!=NULL) RemoveInputEdge(blk->input_edges);
                RemoveBlock(blk);
                change=1;
            } else if (blk->class&4) {
                change|=SameTarget(blk);
            } else if (blk->class&2) {
                target=blk->edge[0].destination;
                if (target!=blk && !(target->class&0x2000)) {
                    ins=blk->ins.hd.next;
                    /* This old loop skips all NOPs, before later DBGINFO
                     * NOPs acquired the special preservation test. */
                    while (ins->head.opcode==0) ins=ins->head.next;
                    if (ins->head.opcode==0x4b) {
                        if (!(blk->class&0x20)) change|=Retarget(blk);
                    } else if (target->inputs==1 && !(target->class&0x20) &&
                               CountIns(blk)+CountIns(target)<=75) {
                        if (!(blk->class&(0x1000|0x20)) && !(target->class&0x40)) {
                            JoinBlocks(blk,target);
                            change=1;
                        }
                    }
                }
            }
            blk=next;
        }
        UnMarkBlocks();
        if (!change) break;
        BlocksUnTrimmed=0;
    }
    if (HeadBlock!=NULL) {
        HeadBlock->id=1;
        blk=HeadBlock;
        for (;;) {
            next=blk->next_block;
            if (next==NULL) break;
            next->id=blk->id+1;
            blk=next;
        }
    }
}

/* The original deliberately avoids recursion here to bound host stack use. */
void MarkReachableBlocks(void)
{
    block *blk,*son;
    unsigned char change;
    int index;
    for (;;) {
        change=0;
        blk=HeadBlock;
        while (blk!=NULL) {
            if (blk->class&(0x20|0x800|8)) { /* BIG_LABEL / VISITED / SELECT */
                blk->class|=0x800;
                index=blk->targets;
                while (--index>=0) {
                    son=blk->edge[index].destination;
                    if (!(son->class&0x800)) {
                        son->class|=0x800;
                        change=1;
                    }
                }
            }
            blk=blk->next_block;
        }
        if (!change) break;
    }
}

unsigned char Retarget(struct block *blk)
{
    block_edge *edge,*next;
    block *target;
    unsigned char success;
    success=1;
    target=blk->edge[0].destination;
    edge=blk->input_edges;
    blk->input_edges=NULL;
    while (edge!=NULL) {
        next=edge->next_source;
        if (edge->source->class&(8|0x80)) { /* SELECT / LABEL_RETURN */
            success=0;
            edge->next_source=blk->input_edges;
            blk->input_edges=edge;
        } else {
            edge->destination=target;
            edge->next_source=target->input_edges;
            target->input_edges=edge;
            ++target->inputs;
            --blk->inputs;
        }
        edge=next;
    }
    if (success) {
        MoveHead(blk,target);
        RemoveBlock(blk);
    }
    return success;
}

void MoveHead(struct block *old,struct block *target)
{
    block *blk;
    if (!(old->class&0x100)) return; /* LOOP_HEADER */
    for (blk=HeadBlock;blk!=NULL;blk=blk->next_block) {
        if (blk->loop_head==old) blk->loop_head=target;
    }
    target->class|=old->class&0x110; /* LOOP_HEADER / ITERATIONS_KNOWN */
    old->class&=~0x110;
    target->iterations=old->iterations;
    target->loop_head=old->loop_head;
    old->loop_head=target;
}

void RemoveBlock(struct block *blk)
{
    int index;
    unsigned last_line;
    block *check,*next;
    instruction *ins;
    if (blk->prev_block!=NULL) blk->prev_block->next_block=blk->next_block;
    if (blk->next_block!=NULL) blk->next_block->prev_block=blk->prev_block;
    for (index=0;index<blk->targets;++index) {
        if (FindBlock(blk->edge[index].destination)) RemoveInputEdge(&blk->edge[index]);
    }
    last_line=blk->ins.hd.line_num;
    for (;;) {
        ins=blk->ins.hd.next;
        if (ins==(instruction *)&blk->ins) break;
        if (ins->head.line_num!=0) last_line=ins->head.line_num;
        FreeIns(ins);
    }
    if (blk->next_block!=NULL && blk->next_block->gen_id==blk->gen_id+1) {
        next=blk->next_block;
    } else {
        next=NULL;
        for (check=HeadBlock;check!=NULL;check=check->next_block) {
            if (check!=blk && check->gen_id>blk->gen_id &&
                (next==NULL || next->gen_id>check->gen_id)) next=check;
        }
    }
    if (next!=NULL && next->ins.hd.line_num==0) next->ins.hd.line_num=last_line;
    if (HeadBlock==blk) HeadBlock=blk->next_block;
    if (BlockList==blk) {
        BlockList=blk->prev_block;
        if (BlockList==NULL) BlockList=HeadBlock;
    }
    TellScrapLabel(blk->label);
    if (blk->dataflow!=NULL) CGFreeSize(blk->dataflow,sizeof(data_flow_def));
    FreeABlock(blk);
}

unsigned char FindBlock(struct block *target)
{
    block *blk;
    for (blk=HeadBlock;blk!=NULL;blk=blk->next_block) {
        if (blk==target) return 1;
    }
    return 0;
}

void RemoveInputEdge(struct block_edge *edge)
{
    block *dest;
    block_edge *prev;
    /* The old routine requires a linked block edge; it predates the later
     * DEST_IS_BLOCK guard. Its callers establish that ownership. */
    dest=edge->destination;
    --dest->inputs;
    prev=dest->input_edges;
    if (prev==edge) {
        dest->input_edges=edge->next_source;
    } else {
        while (prev->next_source!=edge) prev=prev->next_source;
        prev->next_source=edge->next_source;
    }
}

unsigned char SameTarget(struct block *blk)
{
    instruction *ins;
    block *first,*second;
    first=blk->edge[0].destination;
    second=blk->edge[1].destination;
    if (first!=second) return 0;
    if ((first->class|second->class)&0x2000) return 0;
    blk->class=(blk->class&~6)|2;
    RemoveEdge(&blk->edge[1]);
    ins=blk->ins.hd.prev;
    while (ins->head.opcode<0x2e || ins->head.opcode>0x35) ins=ins->head.prev;
    FreeIns(ins);
    return 1;
}

void UnMarkBlocks(void)
{
    block *blk;
    blk=HeadBlock;
    while (blk!=NULL) {
        blk->class&=~0x800;
        blk=blk->next_block;
    }
}

instruction *FindOneCond(block *blk);
unsigned char DeadBlocks(void)
{
    block *blk,*dest_blk;
    block_edge *edge;
    instruction *ins;
    int dest;
    unsigned char change;
    change=0;
    for (blk=HeadBlock;blk!=NULL;blk=blk->next_block) {
        if (!(blk->class&4)) continue;
        ins=FindOneCond(blk);
        if (ins==NULL || ins->result!=NULL) continue;
        dest=ins->flags.byte&0xf;
        if (dest!=(ins->flags.byte>>4)) continue;
        /* This older body contains KillCondBlk's logic inline. */
        RemoveInputEdge(&blk->edge[0]);
        RemoveInputEdge(&blk->edge[1]);
        blk->targets=1;
        blk->class&=~6U;
        blk->class|=2;
        dest_blk=blk->edge[dest].destination;
        edge=&blk->edge[0];
        edge->flags=blk->edge[dest].flags;
        edge->source=blk;
        edge->destination=dest_blk;
        edge->next_source=dest_blk->input_edges;
        dest_blk->input_edges=edge;
        ++dest_blk->inputs;
        FreeIns(ins);
        change=1;
    }
    if (change) { BlockTrim();return 1; }
    return 0;
}
instruction *FindOneCond(block *blk)
{
    instruction *ins,*cond;
    cond=NULL;
    for (ins=blk->ins.hd.next;ins->head.opcode!=0x4b;ins=ins->head.next) {
        if (ins->head.opcode>=0x2e && ins->head.opcode<=0x35) {
            if (cond!=NULL) return NULL;
            cond=ins;
        }
    }
    return cond;
}

void JoinBlocks(block *jump,block *target)
{
    block_edge          *edge;
    unsigned short  line_num;
    struct code_lbl *        label;
    instruction         *nop;

    /*  To get here, 'target' is only entered from 'jump'*/
    /*  Thus, the only input edge to 'target' is from jump, and can be tossed*/

    /* keep the label from jump in case it's referenced in a SELECT block*/

    label = target->label;
    target->label = jump->label;
    if( jump->class & 0x20 ) target->class |= 0x20;
    jump->label = label;
    line_num = target->ins.hd.line_num;
    target->ins.hd.line_num = jump->ins.hd.line_num;

    /*  Move the inputs to 'jump' to be inputs to 'target'*/

    target->inputs = jump->inputs;
    edge = jump->input_edges;
    target->input_edges = edge;
    while( edge != NULL ) {
        edge->destination = target;    /* was 'jump' before*/
        edge = edge->next_source;
    }

    /*  Now join the instruction streams*/

    nop = jump->ins.hd.prev;
    if( nop->head.opcode == 0 ) {
        if( nop->flags.nop_flags & 4 ) {
            /* this nop is only here to hold source info so we just
             * attach the source info to the next instruction and
             * nuke this nop so that it can't inhibit optimization */
             if( target->ins.hd.next->head.line_num == 0 ) {
                 target->ins.hd.next->head.line_num = nop->head.line_num;
             }
             FreeIns( nop );
        }
    }

    if( jump->ins.hd.next != (instruction *)&jump->ins ) {
        if( line_num != 0 ) {
            jump->ins.hd.prev->head.line_num = line_num;
        }
        jump->ins.hd.prev->head.next = target->ins.hd.next;
        target->ins.hd.next->head.prev = jump->ins.hd.prev;
        target->ins.hd.next = jump->ins.hd.next;
        target->ins.hd.next->head.prev = (instruction *)&target->ins;
        jump->ins.hd.next = (instruction *)&jump->ins;/* so RemoveBlock won't*/
        jump->ins.hd.prev = (instruction *)&jump->ins;/* free the instr list*/
    }

    jump->inputs = 0;
    jump->targets = 0;
    MoveHead( jump, target );
    RemoveBlock( jump );
}
