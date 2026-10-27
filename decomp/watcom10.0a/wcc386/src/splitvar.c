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
* Description:  Split independent uses of temporaries into separate variables.
*
****************************************************************************/


#include "block.h"
#include "genopt.h"
#include "stubs.h"
#include <recon.h>

unsigned char Split1Var(conflict_node *conf);
void CleanUp(void);
void SplitVars(void)
{
    name *op;
    conflict_node *conf;
    for (;;) {
        for (conf=ConfList;conf!=NULL;conf=conf->next_conflict) {
            op=conf->name;
            if (!(op->v.usage&2)) continue;
            if (op->n.class!=2) continue;
            if (conf->state&0x4000) continue; /* CONF_VISITED */
            conf->state&=~4U; /* CONFLICT_ON_HOLD */
            if ((conf->id.out_of_block.bits[0]|conf->id.out_of_block.bits[1]|
                 conf->id.out_of_block.bits[2]|conf->id.out_of_block.bits[3])==0) continue;
            if ((name *)op->t.alias==op) Split1Var(conf);
            conf->id.out_of_block.bits[0]=conf->id.out_of_block.bits[1]=0;
            conf->id.out_of_block.bits[2]=conf->id.out_of_block.bits[3]=0;
            conf->state|=0x4004;
        }
        if (!MoreConflicts()) break;
    }
    CleanUp();
    FreeConflicts();
    InsDead();
    NullConflicts(0);
    FindReferences();
    MakeConflicts();
}
static unsigned short Instance;
static global_bit_set Id;
void NotVisited(void);
block *FindUnMarkedInstance(void);
void MarkInstance(block *blk);
void ReplaceInstances(name *of,name *with);
unsigned char Split1Var(conflict_node *conf)
{
    block *unlabeled;
    name *op;
    unsigned char change;
    NotVisited();
    Instance=0;
    Id=conf->id.out_of_block;
    for (;;) {
        unlabeled=FindUnMarkedInstance();
        if (unlabeled==NULL) break;
        ++Instance;
        MarkInstance(unlabeled);
    }
    op=conf->name;
    change=0;
    while (Instance>1) {
        change=1;
        ReplaceInstances(op,SAllocTemp(op->n.name_class,op->n.size));
        --Instance;
    }
    return change;
}
void NotVisited(void)
{
    block *blk;
    blk=HeadBlock;
    while (blk!=NULL) {
        blk->class&=~0x800U;
        blk->id=0;
        blk=blk->next_block;
    }
}
#define OVERLAP(a,b) (((a).bits[0]&(b).bits[0])|((a).bits[1]&(b).bits[1])|\
                      ((a).bits[2]&(b).bits[2])|((a).bits[3]&(b).bits[3]))
block *FindUnMarkedInstance(void)
{
    block *blk;
    data_flow_def *flow;
    for (blk=HeadBlock;blk!=NULL;blk=blk->next_block) {
        if (blk->class&0x800) continue;
        flow=blk->dataflow;
        if (OVERLAP(Id,flow->in) || OVERLAP(Id,flow->out) ||
            OVERLAP(Id,flow->def) || OVERLAP(Id,flow->use)) break;
    }
    return blk;
}
void MarkInstance(block *blk)
{
    block_edge *edge;
    int i;
    data_flow_def *flow;
    global_bit_set *bitp;
    if (blk->class&0x800) return;
    blk->class|=0x800;
    blk->id=Instance;
    flow=blk->dataflow;
    /* Same historical void-callback ABI as MarkVisited: one object
     * pointer, with every returned EAX discarded. No pointer result or
     * low-stack relocation equivalence is asserted by these local casts. */
    if (OVERLAP(flow->in,Id)) {
        edge=blk->input_edges;
        while (edge!=NULL) {
            bitp=&edge->source->dataflow->out;
            if (OVERLAP(*bitp,Id))
                SafeRecurse((recurse_callback)MarkInstance,edge->source);
            edge=edge->next_source;
        }
    }
    if (OVERLAP(flow->out,Id)) {
        i=blk->targets;
        edge=&blk->edge[0];
        while (--i>=0) {
            bitp=&edge->destination->dataflow->in;
            if (OVERLAP(*bitp,Id))
                SafeRecurse((recurse_callback)MarkInstance,edge->destination);
            ++edge;
        }
    }
}
name *ScaleIndex(name *index,name *base,long offset,unsigned char type_class,
                 long size,int scale,unsigned char flags);
unsigned char RepOp(name **slot,name *of,name *with)
{
    name *op,*base,*index;
    unsigned char change;
    op=*slot;
    change=0;
    if (op==of) {
        *slot=with;
        change=1;
    } else if (op->n.class==4) {
        base=op->i.base;
        if (base!=NULL && !(op->i.index_flags&0x10) && base==of) {
            base=with;
            change=1;
        }
        index=op->i.index;
        if (index==of) {
            index=with;
            change=1;
        }
        if (change)
            *slot=ScaleIndex(index,base,op->i.constant,op->n.name_class,
                             op->n.size,op->i.scale,op->i.index_flags);
    }
    return change;
}
void ReplaceInstances(name *of,name *with)
{
    block *blk;
    instruction *ins;
    int i;
    for (blk=HeadBlock;blk!=NULL;blk=blk->next_block) {
        if (blk->id!=Instance) continue;
        for (ins=blk->ins.hd.next;ins->head.opcode!=0x4b;ins=ins->head.next) {
            i=ins->num_operands;
            while (--i>=0) RepOp(&ins->operands[i],of,with);
            if (ins->result!=NULL) RepOp(&ins->result,of,with);
        }
    }
}
void CleanUp(void)
{
    block *blk;
    unsigned short id;
    blk=HeadBlock;
    id=0;
    while (blk!=NULL) {
        blk->class&=~0x800U;
        blk->id=++id;
        blk=blk->next_block;
    }
}
