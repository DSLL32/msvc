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
* Description:  Prepare 386 memory operands and index addressing.
*
****************************************************************************/


#include "block.h"
#include "genopt.h"
#include "stubs.h"
#include <recon.h>

void FixMemRefs(void)
{
    block *blk;
    instruction *ins;
    /* The 1994 body predates ExpandThreadDataRef and its TLS models. */
    for (blk=HeadBlock;blk!=NULL;blk=blk->next_block) {
        ins=blk->ins.hd.next;
        while (ins->head.opcode!=0x4b) {
            NoMemIndex(ins);
            ins=ins->head.next;
        }
    }
}

/* Released i86stubs.c: target memory-base fixup is an empty hook. */
void FixMemBases(void)
{
}
extern unsigned TargetSwitches;
void AddSegment(instruction *ins);
void FEMessage(unsigned msg,void *value);
void FixSegments(void)
{
    block *blk;
    instruction *ins;
    for (blk=HeadBlock;blk!=NULL;blk=blk->next_block) {
        ins=blk->ins.hd.next;
        while (ins->head.opcode!=0x4b) {
            AddSegment(ins);
            if (!(TargetSwitches&0x5c0) && ins->num_operands>NumOperands(ins)) {
                --ins->num_operands;
                FEMessage(17,AskForLblSym(CurrProc->label));
            }
            ins=ins->head.next;
        }
    }
}
name *FindSegment(instruction *ins);
unsigned char SegIsBase(name **index);
name *GetSegment(name *opnd);
name *NearSegment(void);
name *OffsetPart(name *opnd);
name *SegmentPart(name *opnd);
void MarkSegment(instruction *ins,name *opnd);
conflict_node *NameConflict(instruction *ins,name *opnd);
void AddSegment( instruction *ins ) {
/***********************************************
    Look at instruction "ins" and if any of its operands need a segment
    override, add it to the instruction as an extra operand.
*/

    int                 i;
    name                *index;
    name                *new_index;
    instruction         *new_ins;
    name                **seg_ptr;
    name                *seg;
    conflict_node       *conf;


    index = FindSegment( ins );
    conf = NULL;
    if( index != NULL ) {
        seg_ptr = &ins->operands[ ins->num_operands ];
        if( SegIsBase( &index ) ) {
            if( index != NULL ) {
                seg = GetSegment( index );
                if( seg->n.class != 3 ) {
                    new_ins = MakeMove( seg, AllocTemp( 2 ), 2 );
                    *seg_ptr = new_ins->result;
                    ins->num_operands++;
                    PrefixIns( ins, new_ins );
                    MarkSegment( ins, *seg_ptr );
                    conf = NameConflict( new_ins, *seg_ptr );
                }
            } else {
                *seg_ptr = NearSegment();
                ins->num_operands++;
                if( (*seg_ptr)->n.class == 0 ) {
                    new_ins = MakeMove( *seg_ptr, AllocTemp( 2 ), 2 );
                    *seg_ptr = new_ins->result;
                    PrefixIns( ins, new_ins );
                    MarkSegment( ins, *seg_ptr );
                    conf = NameConflict( new_ins, *seg_ptr );
                }
            }
        } else { /* segment is the high part of the index */
            new_index = ScaleIndex( OffsetPart( index->i.index ),
                                    index->i.base,
                                    index->i.constant,
                                    index->n.name_class,
                                    index->n.size,
                                    index->i.scale,
                                    index->i.index_flags | 1 );
            if( ins->result == index ) {
                ins->result = new_index;
            }
            i = ins->num_operands;
            while( --i >= 0 ) {
                if( ins->operands[ i ] == index ) {
                    ins->operands[ i ] = new_index;
                }
            }
            *seg_ptr = SegmentPart( index->i.index );
            MarkSegment( ins, *seg_ptr );
            ins->num_operands++;
        }
        if( conf != NULL ) {
            conf->state |= 0x0800;
        }
    }
}

unsigned char SegOver(name *opnd);
name *FindSegment(instruction *ins)
{
    name *index;
    int i;
    if (ins->num_operands>NumOperands(ins)) return NULL;
    if (ins->type_class==10 && ins->head.opcode!=0x29) {
        if (ins->head.opcode!=0x2a) return NULL;
        if (ins->operands[0]->n.size>16) return NULL;
    }
    i=ins->num_operands;
    if (ins->head.opcode!=0x28 && ins->head.opcode!=0x27) {
        while (--i>=0) {
            index=ins->operands[i];
            if (SegOver(index)) return index;
        }
    }
    if (ins->result!=NULL && SegOver(ins->result)) return ins->result;
    return NULL;
}
unsigned char SegIsBase(name **index) { STUB_TRAP();return 0; }


unsigned char FPCInCode(void);
unsigned char FltClass(instruction *ins);
name *Addressable(name *opnd,unsigned char class);
void FixFPConsts(instruction *ins)
{
    int i;
    unsigned char class;
    if (!FPCInCode() && (TargetSwitches&0x200) && (TargetSwitches&0x400)) {
        class=FltClass(ins);
        if (class!=10) {
            i=ins->num_operands;
            while (--i>=0) ins->operands[i]=Addressable(ins->operands[i],class);
        }
    }
}
unsigned char FltClass(instruction *ins) { STUB_TRAP();return 10; }


