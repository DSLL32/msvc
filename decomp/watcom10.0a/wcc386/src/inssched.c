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


#include <limits.h>
#include "block.h"
#include "cg-sched.h"
#include "cgmemory.h"
#include "genopt.h"
#include "stubs.h"
#include <recon.h>

struct dep_list_entry;
#pragma pack(4)
typedef struct data_dag {
    struct data_dag *prev;
    instruction *ins;
    struct dep_list_entry *deps;
    struct data_dag *ready;
    unsigned stallable:8,visited:1,scheduled:1;
    int height,anc_count;
} data_dag;
#pragma pack()
typedef char old_data_dag_size[(sizeof(data_dag)==28)?1:-1];
typedef struct dep_list_entry {
    struct dep_list_entry *next;
    data_dag *dep;
} dep_list_entry;
typedef struct dep_list_block {
    struct dep_list_block *next;
    unsigned used;
    dep_list_entry entries[20];
} dep_list_block;
typedef char old_dep_list_block_size[(sizeof(dep_list_block)==168)?1:-1];
static dep_list_block *CurrDepBlock;
data_dag *DataDag;
static void *DepFrl;
static block *SBlock;
void FreeDataDag(void);
unsigned InsStallable(instruction *ins)
{
    unsigned stallable;
    int i;
    stallable=0;
    for (i=ins->num_operands-1;i>=0;--i) {
        switch (ins->operands[i]->n.class) {
        case 4: stallable+=3;break;
        case 3: stallable+=2;break;
        case 1: ++stallable;break;
        }
    }
    if (ins->result!=NULL && ins->result->n.class==4) stallable+=3;
    return stallable;
}
void InitDag(void)
{
    data_dag *dag,*head;
    instruction *ins;
    head=NULL;
    for (ins=SBlock->ins.hd.next;ins->head.opcode!=0x4b;ins=ins->head.next) {
        ins->ins_flags&=~0x80U;
        switch (ins->head.opcode) {
        case 1: case 3:
            if (ins->operands[0]==ins->result && ins->operands[0]->n.class==3
              && ins->operands[1]->n.class==0 && ins->operands[1]->c.const_type==0
              && (ins->type_class==U4 || ins->type_class==I4)) ins->ins_flags|=0x80;
            break;
        }
        dag=CGAlloc(sizeof(data_dag));
        dag->ins=ins;
        dag->height=0;
        dag->anc_count=0;
        dag->deps=NULL;
        dag->visited=0;
        dag->stallable=InsStallable(ins);
        dag->prev=head;
        head=dag;
    }
    DataDag=head;
}
void FPPreSched(block *blk);
dep_list_entry *AllocDep(void)
{
    dep_list_block *new;
    if (CurrDepBlock==NULL || CurrDepBlock->used>=20) {
        new=AllocFrl(&DepFrl,sizeof(dep_list_block));
        new->next=CurrDepBlock;
        new->used=0;
        CurrDepBlock=new;
    }
    return &CurrDepBlock->entries[CurrDepBlock->used++];
}
void BuildLink(data_dag *after,data_dag *before)
{
    dep_list_entry *dep;
    dep=AllocDep();
    dep->next=after->deps;
    after->deps=dep;
    dep->dep=before;
}
unsigned char StackOp(instruction *ins)
{
    int i;
    hw_reg_set sp;
    name *op;
    switch (ins->head.opcode) {
    case 0x36: case 0x29: case 0x2a: case 0x2b: return 1;
    }
    sp=StackReg();
    for (i=ins->num_operands-1;i>=0;--i) {
        op=ins->operands[i];
        if (op->n.class==4) op=op->i.index;
        if (op->n.class==3 && (sp&op->r.reg)) return 1;
    }
    op=ins->result;
    if (op==NULL) return 0;
    if (op->n.class==4) op=op->i.index;
    return op->n.class==3 && (sp&op->r.reg)!=0;
}
unsigned char FPStackIns(instruction *ins);
unsigned char FPInsIntroduced(instruction *ins);
unsigned char OkToSlide(instruction *ins,name *op)
{
    int i;
    if (op->i.base!=NULL || op->i.constant!=0) return 1;
    if (OptForSize>=50) return 0;
    for (i=ins->num_operands-1;i>=0;--i)
        if (ins->operands[i]->n.class==0) return 0;
    return 1;
}
unsigned char ReallyDefinedBy(instruction *after,instruction *before,name *op,unsigned char linked)
{
    unsigned char redefd;
    instruction *ins;
    redefd=ReDefinedBy(after,op);
    if (redefd!=2 || !linked) return redefd;
    if ((int)after->id>(int)before->id) {
        ins=after;
        after=before;
        before=ins;
    }
    while (after!=before) {
        if (ReDefinedBy(after,op->i.index)) return 1;
        after=after->head.next;
    }
    return 0;
}
hw_reg_set FullReg(hw_reg_set regs);
unsigned char HiddenDependancy(instruction *ins,name *op)
{
    int i;
    hw_reg_set full;
    if (FPStackIns(ins) || op->n.class!=3) return 0;
    full=FullReg(op->r.reg);
    op=ins->result;
    for (i=0;i<ins->num_operands;++i) {
        /* Preserve the original walk: these continues do not advance op. */
        if (op==NULL) continue;
        if (op->n.class!=3) continue;
        if (full&op->r.reg) return 1;
        op=ins->operands[i];
    }
    return 0;
}
unsigned char FPStackReg(name *op);
unsigned char CheckOneOp(instruction *after,instruction *before,name *op,unsigned char linked)
{
    if (op->n.class==4 && op->i.index!=NULL) {
        if (linked && (after->ins_flags&0x80)
          && (after->result->r.reg&op->i.index->r.reg) && OkToSlide(before,op)) return 0;
        if (ReDefinedBy(after,op->i.index)) return 2;
    }
    if (ReallyDefinedBy(after,before,op,linked)) {
        if (!FPStackReg(op) || after->sequence==before->sequence) return 1;
    }
    if (HiddenDependancy(after,op)) return 1;
    return 0;
}
unsigned char DataDependant(instruction *after,instruction *before,unsigned char linked)
{
    int k;
    unsigned char ret;
    ret=0;
    for (k=before->num_operands-1;k>=0;--k) {
        if (k==0 && before->head.opcode==0x28 && before->operands[0]->n.class!=4) continue;
        ret=CheckOneOp(after,before,before->operands[k],linked);
        if (ret!=0) return ret;
    }
    if (before->result!=NULL) {
        ret=CheckOneOp(after,before,before->result,linked);
        if (ret!=0) return ret;
    }
    if ((Model&0x20000000U) && (before->head.opcode==0x36 || before->head.opcode==0x29)) {
        if (!linked) return 1;
        for (;;) {
            before=before->head.next;
            if (before->head.opcode==0x4b || before->head.opcode==0x36 || before->head.opcode==0x29) break;
            if (before->head.opcode==0 && before->result!=NULL) {
                ret=DataDependant(after,before,linked);
                if (ret!=0) break;
            }
        }
    }
    return ret;
}
unsigned char IsSegReg(hw_reg_set regs);
unsigned char ImplicitDependancy(instruction *imp,instruction *ins)
{
    name *op;
    int i;
    op=imp->result;
    if (op==NULL || op->n.class!=3 || !IsSegReg(op->r.reg)) return 0;
    if (ins->head.opcode==0x36 || ins->head.opcode==0x29) return 1;
    i=ins->num_operands;
    while (--i>=0) {
        op=ins->operands[i];
        if (op->n.class==1 || op->n.class==4) return 1;
    }
    op=ins->result;
    return op!=NULL && (op->n.class==1 || op->n.class==4);
}
unsigned char InsOrderDependant(instruction *after,instruction *before)
{
    unsigned char opcode;
    if (before->head.opcode==0 && before->result==NULL && !DoesSomething(before)) return 1;
    opcode=after->head.opcode;
    if ((opcode>=0x2e && opcode<=0x35) || opcode==0x2d) return 1;
    if (opcode==0x36 || opcode==0x29) {
        if (!(after->flags.call_flags&1) && before->result!=NULL
          && VisibleToCall(after,before->result,0)) return 1;
        if (FPStackIns(before)) return 1;
    }
    if (StackOp(after) && StackOp(before)) return 1;
    if (FPStackIns(after)) {
        if (FPStackIns(before)) {
            if (after->sequence==before->sequence || FPInsIntroduced(before)
              || FPInsIntroduced(after)) return 1;
        }
        if (before->head.opcode==0x36 || before->head.opcode==0x29) return 1;
    }
    if (DataDependant(after,before,1) || DataDependant(before,after,1)) return 1;
    if (ImplicitDependancy(after,before) || ImplicitDependancy(before,after)) return 1;
    return 0;
}
unsigned char MultiIns(instruction *ins) { STUB_TRAP();return 0; }
unsigned char InsUsesCC(instruction *ins)
{
    switch (ins->head.opcode) {
    case 2: case 4: case 6: return 1; /* Extended carry-dependent operations. */
    }
    return ins->head.opcode>=0x2e && ins->head.opcode<=0x35 &&
           ins->u.gen_table->generate==0;
}
void BuildDag(void)
{
    data_dag *dag_i,*dag_j;
    unsigned char link_multi,link_all,used_cc;
    for (dag_i=DataDag;dag_i!=NULL;dag_i=dag_i->prev) {
        used_cc=(dag_i->ins->ins_flags&0x20)!=0;
        link_all=0;
        link_multi=0;
        switch (dag_i->ins->head.opcode) {
        case 2: case 4: case 6: link_multi=1;break;
        case 0: link_all=1;break;
        }
        for (dag_j=dag_i->prev;dag_j!=NULL;dag_j=dag_j->prev) {
            if (link_all || InsOrderDependant(dag_i->ins,dag_j->ins)) {
                BuildLink(dag_i,dag_j);
            } else if (link_multi && MultiIns(dag_j->ins)) {
                link_multi=0;
                BuildLink(dag_i,dag_j);
            } else if (used_cc && (dag_j->ins->u.gen_table->op_type&0xc000)!=0x8000) {
                BuildLink(dag_i,dag_j);
            } else if ((dag_i->ins->u.gen_table->op_type&0xc000)!=0x8000 && InsUsesCC(dag_j->ins)) {
                BuildLink(dag_i,dag_j);
            }
        }
    }
}
void *AnnointADag(void *owner)
{
    unsigned max_cycle_count;
    data_dag *dag,*pred;
    dep_list_entry *dep;
    dag=owner;
    max_cycle_count=0;
    dag->visited=1;
    for (dep=dag->deps;dep!=NULL;dep=dep->next) {
        pred=dep->dep;
        ++pred->anc_count;
        if (!pred->visited) SafeRecurse(AnnointADag,pred);
        if ((unsigned)pred->height>max_cycle_count) max_cycle_count=pred->height;
    }
    dag->height=max_cycle_count+FUEntry(dag->ins)->opnd_stall;
    return NULL;
}
void AnnointDag(void)
{
    data_dag *dag;
    for (dag=DataDag;dag!=NULL;dag=dag->prev) if (!dag->visited) AnnointADag(dag);
}
int FPStackExit(block *blk);
int FPStkOver(instruction *ins,int depth);
unsigned char FPFreeIns(instruction *ins);
int StallCost( instruction *ins, instruction *top )
/*********************************************************
    If instruction 'ins' were placed before instruction 'top', how long
    would instructions following 'ins' have to wait before they could
    complete?
*/
{
    unsigned    avail_fu;
    unsigned    fu_overlap;
    int         unit_stall;
    int         opnd_stall;
    const FU_entry *entry;
    instruction *curr;
    instruction *last;
    unsigned char ins_stack;

    /*
        NOP's always get a stall cost of -1 so that they are scheduled
        as soon as they become ready.
    */
    if( ins->head.opcode == 0 ) return( -1 );
    /* or instructions whose condition code results are used */
    if( ins->ins_flags & 0x20 ) return( -1 );
    if( top == NULL ) return( 0 );
    if( FPFreeIns( ins ) ) return( -1 );
    last = top;
    while( FPFreeIns( top ) ) top = top->head.next;
    entry = FUEntry( ins );
    avail_fu = entry->good_fu;
    unit_stall = entry->unit_stall;
    opnd_stall = entry->opnd_stall;
    ins_stack = StackOp( ins );
    curr = top;
    for( ;; ) {
        if( unit_stall == 0 && opnd_stall == 0 ) return( 0 );
        if( opnd_stall > 0 ) {
            --opnd_stall;
            switch( DataDependant( ins, curr, 0 ) ) {
            case 1:
                return( opnd_stall * 2 );
            case 2:
                /* make stalls due to index usage slightly worse */
                return( opnd_stall * 2 + 1 );
            }
            /* two stack ops will stall on the implicit use of SP */
            if( ins_stack && StackOp( curr ) ) return( opnd_stall*2 + 1 );
        }
        if( unit_stall > 0 ) {
            --unit_stall;
            entry = FUEntry( curr );
            fu_overlap = avail_fu & entry->good_fu;
            if( fu_overlap != 0 ) {
                /* turn off one of the fu bits */
                fu_overlap &= fu_overlap - 1;
                avail_fu &= fu_overlap;
                if( avail_fu == 0 ) return( unit_stall * 2 );
            }
        }
        curr = curr->head.next;
        if( curr == last ) return( 0 );
    }
}



