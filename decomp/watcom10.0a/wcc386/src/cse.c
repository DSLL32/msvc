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
* Description:  Perform copy propagation and common subexpression optimization.
*
****************************************************************************/


#include "block.h"
#include "genopt.h"
#include "memory.h"
#include "address.h"
#include "stubs.h"
#include <recon.h>

unsigned char PropagateMoves(void)
{
    unsigned char change;
    change=0;
    FindPartition();
    while (DoPropagateMoves()) change=1;
    CleanPartition();
    return change;
}
unsigned char DoPropagateMoves(void)
{
    unsigned char change,block_changed;
    block *blk;
    change=0;
    for (blk=HeadBlock;blk!=NULL;blk=blk->next_block) {
        DGBlip(); /* SXBlip is the same physical progress hook in this build. */
        if (blk->class&0x800) {
            do {
                block_changed=0;
                if (FixStructRet(blk)) { block_changed=1;change=1; }
                if (PeepOpt(blk,NextBlock,blk,0)) { block_changed=1;change=1; }
                if (ConstFold(blk)) { block_changed=1;change=1; }
                LinkMoves(blk);
                if (PropMoves(blk,0)) { block_changed=1;change=1; }
                CleanMoves(blk);
                LinkMemMoves(blk);
                if (PropMoves(blk,1)) { block_changed=1;change=1; }
                CleanMoves(blk);
            } while (block_changed);
        }
    }
    return change;
}
static unsigned char LeaveIndVars;
unsigned char LoadAddr(void);
unsigned char PropRegsOne(void);
void ReCalcAddrTaken(void);
unsigned char StretchEdges(void);
unsigned char PropagateExprs(void);
unsigned char CommonSex(unsigned char leave_indvars_alone)
{
    unsigned char loops_killed,change,anychange;
    loops_killed=0;
    LeaveIndVars=leave_indvars_alone;
    FindPartition();
    anychange=LoadAddr();
    if (PropRegsOne()) anychange=1;
    for (;;) {
        ReCalcAddrTaken();
        change=0;
        if (DoPropagateMoves()) change=1;
        if (StretchEdges()) {
            change=1;loops_killed=1;
            CleanPartition();FindPartition();
        }
        if (DeadBlocks()) {
            change=1;loops_killed=1;
            CleanPartition();FindPartition();
        }
        if (PropagateExprs()) change=1;
        if (change) InsDead();
        if (change) anychange=1;
        else break;
    }
    CleanPartition();
    if (loops_killed) { BlockTrim();MakeFlowGraph(); }
    return anychange;
}

void FindPartition(void)
{
    block *blk,*other,*temp;
    block_edge *edge;
    int i;
    for (blk=HeadBlock;blk!=NULL;blk=blk->next_block) {
        if ((blk->class&0x20) || blk->inputs!=1) blk->class|=0x800;
        blk->u.partition=blk;
        blk->available_bit=0;
    }
    for (blk=HeadBlock;blk!=NULL;blk=blk->next_block) {
        i=blk->targets;
        edge=&blk->edge[0];
        while (--i>=0) {
            if (edge->flags&8) {
                other=edge->destination;
                if (!(other->class&0x800) && other->inputs==1) {
                    temp=other->u.partition;
                    other->u.partition=blk->u.partition;
                    blk->u.partition=temp;
                }
            }
            ++edge;
        }
    }
    for (blk=HeadBlock;blk!=NULL;blk=blk->next_block)
        if (blk->class&0x800) TreeBits(blk);
}
void TreeBits(block *root)
{
    block *daddy,*blk,**owner;
    unsigned next_bit;
    unsigned char change;
    instruction *ins;
    next_bit=1;
    root->available_bit=next_bit;
    for (;;) {
        change=0;
        blk=root->u.partition;
        while (blk!=root) {
            daddy=blk->input_edges->source;
            if (blk->available_bit==0 && daddy->available_bit!=0) {
                next_bit<<=1;
                if (next_bit==0) break;
                blk->available_bit=daddy->available_bit|next_bit;
                change=1;
            }
            blk=blk->u.partition;
        }
        if (next_bit==0 || !change) break;
    }
    owner=&root->u.partition;
    for (;;) {
        blk=*owner;
        ins=blk->ins.hd.next;
        while (ins->head.opcode!=0x4b) {
            ins->head.live.within_block=blk->available_bit;
            ins=ins->head.next;
        }
        if (blk->available_bit==0) *owner=blk->u.partition;
        else owner=&blk->u.partition;
        if (blk==root) break;
    }
}
void CleanPartition(void)
{
    block *blk;
    for (blk=HeadBlock;blk!=NULL;blk=blk->next_block) {
        blk->class&=~0x800U;
        blk->u.partition=NULL;
    }
}

