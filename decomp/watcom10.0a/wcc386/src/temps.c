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
#include "cgmemory.h"
#include "pragma.h"
#include "genopt.h"
#include "stubs.h"
#include <recon.h>

#pragma pack(4)
typedef struct stack_temp {
    struct stack_temp *others;
    int first,last; /* instruction_id is int in the released inslist.h. */
} stack_temp;
typedef struct stack_entry {
    stack_temp temp;
    struct stack_entry *link;
    long location,size;
} stack_entry;
#pragma pack()
typedef char old_stack_temp_size[(sizeof(stack_temp)==12)?1:-1];
typedef char old_stack_entry_size[(sizeof(stack_entry)==24)?1:-1];
static stack_entry *StackMap;
void InitStackMap(void)
{
    StackMap=NULL;
}

void PropAParm(name *temp)
{
    name *base;
    if (temp->n.class==4) {
        temp=temp->i.base;
        if (temp==NULL) return;
    }
    if (temp->n.class!=2 || (temp->v.usage&0x40) || temp->t.location==0x7fffffffL) return;
    base=DeAlias(temp);
    if (base!=temp) PropAParm(base);
    temp->t.location+=temp->v.offset-base->v.offset;
    temp->v.usage|=0x40;
    temp->t.temp_flags|=0x20;
}
void ParmPropagate(void)
{
    instruction *ins;
    block *blk;
    int i;
    for (blk=HeadBlock;blk!=NULL;blk=blk->next_block) {
        for (ins=blk->ins.hd.next;ins->head.opcode!=0x4b;ins=ins->head.next) {
            i=ins->num_operands;
            while (--i>=0) PropAParm(ins->operands[i]);
            if (ins->result!=NULL) PropAParm(ins->result);
        }
    }
}
void PushLocals(void);
unsigned char TempAllocBefore(void *left,void *right);
void *SortList(void *list,unsigned next_offset,unsigned char (*before)(void *,void *));
void CalcNumberOfUses(void);
void AllocNewLocal(name *temp);
void AssignOtherLocals(void)
{
    name *temp,**owner,*rest;
    if (LastTemp!=NULL) {
        rest=LastTemp->n.next_name;
        LastTemp->n.next_name=NULL;
    }
    Names[2]=SortList(Names[2],offsetof(name,n.next_name),TempAllocBefore);
    if (LastTemp!=NULL) {
        owner=&Names[2];
        while (*owner!=NULL) owner=&(*owner)->n.next_name;
        *owner=rest;
    }
    if (!BlockByBlock) CalcNumberOfUses();
    for (temp=Names[2];temp!=LastTemp;temp=temp->n.next_name) {
        if (!(temp->v.usage&0x80) || (temp->v.usage&0x40) || (temp->t.temp_flags&2)) continue;
        AllocNewLocal(temp);
    }
}
void AssignTemps(void)
{
    TransferTempFlags();
    ParmPropagate();
    PushLocals();
    AssignOtherLocals();
}

