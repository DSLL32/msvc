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
* Description:  Maintain backend instruction rings, IDs and live ranges.
*
****************************************************************************/


#include "block.h"
#include "genopt.h"
#include "stubs.h"
#include <recon.h>

unsigned Renumber(void)
{
    block *blk;
    instruction *ins;
    unsigned id,increment,count;
    count=0;
    blk=HeadBlock;
    while (blk!=NULL) {
        count+=CountIns(blk);
        blk=blk->next_block;
    }
    if (count==0) increment=1;
    else if (count>0x7fffffffU) {
        Zoiks(69);
        increment=1;
    } else {
        /* The earlier implementation spreads IDs through the available
         * range; the released source later changed this to a fixed ten. */
        increment=0x7fffffffU/count;
    }
    id=0;
    blk=HeadBlock;
    while (blk!=NULL) {
        ins=blk->ins.hd.next;
        while (ins->head.opcode!=0x4b) { /* OP_BLOCK */
            ins->id=id;
            id+=increment;
            ins=ins->head.next;
        }
        blk=blk->next_block;
    }
    return id;
}

void FlipCond(instruction *ins);
/* Actual old MAX_OPS_PER_INS=4 gives eleven 8-byte conflict slots. */
#pragma pack(4)
typedef struct conflict_info { conflict_node *conf;unsigned char flags; } conflict_info;
#pragma pack()
typedef char old_conflict_info_size[(sizeof(conflict_info)==8)?1:-1];
static conflict_info ConflictInfo[11];
void RenumFrom(instruction *ins);
void MakeConflictInfo(instruction *ins1,instruction *ins2);
void UpdateLive(instruction *first,instruction *last);
void SuffixIns(instruction *ins,instruction *suff)
{
    conflict_info *info;
    conflict_node *conf;
    suff->head.next=ins->head.next;
    suff->head.next->head.prev=suff;
    suff->head.prev=ins;
    ins->head.next=suff;
    suff->head.line_num=0;
    suff->id=ins->id;
    if (ins->head.opcode==0 && (ins->flags.nop_flags&4)) {
        suff->head.line_num=ins->head.line_num;
        ins=ins->head.prev;
        FreeIns(ins->head.next);
    }
    if (ins->head.opcode!=0x4b) {
        suff->stk_entry=ins->stk_exit;
        suff->stk_exit=ins->stk_exit;
        suff->sequence=ins->sequence;
    }
    RenumFrom(ins);
    if (HaveLiveInfo) {
        if (ins->head.opcode!=0x4b) {
            MakeConflictInfo(ins,suff);
            for (info=ConflictInfo;info->conf!=NULL;++info) {
                if (info->flags&2) {
                    conf=info->conf;
                    if (!(info->flags&1) && conf->ins_range.first==ins)
                        conf->ins_range.first=suff;
                    if (conf->ins_range.last==ins) conf->ins_range.last=suff;
                }
            }
        }
        ins->head.state=0;
        suff->head.state=0;
        UpdateLive(ins,suff);
    }
}
void RenumFrom(instruction *ins)
{
    block *blk;
    unsigned id;
    if (ins->head.opcode==0x4b) Renumber();
    else {
        id=ins->id;
        ins=ins->head.next;
        for (;;) {
            while (ins->head.opcode!=0x4b) {
                ++id;
                /* The old instruction-ID comparison is signed. */
                if ((long)ins->id>(long)id) return;
                ins->id=id;
                ins=ins->head.next;
            }
            blk=((block_ins *)ins)->blk->next_block;
            if (blk==NULL) break;
            ins=blk->ins.hd.next;
        }
    }
}
static int CurrInfo;
void FindAllConflicts(instruction *ins,instruction *other,unsigned char for_which);
void MakeConflictInfo(instruction *ins1,instruction *ins2)
{
    int i;
    conflict_info *info;
    for (i=0;i<11;++i) {
        ConflictInfo[i].conf=NULL;
        ConflictInfo[i].flags=0;
    }
    CurrInfo=0;
    FindAllConflicts(ins1,ins2,1);
    FindAllConflicts(ins2,ins1,2);
    for (info=ConflictInfo;info->conf!=NULL;++info) info->conf->state&=~0x4000;
}
void FindNameConf(name *opnd,instruction *ins,instruction *other,unsigned char for_which);
void FindAllConflicts(instruction *ins,instruction *other,unsigned char for_which)
{
    int i;
    i=ins->num_operands;
    while (--i>=0) FindNameConf(ins->operands[i],ins,other,for_which);
    if (ins->result!=NULL) FindNameConf(ins->result,ins,other,for_which);
}
conflict_info *AddConfInfo(conflict_node *conf);
void FindNameConf(name *opnd,instruction *ins,instruction *other,unsigned char for_which)
{
    conflict_node *conf;
    conflict_info *info;
    if (opnd->n.class==4) opnd=opnd->i.index;
    if (opnd->n.class==2) opnd=DeAlias(opnd);
    else if (opnd->n.class!=1) return;
    conf=opnd->v.conflict;
    info=NULL;
    while (conf!=NULL && info==NULL) {
        if (conf->ins_range.first==ins || conf->ins_range.first==other ||
            conf->ins_range.last==ins || conf->ins_range.last==other) {
            info=AddConfInfo(conf);
            info->flags|=for_which;
        }
        conf=conf->next_for_name;
    }
}
conflict_info *AddConfInfo(conflict_node *conf)
{
    conflict_info *info;
    if (conf->state&0x4000) {
        for (info=ConflictInfo;;++info) if (info->conf==conf) return info;
    } else {
        info=&ConflictInfo[CurrInfo++];
        info->conf=conf;
        conf->state|=0x4000;
    }
    return info;
}


