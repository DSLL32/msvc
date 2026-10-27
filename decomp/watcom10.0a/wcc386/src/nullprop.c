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
* Description:  Use dereference information to simplify pointer comparisons.
*
****************************************************************************/


#include "block.h"
#include "genopt.h"
#include "cgmemory.h"
#include "stubs.h"
#include <recon.h>

void NullProp(block *blk);
void PropNullInfo(void)
{
    block *blk;
    if (Model&0x80000000U) return;
    if (Model&0x40000) return; /* NULL_DEREF_OK */
    /* The old pass has no change-result accumulation, ClearBlockBits or
     * final BlockTrim call; NullProp's result is not consumed. */
    blk=HeadBlock;
    while (blk!=NULL) {
        NullProp(blk);
        blk=blk->next_block;
    }
}
#pragma pack(4)
typedef struct null_parms {
    block *blk;
    instruction *ins;
    name *op;
    unsigned char forward;
} null_parms;
#pragma pack()
instruction *CompareIns(block *blk);
unsigned char IsZero(name *op);
void *DominatingDeref(void *value);
unsigned char EdgeHasSideEffect(block *blk,instruction *cmp,unsigned char cmp_result);
void NullProp(block *blk)
{
    instruction *cmp;
    name **ptr;
    null_parms parms;
    cmp=CompareIns(blk);
    if (cmp==NULL) return;
    if (cmp->head.opcode!=0x30 && cmp->head.opcode!=0x31) return;
    if (IsZero(cmp->operands[0])) ptr=&cmp->operands[1];
    else if (IsZero(cmp->operands[1])) ptr=&cmp->operands[0];
    else return;
    parms.blk=blk;
    parms.ins=cmp;
    parms.op=*ptr;
    parms.forward=1;
    if (!DominatingDeref(&parms) ||
        EdgeHasSideEffect(blk,cmp,cmp->head.opcode==0x31)) {
        parms.forward=0;
        if (!DominatingDeref(&parms)) return;
    }
    /* The older pass substitutes a known non-NULL value and folds the
     * compare; later source directly deletes conditional block edges. */
    *ptr=AllocS32Const(1);
    FoldIns(cmp);
}
instruction *CompareIns(block *blk)
{
    instruction *last;
    if (blk->class&4) {
        last=blk->ins.hd.prev;
        while (last->head.opcode!=0x4b) {
            if (last->head.opcode>=0x30 && last->head.opcode<=0x35) return last;
            last=last->head.prev;
        }
    }
    return NULL;
}
unsigned char IsZero(name *op)
{
    return op->n.class==0 && op->c.const_type==0 && op->c.int_value==0;
}
/* nullprop.c private work stack and direction-aware CFG queries. Prefix the
 * private NextIns name to distinguish the object-queue routine. FirstIns is
 * folded into that same header-pointer entry in the actual 1994 image. */