unsigned char FixStructRet(block *root)
{
    block *blk;
    instruction *ins;
    unsigned char change;
    change=0;
    blk=root;
    for (;;) {
        for (ins=blk->ins.hd.next;ins->head.opcode!=0x4b;ins=ins->head.next) {
            if (ins->head.opcode==0x36 || ins->head.opcode==0x29) {
                if (ins->result!=NULL && ins->result->n.class==2 &&
                    (ins->flags.call_flags&0x20) && FixOneStructRet(ins)) change=1;
                ins->flags.call_flags&=~0x20U;
            }
        }
        blk=blk->u.partition;
        if (blk==root) break;
    }
    return change;
}
unsigned char FixOneStructRet(instruction *ins)
{
    instruction *mova,*movr;
    name *res,*op;
    res=ins->result;
    mova=ins->head.prev;
    if (mova->head.opcode!=0x26) return 0;
    op=mova->operands[0];
    if (op->n.class!=0 || op->c.const_type!=4 || op->c.value!=res) return 0;
    movr=ins->head.next;
    while (movr->head.opcode==0) movr=movr->head.next;
    if (movr->head.opcode!=0x26 || movr->type_class!=XX || movr->operands[0]!=res) return 0;
    res=movr->result;
    if (res->n.class!=2) return 0;
    ins->result=res;
    mova->operands[0]=AllocAddrConst(res,0,4,mova->type_class);
    FreeIns(movr);
    return 1;
}
block *NextBlock(block *blk,block *root)
{
    if (blk->u.partition==root) return NULL;
    return blk->u.partition;
}
void LinkMoves(block *root)
{
    block *blk;
    instruction *ins;
    blk=root;
    for (;;) {
        for (ins=blk->ins.hd.next;ins->head.opcode!=0x4b;ins=ins->head.next)
            if (LinkableMove(ins)) CreateLink(ins,ins->result);
        blk=blk->u.partition;
        if (blk==root) break;
    }
}
unsigned char LinkableMove(instruction *ins)
{
    if (!MoveIns(ins)) return 0;
    if (!CanLinkMove(ins)) return 0;
    if (ins->operands[0]->n.class==1 || ins->operands[0]->n.class==4) return 0;
    if (ins->operands[0]->n.class==0 && ins->operands[0]->c.const_type==0) {
        ins->operands[0]=AllocConst(CnvCFToType(ins->operands[0]->c.value,
                                            ClassType(ins->type_class)));
    }
    return 1;
}
unsigned char MoveIns(instruction *ins)
{
    if (ins->head.opcode==0x26) return 1;
    if ((ins->head.opcode==0x24 || ins->head.opcode==0x25) &&
        ins->operands[0]->n.class==0 && ins->operands[0]->c.const_type==0) return 1;
    return 0;
}
unsigned char CanLinkMove(instruction *ins)
{
    if (ins->num_operands!=1) return 0;
    if (ins->operands[0]->n.class==3) return 0;
    if (ins->operands[0]->n.class==2 && (ins->operands[0]->t.temp_flags&0x20)) return 0;
    if (ins->result->n.class==3) return 0;
    if (ins->operands[0]==ins->result) return 0;
    if (IsVolatile(ins->result)) return 0;
    if (ReDefinedBy(ins,ins->operands[0])) return 0;
    if (FPIsConvert(ins)) return 0;
    return 1;
}
void CreateLink(instruction *ins,name *op)
{
    if (op->n.class==4) op=op->i.index;
    if (op->n.class!=3) {
        /* The optimizer temporarily borrows the name's conflict pointer
         * for an instruction list, before conflict construction. */
        ins->u.cse_link=(instruction *)op->v.conflict;
        op->v.conflict=(conflict_node *)ins;
    }
}
unsigned char PropMoves(block *root,unsigned char backward)
{
    int i;
    block *blk;
    instruction *ins;
    name *idx;
    unsigned char change;
    change=0;
    blk=root;
    for (;;) {
        ins=blk->ins.hd.next;
        while (ins->head.opcode!=0x4b) {
            /* No later unaligned load/store opcodes exist in this target. */
            if (ins->head.opcode!=0x28 && ins->head.opcode!=0x27 &&
                ins->head.opcode!=2 && ins->head.opcode!=4 && ins->head.opcode!=0x29) {
                i=NumOperands(ins);
                while (--i>=0) {
                    switch (ins->operands[i]->n.class) {
                    case 4:
                        idx=ins->operands[i]->i.index;
                        if (idx->n.class!=3 && idx->n.class!=0)
                            change|=PropOpnd(ins,&ins->operands[i],(instruction *)idx->v.conflict,backward,1);
                        break;
                    case 0: case 3: break;
                    default:
                        change|=PropOpnd(ins,&ins->operands[i],(instruction *)ins->operands[i]->v.conflict,backward,1);
                        break;
                    }
                }
                if (ins->result!=NULL && ins->result->n.class==4) {
                    idx=ins->result->i.index;
                    if (idx->n.class!=3) {
                        RemoveLink(ins,ins->result);
                        change|=PropOpnd(ins,&ins->result,(instruction *)idx->v.conflict,backward,0);
                        if (LinkableMove(ins)) CreateLink(ins,ins->result);
                    }
                }
            }
            ins=ins->head.next;
        }
        blk=blk->u.partition;
        if (blk==root) break;
    }
    return change;
}
/* These are source macros, not new graph vertices. */
#define CSE_FLOATING(c) ((c)==FS || (c)==FD)
#define CSE_OP_CLASS(ins) (((ins)->head.opcode==0x24 || (ins)->head.opcode==0x25) ? (ins)->base_type_class : (ins)->type_class)
unsigned char PropOpnd( instruction *ins, name **op,
                          instruction *definition, unsigned char backward,
                          unsigned char is_opnd ) {
/******************************************************************
    See if we can propagate the operand from a move into the spot for
    operands "*op" in instruction "ins" given definition (move) list
    "definition".  (See DoPropagateMoves).
*/

    unsigned char       change;
    name                *opnd;
    name                *defop;
    name                *defres;
    long                disp;
    name                *base;

    enum { N_CONSTANT=0,N_MEMORY=1,N_TEMP=2,N_INDEXED=4,
           CONS_ABSOLUTE=0,CONS_ADDRESS=1,CONS_OFFSET=2,CONS_SEGMENT=3,
           CONS_TEMP_ADDR=4,OP_SELECT=0x2d,X_FAKE_BASE=0x10 };
    opnd = *op;
    change = 0;
    while( definition != NULL ) {
        if( WhichIsAncestor( definition, ins ) == definition &&
            UnOpsLiveFrom( definition, ins ) ) {
            defop = definition->operands[ 0 ];
            defres = definition->result;
            if( backward ) {
                if( defop == opnd && defres->n.class == N_TEMP && is_opnd ) {
                    if( ( CSE_FLOATING( definition->type_class )
                       == CSE_FLOATING( CSE_OP_CLASS( ins ) ) )
                     && CanCrossBlocks( definition, ins, defres )
                     && !FPStackOp( defres ) ) {
                        UseInOther( definition, ins, defres );
                        *op = defres;
                        change = 1;
                    }
                }
            } else {
                if( defres == opnd ) {
                    if( defres->n.class != N_INDEXED && is_opnd ) {
                        if( ( CSE_FLOATING( definition->type_class )
                           == CSE_FLOATING( CSE_OP_CLASS( ins ) ) )
                         && CanCrossBlocks( definition, ins, defop )
                         && !FPStackOp( defop ) ) {
                            UseInOther( definition, ins, defop );
                            *op = defop;
                            change = 1;
                        }
                    }
                } else if( opnd->n.class == N_INDEXED
                      && definition->result->n.class == N_TEMP ) {
                    if( defop->n.class == N_TEMP
                          && defop->n.name_class==opnd->i.index->n.name_class
                          && CanCrossBlocks( definition, ins, defop ) ) {
                        UseInOther( definition, ins, defop );
                        *op = ScaleIndex( defop, opnd->i.base,
                                        opnd->i.constant,
                                        opnd->n.name_class, opnd->n.size,
                                        opnd->i.scale, opnd->i.index_flags );
                        change = 1;
                    } else if( defop->n.class == N_CONSTANT
                            && ins->head.opcode != OP_SELECT ) {
                        base = NULL;
                        switch( defop->c.const_type ) {
                        case CONS_ABSOLUTE:
                            if( opnd->i.base != NULL
                                && !(opnd->i.index_flags & X_FAKE_BASE) ) {
                                disp = (long)((unsigned long)opnd->i.constant + (unsigned long)defop->c.int_value);
                                base = opnd->i.base;
                            }
                            break;
                        case CONS_ADDRESS:
                        case CONS_OFFSET:
                        case CONS_TEMP_ADDR:
                            if( opnd->i.base == NULL
                                || (opnd->i.index_flags & X_FAKE_BASE) ) {
                                disp = opnd->i.constant;
                                base = defop->c.value;
                            }
                            break;
                        case CONS_SEGMENT:
                            Zoiks( 86 );
                            break;
                        default:
                            Zoiks( 78 );
                            break;
                        }
                        if( base != NULL ) {
                            switch( base->n.class ) {
                            case N_TEMP:
                                *op = STempOffset( base,
                                                disp,
                                                opnd->n.name_class,
                                                opnd->n.size );
                                break;
                            case N_MEMORY:
                                *op = SAllocMemory( base->v.symbol,
                                            (long)((unsigned long)base->v.offset + (unsigned long)disp),
                                            base->m.memory_type,
                                            opnd->n.name_class,
                                            opnd->n.size );
                                break;
                            default:
                                Zoiks( 87 );
                                break;
                            }
                            change = 1;
                        }
                    }
                }
            }
        }
        definition = definition->u.cse_link;
    }
    return( change );
}