hw_reg_set HighOffsetReg(hw_reg_set reg);
hw_reg_set LowOffsetReg(hw_reg_set reg);
int ScaleAdjust(name *op,hw_reg_set reg)
{
    hw_reg_set idx_reg;
    if (op->n.class!=4) return -1;
    idx_reg=op->i.index->r.reg;
    if (!(idx_reg&reg)) return -1;
    if ((op->i.index_flags&4) && (HighOffsetReg(idx_reg)&reg)) return 0;
    if ((op->i.index_flags&8) && (LowOffsetReg(idx_reg)&reg)) return 0;
    return op->i.scale;
}
void FixIndexAdjust(instruction *adj,unsigned char forward)
{
    instruction *chk;
    name *op;
    unsigned long bias;
    hw_reg_set reg;
    int i,scale;
    bias=(unsigned long)adj->operands[1]->c.int_value;
    if (adj->head.opcode==3) bias=0UL-bias;
    if (forward) bias=0UL-bias;
    reg=adj->result->r.reg;
    chk=adj;
    for (;;) {
        chk=forward?chk->head.next:chk->head.prev;
        if (chk==adj || chk->head.opcode==0x4b) break;
        /* Both the released logical-AND expression and actual63da6
         * test the entire flags byte, not just INS_INDEX_ADJUST. */
        if (!chk->ins_flags && ReDefinedBy(chk,adj->result)) break;
        if (forward) {
            if ((int)chk->id>(int)adj->id) continue;
        } else if ((int)chk->id<(int)adj->id) continue;
        for (i=chk->num_operands-1;i>=0;--i) {
            op=chk->operands[i];
            scale=ScaleAdjust(op,reg);
            if (scale>=0)
                chk->operands[i]=ScaleIndex(op->i.index,op->i.base,
                    (long)((unsigned long)op->i.constant+(bias<<(scale&31))),
                    op->n.name_class,op->n.size,op->i.scale,op->i.index_flags);
        }
        op=chk->result;
        if (op!=NULL) {
            scale=ScaleAdjust(op,reg);
            if (scale>=0)
                chk->result=ScaleIndex(op->i.index,op->i.base,
                    (long)((unsigned long)op->i.constant+(bias<<(scale&31))),
                    op->n.name_class,op->n.size,op->i.scale,op->i.index_flags);
        }
    }
}
void FPCalcStk(instruction *ins,int *depth);
void ScheduleIns()
/************************
    Rearrange the instructions in a block according to the data dependancy
    DAG for maximum overlap of the instruction pipeline. The block is built
    from bottom to the top. At each point we have an list of instructions
    that are ready to execute. We run down this list and pick the instruction
    with the minimum stall cost. If two instructions on the ready list have
    the same stall cost, pick the one that has the longest execution time
    of instructions that depend on the one we're trying to schedule (height).
    If two instructions are equal in stall cost and height pick the one
    with the most likelyhood of being stalled by an instruction preceeding
    it (more indexes, register usage, etc). Otherwise, choose the one
    that came last in the source order.

    NOTE: This routine does not maintain the live information when it
          rebuilds the block. Don't expect anything that depends on live
          analysis to work after this routine is done.
*/
{
    data_dag        *ready;
    data_dag        *curr;
    data_dag        *best;
    data_dag        *fp_stack_just_squeeks_by;
    dep_list_entry  *dep;
    instruction     *top;
    int             best_cost;
    int             curr_cost;
    int             stk_depth;
    int             stk_over;
    unsigned short     last_seq;


    /*
        What the hell is he doing with the visited bit here!?...
        Here's what's I'm doing. Since the ready list is only singly linked,
        it's tough to delete things efficiently. The visited bit is
        used to indicate that the instruction has been scheduled, and the
        dag has been 'virtually' deleted from the list. All the virtually
        deleted dags at the front of the ready list are really deleted just
        before any newly ready dags are added.
    */
    /* get initial 'ready' list */
    ready = NULL;
    for( curr = DataDag; curr != NULL; curr = curr->prev ) {
        curr->visited = 0;
        if( curr->anc_count == 0 ) {
            curr->ready = ready;
            ready = curr;
        }
    }
    last_seq = 0;
    top = NULL;
    stk_depth = FPStackExit( SBlock );
    SBlock->ins.hd.next = (instruction *)&SBlock->ins; // so we can dump!
    SBlock->ins.hd.prev = (instruction *)&SBlock->ins;
    while( ready != NULL ) {
        /* find best instruction to schedule */
        best_cost = INT_MAX;
        best = NULL;
        fp_stack_just_squeeks_by = NULL;
        for( curr = ready; curr != NULL; curr = curr->ready ) {
            if( curr->visited ) continue;
            stk_over = FPStkOver( curr->ins, stk_depth );
            if( stk_over == 0 ) {
                fp_stack_just_squeeks_by = curr;
                continue;
            } else if( stk_over > 0 ) {
                continue;
            }
            curr_cost = StallCost( curr->ins, top );
#define MARK_BEST {best = curr; best_cost = curr_cost; }
            if( best == NULL || curr_cost < best_cost ) {
                MARK_BEST;
            } else if( curr_cost == best_cost ) {
                if( curr->height > best->height ) {
                    MARK_BEST;
                } else if( curr->height == best->height ) {
                    if( (curr->ins->ins_flags & 0x80)
                        && DataDependant( curr->ins, best->ins, 0 ) ) {
                        /*
                           drop in a INDEX_ADJUST as soon as it doesn't
                           hurt, otherwise we end up putting it too close
                           to its use.
                        */
                        MARK_BEST;
                    } else if( curr->stallable > best->stallable ) {
                        MARK_BEST;
                    } else if( curr->stallable == best->stallable ) {
                        if( curr->ins->sequence == last_seq && best->ins->sequence != last_seq ) {
                            // try to avoid fxch instructions
                            MARK_BEST;
                        } else {
                            if( (int)curr->ins->id > (int)best->ins->id ) {
                                MARK_BEST;
                            }
                        }
                    }
                }
            }
        }
        if( best == NULL ) {
            /*
               for pow( x, y ) we need 4 entries. This may be all we have.
               so our usual rules won't let us schedule anything. This is
               an escape hatch.
            */
            best = fp_stack_just_squeeks_by;
        }
        if( best == NULL ) {
            Zoiks(81);
            best = ready;
        }
        /* insert instruction in ring before the 'top' instruction */
        if( top == NULL ) {
            top = best->ins;
            top->head.prev = top;
            top->head.next = top;
        } else {
            best->ins->head.next = top;
            best->ins->head.prev = top->head.prev;
            top->head.prev->head.next = best->ins;
            top->head.prev = best->ins;
            top = best->ins;
        }
        if( FPStackIns( top ) ) {
            last_seq = top->sequence;
        }
        if( top->ins_flags & 0x80 ) {
            FixIndexAdjust( top, 1 );
        }
        FPCalcStk( best->ins, &stk_depth );
        best->scheduled = 1;
        /* really delete any virtually deleted dags */
        best->visited = 1;
        for( ;; ) {
            if( ready == NULL ) break;
            if( !ready->visited ) break;
            ready = ready->ready;
        }
        /* find out who becomes ready now that this instruction is scheduled */
        for( dep = best->deps; dep != NULL; dep = dep->next ) {
            curr = dep->dep;
            if( --(curr->anc_count) == 0 ) {
                curr->ready = ready;
                ready = curr;
            }
        }
    }
    SBlock->ins.hd.next = top;
    SBlock->ins.hd.prev = top->head.prev;
    top->head.prev->head.next = (instruction *)&SBlock->ins;
    top->head.prev = (instruction *)&SBlock->ins;
    /*
       We have to run backwards through the block, adjusting all
       the index offsets that were scheduled before their corresponding
       index adjustment instructions. The opposite case of an index
       adjustment being moved before the use of the index register was
       handled when we scheduled the 0x80 instruction.
    */
    for( top = SBlock->ins.hd.prev; top->head.opcode != 0x4b;
                top = top->head.prev ) {
        if( top->ins_flags & 0x80 ) {
            FixIndexAdjust( top, 0 );
        }
    }
}