typedef struct null_edge_entry {
    struct null_edge_entry *next;
    block_edge *edge;
} null_edge_entry;
typedef struct null_edge_stack { null_edge_entry *top; } null_edge_stack;
null_edge_stack *InitStack(void)
{
    null_edge_stack *stack;
    stack=CGAlloc(sizeof(*stack));
    stack->top=NULL;
    return stack;
}
unsigned char Empty(null_edge_stack *stack)
{
    return stack->top==NULL;
}
block_edge *Pop(null_edge_stack *stack)
{
    null_edge_entry *top;
    block_edge *edge;
    if (stack->top==NULL) return NULL;
    top=stack->top;
    edge=top->edge;
    stack->top=top->next;
    CGFreeSize(top,sizeof(*top));
    return edge;
}
void FiniStack(null_edge_stack *stack)
{
    while (!Empty(stack)) Pop(stack);
    CGFreeSize(stack,sizeof(*stack));
}
/* Private nullprop.c Push, distinct from the i86proc register-push helper. */
void NullPush(null_edge_stack *stack,block_edge *edge)
{
    null_edge_entry *entry;
    entry=CGAlloc(sizeof(*entry));
    entry->edge=edge;
    entry->next=stack->top;
    stack->top=entry;
}
void PushTargets(null_edge_stack *stack,block *blk,unsigned char forward)
{
    unsigned short i;
    block_edge *edge;
    if (forward) {
        for (i=0;i<blk->targets;++i) NullPush(stack,&blk->edge[i]);
    } else {
        for (edge=blk->input_edges;edge!=NULL;edge=edge->next_source)
            NullPush(stack,edge);
    }
}
instruction *NullNextIns(ins_header *ins,unsigned char forward)
{
    return forward?ins->next:ins->prev;
}
block *EdgeBlock(block_edge *edge,unsigned char forward)
{
    return forward?edge->destination:edge->source;
}
unsigned char LastBlock(block *blk,unsigned char forward)
{
    if (forward) return (blk->class&1)!=0;
    return blk==HeadBlock;
}
unsigned char DereferencedBy(instruction *ins,name *ptr)
{
    int i;
    name *op;
    op=ins->result;
    for (i=0;i<=ins->num_operands;++i) {
        if (op!=NULL && op->n.class==4 && op->i.index==ptr && op->i.base==NULL)
            return 1;
        if (i<ins->num_operands) op=ins->operands[i];
    }
    return 0;
}
unsigned char ReDefinedBy(instruction *ins,name *op);
int BlockSearch(block *blk,instruction *ins,name *op,unsigned char forward)
{
    instruction *current;
    null_parms parms;
    current=ins;
    while (current->head.opcode!=0x4b) {
        if (forward) {
            if (DereferencedBy(current,op)) return 0;
            if (ReDefinedBy(current,op)) return 1;
            /* Active in the 1994 body, disabled by #if0 in later nullprop.c. */
            if (current->head.opcode==0x26 && current->operands[0]==op &&
                current->result!=op) {
                parms.blk=blk;
                parms.ins=current;
                parms.op=current->result;
                parms.forward=1;
                if (SafeRecurse(DominatingDeref,&parms)!=NULL) return 0;
            }
        } else {
            if (ReDefinedBy(current,op)) return 1;
            if (DereferencedBy(current,op)) return 0;
        }
        current=NullNextIns(&current->head,forward);
    }
    return 2;
}
void ClearBlockBits(unsigned mask)
{
    block *blk;
    mask=~mask;
    for (blk=HeadBlock;blk!=NULL;blk=blk->next_block) blk->class&=mask;
}

void *DominatingDeref(void *value)
{
    null_parms *parms=value;
    block_edge *edge;
    null_edge_stack *stack;
    block *blk;
    int result;
    unsigned char dominated;
    result=BlockSearch(parms->blk,NullNextIns(&parms->ins->head,parms->forward),
                       parms->op,parms->forward);
    if (result==0) return (void *)1;
    if (result==1) return NULL;
    if (LastBlock(parms->blk,parms->forward) || !(parms->op->v.usage&2)) return NULL;
    stack=InitStack();
    PushTargets(stack,parms->blk,parms->forward);
    dominated=1;
    while (dominated && !Empty(stack)) {
        edge=Pop(stack);
        blk=EdgeBlock(edge,parms->forward);
        if (blk->class&0x800) continue;
        result=BlockSearch(blk,NullNextIns(&blk->ins.hd,parms->forward),
                           parms->op,parms->forward);
        switch (result) {
        case 0:break; /* BLOCK_DEREFS */
        case 1:dominated=0;break; /* BLOCK_REDEFS */
        default:
            PushTargets(stack,blk,parms->forward);
            if (LastBlock(blk,parms->forward)) dominated=0;
            break;
        }
        blk->class|=0x800;
    }
    FiniStack(stack);
    /* The older body clears its visited marks here, unlike later source. */
    ClearBlockBits(0x800);
    return dominated?(void *)1:NULL;
}
unsigned char EdgeHasSideEffect(block *blk,instruction *cmp,unsigned char cmp_result) { STUB_TRAP(); return 0; }