void MarkUsage(name *op)
{
    for (;;) {
        if (op->n.class==4) {
            if (op->i.index!=NULL) MarkUsage(op->i.index);
            if (op->i.base!=NULL && !(op->i.index_flags&0x10)) {
                op=op->i.base;
                continue;
            }
        } else if (op->n.class==2) {
            op=DeAlias(op);
            op->v.block_usage=op->v.block_usage==0 ? 1 : 2;
        }
        return;
    }
}
void CalcNumberOfUses(void)
{
    name *temp;
    block *blk;
    instruction *ins;
    int i;
    for (temp=Names[2];temp!=NULL;temp=temp->n.next_name)
        temp->v.block_usage=(temp->t.temp_flags&0x20) ? 2 : 0;
    for (blk=HeadBlock;blk!=NULL;blk=blk->next_block) {
        for (ins=blk->ins.hd.next;ins->head.opcode!=0x4b;ins=ins->head.next) {
            i=ins->num_operands;
            while (--i>=0) MarkUsage(ins->operands[i]);
            if (ins->result!=NULL) MarkUsage(ins->result);
        }
    }
}
void ScanForFirstDefn(block *blk,stack_temp *range,name *temp)
{
    instruction *ins;
    int i;
    ins=blk->ins.hd.next;
    for (;;) {
        if (ins->head.opcode==0x4b) {
            range->first=0;
            return;
        }
        if (ins->result!=NULL && ins->result->n.class==2 && DeAlias(ins->result)==temp) {
            range->first=ins->id;
            return;
        }
        i=ins->num_operands;
        while (--i>=0) {
            if (ins->operands[i]->n.class==2 && DeAlias(ins->operands[i])==temp) {
                range->first=ins->id;
                return;
            }
        }
        ins=ins->head.next;
    }
}
unsigned char SetLastUse(name *op,name *temp,stack_temp *range,instruction *ins)
{
    if (op->n.class==4 && op->i.base!=NULL) op=op->i.base;
    if (op->n.class==2 && DeAlias(op)==temp) {
        range->last=ins->id;
        return 1;
    }
    return 0;
}
void ScanForLastUse(block *blk,stack_temp *range,name *temp)
{
    instruction *ins;
    int i;
    ins=blk->ins.hd.prev;
    for (;;) {
        if (ins->head.opcode==0x4b) {
            range->last=range->first;
            return;
        }
        if (ins->result!=NULL && ins->result->n.class==2 && DeAlias(ins->result)==temp) {
            if (SideEffect(ins)) {
                range->last=range->first+1;
                return;
            }
            DoNothing(ins);
        } else {
            i=ins->num_operands;
            while (--i>=0) if (SetLastUse(ins->operands[i],temp,range,ins)) return;
            if (ins->result!=NULL && SetLastUse(ins->result,temp,range,ins)) return;
        }
        ins=ins->head.prev;
    }
}
void CalcRange(stack_temp *range,name *temp)
{
    block *blk;
    blk=HeadBlock;
    while (blk->id!=temp->t.u.block_id) blk=blk->next_block;
    ScanForFirstDefn(blk,range,temp);
    ScanForLastUse(blk,range,temp);
}
stack_entry *ReUsableStack(stack_temp *range,name *temp)
{
    stack_entry *stack;
    stack_temp *other;
    long size;
    stack=StackMap;
    size=temp->n.size;
    while (stack!=NULL) {
        other=&stack->temp;
        if (size<=stack->size) {
            for (;;) {
                if (other->first<=range->last && other->last>=range->first) break;
                other=other->others;
                if (other==NULL) return stack;
            }
        }
        stack=stack->link;
    }
    return stack;
}
void StackEntry(stack_temp *range,name *temp)
{
    stack_entry *entry;
    entry=CGAlloc(sizeof(*entry));
    entry->link=StackMap;
    entry->size=temp->n.size;
    entry->location=temp->t.location;
    entry->temp.first=range->first;
    entry->temp.last=range->last;
    entry->temp.others=NULL;
    StackMap=entry;
}
void SetTempLocation(name *temp,long size);
void PropLocal(name *temp);
void NewLocation(name *temp,long size)
{
    SetTempLocation(temp,size);
    temp->v.usage|=0x40;
    PropLocal(temp);
}
unsigned char In(name *op,name *temp) { STUB_TRAP();return 0; }
instruction *FindOnlyIns(name *temp,unsigned char *referenced)
{
    block *blk;
    instruction *ins,*only;
    int i;
    if (temp->v.block_usage==0) {
        *referenced=0;
        return NULL;
    }
    if (temp->v.block_usage==2) {
        *referenced=1;
        return NULL;
    }
    *referenced=1;
    only=NULL;
    for (blk=HeadBlock;blk!=NULL;blk=blk->next_block) {
        for (ins=blk->ins.hd.next;ins->head.opcode!=0x4b;ins=ins->head.next) {
            i=ins->num_operands;
            while (--i>=0) {
                if (In(ins->operands[i],temp)) {
                    if (only!=NULL) return NULL;
                    only=ins;
                }
            }
            if (ins->result!=NULL && In(ins->result,temp)) {
                if (only!=NULL) return NULL;
                only=ins;
            }
        }
    }
    if (only==NULL) *referenced=0;
    return only;
}
void AllocNewLocal(name *temp)
{
    long size;
    stack_entry *stack;
    stack_temp *new_st_temp,st_temp;
    instruction *ins;
    unsigned char any_references;
    size=(temp->n.size+3)&~3L;
    if (!(temp->v.usage&0xa) && temp->t.u.block_id!=0xffffU) {
        CalcRange(&st_temp,temp);
        if (st_temp.first!=st_temp.last) {
            stack=ReUsableStack(&st_temp,temp);
            if (stack!=NULL) {
                temp->t.location=stack->location;
                new_st_temp=CGAlloc(sizeof(stack_temp));
                new_st_temp->first=st_temp.first;
                new_st_temp->last=st_temp.last;
                new_st_temp->others=stack->temp.others;
                stack->temp.others=new_st_temp;
            } else {
                SetTempLocation(temp,size);
                StackEntry(&st_temp,temp);
            }
            temp->v.usage|=0x40;
            PropLocal(temp);
        } else {
            temp->v.usage&=~0x80;
        }
    } else {
        if (BlockByBlock || (temp->v.usage&8)) {
            NewLocation(temp,size);
            return;
        }
        ins=FindOnlyIns(temp,&any_references);
        /* Later UsedByLA protection is absent from the actual1994 CFG. */
        if (ins==NULL || SideEffect(ins)) {
            if (any_references) NewLocation(temp,size);
            else temp->v.usage&=~0x80;
            return;
        }
        DoNothing(ins);
        temp->v.usage&=~0x80;
    }
}


void PropLocal(name *temp)
{
    name *scan;
    scan=(name *)temp->t.alias;
    while (scan!=temp) {
        scan->t.location=temp->t.location;
        scan->v.usage|=0x40; /* HAS_MEMORY. */
        scan=(name *)scan->t.alias;
    }
}

void TellTempLocs(void)
{
    name *temp;
    int ans;
    for (temp=Names[2];temp!=NULL;temp=temp->n.next_name) {
        if (temp->v.symbol!=NULL && !(temp->t.temp_flags&2)) {
            ans=(int)FEAuxInfo(temp->v.symbol,11); /* TEMP_LOC_NAME */
            if (ans==0) break;
            /* The1994 Intel body tells the raw relative location; the later
             * NO_LOCATION check and TempLocation helper are absent. */
            if (ans==2) FEAuxInfo((void *)temp->t.location,12);
        }
    }
}
void FiniStackMap(void)
{
    stack_entry *junk1;
    stack_temp *junk2;
    name *temp;
    for (temp=Names[2];temp!=NULL;temp=temp->n.next_name) {
        if ((temp->t.temp_flags&2) || !(temp->v.usage&8) || (temp->v.usage&0x40)) continue;
        AllocNewLocal(temp);
    }
    while (StackMap!=NULL) {
        junk1=StackMap;
        StackMap=StackMap->link;
        while (junk1->temp.others!=NULL) {
            junk2=junk1->temp.others;
            junk1->temp.others=junk2->others;
            CGFreeSize(junk2,sizeof(stack_temp));
        }
        CGFreeSize(junk1,sizeof(stack_entry));
    }
    TellTempLocs();
}