name *FindIndex(instruction *ins);
unsigned char MarkIndex(instruction *ins,name *opnd,unsigned char is_temp_index);
instruction *NeedIndex(instruction *ins)
{
    name *temp,*index,*opnd;
    conflict_node *conf;
    if (ins->num_operands>NumOperands(ins)) {
        opnd=ins->operands[ins->num_operands-1];
        conf=NameConflict(ins,opnd);
        if (conf!=NULL && !(conf->state&0x0400)) {
            conf->state|=0x0200;
            MarkSegment(ins,opnd);
        } else if (opnd->n.class!=3) {
            if (conf!=NULL) {
                conf->state&=~0x1200;
                conf->state|=0x1000;
            }
            temp=AllocTemp(U2);
            ins->operands[ins->num_operands-1]=temp;
            PrefixIns(ins,MakeMove(opnd,temp,U2));
            MarkSegment(ins,temp);
            NameConflict(ins,temp)->state|=0x0800;
            ins=ins->head.prev;
        }
    }
    index=FindIndex(ins);
    if (index!=NULL) {
        temp=IndexToTemp(ins,index);
        conf=NameConflict(ins,temp);
        conf->state|=0x0100;
        if (index->i.base!=NULL && !(index->i.index_flags&0x10) &&
            index->i.base->n.class==2) MarkIndex(ins,temp,1);
        else MarkIndex(ins,temp,0);
        ins=ins->head.prev;
    }
    return ins;
}

extern opcode_entry String[];
unsigned char IsIndexReg(hw_reg_set regs,unsigned char class,unsigned char is_temp_index);
unsigned char IndexOkay(instruction *ins,name *index)
{
    name *opnd;
    unsigned char is_temp_index;
    conflict_node *conf;
    is_temp_index=index->i.base!=NULL && !(index->i.index_flags&0x10) &&
                  index->i.base->n.class==2;
    opnd=index->i.index;
    if (ins->table==String) return 1;
    if (opnd->n.class==3)
        return IsIndexReg(opnd->r.reg,opnd->n.name_class,is_temp_index);
    /* A reduction may leave a complete far index for this later pass. */
    if (opnd->n.name_class==CP || opnd->n.name_class==PT) return 1;
    if (opnd->v.conflict==NULL || (opnd->v.usage&0x10) || opnd->n.class!=2)
        return 0;
    conf=NameConflict(ins,opnd);
    if (conf==NULL) return 0;
    if (conf->state&0x80) {
        conf->state&=~0x40;
        return 0;
    }
    conf->state|=0x40;
    ins->head.state=1;
    ins->t.index_needs=MarkIndex(ins,opnd,is_temp_index);
    return 1;
}

void PropSegments(void);
unsigned char NoSegments(unsigned char possible);
void FixChoices(void)
{
    conflict_node *conf;
    name *temp,*alias;
    PropSegments();
    for (conf=ConfList;conf!=NULL;conf=conf->next_conflict) {
        if (!(conf->state&0x1e00)) { /* VALID_SEGMENT */
            conf->possible=NoSegments(conf->possible);
            temp=conf->name;
            if (temp->n.class==2) {
                alias=temp;
                do {
                    alias=(name *)alias->t.alias;
                    alias->t.possible=NoSegments(alias->t.possible);
                } while (alias!=temp);
            }
        }
    }
}
void PropSegments(void)
{
    block *blk;
    instruction *ins;
    conflict_node *src,*dst;
    unsigned char change;
    for (src=ConfList;src!=NULL;src=src->next_conflict)
        if (src->state&0x1e00) src->state|=0x1000;
    do {
        change=0;
        for (blk=HeadBlock;blk!=NULL;blk=blk->next_block) {
            ins=blk->ins.hd.prev;
            while (ins->head.opcode!=0x4b) {
                if (ins->head.opcode==0x26) {
                    dst=NameConflict(ins,ins->result);
                    if (dst!=NULL && (dst->state&0x1000)) {
                        src=NameConflict(ins,ins->operands[0]);
                        if (src!=NULL && !(src->state&0x1000)) {
                            src->state|=0x1000;
                            change=1;
                        }
                    }
                }
                ins=ins->head.prev;
            }
        }
    } while (change);
}

void Merge(name **owner,instruction *ins);
void MergeIndex(void)
{
    block *blk;
    instruction *ins;
    int i;
    name **owner;
    unsigned char dec;
    for (blk=HeadBlock;blk!=NULL;blk=blk->next_block) {
        for (ins=blk->ins.hd.next;ins->head.opcode!=0x4b;ins=ins->head.next) {
            i=NumOperands(ins);
            if (i<ins->num_operands) {
                dec=0;
                while (--i>=0) {
                    owner=&ins->operands[i];
                    if ((*owner)->n.class==4) {
                        Merge(owner,ins);
                        dec=1;
                    }
                }
                if (ins->result!=NULL) {
                    owner=&ins->result;
                    if ((*owner)->n.class==4) {
                        Merge(owner,ins);
                        dec=1;
                    }
                }
                if (dec) --ins->num_operands;
            }
        }
    }
}
void Merge(name **owner,instruction *ins) { STUB_TRAP(); }
