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
* Description:  Identify loop induction expressions.
*
****************************************************************************/


#include "block.h"
#include "genopt.h"
#include "cgmemory.h"
#include "address.h"
#include "stubs.h"
#include <recon.h>

unsigned char Inducable(block *blk,instruction *ins)
{
    name *op,*cons;
    if (blk->depth==0) return 0;
    if (ins->head.opcode!=1 && ins->head.opcode!=3) return 0;
    /* The old type universe has no later I64 classes. PT is segmented. */
    if (ins->type_class==FS || ins->type_class==FD || ins->type_class==PT) return 0;
    op=ins->operands[0];
    cons=ins->operands[1];
    if (cons->n.class!=0) { op=ins->operands[1];cons=ins->operands[0]; }
    if (cons->n.class!=0 || cons->c.const_type!=0) return 0;
    if (op!=ins->result) return 0;
    if (op->n.class!=2 && op->n.class!=1) return 0;
    return 1;
}
unsigned char DoLoopInvariant(unsigned char (*rtn)(void));
unsigned char FindInvariants(void);
unsigned char LoopInvariant(void)
{
    return DoLoopInvariant(FindInvariants);
}



unsigned char Induction(unsigned char invar,unsigned char unroll);
unsigned char TransLoops(unsigned char unroll)
{
    return Induction(0,unroll);
}
typedef struct block_list { block *blk;struct block_list *next; } block_list;
void InitIndVars(void);
void FiniIndVars(void);
unsigned char DoInduction(block_list *header,unsigned char reduce,unsigned char unroll);
unsigned char KillOneTrippers(void);
unsigned char Induction(unsigned char reduce,unsigned char unroll)
{
    block_list list;
    unsigned char change;
    list.blk=NULL;
    list.next=NULL;
    InitIndVars();
    change=DoInduction(&list,reduce,unroll);
    FiniIndVars();
    if (unroll) change|=KillOneTrippers();
    return change;
}
/* Actual old induction layout: WORD state, BYTE class, then aligned
 * use counters. AddIndVar allocates this complete 64-byte owner. */
#pragma pack(4)
struct invariant {
    long times;
    name *name;
    struct invariant *next;
    short id;
};
typedef struct induction {
    long times;
    name *ivtimes;
    struct invariant *invar;
    long plus2,plus;
    name *name;
    struct induction *prev,*basic,*next;
    instruction *ins;
    block *header;
    struct induction *alias;
    short lasttimes;
    unsigned short state;
    unsigned char type_class;
    int use_count,index_use_count;
} induction;
#pragma pack()
typedef char old_invariant_size[(sizeof(struct invariant)==16)?1:-1];
typedef char old_induction_next[(offsetof(induction,next)==32)?1:-1];
typedef char old_induction_state[(offsetof(induction,state)==50)?1:-1];
typedef char old_induction_class[(offsetof(induction,type_class)==52)?1:-1];
typedef char old_induction_use[(offsetof(induction,use_count)==56)?1:-1];
typedef char old_induction_size[(sizeof(induction)==64)?1:-1];
induction *IndVarList;
static block *Head,*PreHead,*Loop;
static int NumIndVars;
static unsigned char LoopProtected;
static unsigned char MemChangedInLoop; /* Actual BSS byte806e0. */
unsigned char InvariantExpr(instruction *ins,block *blk);
name *InitialValue(name *op);
void FPNotStack(name *op);
void SuffixPreHeader(instruction *ins);
unsigned char InvariantOp(name *op);
unsigned char FindInvariants(void)
{
    block *blk;
    instruction *ins,*next;
    name *op,*temp;
    int i;
    unsigned char change;
    change=0;
    for (blk=Loop;blk!=NULL;blk=blk->u.loop) {
        for (ins=blk->ins.hd.next;ins->head.opcode!=0x4b;ins=next) {
            next=ins->head.next;
            if (InvariantExpr(ins,blk)) {
                next=ins->head.next;
                next->head.prev=ins->head.prev;
                ins->head.prev->head.next=next;
                op=ins->result;
                if ((op->n.class==2 || op->n.class==1) && (op->v.block_usage&8) &&
                    !(op->v.usage&2) && !BlockByBlock) {
                    SuffixPreHeader(ins);
                    if (op->n.class==2) op->t.temp_flags|=0x81;
                    op->v.block_usage=2;
                    FPNotStack(op);
                } else {
                    temp=AllocTemp(op->n.name_class);
                    /* Actual1994 body predates the later explicit size copy. */
                    temp->t.temp_flags|=0x81;
                    ins->result=temp;
                    SuffixPreHeader(ins);
                    PrefixIns(next,MakeMove(temp,op,op->n.name_class));
                }
                change=1;
            } else if (ins->head.opcode!=0x29) {
                for (i=0;i<ins->num_operands;++i) {
                    op=ins->operands[i];
                    if (!InvariantOp(op)) continue;
                    op=InitialValue(op);
                    if (op==NULL || op->c.const_type==4) continue;
                    ins->operands[i]=op;
                    change=1;
                }
            }
        }
    }
    return change;
}
unsigned char Hoistable(instruction *ins,void *data);
unsigned char InvariantExpr(instruction *ins,block *blk)
{
    int i;
    if (!Hoistable(ins,blk)) return 0;
    i=ins->num_operands;
    while (--i>=0) if (!InvariantOp(ins->operands[i])) return 0;
    return 1;
}
name *InitialValue(name *op)
{
    instruction *other;
    block *blk;
    other=PreHead->ins.hd.prev;
    for (;;) {
        if (other->head.opcode==0x4b) {
            blk=((block_ins *)other)->blk;
            if ((blk->class&0x200) || blk->inputs!=1) return NULL;
            other=blk->input_edges->source->ins.hd.prev;
        }
        if (ReDefinedBy(other,op)) break;
        other=other->head.prev;
    }
    if (other->head.opcode!=0x26 || other->result!=op) return NULL;
    op=other->operands[0];
    if (op->n.class!=0) return NULL;
    return op;
}
void MarkLoop(void);
void FindBasics(void);
unsigned char TwistLoop(block_list *header,unsigned char unroll);
void MarkInvariants(void);
void FindNonBasics(void);
void MergeVars(void);
unsigned char ReduceInStrength(void);
unsigned char LoopInsDead(void);
void FreeBadVars(void);
void ElimIndVars(void);
void UnMarkInvariants(void);
void UnMarkLoop(void);
void InitIndVars(void)
{
    IndVarList=NULL;
}
void FreeVar(induction *var);
void FiniIndVars(void)
{
    induction *junk;
    while (IndVarList!=NULL) {
        junk=IndVarList;
        IndVarList=IndVarList->next;
        FreeVar(junk);
    }
    NumIndVars=0;
}
void FreeInvariant(struct invariant *invar);
void FreeVar(induction *var)
{
    FreeInvariant(var->invar);
    CGFreeSize(var,sizeof(induction));
}
void FreeInvariant(struct invariant *invar)
{
    struct invariant *junk;
    while (invar!=NULL) {
        junk=invar;
        invar=invar->next;
        CGFreeSize(junk,sizeof(struct invariant));
    }
}
unsigned char DoInduction(block_list *header,unsigned char reduce,unsigned char unroll)
{
    block_list curr_block;
    induction **owner,*list;
    int old;
    unsigned char change;
    change=0;
    list=NULL;
    owner=&list;
    curr_block.blk=HeadBlock;
    while (curr_block.blk!=NULL) {
        if ((curr_block.blk->class&0x100) && curr_block.blk->loop_head==header->blk) {
            curr_block.next=header;
            change|=DoInduction(&curr_block,reduce,unroll);
            while (*owner!=NULL) owner=&(*owner)->next;
            *owner=IndVarList;
            IndVarList=NULL;
        }
        curr_block.blk=curr_block.blk->next_block;
    }
    IndVarList=list;
    if (header->blk!=NULL) {
        DGBlip(); /* Actual physical LPBlip hook. */
        while (list!=NULL) { list->state|=1;list=list->next; }
        Head=header->blk;
        NumIndVars=0;
        MarkLoop();
        FindBasics();
        if (!reduce || NumIndVars==0) {
            if (!BlockByBlock && TwistLoop(header,unroll)) change=1;
        } else {
            LoopProtected=0;
            MarkInvariants();
            for (;;) {
                old=NumIndVars;
                FindNonBasics();
                if (old==NumIndVars) break;
            }
            MergeVars();
            if (ReduceInStrength()) {
                change=1;
                PreHead->class|=0x200; /* IN_LOOP */
                LoopInsDead();
                PreHead->class&=~0x200U;
            }
            FreeBadVars();
            ElimIndVars();
            if (!BlockByBlock && TwistLoop(header,unroll)) change=1;
            UnMarkInvariants();
        }
        UnMarkLoop();
    }
    return change;
}
void MakeJumpBlock(block *blk,block_edge *edge);
unsigned char KillOneTrippers(void)
{
    block       *blk;
    block       *curr;
    block       *loop_head;
    instruction *ins;
    instruction *next;
    int         i;
    unsigned char change;

    change = 0;
    for( blk = HeadBlock; blk != NULL; blk = blk->next_block ) {
        for( i = blk->targets-1; i >= 0; --i ) {
            if( blk->edge[i].flags & 0x20 ) {
                ins = blk->ins.hd.next;
                while( ins->head.opcode != 0x4b ) {
                    next = ins->head.next;
                    if( (ins->head.opcode>=0x2e && ins->head.opcode<=0x35)
                            && ins->result == NULL ) {
                        FreeIns( ins );
                    }
                    ins = next;
                }
                if( blk->class & 0x100U ) {
                    blk->class &= ~0x100U;
                    loop_head = blk;
                } else {
                    blk->loop_head->class &= ~0x100U;
                    loop_head = blk->loop_head;
                }
                for( curr = HeadBlock; curr != NULL; curr = curr->next_block ) {
                    if( curr->loop_head == loop_head ) {
                        curr->loop_head = loop_head->loop_head;
                    }
                }
                MakeJumpBlock( blk, &blk->edge[i] );
                change = 1;
                break;
            }
        }
    }
    return( change );
}
void MakeJumpBlock(block *blk,block_edge *edge) { STUB_TRAP(); }

