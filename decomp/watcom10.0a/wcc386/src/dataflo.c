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
* Description:  Assign global data-flow bits and construct variable conflicts.
*
****************************************************************************/


#include "block.h"
#include "genopt.h"
#include "stubs.h"
#include <recon.h>

static unsigned char MoreUseInOtherTemps;
global_bit_set MemoryBits;
void CheckGlobals(void);
void RoughSortTemps(void);
global_bit_set AssignGlobalBits(unsigned char class,global_bit_set *bit,unsigned char force);
void PropagateConflicts(void);
void MakeConflicts(void)
{
    global_bit_set bit;
    CheckGlobals();
    MoreUseInOtherTemps=1;
    bit.bits[0]=1;
    bit.bits[1]=bit.bits[2]=bit.bits[3]=0;
    if (!BlockByBlock) RoughSortTemps();
    AssignGlobalBits(2,&bit,1);
    MemoryBits=AssignGlobalBits(1,&bit,1);
    PropagateConflicts();
}
void CheckGlobals(void)
{
    name *op;
    if (!(Model&0x4000000)) { /* RELAX_ALIAS */
        op=Names[1];
        while (op!=NULL) {
            if (op->m.memory_type==0 && (op->v.usage&0x18)!=0x18) {
                if (FEAttr(op->v.symbol)&0x44) /* FE_VISIBLE | FE_GLOBAL */
                    op->v.usage|=0x18; /* USE_MEMORY | USE_ADDRESS */
            }
            op=op->n.next_name;
        }
    }
}
conflict_node *AddConflictNode(name *op);
void AddTempSave(name *op,block *blk);
unsigned char AllocBefore(name *t1,name *t2);
void *SortList(void *list,unsigned next_offset,unsigned char (*before)(void *,void *));
void RoughSortTemps(void)
{
    name *actual_name,*opnd;
    block *blk;
    instruction *ins;
    int i;
    for (opnd=Names[2];opnd!=NULL;opnd=opnd->n.next_name) {
        if (opnd->v.usage&0x18) opnd->v.usage|=0x90;
        else if (opnd->v.usage&2) {
            actual_name=DeAlias(opnd);
            if (actual_name->v.conflict==NULL) AddConflictNode(actual_name);
        }
    }
    for (blk=HeadBlock;blk!=NULL;blk=blk->next_block) {
        for (ins=blk->ins.hd.next;ins->head.opcode!=0x4b;ins=ins->head.next) {
            for (i=0;i<ins->num_operands;++i) AddTempSave(ins->operands[i],blk);
            if (ins->result!=NULL) AddTempSave(ins->result,blk);
        }
    }
    /* Historical callback ABI: two object pointers and a defined byte BOOL.
     * Keep the actual name-pointer signature at the callback itself. */
    Names[2]=SortList(Names[2],offsetof(name,n.next_name),
        (unsigned char (*)(void *,void *))AllocBefore);
}
unsigned Weight(unsigned value,block *blk);
void AddTempSave(name *op,block *blk)
{
    conflict_node *conf;
    if (op->n.class==4) op=op->i.index;
    if (op->n.class!=2) return;
    op=DeAlias(op);
    conf=op->v.conflict;
    if (conf==NULL) return;
    conf->savings+=Weight(1,blk);
}
unsigned char AllocBefore(name *t1,name *t2)
{
    if ((t1->t.temp_flags&0x100) && !(t2->t.temp_flags&0x100)) return 0;
    if (!(t1->t.temp_flags&0x100) && (t2->t.temp_flags&0x100)) return 1;
    if (t1->v.conflict==NULL && t2->v.conflict!=NULL) return 0;
    if (t1->v.conflict!=NULL) {
        if (t2->v.conflict==NULL) return 1;
        return t1->v.conflict->savings>t2->v.conflict->savings;
    }
    return t1->t.id.id>t2->t.id.id;
}
void GBitNext(global_bit_set *bit);
#define GEMPTY(set) (((set).bits[0]|(set).bits[1]|(set).bits[2]|(set).bits[3])==0)
global_bit_set AssignGlobalBits(unsigned char class,global_bit_set *bit,unsigned char first_time)
{
    conflict_node *conf;
    global_bit_set all_used;
    name *actual_name,*opnd;
    int word;
    opnd=Names[class];
    all_used.bits[0]=all_used.bits[1]=all_used.bits[2]=all_used.bits[3]=0;
    if (class==2) {
        if (!MoreUseInOtherTemps) return all_used;
        MoreUseInOtherTemps=0;
    }
    while (opnd!=NULL) {
        if (opnd->v.usage&0x18) opnd->v.usage|=0x90;
        else if (opnd->v.usage&2) {
            actual_name=opnd;
            if (class==2) actual_name=DeAlias(actual_name);
            if (GEMPTY(*bit) && class==1) actual_name->v.usage|=0x90;
            else {
                conf=actual_name->v.conflict;
                if (conf==NULL && first_time) conf=AddConflictNode(actual_name);
                if (conf!=NULL && !(conf->state&4) && GEMPTY(conf->id.out_of_block)) {
                    conf->id.out_of_block=*bit;
                    for (word=0;word<4;++word) all_used.bits[word]|=bit->bits[word];
                    GBitNext(bit); /* Out-of-line old _GBitNext expansion. */
                    if (GEMPTY(conf->id.out_of_block)) {
                        conf->state|=4;
                        MoreUseInOtherTemps=1;
                    }
                }
            }
        }
        opnd=opnd->n.next_name;
    }
    return all_used;
}
#undef GEMPTY
/* The old _GBitNext operation is a distinct called entry. Its four-word
 * single-bit iterator precedes the released source's bit-set expansion. */
