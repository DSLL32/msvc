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
* Description:  Allocate and initialize variable conflict nodes.
*
****************************************************************************/


/* cgen2.c's emission-state initializer. In the earlier target the next
 * imported segment starts at -1 directly, before the later SegData field. */
#include "codegen.h"
#include "block.h"
#include "genopt.h"
#include "stubs.h"
#include <recon.h>
struct conflict_node;
struct conflict_node *ConfList;
static void *ConfFrl;

void InitConflict(void)
{
    InitFrl(&ConfFrl);
    ConfList=NULL;
}

conflict_node *AddConflictNode(name *opnd)
{
    conflict_node *new_node;
    name *scan;
    unsigned short flags;
    unsigned attr;
    if (opnd->n.class==2) opnd=DeAlias(opnd);
    new_node=AllocFrl(&ConfFrl,sizeof(conflict_node));
    new_node->name=opnd;
    new_node->next_conflict=ConfList;
    new_node->next_for_name=opnd->v.conflict;
    new_node->id.out_of_block.bits[0]=0;
    new_node->id.out_of_block.bits[1]=0;
    new_node->id.out_of_block.bits[2]=0;
    new_node->id.out_of_block.bits[3]=0;
    new_node->id.within_block=0;
    new_node->start_block=NULL;
    new_node->ins_range.first=NULL;
    new_node->ins_range.last=NULL;
    new_node->num_constrained=0;
    new_node->available=0;
    new_node->possible=26; /* Actual RL_NUMBER_OF_SETS. */
    new_node->savings=0;
    new_node->tree=NULL;
    new_node->state=0;
    if (opnd->n.class==2) {
        flags=1; /* HAD_CONFLICT */
        if (new_node->next_for_name!=NULL) flags|=0x80; /* CROSSES_BLOCKS */
        scan=opnd;
        for (;;) {
            scan->v.conflict=new_node;
            scan->t.temp_flags|=flags;
            scan=(name *)scan->t.alias;
            if (scan==opnd) break;
        }
    } else {
        if (opnd->m.memory_type==0) {
            attr=FEAttr(opnd->v.symbol);
            if ((attr&0x10) || (!(attr&0x44) && (Model&0x4000000)))
                new_node->state|=0x2000; /* OK_ACROSS_CALLS */
        }
        opnd->v.conflict=new_node;
    }
    ConfList=new_node;
    return new_node;
}

void FreeAConflict(conflict_node *conf);
void FreeConflicts(void)
{
    while (ConfList!=NULL) FreeAConflict(ConfList);
}
void FreeAConflict(conflict_node *conf)
{
    name *opnd,*scan;
    conflict_node *prev;
    if (conf->start_block!=NULL)
        conf->start_block->available_bit|=conf->id.within_block;
    opnd=conf->name;
    prev=opnd->v.conflict;
    if (prev==conf) {
        if (opnd->n.class==2) {
            scan=opnd;
            for (;;) {
                scan->v.conflict=conf->next_for_name;
                scan->t.temp_flags|=1; /* HAD_CONFLICT */
                scan=(name *)scan->t.alias;
                if (scan==opnd) break;
            }
        } else opnd->v.conflict=conf->next_for_name;
    } else {
        while (prev->next_for_name!=conf) prev=prev->next_for_name;
        prev->next_for_name=conf->next_for_name;
    }
    prev=ConfList;
    if (prev==conf) ConfList=conf->next_conflict;
    else {
        while (prev->next_conflict!=conf) prev=prev->next_conflict;
        prev->next_conflict=conf->next_conflict;
    }
    if (opnd->n.class==2) {
        opnd->t.u.block_id=0xffff;
        if (((opnd->t.temp_flags&0x100) || opnd->v.symbol==NULL) &&
            opnd->v.conflict==NULL && !(opnd->v.usage&2) &&
            conf->start_block!=NULL && !(opnd->t.temp_flags&0x80))
            opnd->t.u.block_id=conf->start_block->id;
    }
    FrlFreeSize(&ConfFrl,conf,sizeof(conflict_node));
}