unsigned char InLoop(block *blk);
void PreHeader(void);
void MarkLoop(void)
{
    block *blk;
    block_edge *edge;
    int targets;
    Loop=NULL;
    for (blk=HeadBlock;blk!=NULL;blk=blk->next_block) {
        if (InLoop(blk)) {
            blk->class|=0x200; /* IN_LOOP. */
            blk->u.loop=Loop;
            Loop=blk;
        }
    }
    for (blk=Loop;blk!=NULL;blk=blk->u.loop) {
        targets=blk->targets;
        edge=&blk->edge[0];
        while (--targets>=0) {
            if (!(edge->destination->class&0x200)) blk->class|=0x400; /* LOOP_EXIT. */
            ++edge;
        }
    }
    PreHeader();
}
unsigned char InLoop(block *blk)
{
    if (blk==Head) return 1;
    while (blk!=NULL) {
        if (blk->loop_head==Head) return 1;
        blk=blk->loop_head;
    }
    return 0;
}
block *FindPreHeader(void);
block *AddPreBlock(block *post);
void MoveEdge(block_edge *edge,block *destination);
void PreHeader(void)
{
    block_edge *edge,*next;
    PreHead=FindPreHeader();
    if (PreHead==NULL) {
        PreHead=AddPreBlock(Head);
        edge=Head->input_edges;
        while (edge!=NULL) {
            next=edge->next_source;
            if (edge->source!=PreHead && !(edge->source->class&0x200))
                MoveEdge(edge,PreHead);
            edge=next;
        }
    }
}
unsigned char IsPreHeader(block *test);
block *FindPreHeader(void)
{
    block *preheader;
    block_edge *edge;
    for (edge=Head->input_edges;edge!=NULL;edge=edge->next_source) {
        if (edge->flags&4) return edge->source; /* SOURCE_IS_PREHEADER. */
    }
    for (preheader=HeadBlock;preheader!=NULL;preheader=preheader->next_block) {
        if (IsPreHeader(preheader)) return preheader;
    }
    return NULL;
}
unsigned char IsPreHeader(block *test)
{
    block *other;
    int i;
    if (test->targets!=1 || test->edge[0].destination!=Head ||
        (test->class&0x200)) return 0;
    for (other=HeadBlock;other!=NULL;other=other->next_block) {
        if (other!=test && !(other->class&0x200)) {
            i=other->targets;
            while (--i>=0) {
                if (other->edge[i].destination->class&0x200) return 0;
            }
        }
    }
    test->edge[0].flags|=4; /* SOURCE_IS_PREHEADER. */
    return 1;
}

void ScanForBasics(block *blk);
void MarkSurvivors(void);
void FindBasics(void)
{
    block *blk;
    for (blk=Loop;blk!=NULL;blk=blk->u.loop) ScanForBasics(blk);
    MarkSurvivors();
}
void ScanBasic(instruction *ins);
void ScanForBasics(block *blk)
{
    instruction *ins;
    for (ins=blk->ins.hd.next;ins->head.opcode!=0x4b;ins=ins->head.next)
        ScanBasic(ins);
}
unsigned char KillIndVars(instruction *ins);
void CheckBasic(instruction *ins,name *result,name *constant);
void ScanBasic(instruction *ins)
{
    name *op;
    if (KillIndVars(ins)) return;
    if (ins->head.opcode!=1 && ins->head.opcode!=3) return;
    /* No later I64 class exists in this1994 type universe. */
    if (ins->type_class==FS || ins->type_class==FD || ins->type_class==PT) return;
    op=ins->operands[0];
    if (op->n.class==0 && op->c.const_type==0) {
        if (ins->operands[1]==ins->result && ins->head.opcode==1)
            CheckBasic(ins,ins->result,op);
    } else {
        op=ins->operands[1];
        if (op->n.class==0 && op->c.const_type==0 &&
            ins->operands[0]==ins->result) CheckBasic(ins,ins->result,op);
    }
}
unsigned char KillIndVars(instruction *ins)
{
    induction *var;
    unsigned char killed;
    killed=0;
    for (var=IndVarList;var!=NULL;var=var->next) {
        if (var->ins!=ins && !(var->state&2) && ReDefinedBy(ins,var->name)) {
            killed=1;
            var->state|=1; /* IV_DEAD. */
        }
    }
    return killed;
}
induction *FindIndVar(name *value);
induction *AddIndVar(instruction *ins,name *op,induction *prev,
    struct invariant *invar,name *ivtimes,short lasttimes,long times,
    long plus,long plus2,long iv_mult,unsigned char type_class);
void CheckBasic(instruction *ins,name *result,name *constant)
{
    long step;
    if ((result->n.class==2 || result->n.class==1) && FindIndVar(result)==NULL) {
        step=constant->c.int_value;
        if (ins->head.opcode!=1) step=(long)(0UL-(unsigned long)step);
        AddIndVar(ins,result,NULL,NULL,NULL,0,1,step,0,1,ins->type_class);
    }
}
induction *FindIndVar(name *value)
{
    induction *var;
    for (var=IndVarList;var!=NULL;var=var->next)
        if (var->name==value) break;
    return var;
}
void *CopyInvariant(void *value);
void MulInvariant(struct invariant *invar,long by);
unsigned char DifferentIV(induction *alias,induction *other);
induction *AddIndVar(instruction *ins,name *op,induction *prev,
    struct invariant *invar,name *ivtimes,short lasttimes,long times,
    long plus,long plus2,long iv_mult,unsigned char type_class)
{
    induction *var,*alias;
    var=FindIndVar(op);
    if (var==NULL) {
        var=CGAlloc(sizeof(induction));
        var->name=op;
        var->state=0;
        if (prev==NULL) {
            var->basic=var;
            var->state|=0x18; /* IV_BASIC | IV_TOP */
        } else {
            var->basic=prev->basic;
            prev->state&=~0x10;
            var->state|=0x10;
        }
        var->use_count=0;
        var->index_use_count=0;
        var->prev=prev;
        var->times=times;
        var->ivtimes=ivtimes;
        var->lasttimes=lasttimes;
        var->plus=plus;
        var->plus2=plus2;
        var->header=Head;
        var->invar=CopyInvariant(invar);
        MulInvariant(var->invar,iv_mult);
        var->ins=ins;
        var->type_class=type_class;
        var->alias=var;
        var->next=IndVarList;
        IndVarList=var;
        ++NumIndVars;
        for (alias=IndVarList;alias!=NULL;alias=alias->next) {
            if (alias==var || (alias->state&4) || DifferentIV(alias,var)) continue;
            var->alias=alias->alias;
            alias->alias=var;
            var->state|=4;
            break;
        }
    }
    return var;
}
/* SafeRecurse transports one pointer; retain typed invariant ownership
 * inside the callback, with no extra forwarding function. */
void *CopyInvariant(void *value)
{
    struct invariant *invar,*copy;
    invar=value;
    if (invar==NULL) return NULL;
    copy=CGAlloc(sizeof(struct invariant));
    copy->name=invar->name;
    copy->times=invar->times;
    copy->id=invar->id;
    copy->next=SafeRecurse(CopyInvariant,invar->next);
    return copy;
}
void MulInvariant(struct invariant *invar,long by)
{
    for (;invar!=NULL;invar=invar->next)
        invar->times=(long)((unsigned long)invar->times*(unsigned long)by);
}
unsigned char SameInvariant(struct invariant *left,struct invariant *right);
unsigned char DifferentClasses(unsigned char left,unsigned char right);
unsigned char DifferentIV(induction *alias,induction *other)
{
    if (alias->basic!=other->basic || alias->plus2!=other->plus2 ||
        alias->times!=other->times || alias->ivtimes!=other->ivtimes ||
        alias->lasttimes!=other->lasttimes) return 1;
    if (!SameInvariant(alias->invar,other->invar)) return 1;
    if (DifferentClasses(alias->name->n.name_class,other->name->n.name_class)) return 1;
    if (alias->plus!=other->plus) return 2;
    return 0;
}
unsigned char SameInvariant(struct invariant *left,struct invariant *right)
{
    for (;;) {
        if (left==NULL && right==NULL) return 1;
        if (left==NULL || right==NULL) return 0;
        if (left->name!=right->name || left->times!=right->times) return 0;
        left=left->next;
        right=right->next;
    }
}
/* Shared old type-class normalization table at7c156, already recovered
 * for the common-expression pass. Signedness alone is not a difference. */
