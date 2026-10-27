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
* Description:  Reorder post-increment instructions before propagation.
*
****************************************************************************/


#include "block.h"
#include "address.h"
#include "genopt.h"
#include "stubs.h"
#include <recon.h>

/* Released opcode attributes, recovered for the old 76-opcode universe.
 * P5FDIV and later opcode additions are absent; old conversion/address order
 * differs, and the later SET-condition attributes are not present here. */
enum { MOVEABLE=1,SWAPABLE=2,SIDE_EFFECT=4,PUSHABLE=8 };
static unsigned char OpAttrs[0x4c]={
    0, /* OP_NOP */
    PUSHABLE, /* OP_ADD */
    MOVEABLE, /* OP_EXT_ADD */
    PUSHABLE, /* OP_SUB */
    MOVEABLE, /* OP_EXT_SUB */
    MOVEABLE + SWAPABLE, /* OP_MUL */
    MOVEABLE + SWAPABLE, /* OP_EXT_MUL */
    MOVEABLE + SWAPABLE, /* OP_DIV */
    MOVEABLE + SWAPABLE, /* OP_MOD */
    MOVEABLE + SWAPABLE, /* OP_AND */
    MOVEABLE + SWAPABLE, /* OP_OR */
    MOVEABLE + SWAPABLE, /* OP_XOR */
    MOVEABLE + SWAPABLE, /* OP_RSHIFT */
    MOVEABLE + SWAPABLE, /* OP_LSHIFT */
    MOVEABLE + SWAPABLE, /* OP_POW */
    MOVEABLE + SWAPABLE, /* OP_ATAN2 */
    MOVEABLE + SWAPABLE, /* OP_FMOD */
    MOVEABLE + SWAPABLE, /* OP_NEGATE */
    MOVEABLE + SWAPABLE, /* OP_COMPLEMENT */
    MOVEABLE + SWAPABLE, /* OP_LOG */
    MOVEABLE + SWAPABLE, /* OP_COS */
    MOVEABLE + SWAPABLE, /* OP_SIN */
    MOVEABLE + SWAPABLE, /* OP_TAN */
    MOVEABLE + SWAPABLE, /* OP_SQRT */
    MOVEABLE + SWAPABLE, /* OP_FABS */
    MOVEABLE + SWAPABLE, /* OP_ACOS */
    MOVEABLE + SWAPABLE, /* OP_ASIN */
    MOVEABLE + SWAPABLE, /* OP_ATAN */
    MOVEABLE + SWAPABLE, /* OP_COSH */
    MOVEABLE + SWAPABLE, /* OP_EXP */
    MOVEABLE + SWAPABLE, /* OP_LOG10 */
    MOVEABLE + SWAPABLE, /* OP_SINH */
    MOVEABLE + SWAPABLE, /* OP_TANH */
    MOVEABLE + SWAPABLE, /* OP_PTR_TO_NATIVE */
    MOVEABLE + SWAPABLE, /* OP_PTR_TO_FORIEGN */
    0, /* OP_SLACK */
    MOVEABLE + SWAPABLE, /* OP_CONVERT */
    MOVEABLE + SWAPABLE, /* old scalar conversion slot */
    MOVEABLE + SWAPABLE, /* OP_MOV */
    MOVEABLE + SWAPABLE, /* old address operation slot */
    MOVEABLE + SWAPABLE, /* OP_LA */
    SIDE_EFFECT, /* OP_CALL_INDIRECT */
    SWAPABLE, /* OP_PUSH */
    SWAPABLE, /* OP_POP */
    0, /* OP_PARM_DEF */
    0, /* OP_SELECT */
    SIDE_EFFECT, /* OP_BIT_TEST_TRUE */
    SIDE_EFFECT, /* OP_BIT_TEST_FALSE */
    SIDE_EFFECT, /* OP_CMP_EQUAL */
    SIDE_EFFECT, /* OP_CMP_NOT_EQUAL */
    SIDE_EFFECT, /* OP_CMP_GREATER */
    SIDE_EFFECT, /* OP_CMP_LESS_EQUAL */
    SIDE_EFFECT, /* OP_CMP_LESS */
    SIDE_EFFECT, /* OP_CMP_GREATER_EQUAL */
    SIDE_EFFECT, /* OP_CALL */
    0, /* old non-reorderable opcode 0x37 */
    0, /* old non-reorderable opcode 0x38 */
    0, /* old non-reorderable opcode 0x39 */
    0, /* old non-reorderable opcode 0x3a */
    0, /* old non-reorderable opcode 0x3b */
    0, /* old non-reorderable opcode 0x3c */
    0, /* old non-reorderable opcode 0x3d */
    0, /* old non-reorderable opcode 0x3e */
    0, /* old non-reorderable opcode 0x3f */
    0, /* old non-reorderable opcode 0x40 */
    0, /* old non-reorderable opcode 0x41 */
    0, /* old non-reorderable opcode 0x42 */
    0, /* old non-reorderable opcode 0x43 */
    0, /* old non-reorderable opcode 0x44 */
    0, /* old non-reorderable opcode 0x45 */
    0, /* old non-reorderable opcode 0x46 */
    0, /* old non-reorderable opcode 0x47 */
    0, /* old non-reorderable opcode 0x48 */
    0, /* old non-reorderable opcode 0x49 */
    0, /* old non-reorderable opcode 0x4a */
    0, /* OP_BLOCK */
};