#undef CSE_FLOATING
#undef CSE_OP_CLASS

void RemoveLink(instruction *ins,name *op)
{
    instruction **owner,*curr;
    if (op->n.class==4) op=op->i.index;
    if (op->n.class!=3) {
        owner=(instruction **)&op->v.conflict;
        curr=(instruction *)op->v.conflict;
        while (curr!=NULL) {
            if (curr==ins) {
                *owner=ins->u.cse_link;
                ins->u.cse_link=NULL;
                break;
            }
            owner=&curr->u.cse_link;
            curr=curr->u.cse_link;
        }
    }
}
void CleanMoves(block *root)
{
    instruction *ins;
    block *blk;
    int i;
    blk=root;
    for (;;) {
        ins=blk->ins.hd.next;
        while (ins->head.opcode!=0x4b) {
            i=ins->num_operands;
            while (--i>=0) NullNameLink(ins->operands[i]);
            if (ins->result!=NULL) NullNameLink(ins->result);
            ins=ins->head.next;
        }
        blk=blk->u.partition;
        if (blk==root) break;
    }
    CleanTableEntries(root);
}
void NullNameLink(name *op)
{
    for (;;) {
        if (op->n.class==1 || op->n.class==2) {
            op->v.conflict=NULL;
        } else if (op->n.class==4) {
            NullNameLink(op->i.index);
            if (op->i.base!=NULL) {
                op=op->i.base;
                continue;
            }
        }
        return;
    }
}
void CleanTableEntries(block *root)
{
    instruction *ins;
    block *blk;
    blk=root;
    for (;;) {
        ins=blk->ins.hd.next;
        while (ins->head.opcode!=0x4b) {
            ins->u.cse_link=NULL;
            ins=ins->head.next;
        }
        blk=blk->u.partition;
        if (blk==root) break;
    }
}
void LinkMemMoves(block *root)
{
    block *blk;
    instruction *ins;
    blk=root;
    for (;;) {
        for (ins=blk->ins.hd.next;ins->head.opcode!=0x4b;ins=ins->head.next) {
            if (!MoveIns(ins)) continue;
            if (!CanLinkMove(ins)) continue;
            if (ins->operands[0]->n.class!=1 && ins->operands[0]->n.class!=4) continue;
            CreateLink(ins,ins->operands[0]);
        }
        blk=blk->u.partition;
        if (blk==root) break;
    }
}