extern unsigned char Unsigned[];
unsigned char DifferentClasses(unsigned char left,unsigned char right)
{
    return Unsigned[left]!=Unsigned[right];
}
void MarkUses(instruction *ins);
void MarkSurvivors(void)
{
    block *blk;
    instruction *ins;
    induction *var,*next;
    for (blk=Loop;blk!=NULL;blk=blk->u.loop) {
        for (ins=blk->ins.hd.next;ins->head.opcode!=0x4b;ins=ins->head.next) {
            KillIndVars(ins);
            MarkUses(ins);
        }
    }
    for (var=IndVarList;var!=NULL;var=next) {
        next=var->next;
        if (!(var->state&1)) var->state|=2;
    }
}
void ChkIVUses(induction *var,name *op);
void MarkUses(instruction *ins)
{
    induction *var;
    int i;
    for (var=IndVarList;var!=NULL;var=var->next) {
        if ((var->state&2) || ins==var->ins) continue;
        i=ins->num_operands;
        while (--i>=0) ChkIVUses(var,ins->operands[i]);
        if (ins->result!=NULL) ChkIVUses(var,ins->result);
    }
}
unsigned char Uses(name *op,name *indvar);
void ChkIVUses(induction *var,name *op)
{
    unsigned char usage;
    usage=Uses(op,var->name);
    if (usage!=1 && usage!=0) {
        ++var->use_count;
        if (usage!=2) var->state|=0x80;
    }
    if (usage==2) {
        ++var->index_use_count;
        var->state|=0x40;
        op=op->i.index;
        usage=3;
    }
    if (usage==3 && op!=var->name) var->state|=0x100;
}
unsigned char SameThing(name *left,name *right);
unsigned char InvariantOp(name *op);
unsigned char Uses(name *op,name *indvar)
{
    unsigned char usage;
    usage=1; /* UNUSED */
    if (op->n.class==4) {
        if (SameThing(op->i.index,indvar)) usage=2;
    } else {
        if (SameThing(op,indvar)) usage=3;
        else if (InvariantOp(op)) usage=0;
    }
    return usage;
}
unsigned char InvariantOp(name *op)
{
    switch (op->n.class) {
    case 0: return 1;
    case 1:
    case 2: return (op->v.block_usage&2)!=0;
    case 4:
        if (op->i.index_flags&2) return 0;
        if (!InvariantOp(op->i.index)) return 0;
        if (op->i.base==NULL) return !MemChangedInLoop;
        return InvariantOp(op->i.base);
    default: return 0;
    }
}

unsigned char AnalyseLoop(induction *list,unsigned char *onecond,instruction **cond,block **cond_blk);
unsigned char CalcFinalValue(induction *var,block *cond_blk,instruction *cond,long *final,long *initial);
unsigned char UnRoll(void);
void PropIndexes(block *blk);
void NewTarget(block_edge *edge,block *destination);
void MoveDownLoop(block *cond);
block *ReGenBlock(block *blk,struct code_lbl *label);
void SuffixPreHeader(instruction *ins);
void MarkWillExecBlocks(void);
void UnMarkWillExecBlocks(void);
unsigned char TwistLoop(block_list *header,unsigned char unroll)
{
    instruction *cond,*dupcond;
    block *cond_blk,*old_head,*blk,*old_prehead,*new_head;
    block_edge *loop_edge,*exit_edge,*edge;
    block_list *list;
    induction *var;
    unsigned char onecond,know_bounds,do_the_twist;
    long final,initial;
    AnalyseLoop(IndVarList,&onecond,&cond,&cond_blk);
    if (cond_blk==NULL) return 0;
    know_bounds=0;
    if (onecond) {
        var=FindIndVar(cond->operands[0]);
        if (var==NULL) var=FindIndVar(cond->operands[1]);
        if (var!=NULL && CalcFinalValue(var,cond_blk,cond,&final,&initial))
            if (final!=initial) know_bounds=1;
    }
    loop_edge=&cond_blk->edge[0];
    exit_edge=&cond_blk->edge[1];
    if (!(loop_edge->destination->class&0x200)) {
        loop_edge=&cond_blk->edge[1];
        exit_edge=&cond_blk->edge[0];
    }
    if (unroll && (Head->class&0x10) && Head->iterations==1)
        if (cond_blk!=Head || Loop->u.loop==NULL) exit_edge->flags|=0x20;
    if (unroll) {
        old_head=Head;
        if (UnRoll()) {
            if (Head!=old_head) {
                for (list=header;list!=NULL;list=list->next)
                    if (list->blk==old_head) list->blk=Head;
            }
            blk=HeadBlock;
            do {
                if (blk->class&0x200) PropIndexes(blk);
                blk=blk->next_block;
            } while (blk!=NULL);
            return 1;
        }
    }
    do_the_twist=1;
    edge=&PreHead->edge[0];
    if (edge->destination!=cond_blk) do_the_twist=0;
    if (cond!=cond_blk->ins.hd.next || cond!=cond_blk->ins.hd.prev) do_the_twist=0;
    if (do_the_twist) {
        new_head=NULL;
        if (know_bounds) {
            RemoveInputEdge(edge);
            NewTarget(edge,loop_edge->destination);
            MoveDownLoop(cond_blk);
            new_head=loop_edge->destination;
        } else if (OptForSize<50 && PreHead->gen_id<cond_blk->gen_id) {
            RemoveInputEdge(edge);
            old_prehead=PreHead;
            PreHead=ReGenBlock(PreHead,NULL);
            for (blk=HeadBlock;blk!=NULL;blk=blk->next_block)
                if (blk->loop_head==old_prehead) blk->loop_head=PreHead;
            for (list=header;list!=NULL;list=list->next)
                if (list->blk==old_prehead) list->blk=PreHead;
            PreHead->class&=~2U;
            PreHead->class|=4;
            dupcond=MakeCondition(cond->head.opcode,cond->operands[0],cond->operands[1],
                cond->flags.byte&15,cond->flags.byte>>4,cond->type_class);
            SuffixPreHeader(dupcond);
            NewTarget(&PreHead->edge[0],cond_blk->edge[0].destination);
            NewTarget(&PreHead->edge[1],cond_blk->edge[1].destination);
            PreHead->edge[0].flags=8;
            PreHead->edge[1].flags=8;
            MoveDownLoop(cond_blk);
            new_head=loop_edge->destination;
        }
        if (new_head!=NULL && new_head!=Head) {
            for (blk=Loop;blk!=NULL;blk=blk->u.loop)
                if (blk->loop_head==Head) blk->loop_head=new_head;
            new_head->loop_head=Head->loop_head;
            Head->loop_head=new_head;
            Head->class&=~0x100U;
            Head=new_head;
            Head->class|=0x100;
        }
    }
    if (do_the_twist || know_bounds) {
        UnMarkLoop();
        MarkLoop();
        MarkWillExecBlocks();
        LoopProtected=1;
        MarkInvariants();
        while (FindInvariants()) ;
        UnMarkInvariants();
        UnMarkWillExecBlocks();
        return 1;
    }
    return 0;
}
unsigned char AnalyseLoop(induction *var,unsigned char *onecond,instruction **cond,block **cond_blk)
{
    block *blk,*first;
    instruction *ins;
    int i;
    unsigned char can_replace,usage;
    *onecond=0;
    *cond=NULL;
    *cond_blk=NULL;
    can_replace=1;
    first=PreHead->edge[0].destination;
    for (blk=Loop;blk!=NULL;blk=blk->u.loop) {
        for (ins=blk->ins.hd.next;ins->head.opcode!=0x4b;ins=ins->head.next) {
            if (var!=NULL && ins==var->ins) continue;
            if (ins->head.opcode<0x2e || ins->head.opcode>0x35 || ins->result!=NULL) {
                if (var!=NULL) {
                    i=ins->num_operands;
                    while (--i>=0) {
                        usage=Uses(ins->operands[i],var->name);
                        if (usage>1) can_replace=0;
                    }
                    if (ins->result!=NULL && Uses(ins->result,var->name)>1) can_replace=0;
                }
            } else if (blk->class&0x400) {
                if (*cond==NULL) {
                    *cond=ins;
                    *onecond=1;
                    *cond_blk=blk;
                } else {
                    *onecond=0;
                    if (first==blk) {
                        *cond=ins;
                        *cond_blk=blk;
                    }
                }
                if (var!=NULL) {
                    i=ins->num_operands;
                    while (--i>=0)
                        if (Uses(ins->operands[i],var->name)==2) can_replace=0;
                }
            } else if (var!=NULL) {
                i=ins->num_operands;
                while (--i>=0)
                    if (Uses(ins->operands[i],var->name)>1) can_replace=0;
            }
        }
    }
    return can_replace;
}
void RevCond(instruction *ins);
long Sgn(long value);
long Abs(long value);
unsigned char InstructionWillExec(instruction *ins);
unsigned char CalcFinalValue(induction *var,block *cond_blk,instruction *cond,long *final,long *initial)
{
    name *temp,*op;
    long init,test,incr,dist,remd;
    unsigned char true_index,false_index;
    if ((var->state&1) || cond==NULL) return 0;
    if (cond->head.opcode==0x2e || cond->head.opcode==0x2f) return 0;
    if (cond->operands[0]->n.class==0) {
        temp=cond->operands[0];
        cond->operands[0]=cond->operands[1];
        cond->operands[1]=temp;
        RevCond(cond);
    }
    true_index=cond->flags.byte&15;
    if (cond_blk->edge[true_index].destination->class&0x200) {
        false_index=cond->flags.byte>>4;
        cond->flags.byte=(true_index<<4)|false_index;
        FlipCond(cond);
    }
    if (cond->head.opcode==0x31) return 0;
    op=cond->operands[1];
    if (op->n.class!=0 || op->c.const_type!=0 || cond->operands[0]!=var->name) return 0;
    op=InitialValue(var->name);
    if (op==NULL || op->c.const_type!=0) return 0;
    init=op->c.int_value;
    *initial=init;
    test=cond->operands[1]->c.int_value;
    incr=var->plus;
    dist=(long)((unsigned long)test-(unsigned long)init);
    if (Sgn(dist)!=Sgn(incr)) return 0;
    remd=(Abs(dist)%Abs(incr))*Sgn(incr);
    /* The actual1994 branches predate the later1996 test<=init / test>=init
     * guards. Arithmetic remains target low-DWORD addition. */
    switch (cond->head.opcode) {
    case 0x30:
        if (remd!=0) return 0;
        *final=test;
        break;
    case 0x32:
        if (incr<=0) return 0;
        *final=(long)((unsigned long)test+(unsigned long)incr-(unsigned long)remd);
        break;
    case 0x35:
        if (incr<=0) return 0;
        *final=remd==0?test:(long)((unsigned long)test+(unsigned long)incr-(unsigned long)remd);
        break;
    case 0x34:
        if (incr>=0) return 0;
        *final=(long)((unsigned long)test+(unsigned long)incr-(unsigned long)remd);
        break;
    case 0x33:
        if (incr>=0) return 0;
        *final=remd==0?test:(long)((unsigned long)test+(unsigned long)incr-(unsigned long)remd);
        break;
    }
    if (InstructionWillExec(var->ins)) {
        Head->iterations=(long)((unsigned long)*final-(unsigned long)*initial)/incr;
        Head->class|=0x10;
    }
    return 1;
}
long Sgn(long value) { STUB_TRAP();return 0; }
long Abs(long value) { STUB_TRAP();return 0; }
unsigned char InstructionWillExec(instruction *ins) { STUB_TRAP();return 0; }
#pragma pack(4)
typedef struct loop_condition {
    unsigned char opcode;
    induction *induction;
    name *invariant;
    block *exit_edge,*loop_edge;
    instruction *compare_ins;
    block *compare_block;
    unsigned char clean,complete;
} loop_condition;
typedef struct loop_abstract { block *head,*tail; } loop_abstract;
#pragma pack()
typedef char old_loop_condition_size[(sizeof(loop_condition)==32)?1:-1];
typedef char old_loop_condition_clean[(offsetof(loop_condition,clean)==28)?1:-1];
/* Released static UnrollCount, safely named here to avoid the existing
 * frontend UnrollCount option global in GCC's undecorated ELF namespace.
 * Registry retains the recovered logical function name. */