void GBitNext(global_bit_set *bit)
{
    if (bit->bits[0]&0x80000000U) {
        bit->bits[0]=0;bit->bits[1]=1;bit->bits[2]=0;bit->bits[3]=0;
    } else if (bit->bits[1]&0x80000000U) {
        bit->bits[0]=0;bit->bits[1]=0;bit->bits[2]=1;bit->bits[3]=0;
    } else if (bit->bits[2]&0x80000000U) {
        bit->bits[0]=0;bit->bits[1]=0;bit->bits[2]=0;bit->bits[3]=1;
    } else {
        bit->bits[0]<<=1;bit->bits[1]<<=1;
        bit->bits[2]<<=1;bit->bits[3]<<=1;
    }
}

void LiveAnalysis(block *tail,global_bit_set memory_bits);
void SetInOut(block *blk);
void PropagateConflicts(void)
{
    block *blk;
    FindReferences();
    if (!BlockByBlock) LiveAnalysis(HeadBlock,MemoryBits);
    blk=HeadBlock;
    for (;;) {
        SetInOut(blk);
        blk=blk->next_block;
        if (blk==NULL) break;
    }
}
#define GSAME(a,b) ((a).bits[0]==(b).bits[0] && (a).bits[1]==(b).bits[1] && \
                    (a).bits[2]==(b).bits[2] && (a).bits[3]==(b).bits[3])
void LiveAnalysis(block *tail,global_bit_set memory_bits)
{
    block *blk,*target;
    data_flow_def *data;
    global_bit_set new_bits;
    int i,word;
    unsigned char change;
    while (tail->next_block!=NULL) tail=tail->next_block;
    for (;;) {
        change=0;
        blk=tail;
        for (;;) {
            data=blk->dataflow;
            if (blk->class&0x81) new_bits=memory_bits; /* RETURN | LABEL_RETURN */
            else new_bits.bits[0]=new_bits.bits[1]=new_bits.bits[2]=new_bits.bits[3]=0;
            /* The old body unions every successor; no later BIG_JUMP test. */
            i=blk->targets;
            while (--i>=0) {
                target=blk->edge[i].destination;
                for (word=0;word<4;++word) new_bits.bits[word]|=target->dataflow->in.bits[word];
            }
            if (!GSAME(data->out,new_bits)) { data->out=new_bits;change=1; }
            for (word=0;word<4;++word)
                new_bits.bits[word]=(new_bits.bits[word]&~data->def.bits[word])|data->use.bits[word];
            if (!GSAME(data->in,new_bits)) { data->in=new_bits;change=1; }
            blk=blk->prev_block;
            if (blk==NULL) break;
        }
        if (!change) break;
    }
}
#undef GSAME
void SetInOut(block *blk)
{
    int word;
    if (BlockByBlock) {
        blk->dataflow->in=blk->dataflow->use;
        for (word=0;word<4;++word)
            blk->dataflow->in.bits[word]|=blk->dataflow->def.bits[word];
        blk->dataflow->out=blk->dataflow->in;
    }
    if (blk->class&1) blk->ins.hd.live.regs|=CurrProc->state.return_reg;
    blk->ins.hd.live.within_block=0;
    for (word=0;word<4;++word)
        blk->ins.hd.live.out_of_block[word]=blk->dataflow->out.bits[word];
    blk->available_bit=~0U;
}

unsigned char MoreConflicts(void)
{
    global_bit_set bit;
    bit.bits[0]=1;
    bit.bits[1]=bit.bits[2]=bit.bits[3]=0;
    MemoryBits.bits[0]=MemoryBits.bits[1]=MemoryBits.bits[2]=MemoryBits.bits[3]=0;
    bit=AssignGlobalBits(2,&bit,0);
    if ((bit.bits[0]|bit.bits[1]|bit.bits[2]|bit.bits[3])!=0) {
        PropagateConflicts();
        return 1;
    }
    return 0;
}
