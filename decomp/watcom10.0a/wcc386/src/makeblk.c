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
* Description:  Basic type initialization and type-node allocation.
*
****************************************************************************/


/* cgen2.c's emission-state initializer. In the earlier target the next
 * imported segment starts at -1 directly, before the later SegData field. */
#include "block.h"
#include "cgmemory.h"
#include "dbinfo.h"
#include "memory.h"
#include <recon.h>

unsigned char HaveCurrBlock,BlocksUnTrimmed;
long MaxStack;

/* makeblk.c before the later blanket zero-fill of newly allocated state. */
void NewProc(int level)
{
    proc_def *next;
    if (CurrProc!=NULL) {
        SaveToTargProc();
        CurrProc->head_block=HeadBlock;
        CurrProc->tail_block=BlockList;
        CurrProc->curr_block=CurrBlock;
        CurrProc->lasttemp=LastTemp;
        CurrProc->dummy_index=DummyIndex;
        CurrProc->names[0]=Names[0];
        CurrProc->names[1]=Names[1];
        CurrProc->names[2]=Names[2];
        CurrProc->names[3]=Names[3];
        CurrProc->names[4]=Names[4];
        CurrProc->block_by_block=BlockByBlock;
        CurrProc->ins_id=InsId;
        CurrProc->untrimmed=BlocksUnTrimmed;
    }
    HaveCurrBlock=1;
    BlocksUnTrimmed=1;
    MaxStack=0;
    InsId=0;
    HeadBlock=NULL;
    BlockList=NULL;
    ReInitNames();
    next=CGAlloc(sizeof(*next));
    next->frame_index=NULL;
    next->parms.size=0;
    next->parms.base=0;
    next->locals.size=0;
    next->next_proc=CurrProc;
    next->lex_level=(unsigned char)level;
    CurrProc=next;
    next->prolog_state=0;
    next->parms_list=NULL;
    next->locals.base=AskDisplaySize(level);
    InitTargProc();
}

void EnLink(struct code_lbl *label,unsigned char label_dies)
{
    block *next;
    next=NewBlock(label,label_dies);
    CurrBlock=next;
    next->ins.hd.line_num=SrcLine;
    SrcLine=0;
}

block *NewBlock(struct code_lbl *label,unsigned char label_dies)
{
    block *next;
    next=MakeBlock(label,1);
    next->edge[0].flags=label_dies ? 1:0;
    return next;
}

/* Older makeblk.c initializes the instruction sentinel and edge sources here;
 * next/previous block links are supplied when the block list is built. */
block *MakeBlock(struct code_lbl *label,unsigned short edges)
{
    block *next;
    unsigned short index;
    next=CGAlloc(offsetof(block,edge)+edges*sizeof(block_edge));
    next->ins.hd.next=(struct instruction *)&next->ins;
    next->ins.hd.prev=(struct instruction *)&next->ins;
    next->class=0;
    next->ins.hd.opcode=0x4b;
    next->label=label;
    next->ins.hd.live.regs=0;
    next->ins.hd.live.out_of_block[0]=0;
    next->ins.hd.live.out_of_block[1]=0;
    next->ins.hd.live.out_of_block[2]=0;
    next->ins.hd.live.out_of_block[3]=0;
    next->ins.hd.live.within_block=0;
    next->u.interval=NULL;
    next->inputs=0;
    next->input_edges=NULL;
    next->targets=0;
    next->dataflow=NULL;
    next->cc=NULL;
    next->loop_head=NULL;
    next->unroll_count=0;
    next->ins.blk=next;
    for (index=0;index<edges;++index) next->edge[index].source=next;
    return next;
}

unsigned char InitCallState(type_def *type)
{
    void *symbol,*aux;
    name *entry;
    symbol=AskForLblSym(CurrProc->label);
    entry=AllocMemory(symbol,0,CG_FE,TypeClass(type));
    entry->v.usage|=0x10;
    aux=FEAuxInfo(symbol,0);
    return CallState(aux,type,&CurrProc->state);
}

void AddIns(instruction *entry)
{
    if (!HaveCurrBlock) {
        EnLink(AskForNewLabel(),1);
        HaveCurrBlock=1;
    }
    entry->head.next=(instruction *)&CurrBlock->ins;
    entry->head.prev=CurrBlock->ins.hd.prev;
    CurrBlock->ins.hd.prev->head.next=entry;
    CurrBlock->ins.hd.prev=entry;
    entry->head.line_num=SrcLine;
    entry->id=++InsId;
    SrcLine=0;
}

void GenBlock(int class,int targets)
{
    block *next;
    block_edge *edge;
    instruction *ins;
    NamesCrossBlocks();
    if (HeadBlock==NULL) {
        HeadBlock=CurrBlock;
        CurrBlock->id=1;
        CurrBlock->gen_id=1;
    } else {
        BlockList->next_block=CurrBlock;
        CurrBlock->id=BlockList->id+1;
        CurrBlock->gen_id=BlockList->gen_id+1;
    }
    if (SrcLine!=0) {
        ins=MakeNop();
        ins->flags.nop_flags|=4; /* NOP_SOURCE_QUEUE */
        AddIns(ins);
    }
    CurrBlock->prev_block=BlockList;
    BlockList=CurrBlock;
    CurrBlock->next_block=NULL;
    if (targets>1) {
        next=CGAlloc(offsetof(block,edge)+targets*sizeof(block_edge));
        Copy((char *)CurrBlock,(char *)next,sizeof(block));
        if (CurrBlock->ins.hd.next==(instruction *)&CurrBlock->ins) {
            next->ins.hd.next=(instruction *)&next->ins;
            next->ins.hd.prev=(instruction *)&next->ins;
        } else {
            next->ins.hd.next->head.prev=(instruction *)&next->ins;
            next->ins.hd.prev->head.next=(instruction *)&next->ins;
        }
        next->ins.blk=next;
        if (HeadBlock==CurrBlock) HeadBlock=next;
        if (BlockList==CurrBlock) BlockList=next;
        if (next->prev_block!=NULL) next->prev_block->next_block=next;
        if (next->next_block!=NULL) next->next_block->prev_block=next;
        edge=next->input_edges;
        while (edge!=NULL) {
            edge->destination=next;
            edge=edge->next_source;
        }
        CGFreeSize(CurrBlock,sizeof(block));
        CurrBlock=next;
    }
    CurrBlock->class&=0x20; /* BIG_LABEL */
    CurrBlock->class|=class;
    while (--targets>=1) CurrBlock->edge[targets].flags=0;
}

void AddTarget(struct code_lbl *label,unsigned char dies)
{
    block_edge *edge;
    edge=&CurrBlock->edge[CurrBlock->targets++];
    edge->source=CurrBlock;
    /* Before FixEdges, destination stores a label rather than a block. */
    edge->destination=(block *)label;
    edge->next_source=NULL;
    if (dies) edge->flags|=2; /* DEST_LABEL_DIES */
}

unsigned char BlkTooBig(void)
{
    struct code_lbl *label;
    if (!HaveCurrBlock || CurrBlock==NULL) return 0;
    if (CurrBlock->ins.hd.next==(instruction *)&CurrBlock->ins) return 0;
    if ((int)(InsId-CurrBlock->ins.hd.next->id)<75) return 0;
    if (CurrBlock->targets!=0) return 0;
    label=AskForNewLabel();
    GenBlock(2,1); /* JUMP */
    AddTarget(label,0);
    EnLink(label,1);
    return 1;
}