void PushPostOps(void)
{
    block *blk;
    for (blk=HeadBlock;blk!=NULL;blk=blk->next_block) PushInsForward(blk);
    PropagateMoves();
    Renumber();
}
void PushInsForward(block *blk)
{
    instruction *ins,*prev,*next;
    ins=blk->ins.hd.prev;
    for (;;) {
        if (ins->head.opcode==0x4b) break;
        prev=ins->head.prev;
        next=CanMoveAfter(ins);
        if (next!=NULL) {
            ins->head.next->head.prev=ins->head.prev;
            ins->head.prev->head.next=ins->head.next;
            ins->head.prev=next->head.prev;
            next->head.prev=ins;
            ins->head.prev->head.next=ins;
            ins->head.next=next;
        }
        ins=prev;
    }
}
instruction *CanMoveAfter(instruction *ins)
{
    instruction *next;
    /* No later opcode may silently acquire the old table's attributes. */
    if (ins->head.opcode>=sizeof(OpAttrs)) STUB_TRAP();
    if (!(OpAttrs[ins->head.opcode]&PUSHABLE)) return NULL;
    if (ins->operands[0]!=ins->result) return NULL;
    if (ins->operands[1]->n.class!=0) return NULL;
    if (ins->type_class==FS || ins->type_class==FD ||
        ins->base_type_class==FS || ins->base_type_class==FD) return NULL;
    next=ins->head.next;
    for (;;) {
        if (next->head.opcode>=sizeof(OpAttrs)) STUB_TRAP();
        if (!(OpAttrs[next->head.opcode]&SWAPABLE)) break;
        if (!CanReorder(ins,next)) break;
        next=next->head.next;
        if (next==NULL) break;
    }
    if (next==ins->head.next) return NULL;
    return next;
}
unsigned char ReDefinesOps(instruction *of,instruction *ins)
{
    int i;
    i=of->num_operands;
    while (--i>=0) {
        if (of->operands[i]->n.class==3) return 1;
        if (ReDefinedBy(ins,of->operands[i])) return 1;
    }
    if (of->result!=NULL) {
        if (of->result->n.class==3) return 1;
        if (ReDefinedBy(ins,of->result)) return 1;
    }
    return 0;
}
unsigned char CanReorder(instruction *try_ins,instruction *after)
{
    if (ReDefinesOps(try_ins,after)) return 0;
    if (ReDefinesOps(after,try_ins)) return 0;
    return 1;
}

void DeadTemps(void)
{
    block *blk;
    instruction *ins,*next;
    for (blk=HeadBlock;blk!=NULL;blk=blk->next_block) {
        ins=blk->ins.hd.next;
        while (ins->head.opcode!=0x4b) {
            next=ins->head.next;
            if (!SideEffect(ins)) {
                if (ins->head.opcode>=sizeof(OpAttrs)) STUB_TRAP();
                if (!(OpAttrs[ins->head.opcode]&SIDE_EFFECT) && ins->result!=NULL &&
                    ins->result->n.class==2 && !(ins->result->v.usage&0x4b)) FreeIns(ins);
            }
            ins=next;
        }
    }
}

unsigned char IsDeadIns(block *blk,instruction *ins,instruction *next);
void AxeDeadCode(void)
{
    block *blk;
    instruction *ins,*next;
    unsigned char change;
    for (;;) {
        change=0;
        for (blk=HeadBlock;blk!=NULL;blk=blk->next_block) {
            ins=blk->ins.hd.next;
            while (ins->head.opcode!=0x4b) {
                next=ins->head.next;
                if (IsDeadIns(blk,ins,next)) {
                    ins->result=NULL;
                    change=1;
                    if (!(OpAttrs[ins->head.opcode]&SIDE_EFFECT)) FreeIns(ins);
                    else if (ins->head.opcode>=0x2e && ins->head.opcode<=0x35)
                        ins->flags.byte=0x22; /* true and false NO_JUMP=2 */
                }
                ins=next;
            }
        }
        if (!change) break;
        FreeConflicts();
        NullConflicts(0);
        FindReferences();
        MakeConflicts();
        MakeLiveInfo();
    }
}
conflict_node *FindConflictNode(name *opnd,block *blk,instruction *ins);
unsigned char IsDeadIns(block *blk,instruction *ins,instruction *next)
{
    conflict_node *conf;
    name *op;
    if (SideEffect(ins)) return 0;
    op=ins->result;
    if (op==NULL || (op->v.usage&8)) return 0;
    conf=FindConflictNode(op,blk,ins);
    if (conf==NULL) return 0;
    if (conf->id.within_block==0 &&
        conf->id.out_of_block.bits[0]==0 &&
        conf->id.out_of_block.bits[1]==0 &&
        conf->id.out_of_block.bits[2]==0 &&
        conf->id.out_of_block.bits[3]==0) return 0;
    if ((conf->id.within_block&next->head.live.within_block) ||
        (conf->id.out_of_block.bits[0]&next->head.live.out_of_block[0]) ||
        (conf->id.out_of_block.bits[1]&next->head.live.out_of_block[1]) ||
        (conf->id.out_of_block.bits[2]&next->head.live.out_of_block[2]) ||
        (conf->id.out_of_block.bits[3]&next->head.live.out_of_block[3])) return 0;
    return 1;
}

void FreeJunk(block *blk);
void DeadInstructions(void)
{
    block *blk;
    blk=HeadBlock;
    while (blk!=NULL) {
        FreeJunk(blk);
        blk=blk->next_block;
    }
}

unsigned char FPIsStack(name *value);
unsigned char SameThing(name *left,name *right)
{
    if (left==right) return 1;
    if (left==NULL || right==NULL) return 0;
    if (FPIsStack(left) && FPIsStack(right)) return 1;
    if (left->n.class==1 && right->n.class==1 && left->v.symbol==right->v.symbol) return 1;
    if (left->n.class==2 && right->n.class==2) {
        if (left->t.id.id!=right->t.id.id) return 0;
        return (left->t.temp_flags&2) || (right->t.temp_flags&2);
    }
    return 0;
}