long LoopUnrollCount(block *tail,unsigned char *clean,unsigned char *complete);
unsigned char Hoisted(block *head);
unsigned char TractableCond(loop_condition *cond);
void MarkHeaderEdges(block *tail,block *head);
block *DupLoop(block *tail,loop_abstract *copy);
void RedirectHeaderEdges(block *tail,block *head);
void MarkLoopHeader(block *tail,block *head);
block *DoUnroll(block *tail,long reps,unsigned char replace_vars);
void MakeWorldGoAround(block *last,loop_abstract *copy,loop_condition *cond,long count);
void AddBlocks(block *tail,block *head);
void UnMarkHeaderEdges(block *last);
void FixBlockIds(void);
void ClearCopyPtrs(block *tail);
unsigned char UnRoll(void)
{
    loop_condition cond;
    loop_abstract cleanup_copy;
    block *last;
    long count;
    unsigned char onecond;
    if (Head->class&0x10000) return 0;
    count=LoopUnrollCount(Loop,&cond.clean,&cond.complete);
    if (count<=0) return 0;
    AnalyseLoop(NULL,&onecond,&cond.compare_ins,&cond.compare_block);
    /* Actual1994 Hoisted takes only Head and examines its first ins,
     * rather than taking the later explicit compare_ins argument. */
    if (onecond && Hoisted(Head) && TractableCond(&cond)) {
        MarkHeaderEdges(Loop,Head);
        DupLoop(Loop,&cleanup_copy);
        RedirectHeaderEdges(cleanup_copy.tail,cleanup_copy.head);
        cleanup_copy.head->class|=0x10100;
        MarkLoopHeader(cleanup_copy.tail,cleanup_copy.head);
        Head->class|=0x20000;
        last=DoUnroll(Loop,count,0);
        Head->class&=~0x20000U;
        MarkLoopHeader(last,Head);
        MakeWorldGoAround(last,&cleanup_copy,&cond,count);
        cleanup_copy.head->loop_head=Head->loop_head;
        AddBlocks(last,cleanup_copy.head);
        UnMarkHeaderEdges(last);
    } else if (Head->unroll_count>0) {
        last=DoUnroll(Loop,Head->unroll_count,0);
        MarkLoopHeader(last,Head);
    }
    Head->class|=0x10000;
    FixBlockIds();
    ClearCopyPtrs(Loop);
    return 1;
}
long LoopUnrollCount(block *tail,unsigned char *clean,unsigned char *complete)
{
    long count,num_ins;
    block *blk;
    *complete=0;
    *clean=0;
    count=Head->unroll_count;
    if (count==0) {
        if (!(Model&0x800000UL) || OptForSize!=0) return 0;
        num_ins=0;
        for (blk=tail;blk!=NULL;blk=blk->u.loop) {
            if (blk->class&8) return 0;
            num_ins=(long)((unsigned long)num_ins+(unsigned long)CountIns(blk));
        }
        if (Head->class&0x10) {
            count=32/num_ins;
            while (count!=0) {
                if (Head->iterations%(count+1)==0) break;
                --count;
            }
            if (count>=Head->iterations-1) {
                count=Head->iterations-1;
                *complete=1;
            }
        }
    }
    return count;
}
unsigned char Hoisted(block *head) { STUB_TRAP();return 0; }
unsigned char TractableCond(loop_condition *cond) { STUB_TRAP();return 0; }
void MarkHeaderEdges(block *tail,block *head) { STUB_TRAP(); }
block *DupLoop(block *tail,loop_abstract *copy) { STUB_TRAP();return NULL; }
void RedirectHeaderEdges(block *tail,block *head) { STUB_TRAP(); }
void MarkLoopHeader(block *tail,block *head) { STUB_TRAP(); }
block *DoUnroll(block *tail,long reps,unsigned char replace_vars) { STUB_TRAP();return NULL; }
void MakeWorldGoAround(block *last,loop_abstract *copy,loop_condition *cond,long count) { STUB_TRAP(); }
void AddBlocks(block *tail,block *head) { STUB_TRAP(); }
void UnMarkHeaderEdges(block *last) { STUB_TRAP(); }
void FixBlockIds(void) { STUB_TRAP(); }
void ClearCopyPtrs(block *tail) { STUB_TRAP(); }
void PropIndexes(block *blk) { STUB_TRAP(); }
void NewTarget(block_edge *edge,block *destination) { STUB_TRAP(); }
void MoveDownLoop(block *cond) { STUB_TRAP(); }
block *ReGenBlock(block *blk,struct code_lbl *label) { STUB_TRAP();return NULL; }
void SuffixPreHeader(instruction *ins)
{
    instruction *last;
    last=PreHead->ins.hd.prev;
    while (last->head.opcode==0) {
        if (last->flags.nop_flags&8) break;
        last=last->head.prev;
    }
    SuffixIns(last,ins);
}
void MarkDown(block *blk);
unsigned char PathFrom(instruction *ins1,instruction *ins2,instruction *ins3,unsigned char gen);
void MarkWillExecBlocks(void)
{
    block *blk;
    instruction *nop;
    int i;
    for (blk=Loop;blk!=NULL;blk=blk->u.loop) blk->class|=0x4000;
    for (blk=Loop;blk!=NULL;blk=blk->u.loop) {
        if (!(blk->class&0x400)) continue;
        for (i=blk->targets-1;i>=0;--i) MarkDown(blk->edge[i].destination);
    }
    for (blk=Loop;blk!=NULL;blk=blk->u.loop) {
        if (blk->targets<=1) continue;
        for (i=blk->targets-1;i>=0;--i)
            if (blk->edge[i].destination->inputs==1)
                blk->edge[i].destination->class&=~0x4000U;
    }
    if (Head->ins.hd.prev==Head->ins.hd.next) {
        nop=MakeNop();
        PrefixIns(Head->ins.hd.next,nop);
    } else nop=NULL;
    for (blk=Loop;blk!=NULL;blk=blk->u.loop) {
        if (!(blk->class&0x4000)) continue;
        if (blk->ins.hd.next==(instruction *)&blk->ins ||
            PathFrom(Head->ins.hd.prev,Head->ins.hd.next,blk->ins.hd.next,1))
            blk->class&=~0x4000U;
    }
    if (nop!=NULL) FreeIns(nop);
}
void MarkDown(block *blk)
{
    int i;
    if (!(blk->class&0x200) || blk==Head || !(blk->class&0x4000)) return;
    blk->class&=~0x4000U;
    for (i=blk->targets-1;i>=0;--i) {
        /* Historical one-pointer callback ABI. MarkDown is void and this
         * caller discards SafeRecurse's result; no pointer result is defined. */
        SafeRecurse((recurse_callback)MarkDown,blk->edge[i].destination);
    }
}
void LabelDown(instruction *from,instruction *avoiding,unsigned char go_around);
unsigned char PathFrom(instruction *from,instruction *to,instruction *avoiding,unsigned char go_around)
{
    block *blk;
    instruction *ins;
    unsigned char change,found;
    LabelDown(from,avoiding,go_around);
    do {
        change=0;
        for (blk=Loop;blk!=NULL;blk=blk->u.loop) {
            if (blk->class&0x800) {
                blk->class&=~0x800U;
                change=1;
                LabelDown(blk->ins.hd.next,avoiding,go_around);
            }
        }
    } while (change);
    found=(to->ins_flags&1)!=0;
    for (blk=Loop;blk!=NULL;blk=blk->u.loop) {
        blk->class&=~0x800U;
        for (ins=blk->ins.hd.next;ins->head.opcode!=0x4b;ins=ins->head.next)
            ins->ins_flags&=~1;
    }
    return found;
}
void LabelDown(instruction *from,instruction *avoiding,unsigned char go_around)
{
    block *blk;
    block_edge *edge;
    instruction *ins;
    int i;
    while (from->head.opcode!=0x4b) {
        if (from==avoiding) return;
        from->ins_flags|=1;
        from=from->head.next;
    }
    if (from==avoiding) return;
    blk=((block_ins *)from)->blk;
    edge=&blk->edge[0];
    i=blk->targets;
    while (--i>=0) {
        blk=edge->destination;
        if ((go_around || blk!=Head) && (blk->class&0x200)) {
            ins=blk->ins.hd.next;
            if (ins->head.opcode==0x4b || !(ins->ins_flags&1)) blk->class|=0x800;
            /* Actual602e0..6030a, also in the released source: the edge
             * advances only inside this eligible-successor branch. */
            ++edge;
        }
    }
}
void UnMarkWillExecBlocks(void)
{
    block *blk;
    for (blk=Loop;blk!=NULL;blk=blk->u.loop) blk->class&=~0x4000U;
}