instruction *WhichIsAncestor(instruction *first,instruction *second)
{
    instruction *result,*ins;
    unsigned bits1,bits2;
    block *blk;
    bits1=first->head.live.within_block;
    bits2=second->head.live.within_block;
    if (bits1==bits2) {
        result=NULL;
        if (first!=second) {
            ins=first;
            for (;;) {
                ins=ins->head.next;
                if (ins==second) result=first;
                else if (ins->head.opcode==0x4b) result=second;
                if (result!=NULL) break;
            }
        }
    } else if ((bits1&bits2)==bits1) result=first;
    else if ((bits1&bits2)==bits2) result=second;
    else {
        bits1&=bits2;
        ins=first;
        while (ins->head.opcode!=0x4b) ins=ins->head.next;
        blk=((block_ins *)ins)->blk;
        while (blk->available_bit!=bits1) blk=blk->u.partition;
        result=blk->ins.hd.prev;
        while (result->head.opcode==0) {
            if (result->flags.nop_flags&8) break;
            result=result->head.prev;
        }
        while (result->head.opcode==0x2d ||
            (result->head.opcode>=0x2e && result->head.opcode<=0x35))
            result=result->head.prev;
    }
    return result;
}
unsigned char UnOpsLiveFrom(instruction *first,instruction *last)
{
    instruction *ins;
    ins=last->head.prev;
    for (;;) {
        while (ins->head.opcode==0x4b)
            ins=((block_ins *)ins)->blk->input_edges->source->ins.hd.prev;
        if (ins==first) break;
        if (ReDefinedBy(ins,first->operands[0]) || ReDefinedBy(ins,first->result)) return 0;
        ins=ins->head.prev;
    }
    return 1;
}
unsigned char CanCrossBlocks(instruction *first,instruction *second,name *op)
{
    if (!BlockByBlock || op->n.class!=2) return 1;
    if (first->head.live.within_block==second->head.live.within_block) return 1;
    return (op->t.temp_flags&0x80)!=0;
}
void UseInOther(instruction *first,instruction *second,name *op)
{
    if (first->head.live.within_block!=second->head.live.within_block) {
        if (op->n.class==1) op->v.usage|=2;
        else if (op->n.class==2) op->t.temp_flags|=0x80;
    }
}