void PrefixIns(instruction *ins,instruction *pref)
{
    conflict_info *info;
    conflict_node *conf;
    pref->head.prev=ins->head.prev;
    pref->head.prev->head.next=pref;
    pref->head.next=ins;
    ins->head.prev=pref;
    if (ins->head.opcode!=0x4b) {
        pref->head.line_num=ins->head.line_num;
        ins->head.line_num=0;
    } else pref->head.line_num=0;
    pref->id=ins->id;
    if (ins->head.opcode!=0x4b) {
        pref->stk_entry=ins->stk_entry;
        pref->stk_exit=ins->stk_entry;
        pref->sequence=ins->sequence;
    }
    RenumFrom(pref);
    if (HaveLiveInfo) {
        if (ins->head.opcode!=0x4b) {
            MakeConflictInfo(pref,ins);
            for (info=ConflictInfo;info->conf!=NULL;++info) {
                if (info->flags&1) {
                    conf=info->conf;
                    if (!(info->flags&2) && conf->ins_range.last==ins)
                        conf->ins_range.last=pref;
                    if (conf->ins_range.first==ins) conf->ins_range.first=pref;
                }
            }
        }
        ins->head.state=0;
        pref->head.state=0;
        UpdateLive(pref,ins);
    }
}



block *GetBlockPointer(block *blk,instruction *ins);
void ReplIns(instruction *ins,instruction *new_ins)
{
    block *blk;
    conflict_info *info;
    conflict_node *conf;
    new_ins->head.next=ins->head.next;
    new_ins->head.next->head.prev=new_ins;
    new_ins->head.prev=ins;
    ins->head.next=new_ins;
    new_ins->stk_entry=ins->stk_entry;
    new_ins->stk_exit=ins->stk_exit;
    new_ins->sequence=ins->sequence;
    new_ins->head.line_num=ins->head.line_num;
    new_ins->id=ins->id;
    new_ins->head.state=0;
    blk=NULL;
    if (HaveLiveInfo) {
        new_ins->head.live=ins->head.live;
        MakeConflictInfo(ins,new_ins);
        for (info=ConflictInfo;info->conf!=NULL;++info) {
            if (info->flags&1) {
                conf=info->conf;
                if (conf->ins_range.first==ins) conf->ins_range.first=new_ins;
                if (conf->ins_range.last==ins) conf->ins_range.last=new_ins;
                if (info->flags&2) continue;
                if (conf->ins_range.first==new_ins) {
                    blk=GetBlockPointer(blk,ins);
                    conf->ins_range.first=blk->ins.hd.prev;
                }
                if (conf->ins_range.last==new_ins) {
                    blk=GetBlockPointer(blk,ins);
                    /* Old body predates the later extra ins-next adjustment. */
                    conf->ins_range.last=blk->ins.hd.next;
                }
            }
        }
    }
    FreeIns(ins);
    if (blk!=NULL) UpdateLive(blk->ins.hd.next,blk->ins.hd.prev);
}
block *GetBlockPointer(block *blk,instruction *ins) { STUB_TRAP();return NULL; }

void ClearInsBits(unsigned char mask)
{
    block *blk;
    instruction *ins;
    for (blk=HeadBlock;blk!=NULL;blk=blk->next_block)
        for (ins=blk->ins.hd.next;ins->head.opcode!=0x4b;ins=ins->head.next)
            ins->ins_flags&=~mask;
}