void ZapRegister(hw_reg_set regs);
void ZapTemp(name *op);
void ZapMemory(name *op);
void MarkInvariants(void)
{
    name *op;
    block *blk;
    instruction *ins;
    unsigned char have_call,free_index;
    for (op=Names[2];op!=NULL;op=op->n.next_name)
        op->v.block_usage=(op->v.usage&0x20)?0:2;
    for (op=Names[1];op!=NULL;op=op->n.next_name)
        op->v.block_usage=(op->v.usage&0x20)?0:2;
    for (op=Names[3];op!=NULL;op=op->n.next_name) op->r.reg_index=0;
    have_call=0;
    free_index=0;
    MemChangedInLoop=0;
    for (blk=Loop;blk!=NULL;blk=blk->u.loop) {
        for (ins=blk->ins.hd.next;ins->head.opcode!=0x4b;ins=ins->head.next) {
            if ((ins->head.opcode==0x36 || ins->head.opcode==0x29) && !(ins->flags.call_flags&2)) {
                MemChangedInLoop=1;
                have_call=1;
            }
            ZapRegister(ins->zap->reg);
            op=ins->result;
            if (op==NULL) continue;
            switch (op->n.class) {
            case 3: ZapRegister(op->r.reg);break;
            case 4:
                if (op->i.base==NULL) {
                    if (!(Model&0x20000000UL)) free_index=1;
                } else if (op->i.base->n.class==2) ZapTemp(op->i.base);
                else if (!free_index) ZapMemory(op->i.base);
                MemChangedInLoop=1;
                break;
            case 2:
                if (op->v.usage&8) MemChangedInLoop=1;
                ZapTemp(op);
                break;
            case 1:
                MemChangedInLoop=1;
                if (!free_index) ZapMemory(op);
                break;
            }
        }
    }
    if (have_call || free_index) {
        if (!(Model&0x20000000UL)) {
            for (op=Names[2];op!=NULL;op=op->n.next_name)
                if (op->v.usage&8) op->v.block_usage=1;
            for (op=Names[1];op!=NULL;op=op->n.next_name)
                if (!NameIsConstant(op)) op->v.block_usage=1;
        } else {
            for (op=Names[1];op!=NULL;op=op->n.next_name) {
                if (NameIsConstant(op)) continue;
                switch (op->m.memory_type) {
                case 0:
                    if (FEAttr(op->v.symbol)&0x40) op->v.block_usage=1;
                    break;
                case 2: case 3: case 4: break;
                default: op->v.block_usage=1;break;
                }
            }
        }
    }
}
void ZapRegister(hw_reg_set regs)
{
    name *other;
    for (other=Names[3];other!=NULL;other=other->n.next_name)
        if (other->r.reg&regs) ++other->r.reg_index;
}
void ZapTemp(name *op)
{
    name *alias;
    alias=op;
    do {
        if (TempsOverlap(alias,op))
            alias->v.block_usage=(alias->v.block_usage&2)?9:1;
        /* The actual old body has none of the later LOOP_DEFINITION /
         * MULT_DEFINITION flag updates in the released source. */
        alias=(name *)alias->t.alias;
    } while (alias!=op);
}
void ZapMemory(name *op)
{
    name *other;
    for (other=Names[1];other!=NULL;other=other->n.next_name) {
        if (other->v.symbol==op->v.symbol)
            other->v.block_usage=(other->v.usage&2)?9:1;
    }
}