void FPPostSched(block *blk);
int CountIns(block *blk);
void SchedBlock(void)
{
    unsigned num_instrs;
    num_instrs=CountIns(SBlock);
    if (num_instrs>2) {
        InitDag();
        FPPreSched(SBlock);
        BuildDag();
        AnnointDag();
        SetMemOut(MO_FATAL);
        ScheduleIns();
        FreeDataDag();
        FPPostSched(SBlock);
        SetMemOut(MO_SUICIDE);
    }
}
void FreeDataDag(void)
{
    dep_list_block *dep,*next;
    data_dag *dag,*prev;
    for (dag=DataDag;dag!=NULL;dag=prev) {
        prev=dag->prev;
        CGFreeSize(dag,sizeof(data_dag));
    }
    DataDag=NULL;
    for (dep=CurrDepBlock;dep!=NULL;dep=next) {
        next=dep->next;
        FrlFreeSize(&DepFrl,dep,sizeof(dep_list_block));
    }
    CurrDepBlock=NULL;
}
unsigned char FrlFreeAll(void **head,int size);
unsigned char SchedFrlFree(void)
{
    return FrlFreeAll(&DepFrl,sizeof(dep_list_block));
}
void ClearInsBits(unsigned char flags);
int Spawn(void (*rtn)(void));
void ProcMessage(unsigned message);
unsigned Renumber(void);
void Schedule(void)
{
    unsigned char old_memout,first_time;
    first_time=1;
    InitFrl(&DepFrl);
    old_memout=SetMemOut(MO_SUICIDE);
    for (SBlock=HeadBlock;SBlock!=NULL;SBlock=SBlock->next_block) {
        if (Spawn(SchedBlock)!=0) {
            if (first_time) {
                ProcMessage(16);
                first_time=0;
            }
            FreeDataDag();
        }
    }
    SchedFrlFree();
    SetMemOut(old_memout);
    Renumber();
    ClearInsBits(0x80);
}