conflict_node *FindConf(name *opnd,block *blk,instruction *ins);
conflict_node *FindConflictNode(name *opnd,block *blk,instruction *ins)
{
    conflict_node *conf;
    name *old,*scan;
    old=opnd;
    if (opnd->n.class==2) opnd=DeAlias(opnd);
    else if (opnd->n.class!=1 || !(Model&0x04000000U)) return NULL;
    conf=FindConf(opnd,blk,ins);
    if (conf==NULL) return NULL;
    if (conf->start_block==NULL) {
        conf->ins_range.first=ins;
        conf->ins_range.last=ins;
        conf->start_block=blk;
    } else {
        /* Actual old instruction IDs use signed comparisons here. */
        if ((long)ins->id<(long)conf->ins_range.first->id) {
            conf->ins_range.first=ins;
            conf->start_block=blk;
        }
        if ((long)ins->id>(long)conf->ins_range.last->id)
            conf->ins_range.last=ins;
    }
    if (opnd->n.class==2) {
        scan=opnd;
        for (;;) {
            scan->v.conflict=opnd->v.conflict;
            scan->t.temp_flags|=1;
            scan=(name *)scan->t.alias;
            if (scan==opnd) break;
        }
    } else old->v.conflict=opnd->v.conflict;
    return conf;
}
conflict_node *AddOne(name *opnd,block *blk);
conflict_node *FindConf(name *opnd,block *blk,instruction *ins)
{
    conflict_node *conf;
    conf=opnd->v.conflict;
    if (conf==NULL) return AddOne(opnd,blk);
    if (conf->start_block==NULL) return conf;
    for (;;) {
        if (conf->start_block!=NULL &&
            (long)ins->id>=(long)conf->ins_range.first->id &&
            (long)ins->id<=(long)conf->ins_range.last->id) return conf;
        conf=conf->next_for_name;
        if (conf==NULL) break;
    }
    conf=opnd->v.conflict;
    if (!(opnd->v.usage&2)) {
        for (;;) {
            if (conf->start_block==blk) break;
            conf=conf->next_for_name;
            if (conf==NULL) return AddOne(opnd,blk);
        }
    }
    return conf;
}
conflict_node *AddOne(name *opnd,block *blk)
{
    if (opnd->v.usage&0x10) {
        if (opnd->n.class==2 && !(opnd->v.usage&2) &&
            !(opnd->t.temp_flags&1)) {
            if (opnd->t.temp_flags&0x80) opnd->t.u.block_id=0xffff;
            else opnd->t.u.block_id=blk->id;
        }
        return NULL;
    }
    return AddConflictNode(opnd);
}

void MarkSegment(instruction *ins,name *opnd) { STUB_TRAP(); }
conflict_node *NameConflict(instruction *ins,name *opnd)
{
    conflict_node *conf;
    if (opnd->n.class==2) opnd=DeAlias(opnd);
    else if (opnd->n.class!=1) return NULL;
    conf=opnd->v.conflict;
    while (conf!=NULL) {
        if ((long)conf->ins_range.first->id<=(long)ins->id &&
            (long)conf->ins_range.last->id>=(long)ins->id) return conf;
        conf=conf->next_for_name;
    }
    return NULL;
}

unsigned char RegIntersect(unsigned char left,unsigned char right);
void    MarkPossible( instruction *ins,
                              name *opnd, unsigned char idx ) {
/*************************************************************/


    conflict_node       *conf;
    unsigned char       possible;

    conf = NameConflict( ins, opnd );
    if( conf != NULL ) {
        if( opnd->n.class == 2 ) {
            if( !( opnd->t.temp_flags & 2 ) ) {
                possible = RegIntersect( conf->possible, idx );
                conf->possible = possible;
            } else {
                possible = RegIntersect( opnd->t.possible, idx );
            }
            opnd->t.possible = possible;
        } else {
            conf->possible = RegIntersect( conf->possible, idx );
        }
    }
}




unsigned char IndexIntersect(unsigned char current,unsigned char class,unsigned char is_temp_index);
unsigned char MarkIndex(instruction *ins,name *opnd,unsigned char is_temp_index)
{
    conflict_node *conf;
    unsigned char possible,class;
    conf=NameConflict(ins,opnd);
    if (conf==NULL) return 0;
    class=opnd->n.name_class;
    if (opnd->n.class==2) {
        if (!(opnd->t.temp_flags&2)) {
            possible=IndexIntersect(conf->possible,class,is_temp_index);
            conf->possible=possible;
        } else possible=IndexIntersect(opnd->t.possible,class,is_temp_index);
        opnd->t.possible=possible;
    } else {
        possible=IndexIntersect(conf->possible,class,is_temp_index);
        conf->possible=possible;
    }
    return possible;
}