void ScanForNonBasics(block *blk);
void FindNonBasics(void)
{
    block *blk;
    for (blk=Loop;blk!=NULL;blk=blk->u.loop) ScanForNonBasics(blk);
    MarkSurvivors();
}
void ScanNonBasic(instruction *ins);
void ScanForNonBasics(block *blk)
{
    instruction *ins;
    for (ins=blk->ins.hd.next;ins->head.opcode!=0x4b;ins=ins->head.next) ScanNonBasic(ins);
}
void CheckNonBasic(instruction *ins,induction *var,name *op,unsigned char flip);
induction *FindOrAddIndVar(name *op,unsigned char type_class);
void CheckInvariant(instruction *ins,induction *var,name *op,unsigned char flip);
void ScanNonBasic(instruction *ins)
{
    induction *var;
    name *op;
    if (KillIndVars(ins)) return;
    if (ins->type_class==FS || ins->type_class==FD) return;
    if (ins->head.opcode==0x26) {
        var=FindIndVar(ins->operands[0]);
        CheckNonBasic(ins,var,NULL,0);
        return;
    }
    if (ins->head.opcode!=1 && ins->head.opcode!=3 &&
        ins->head.opcode!=5 && ins->head.opcode!=0x0d) return;
    if (ins->operands[0]->n.class==0) {
        op=ins->operands[0];
        var=FindOrAddIndVar(ins->operands[1],ins->type_class);
        if (op->c.const_type==0) CheckNonBasic(ins,var,op,1);
        else CheckInvariant(ins,var,op,1);
    } else if (ins->operands[1]->n.class==0) {
        op=ins->operands[1];
        var=FindOrAddIndVar(ins->operands[0],ins->type_class);
        if (op->c.const_type==0) CheckNonBasic(ins,var,op,0);
        else CheckInvariant(ins,var,op,0);
    } else if (InvariantOp(ins->operands[0])) {
        var=FindOrAddIndVar(ins->operands[1],ins->type_class);
        CheckInvariant(ins,var,ins->operands[0],1);
    } else if (InvariantOp(ins->operands[1])) {
        var=FindOrAddIndVar(ins->operands[0],ins->type_class);
        CheckInvariant(ins,var,ins->operands[1],0);
    }
}
unsigned char BasicNotRedefined(induction *var,instruction *ins);
void CheckNonBasic(instruction *ins,induction *var,name *op,unsigned char flip)
{
    unsigned long plus,plus2,times,c,mul;
    struct invariant *invar;
    name *ivtimes;
    short lasttimes;
    if (var==NULL || (var->state&1)) return;
    if (ins->result->n.class!=2 && ins->result->n.class!=1) return;
    if (flip && (ins->head.opcode==7 || ins->head.opcode==0x0d)) return;
    if (var->state&8) {
        plus=0;
        plus2=0;
        times=1;
    } else {
        plus2=var->plus2;
        plus=var->plus;
        times=var->times;
        if (!BasicNotRedefined(var,ins)) return;
    }
    invar=var->invar;
    ivtimes=var->ivtimes;
    lasttimes=var->lasttimes;
    if (op!=NULL) c=op->c.int_value;
    switch (ins->head.opcode) {
    case 0x26: case 0x24:
        AddIndVar(ins,ins->result,var,invar,ivtimes,lasttimes,times,plus,plus2,1,ins->type_class);
        break;
    case 1:
        AddIndVar(ins,ins->result,var,invar,ivtimes,lasttimes,times,plus+c,plus2,1,ins->type_class);
        break;
    case 5:
        AddIndVar(ins,ins->result,var,invar,ivtimes,lasttimes,c*times,c*plus,c*plus2,c,ins->type_class);
        break;
    case 3:
        if (flip)
            AddIndVar(ins,ins->result,var,invar,ivtimes,lasttimes,0UL-times,c-plus,0UL-plus2,-1,ins->type_class);
        else
            AddIndVar(ins,ins->result,var,invar,ivtimes,lasttimes,times,plus-c,plus2,1,ins->type_class);
        break;
    case 0x0d:
        mul=1;
        while (c!=0) { --c;mul+=mul; }
        AddIndVar(ins,ins->result,var,invar,ivtimes,lasttimes,mul*times,mul*plus,mul*plus2,mul,ins->type_class);
        break;
    }
}
unsigned char BasicNotRedefined(induction *var,instruction *ins)
{
    instruction *other;
    for (other=var->ins->head.next;;other=other->head.next) {
        if (other->head.opcode==0x4b) return 0;
        if (other==ins) break;
        if (ReDefinedBy(other,var->basic->name)) return 0;
    }
    return 1;
}
induction *FindOrAddIndVar(name *op,unsigned char type_class)
{
    induction *var,*alias;
    var=FindIndVar(op);
    if (var!=NULL) return var;
    if (op->n.class!=2 || op->v.offset!=0) return NULL;
    var=FindIndVar(DeAlias(op));
    if (var==NULL || (var->state&1) || !(var->state&8)) return NULL;
    alias=AddIndVar(var->ins,op,var->prev,var->invar,var->ivtimes,
        var->lasttimes,var->times,var->plus,var->plus2,1,type_class);
    if (var->state&2) alias->state|=2;
    if (var->state&0x20) alias->state|=0x20;
    return alias;
}
struct invariant *NewInvariant(name *op,long times);
void CheckInvariant(instruction *ins,induction *var,name *op,unsigned char flip)
{
    unsigned long plus,plus2,times;
    long iv_mult;
    struct invariant *invar;
    name *ivtimes;
    short lasttimes;
    if (var==NULL || (var->state&1)) return;
    if (ins->result->n.class!=2 && ins->result->n.class!=1) return;
    iv_mult=1;
    ivtimes=var->ivtimes;
    lasttimes=var->lasttimes;
    plus=var->plus;
    plus2=var->plus2;
    times=var->times;
    if (var->state&8) { times=1;plus=0;plus2=0; }
    else if (!BasicNotRedefined(var,ins)) return;
    if (ins->head.opcode!=3) flip=0;
    switch (ins->head.opcode) {
    case 3:
        if (ins->type_class==CP || ins->type_class==PT) return;
        if (flip) {
            times=0UL-times;
            plus=0UL-plus;
            plus2=0UL-plus2;
        } else iv_mult=-1;
        /* fall through */
    case 1:
        invar=NewInvariant(op,iv_mult);
        invar->next=CopyInvariant(var->invar);
        if (invar->next==NULL) invar->id=1;
        else invar->id=invar->next->id+1;
        if (flip) MulInvariant(invar->next,-1);
        break;
    case 5:
        if (var->ivtimes!=NULL) return;
        ivtimes=op;
        plus2=plus;
        plus=0;
        invar=CopyInvariant(var->invar);
        if (invar!=NULL) lasttimes=invar->id;
        else lasttimes=0;
        break;
    default: return;
    }
    AddIndVar(ins,ins->result,var,invar,ivtimes,lasttimes,times,plus,plus2,1,ins->type_class);
    FreeInvariant(invar);
}
struct invariant *NewInvariant(name *op,long times)
{
    struct invariant *invar;
    invar=CGAlloc(sizeof(struct invariant));
    invar->name=op;
    invar->times=times;
    invar->next=NULL;
    invar->id=0;
    return invar;
}

void AdjustIndex(induction *var,induction *replacement);
void MergeVars(void)
{
    induction *var,*other;
    instruction *ins;
    block *varblock;
    for (var=IndVarList;var!=NULL;var=var->next) {
        if (!(var->state&0x10) || (var->state&1) || (var->state&0x0c)) continue;
        ins=var->ins;
        while (ins->head.opcode!=0x4b) ins=ins->head.next;
        varblock=((block_ins *)ins)->blk;
        for (other=var->next;other!=NULL;other=other->next) {
            if (!(other->state&0x10) || (other->state&1) || (other->state&0x0c)) continue;
            if (DifferentIV(var,other)!=2 || (other->state&0x80) || !(other->state&0x40)) continue;
            ins=other->ins;
            while (ins->head.opcode!=0x4b) ins=ins->head.next;
            if (((block_ins *)ins)->blk!=varblock) continue;
            /* Actual5fc5b..5fc61 compares these DWORD IDs signed. */
            if ((int)var->ins->id<(int)other->ins->id) {
                if (!BasicNotRedefined(var,other->ins)) continue;
                AdjustIndex(other,var);
            } else {
                if (!BasicNotRedefined(other,var->ins)) continue;
                AdjustIndex(var,other);
                var=var->next;
                break;
            }
        }
    }
}
void AdjustIndex(induction *var,induction *replacement) { STUB_TRAP(); }

unsigned char ReduceVar(induction *var);
unsigned char ReduceInStrength(void)
{
    induction *var;
    unsigned char change;
    change=0;
    for (var=IndVarList;var!=NULL;var=var->next) change|=ReduceVar(var);
    return change;
}
unsigned char IsAddressMode(induction *var);
void IncAndInit(induction *var,name *temp,unsigned char type_class);
unsigned char NoPathThru(instruction *from,instruction *to,instruction *through);
void ReplaceOccurences(name *old,name *replacement);
unsigned char ReduceVar(induction *var)
{
    induction *alias,*new_var;
    name *temp;
    instruction *ins;
    unsigned char type_class;
    if (!(var->state&0x10) || (var->state&1)) return 0;
    while (var->ins->num_operands==1) var=FindIndVar(var->ins->operands[0]);
    if ((var->state&0x0c) || IsAddressMode(var)) return 0;
    type_class=var->type_class;
    temp=AllocTemp(type_class);
    temp->t.temp_flags|=0x80;
    ins=MakeMove(temp,var->name,type_class);
    ReplIns(var->ins,ins);
    var->ins=ins;
    IncAndInit(var,temp,type_class);
    for (alias=var->alias;alias!=var;alias=alias->alias) {
        if (NoPathThru(var->ins,alias->ins,var->basic->ins) &&
            !(alias->name->v.usage&2) && !(var->name->v.usage&2)) {
            ReplaceOccurences(alias->name,var->name);
            ins=MakeMove(temp,alias->name,type_class);
            PrefixIns(alias->ins,ins);
            ins=MakeMove(temp,var->name,type_class);
        } else ins=MakeMove(temp,alias->name,type_class);
        ReplIns(alias->ins,ins);
        alias->ins=ins;
    }
    new_var=AddIndVar(var->basic->ins->head.prev,temp,var->basic,var->invar,
        var->ivtimes,var->lasttimes,var->times,var->plus,var->plus2,1,ins->type_class);
    new_var->state|=0x20;
    return 1;
}
unsigned char IsAddressMode(induction *var)
{
    /* The released loopopts.c explicitly disables this optimization.
     * Actual5f542 is XOR AL,AL; RET, with no owner read. */
    (void)var;
    return 0;
}
instruction *Multiply(name *op,long by,name *temp,unsigned char type_class,instruction *prev);
instruction *MakeMul(instruction *prev,name *op,long by,name *ivtimes);
name *FindPointerPart(induction *var);
void IncAndInit(induction *var,name *iv,unsigned char type_class)
{
    instruction *ins;
    struct invariant *invar;
    name *first,*temp;
    induction *basic;
    basic=var->basic;
    if (var->ivtimes!=NULL) {
        temp=AllocTemp(var->ivtimes->n.name_class);
        ins=Multiply(var->ivtimes,(long)((unsigned long)var->times*(unsigned long)basic->plus),
            temp,temp->n.name_class,PreHead->ins.hd.prev);
        temp=ins->result;
        temp->t.temp_flags|=0x80;
    } else temp=AllocS32Const((long)((unsigned long)var->times*(unsigned long)basic->plus));
    ins=MakeBinary(1,iv,temp,iv,type_class);
    PrefixIns(basic->ins,ins);
    temp=AllocTemp(type_class);
    if (type_class!=CP && type_class!=PT) first=NULL;
    else first=FindPointerPart(var);
    if (first!=NULL) {
        ins=MakeBinary(1,first,AllocS32Const(var->plus),temp,type_class);
        SuffixPreHeader(ins);
    }
    if (first!=basic->name) {
        ins=MakeMul(PreHead->ins.hd.prev,basic->name,var->times,var->ivtimes);
        if (first==NULL) ins->result=temp;
        else {
            ins=MakeBinary(1,temp,ins->result,temp,type_class);
            SuffixPreHeader(ins);
        }
    }
    if (first==NULL) {
        ins=MakeBinary(1,temp,AllocS32Const(var->plus),temp,type_class);
        SuffixPreHeader(ins);
    }
    for (invar=var->invar;invar!=NULL;invar=invar->next) {
        if (invar->name==first) continue;
        if (invar->times==1 && invar->id>var->lasttimes) {
            ins=MakeBinary(1,temp,invar->name,temp,type_class);
            SuffixPreHeader(ins);
        } else {
            ins=MakeMul(PreHead->ins.hd.prev,invar->name,invar->times,
                invar->id>var->lasttimes?NULL:var->ivtimes);
            ins=MakeBinary(1,temp,ins->result,temp,type_class);
            SuffixPreHeader(ins);
        }
    }
    if (var->plus2!=0) {
        ins=Multiply(var->ivtimes,var->plus2,AllocTemp(var->ivtimes->n.name_class),
            var->ivtimes->n.name_class,PreHead->ins.hd.prev);
        ins=MakeBinary(1,temp,ins->result,iv,type_class);
    } else ins=MakeMove(temp,iv,type_class);
    SuffixPreHeader(ins);
}
instruction *Multiply(name *op,long by,name *temp,unsigned char type_class,instruction *prev)
{
    unsigned char negative;
    int log2;
    instruction *ins;
    negative=by<0;
    if (negative) by=(long)(0UL-(unsigned long)by);
    log2=GetLog2(by);
    if (log2==0) ins=MakeMove(op,temp,type_class);
    else if (log2==-1) ins=MakeBinary(5,op,AllocS32Const(by),temp,type_class);
    else ins=MakeBinary(0x0d,op,AllocS32Const(log2),temp,type_class);
    SuffixIns(prev,ins);
    if (negative) {
        prev=MakeUnary(0x11,temp,temp,op->n.name_class);
        SuffixIns(ins,prev);
        ins=prev;
    }
    return ins;
}
instruction *MakeMul(instruction *prev,name *op,long by,name *ivtimes)
{
    instruction *ins;
    name *temp;
    unsigned char type_class;
    type_class=op->n.name_class;
    temp=AllocTemp(type_class);
    ins=Multiply(op,by,temp,type_class,prev);
    if (ivtimes!=NULL) {
        prev=MakeBinary(5,temp,ivtimes,temp,type_class);
        SuffixIns(ins,prev);
        ins=prev;
    }
    return ins;
}
name *FindPointerPart(induction *var) { STUB_TRAP();return NULL; }
unsigned char NoPathThru(instruction *from,instruction *to,instruction *through)
{
    if (PathFrom(through,to,from,0) && PathFrom(from,through,to,0)) return 0;
    if (PathFrom(to,through,from,0) && PathFrom(through,from,to,0)) return 0;
    return 1;
}
void ReplaceOccurences(name *old,name *replacement) { STUB_TRAP(); }