unsigned char LoadAddr(void)
{
    block *blk;
    instruction *ins;
    unsigned char change;
    change=0;
    blk=HeadBlock;
    while (blk!=NULL) {
        ins=blk->ins.hd.next;
        while (ins->head.opcode!=0x4b) {
            change|=LoadAToMove(ins);
            ins=ins->head.next;
        }
        blk=blk->next_block;
    }
    return change;
}
unsigned char PropRegsOne(void)
{
    /* The register-copy pass is disabled in this original build, just as
     * in the released source's active #else branch. */
    return 0;
}
void ReCalcAddrTaken(void)
{
    name *temp;
    for (temp=Names[2];temp!=NULL;temp=temp->n.next_name) {
        if (temp->v.usage&0x20) continue; /* VAR_VOLATILE */
        if (temp->v.symbol!=NULL && (FEAttr(temp->v.symbol)&0x400)) continue;
        temp->v.usage&=~8U; /* USE_ADDRESS */
    }
    FindReferences();
}
unsigned char StretchABlock(block *blk);
unsigned char StretchEdges(void)
{
    block *blk;
    unsigned char change;
    change=0;
    for (blk=HeadBlock;blk!=NULL;blk=blk->next_block)
        if (StretchABlock(blk)) change=1;
    return change;
}
unsigned char FindDefnBlocks(block *blk,instruction *ins,int i);
unsigned char StretchABlock(block *blk)
{
    instruction *ins;
    name *op;
    int i;
    if (blk->ins.hd.prev!=blk->ins.hd.next) return 0;
    ins=blk->ins.hd.next;
    if (ins->head.opcode<0x2e || ins->head.opcode>0x35) return 0;
    op=ins->operands[0];
    i=1;
    if (op->n.class!=0) { op=ins->operands[1];i=0; }
    if (op->n.class!=0) return 0;
    if (IsVolatile(ins->operands[i])) return 0;
    return FindDefnBlocks(blk,ins,i);
}
unsigned char FindDefnBlocks(block *blk,instruction *cond,int i)
{
    block_edge *edge,*next_source,*other_input,*jump_edge;
    block *input,*destination;
    instruction *prev,*new_cond,*new_ins;
    name *op;
    unsigned char change,index;
    change=0;
    op=cond->operands[i];
    for (edge=blk->input_edges;edge!=NULL;edge=next_source) {
        next_source=edge->next_source;
        input=edge->source;
        if (!(input->class&2)) continue; /* JUMP. */
        for (prev=input->ins.hd.prev;prev->head.opcode!=0x4b;
             prev=prev->head.prev) {
            if (!ReDefinedBy(prev,op)) continue;
            if (prev->head.opcode!=0x26 || prev->result!=op) break;
            if (input->depth<blk->depth) {
                other_input=blk->input_edges;
                while (other_input!=NULL) {
                    if (other_input->source->depth<blk->depth &&
                        other_input->source!=input) break;
                    other_input=other_input->next_source;
                }
                if (other_input!=NULL) break;
            }
            new_cond=NewIns(2);
            Copy((char *)cond,(char *)new_cond,sizeof(instruction));
            new_cond->head.prev=new_cond;
            new_cond->head.next=new_cond;
            new_cond->operands[i]=prev->operands[0];
            new_ins=FoldIns(new_cond);
            if (new_ins==NULL) new_ins=new_cond;
            /* Original defect at CF5d39f/5d3a4: the 1994 instructions compare
             * ESI (the instruction ADDRESS) with opcode bounds2e..35. Later
             * cse.c checks new_ins->head.opcode. Preserve the actual numeric
             * pointer predicate rather than substituting the later test. */
            if ((unsigned long)new_ins>=0x2eUL && (unsigned long)new_ins<=0x35UL) {
                if ((new_ins->flags.bool&15)==((new_ins->flags.bool>>4)&15)) {
                    index=new_ins->flags.bool&15;
                    destination=blk->edge[index].destination;
                    if (destination!=blk) {
                        jump_edge=&input->edge[0];
                        RemoveInputEdge(jump_edge);
                        ++destination->inputs;
                        jump_edge->next_source=destination->input_edges;
                        destination->input_edges=jump_edge;
                        jump_edge->destination=destination;
                        if (input->depth<blk->depth) MoveHead(blk,destination);
                        change=1;
                    }
                }
            }
            FreeIns(new_ins);
            break;
        }
    }
    return change;
}
unsigned char DoArithOps(block *root);
unsigned char PropagateExprs(void)
{
    block *blk;
    unsigned char change;
    change=0;
    blk=HeadBlock;
    while (blk!=NULL) {
        DGBlip(); /* Actual physical SXBlip hook identity. */
        if (blk->class&0x800) change|=DoArithOps(blk);
        blk=blk->next_block;
    }
    return change;
}
static instruction *ExprHeads[0x25]; /* Actual CSE opcodes 1..36. */
unsigned char Inducable(block *blk,instruction *ins);
unsigned char DoOneOpcode(unsigned char opcode);
unsigned char DoDivides(void);
unsigned char DoArithOps(block *root)
{
    block *blk;
    instruction *ins;
    unsigned char opcode,change;
    for (opcode=1;opcode<=0x24;++opcode) ExprHeads[opcode]=NULL;
    blk=root;
    for (;;) {
        ins=blk->ins.hd.next;
        for (;;) {
            opcode=ins->head.opcode;
            if (opcode==0x4b) break;
            if (opcode>=1 && opcode<=0x24 &&
                !(LeaveIndVars && Inducable(blk,ins)) &&
                ins->operands[0]->n.class!=3 &&
                ins->operands[(ins->head.opcode>=1 && ins->head.opcode<=0x10)?1:0]->n.class!=3 &&
                ins->result->n.class!=3 && !IsVolatile(ins->result)) {
                ins->ins_flags&=~2U; /* INS_DEFINES_OWN_OPERAND */
                if (ReDefinedBy(ins,ins->operands[0])) ins->ins_flags|=2;
                if (ins->head.opcode>=1 && ins->head.opcode<=0x10)
                    if (ReDefinedBy(ins,ins->operands[1])) ins->ins_flags|=2;
                ins->u.cse_link=ExprHeads[opcode];
                ExprHeads[opcode]=ins;
            }
            ins=ins->head.next;
        }
        blk=blk->u.partition;
        if (blk==root) break;
    }
    change=0;
    for (opcode=1;opcode<=0x24;++opcode) change|=DoOneOpcode(opcode);
    change|=DoDivides();
    CleanTableEntries(root);
    return change;
}