unsigned char ListContainsVar(name *list,name *value);
void FreeBadVars(void)
{
    induction **owner,*var;
    owner=&IndVarList;
    for (;;) {
        var=*owner;
        if (var==NULL) break;
        if (ListContainsVar(Names[2],var->name) || ListContainsVar(Names[1],var->name)) {
            owner=&var->next;
        } else {
            *owner=var->next;
            FreeVar(var);
        }
    }
}
unsigned char ListContainsVar(name *list,name *value)
{
    while (list!=NULL) {
        if (list==value) return 1;
        list=list->n.next_name;
    }
    /* Actual5f537 tests EAX before the backwards list-advance chunk at
     *5f535. Its NULL path falls into known IsAddressMode5f542, the
     * folded constant-false body. EAX is genuinely NULL here. */
    return IsAddressMode(NULL);
}

induction *FindReplacement(induction *var);
void Replace(induction *var,induction *replacement);
void ElimIndVars(void)
{
    induction *var,*next,*replacement;
    for (var=IndVarList;var!=NULL;var=next) {
        next=var->next;
        if (!(var->state&0x28)) var->state|=1; /* BASIC | INTRODUCED */
    }
    for (var=IndVarList;var!=NULL;var=next) {
        next=var->next;
        if (!(var->state&0x100) && !(var->state&1) && (var->state&8)) {
            replacement=FindReplacement(var);
            if (replacement!=NULL) Replace(var,replacement);
        }
    }
}
induction *FindReplacement(induction *var)
{
    induction *replacement,*other;
    unsigned log2rep,log2other;
    unsigned char varclass,otherclass;
    if (var->ivtimes!=NULL) return NULL;
    replacement=NULL;
    varclass=Unsigned[var->name->n.name_class];
    if (Unsigned[var->ins->type_class]!=varclass) return NULL;
    for (other=IndVarList;other!=NULL;other=other->next) {
        if ((other->state&1) || !(other->state&0x20) || other==var) continue;
        if (other->ivtimes!=NULL || other->basic!=var) continue;
        otherclass=Unsigned[other->name->n.name_class];
        /* The segmented1994 body admits U4 for CP, without the later
         * nonsegmented PT interchange cases. */
        if (otherclass==varclass || (otherclass==U4 && varclass==CP)) {
            log2other=GetLog2(other->times);
            if (replacement==NULL || (replacement->invar!=NULL && other->invar==NULL) ||
                log2other<log2rep || (log2other==log2rep && other->plus<replacement->plus)) {
                replacement=other;
                log2rep=GetLog2(replacement->times);
            }
        }
    }
    return replacement;
}
unsigned char FinalValue(instruction *ins,block *blk,induction *var);
void ReplUses(induction *var,induction *replacement,instruction *ins,unsigned char type_class);
void Replace(induction *var,induction *replacement)
{
    block *blk,*cond_blk;
    instruction *ins,*cond;
    unsigned char onecond,type_class;
    if (!AnalyseLoop(var,&onecond,&cond,&cond_blk)) return;
    type_class=replacement->type_class;
    /* Actual60ebb..60ecf frees the old update before rewriting compares.
     * The later source postpones it and permits ReplUses to fail. */
    if (onecond && FinalValue(cond,cond_blk,var)) FreeIns(var->ins);
    for (blk=Loop;blk!=NULL;blk=blk->u.loop) {
        for (ins=blk->ins.hd.next;ins->head.opcode!=0x4b;ins=ins->head.next)
            if (ins->head.opcode>=0x30 && ins->head.opcode<=0x35)
                ReplUses(var,replacement,ins,type_class);
    }
    var->state|=1;
}
unsigned char FinalValue(instruction *ins,block *blk,induction *var)
{
    instruction *other;
    name *constant;
    long final,initial;
    unsigned char type_class,true_index,false_index;
    block *destination;
    if (!CalcFinalValue(var,blk,ins,&final,&initial)) return 0;
    true_index=ins->flags.byte&15;
    false_index=ins->flags.byte>>4;
    destination=blk->edge[true_index].destination;
    if ((destination->class&0x2000) || (destination->class&0x100)) return 0;
    /* Actual1994 uses the variable's class and always inserts the exit
     * assignment. The later source adds ins->type_class and inputs==1. */
    type_class=var->name->n.name_class;
    constant=AllocS32Const(final);
    other=MakeCondition(0x30,var->name,constant,true_index,false_index,type_class);
    ReplIns(ins,other);
    ins=MakeMove(constant,var->name,type_class);
    SuffixIns((instruction *)&destination->ins,ins);
    return 1;
}
unsigned char DangerousTypeChange(induction *var,induction *replacement);
void RepBoth(instruction *ins,induction *replacement,unsigned char type_class);
void RepIndVar(instruction *ins,induction *replacement,unsigned replace_index,unsigned invariant_index,unsigned char type_class);
void ReplUses(induction *var,induction *replacement,instruction *ins,unsigned char type_class)
{
    unsigned char use0,use1;
    if (ins->head.opcode==0x30 || !DangerousTypeChange(var,replacement)) {
        use0=Uses(ins->operands[0],var->name);
        use1=Uses(ins->operands[1],var->name);
        if (use0==3) {
            if (use1==3) RepBoth(ins,replacement,type_class);
            else if (use1==0) RepIndVar(ins,replacement,0,1,type_class);
        } else if (use1==3 && use0==0) RepIndVar(ins,replacement,1,0,type_class);
    }
}
unsigned char DangerousTypeChange(induction *var,induction *replacement)
{
    struct invariant *invar;
    if (var->type_class==replacement->type_class) return 0;
    /* This is the released PointerOk source role, folded in1994 into
     * physical IsAddressMode5f542. The explicit data-pointer cast only
     * transports the unused pointer to proved XOR AL,AL;RET; it does
     * not claim an induction owner or a different original signature. */
    if (IsAddressMode((induction *)(void *)replacement->name)) return 0;
    for (invar=replacement->invar;invar!=NULL;invar=invar->next)
        if (IsAddressMode((induction *)(void *)invar->name)) return 0;
    return 1;
}
void RepBoth(instruction *ins,induction *replacement,unsigned char type_class) { STUB_TRAP(); }
void DoReplacement(instruction *ins,induction *replacement,unsigned replace_index,unsigned invariant_index,unsigned char type_class);
void RepIndVar(instruction *ins,induction *replacement,unsigned replace_index,unsigned invariant_index,unsigned char type_class)
{
    if (InvariantOp(ins->operands[invariant_index]))
        DoReplacement(ins,replacement,replace_index,invariant_index,type_class);
}
void DoReplacement(instruction *ins,induction *replacement,unsigned replace_index,unsigned invariant_index,unsigned char type_class)
{
    name *op;
    instruction *prev,*new_ins;
    struct invariant *invar;
    ins->type_class=type_class;
    ins->operands[replace_index]=replacement->name;
    op=ins->operands[invariant_index];
    prev=PreHead->ins.hd.prev;
    if (op->n.class==0 && op->c.const_type==0) {
        /* The old body folds in one DWORD, before the later signed64
         * overflow checks and AllocS64Const source changes. */
        ins->operands[invariant_index]=AllocS32Const((long)((unsigned long)op->c.int_value*
            (unsigned long)replacement->times+(unsigned long)replacement->plus));
    } else {
        new_ins=MakeMul(prev,op,replacement->times,NULL);
        prev=new_ins;
        new_ins=MakeBinary(1,new_ins->result,AllocS32Const(replacement->plus),new_ins->result,type_class);
        ins->operands[invariant_index]=new_ins->result;
        SuffixIns(prev,new_ins);
        prev=new_ins;
    }
    if (replacement->times<0) RevCond(ins);
    for (invar=replacement->invar;invar!=NULL;invar=invar->next) {
        new_ins=MakeMul(prev,invar->name,invar->times,NULL);
        prev=new_ins;
        if (DifferentClasses(new_ins->result->n.name_class,type_class)) {
            new_ins=MakeConvert(new_ins->result,AllocTemp(type_class),
                type_class,ins->operands[invariant_index]->n.name_class);
            SuffixIns(prev,new_ins);
            prev=new_ins;
        }
        new_ins=MakeBinary(1,new_ins->result,ins->operands[invariant_index],AllocTemp(type_class),type_class);
        ins->operands[invariant_index]=new_ins->result;
        SuffixIns(prev,new_ins);
        prev=new_ins;
    }
}