instruction *ProcessExpr(instruction *ins1,instruction *ins2,unsigned char signed_matters);
void DeleteFromList(instruction **owner,instruction *ins,instruction *new_ins);
unsigned char DoOneOpcode(unsigned char opcode)
{
    instruction *ins1,*ins2,*next_ins1,*next_ins2,*which_ins;
    unsigned char change,signed_matters;
    change=0;
    ins1=ExprHeads[opcode];
    switch (opcode) {
    case 1:case 2:case 3:case 4: /* ADD/EXT_ADD/SUB/EXT_SUB */
    case 9:case 10:case 11:case 13: /* AND/OR/XOR/LSHIFT */
    case 17:case 18: /* NEGATE/COMPLEMENT */
        signed_matters=0;break;
    default:signed_matters=1;break;
    }
    while (ins1!=NULL) {
        ins2=ins1->u.cse_link;
        next_ins1=ins2;
        while (ins2!=NULL) {
            next_ins2=ins2->u.cse_link;
            which_ins=ProcessExpr(ins1,ins2,signed_matters);
            if (which_ins!=NULL) {
                change=1;
                if (which_ins==ins1) {
                    DeleteFromList(&ExprHeads[opcode],ins1,next_ins1);
                    FreeIns(ins1);
                    break;
                } else if (which_ins==ins2) {
                    DeleteFromList(&ExprHeads[opcode],ins2,next_ins2);
                    FreeIns(ins2);
                    if (ins2==next_ins1) { next_ins1=ins1;break; }
                } else {
                    DeleteFromList(&ExprHeads[opcode],ins1,next_ins1);
                    DeleteFromList(&ExprHeads[opcode],ins2,next_ins2);
                    FreeIns(ins1);FreeIns(ins2);
                    which_ins->u.cse_link=ExprHeads[opcode];
                    ExprHeads[opcode]=which_ins;
                    next_ins1=which_ins;
                    break;
                }
            }
            ins2=next_ins2;
        }
        ins1=next_ins1;
    }
    return change;
}
typedef enum who_dies { ALL_LIVE,OP_DIES,RESULT_DIES } who_dies;
who_dies BinOpsLiveFrom(instruction *first,instruction *last,name *op1,name *op2,name *result)
{
    instruction *ins;
    unsigned char result_dies;
    ins=last->head.prev;
    result_dies=0;
    for (;;) {
        while (ins->head.opcode==0x4b)
            ins=((block_ins *)ins)->blk->input_edges->source->ins.hd.prev;
        if (ins==first) break;
        if (ReDefinedBy(ins,op1)) return OP_DIES;
        if (ReDefinedBy(ins,op2)) return OP_DIES;
        if (result!=NULL && ReDefinedBy(ins,result)) result_dies=1;
        ins=ins->head.prev;
    }
    if (result_dies) return RESULT_DIES;
    return ALL_LIVE;
}
void FPNotStack(name *value);
unsigned char Hoistable(instruction *ins,void *data);
/* Unsigned type-class counterparts in the old eleven-class universe. */
unsigned char Unsigned[]={U1,U1,U2,U2,U4,U4,CP,PT,FS,FD,XX};
instruction *ProcessExpr(instruction *ins1,instruction *ins2,unsigned char signed_matters)
{
    instruction *first,*new_ins,*dead_or_new_ins;
    name *temp;
    unsigned char class;
    who_dies killed;
    int i;
    dead_or_new_ins=NULL;
    i=ins1->head.opcode>=1 && ins1->head.opcode<=0x10;
    if (ins1->head.opcode==0x24 && ins1->base_type_class!=ins2->base_type_class) return NULL;
    if (signed_matters) {
        if (ins1->type_class!=ins2->type_class) return NULL;
    } else if (Unsigned[ins1->type_class]!=Unsigned[ins2->type_class]) return NULL;
    if (ins1->operands[0]!=ins2->operands[0] || ins1->operands[i]!=ins2->operands[i]) {
        switch (ins1->head.opcode) {
        case 1:case 5:case 9:case 10:case 11:break;
        default:return NULL;
        }
        if (ins1->operands[0]!=ins2->operands[i] || ins1->operands[i]!=ins2->operands[0]) return NULL;
    }
    first=WhichIsAncestor(ins1,ins2);
    if (first==ins2) { ins2=ins1;ins1=first; }
    if (first==ins1) {
        if (!(ins1->ins_flags&2)) {
            killed=BinOpsLiveFrom(ins1,ins2,ins1->operands[0],ins1->operands[i],ins1->result);
            if (killed!=OP_DIES) {
                class=ins1->result->n.name_class;
                if (killed==RESULT_DIES || !CanCrossBlocks(ins1,ins2,ins1->result)) {
                    temp=AllocTemp(class);
                    new_ins=MakeMove(temp,ins1->result,class);
                    ins1->result=temp;
                    SuffixIns(ins1,new_ins);
                    SetCSEBits(ins1,new_ins);
                    FPNotStack(temp);
                }
                new_ins=MakeMove(ins1->result,ins2->result,class);
                UseInOther(ins1,ins2,ins1->result);
                dead_or_new_ins=ins2;
                SetCSEBits(ins2,new_ins);
                SuffixIns(ins2,new_ins);
                FPNotStack(ins1->result);
            }
        }
    } else if (first!=NULL) {
        /* HoistLooksGood was added after this1994 body. */
        if (ins1->operands[0]->n.class!=4 && ins1->operands[i]->n.class!=4 &&
            Hoistable(ins1,NULL) &&
            BinOpsLiveFrom(first->head.next,ins1,ins1->operands[0],ins1->operands[i],NULL)==ALL_LIVE &&
            BinOpsLiveFrom(first->head.next,ins2,ins2->operands[0],ins2->operands[i],NULL)==ALL_LIVE) {
            class=ins1->type_class;
            temp=AllocTemp(class);
            temp->t.temp_flags|=0x80;
            if (ins1->head.opcode>=1 && ins1->head.opcode<=0x10)
                new_ins=MakeBinary(ins1->head.opcode,ins1->operands[0],ins1->operands[1],temp,class);
            else new_ins=MakeUnary(ins1->head.opcode,ins1->operands[0],temp,class);
            new_ins->base_type_class=ins1->base_type_class;
            dead_or_new_ins=new_ins;
            SetCSEBits(first,new_ins);
            SuffixIns(first,new_ins);
            new_ins=MakeMove(temp,ins1->result,class);
            SetCSEBits(ins1,new_ins);SuffixIns(ins1,new_ins);
            new_ins=MakeMove(temp,ins2->result,class);
            SetCSEBits(ins2,new_ins);SuffixIns(ins2,new_ins);
        }
    }
    return dead_or_new_ins;
}
void DeleteFromList(instruction **owner,instruction *ins,instruction *new_ins)
{
    while (*owner!=ins) owner=&(*owner)->u.cse_link;
    *owner=new_ins;
}
unsigned char ProcessDivide(instruction *ins1,instruction *ins2);
unsigned char DoDivides(void)
{
    instruction *ins1,*ins2,*next_ins1,*next_ins2;
    unsigned char change;
    change=0;
    ins1=ExprHeads[7]; /* OP_DIV */
    while (ins1!=NULL) {
        ins2=ins1->u.cse_link;
        next_ins1=ins2;
        while (ins2!=NULL) {
            next_ins2=ins2->u.cse_link;
            if (ProcessDivide(ins1,ins2)) {
                DeleteFromList(&ExprHeads[7],ins1,next_ins1);
                DeleteFromList(&ExprHeads[7],ins2,next_ins2);
                next_ins1=ExprHeads[7];
                change=1;
                break;
            }
            ins2=next_ins2;
        }
        ins1=next_ins1;
    }
    return change;
}
unsigned char ProcessDivide(instruction *ins1,instruction *ins2) { STUB_TRAP(); return 0; }



void SetCSEBits(instruction *ins,instruction *new_ins)
{
    unsigned bits;
    if (ins->head.opcode==0x4b) bits=((block_ins *)ins)->blk->available_bit;
    else bits=ins->head.live.within_block;
    new_ins->head.live.within_block=bits;
}