void UnMarkInvariants(void)
{
    name *op;
    for (op=Names[2];op!=NULL;op=op->n.next_name) op->v.block_usage=0;
    for (op=Names[1];op!=NULL;op=op->n.next_name) op->v.block_usage=0;
}

void UnMarkLoop(void)
{
    block *blk;
    for (blk=HeadBlock;blk!=NULL;blk=blk->next_block) {
        blk->u.loop=NULL;
        blk->class&=~0x600U; /* IN_LOOP | LOOP_EXIT */
    }
}

unsigned char MaxDepth(void)
{
    block *blk;
    unsigned char depth;
    blk=HeadBlock;
    depth=0;
    while (blk!=NULL) {
        if (blk->depth>depth) depth=blk->depth;
        blk=blk->next_block;
    }
    return depth;
}

unsigned char DoLoopInvariant(unsigned char (*rtn)(void))
{
    unsigned char depth,i,change;
    block *blk;
    change=0;
    i=1;
    depth=MaxDepth();
    while (i<=depth) {
        blk=HeadBlock;
        while (blk!=NULL) {
            if ((blk->class&0x100) && blk->depth==i) {
                DGBlip(); /* Actual physical LPBlip hook. */
                Head=blk;
                MarkLoop();
                LoopProtected=0;
                MarkInvariants();
                while (rtn()) change=1;
                UnMarkInvariants();
                UnMarkLoop();
            }
            blk=blk->next_block;
        }
        ++i;
    }
    return change;
}

unsigned char IndVars(void)
{
    return Induction(1,0);
}

block *NextInLoop(block *blk);
void ConstToTemp(block *pre,block *loop,block *(*next)(block *));
void LoopEnregister(void)
{
    unsigned char i;
    block *blk;
    i=MaxDepth();
    while (i>=1) {
        blk=HeadBlock;
        while (blk!=NULL) {
            if ((blk->class&0x100) && blk->depth==i) {
                Head=blk;
                MarkLoop();
                ConstToTemp(PreHead,Loop,NextInLoop);
                UnMarkLoop();
            }
            blk=blk->next_block;
        }
        --i;
    }
}
block *NextInLoop(block *blk)
{
    return blk->u.loop;
}

unsigned char InvariantReg(name *op)
{
    return op->n.class==3 && op->r.reg_index==0;
}
void UpdateLoopLiveInfo(instruction *ins)
{
    hw_reg_set reg;
    block *blk;
    int i;
    reg=ins->result->r.reg;
    for (i=0;i<ins->num_operands;++i) {
        if (ins->operands[i]->n.class==3) reg|=ins->operands[i]->r.reg;
    }
    for (;ins->head.opcode!=0x4b;ins=ins->head.next)
        ins->head.live.regs|=reg;
    for (blk=Loop;blk!=NULL;blk=blk->u.loop) {
        ins=(instruction *)&blk->ins;
        do {
            ins->head.live.regs|=reg;
            ins=ins->head.next;
        } while (ins->head.opcode!=0x4b);
    }
}
unsigned char FindRegInvariants(void)
{
    block *blk;
    instruction *ins,*next;
    int i;
    unsigned char change;
    change=0;
    for (blk=Loop;blk!=NULL;blk=blk->u.loop) {
        for (ins=blk->ins.hd.next;ins->head.opcode!=0x4b;ins=next) {
            next=ins->head.next;
            if (ins->result==NULL || ins->result->n.class!=3) continue;
            if (ins->result->r.reg&Head->ins.hd.next->head.live.regs) continue;
            if (ins->result->r.reg_index!=1) continue;
            if (SideEffect(ins)) continue;
            if (ins->head.opcode==0x36 || ins->head.opcode==0x29) continue;
            if (ins->head.opcode>=0x2e && ins->head.opcode<=0x35) continue;
            for (i=0;i<ins->num_operands;++i) {
                if (ins->zap->reg!=0) break;
                if (InvariantReg(ins->operands[i])) continue;
                if (!InvariantOp(ins->operands[i])) break;
            }
            if (i!=ins->num_operands) continue;
            next->head.prev=ins->head.prev;
            ins->head.prev->head.next=next;
            SuffixPreHeader(ins);
            UpdateLoopLiveInfo(ins);
            change=1;
        }
    }
    return change;
}
unsigned char LoopRegInvariant(void)
{
    return DoLoopInvariant(FindRegInvariants);
}

unsigned char OpLTZero(name *op,unsigned char fp);
unsigned char OpEQZero(name *op,unsigned char fp);
unsigned char Hoistable(instruction *ins,void *data)
{
    block *blk;
    unsigned char will_execute,dangerous,fp,value;
    int i;
    blk=data;
    will_execute=blk!=NULL && LoopProtected && (blk->class&0x4000);
    dangerous=0;
    fp=ins->type_class==FS || ins->type_class==FD;
    if (fp && !(TargetModel&1)) dangerous=1;
    else {
        for (i=0;i<ins->num_operands;++i)
            if (ins->operands[i]->n.class==4) dangerous=1;
        if (ins->result!=NULL && ins->result->n.class==4) dangerous=1;
    }
    switch (ins->head.opcode) {
    case 7: case 8: case 0x10: /* DIV, MOD, FMOD */
        value=OpEQZero(ins->operands[1],fp);
        if (value==0) return 0;
        if (value==2) dangerous=1;
        break;
    case 0x17: /* SQRT */
        value=OpLTZero(ins->operands[0],fp);
        if (value==0) return 0;
        if (value==2) dangerous=1;
        break;
    case 0x13: case 0x1e: /* LOG, LOG10 */
        value=OpLTZero(ins->operands[0],fp);
        if (value==0) return 0;
        value=OpEQZero(ins->operands[0],fp);
        if (value==0) return 0;
        if (value==2) dangerous=1;
        break;
    case 0x0e: dangerous=1;break; /* POW */
    case 0x24: case 0x25: case 0x26: /* ROUND, CONVERT, MOV */
        if (ins->operands[0]->n.class!=4) return 0;
        break;
    default:
        if (ins->num_operands!=2 || ins->result==NULL) return 0;
        break;
    }
    if (dangerous && !will_execute) return 0;
    return 1;
}
unsigned char OpLTZero(name *op,unsigned char fp) { STUB_TRAP();return 0; }
unsigned char OpEQZero(name *op,unsigned char fp)
{
    if (op->n.class!=0) return 2; /* CMP_UNKNOWN */
    if (op->c.const_type!=0) return 1; /* CMP_FALSE, including relocations */
    if (fp && CFTest(op->c.value)==0) return 0;
    if (!fp && op->c.int_value==0) return 0;
    return 1;
}

void ReplaceAllOccurences(name *old,name *replacement);
void CommonInvariant(void)
{
    block *blk;
    instruction *ins;
    name *op,*result;
    for (blk=HeadBlock;blk!=NULL;blk=blk->next_block) {
        for (ins=blk->ins.hd.next;ins->head.opcode!=0x4b;ins=ins->head.next) {
            op=ins->operands[0];
            result=ins->result;
            if (ins->head.opcode==0x26 && op->n.class==2 && (op->t.temp_flags&1) &&
                result->n.class==2 && (result->t.temp_flags&1) && !BlockByBlock) {
                /* The old body has no later MULT_DEFINITION exclusion. */
                ReplaceAllOccurences(result,op);
                op->t.temp_flags|=0x80;
            }
        }
    }
    for (op=Names[2];op!=NULL;op=op->n.next_name) op->t.temp_flags&=~1;
}
void ReplaceAllOccurences(name *old,name *replacement) { STUB_TRAP(); }
