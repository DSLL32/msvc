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


#include "score.h"
#include "stubs.h"
#include <recon.h>

/* Address identity distinguishes segment constants from other constants. */
static void *SegConstSymbol;
void ScoreInfo(score_info *info,name *op)
{
    if (op->n.class==4 && op->i.index_flags==0x30) op=op->i.base;
    info->class=op->n.class;
    switch (op->n.class) {
    case 0:
        switch (op->c.const_type) {
        case 0: info->symbol.p=NULL;info->offset=op->c.int_value;break;
        case 3: info->symbol.p=&SegConstSymbol;info->offset=op->c.int_value;break;
        case 1: case 2:
            info->symbol.p=op->c.value;info->offset=op->c.int_value;break;
        default: Zoiks(46);break;
        }
        info->index_reg=NO_INDEX;
        info->base=NULL;
        break;
    case 2:
        if (op->v.usage&0x20) info->class=6;
        info->symbol.t=(temp_name *)op;
        info->offset=op->v.offset;
        info->index_reg=NO_INDEX;
        info->base=NULL;
        break;
    case 1:
        if (op->v.usage&0x20) info->class=6;
        info->symbol.p=op->v.symbol;
        info->offset=op->v.offset;
        info->index_reg=NO_INDEX;
        info->base=NULL;
        break;
    case 4:
        if (op->i.index_flags&2) info->class=6;
        info->symbol.p=NULL;
        info->offset=op->i.constant;
        info->index_reg=op->i.index->r.reg_index;
        info->base=op->i.base;
        break;
    }
}

unsigned char ScoreSame(score_info *left,score_info *right)
{
    if (left->class!=right->class || left->base!=right->base
      || left->index_reg!=right->index_reg) return 0;
    if (left->class==2 && left->symbol.t->id.id==right->symbol.t->id.id) return 1;
    if (left->class==6 || left->class==5) return 0;
    return left->symbol.p==right->symbol.p;
}
unsigned char ScoreLookup(score *sc,score_info *info)
{
    score_list *curr;
    if (info->class==6) return 0;
    curr=*sc->list;
    for (;;) {
        if (curr==NULL) break;
        if (ScoreSame(&curr->info,info) && curr->info.offset==info->offset) return 1;
        curr=curr->next;
    }
    return 0;
}
unsigned char ScoreEqual(score *sc,int index,score_info *info)
{
    /* The 1994 optimizer has no later SUPER_OPTIMAL register-half fallback. */
    return ScoreLookup(&sc[index],info)!=0;
}

extern score_reg **ScoreList;
score_list *NewScListEntry(void);
void RegAdd(score *sc,int dst,int src);
unsigned char ScoreEqual(score *sc,int index,score_info *info);
void ScoreInsert(score *sc,int index,score_info *info)
{
    score_list *entry;
    int j;
    if (info->class==6) return;
    if (ScoreList[index]->reg&CurrProc->state.unalterable) return;
    entry=NewScListEntry();
    Copy(info,&entry->info,sizeof(score_info));
    entry->next=*sc[index].list;
    *sc[index].list=entry;
    j=ScoreCount;
    for (;;) {
        if (--j<0) return;
        if (j!=index && ScoreEqual(sc,j,info)) break;
    }
    RegAdd(sc,index,j);
}
void ScoreAssign(score *sc,int index,score_info *info)
{
    /* Later SUPER_OPTIMAL half-register propagation is absent in this image. */
    if (!ScoreLookup(&sc[index],info)) ScoreInsert(sc,index,info);
}

unsigned char ScoreStomp(score_info *left,score_info *right)
{
    score_info *temp;
    if (right->class==4 && right->base==NULL) {
        temp=left;left=right;right=temp;
    }
    if (left->class==4 && left->base==NULL) {
        switch (right->class) {
        case 1:
            if (Model&0x04000000U) return 0; /* RELAX_ALIAS. */
            /* fall through */
        case 4: return 1;
        case 2: return (right->symbol.v->usage&8)!=0; /* USE_ADDRESS. */
        }
    }
    if (right->class==4) { temp=left;left=right;right=temp; }
    if (left->class==4 && left->base!=NULL) {
        switch (right->class) {
        case 2:
            if (left->base->n.class==2 && right->symbol.t->id.id==left->base->t.id.id)
                return 1;
            break;
        case 1:
            if (left->base->n.class==1 && right->symbol.p==left->base->v.symbol)
                return 1;
            break;
        case 4:
            if (right->base==NULL) return 1;
            if (right->base->n.class!=left->base->n.class) return 0;
            switch (left->base->n.class) {
            case 2: return right->base->t.id.id==left->base->t.id.id;
            case 1: return right->base->v.symbol==left->base->v.symbol;
            }
            break;
        }
    }
    return 0;
}
void FreeScListEntry(score_list *entry);
void ScoreKillInfo(score *scoreboard,name *op,score_info *info,hw_reg_set except)
{
    score_list *curr,**owner;
    score_reg *entry;
    int last_offset,i;
    last_offset=info->offset+op->n.size;
    entry=ScoreList[0];
    i=ScoreCount;
    while (--i>=0) {
        if ((except&entry->reg)!=entry->reg) {
            owner=scoreboard->list;
            for (;;) {
                curr=*owner;
                if (curr==NULL) break;
                if (!ScoreStomp(info,&curr->info)) {
                    if (ScoreSame(info,&curr->info)
                      && last_offset>curr->info.offset
                      && info->offset<curr->info.offset+entry->size) {
                        *owner=curr->next;
                        FreeScListEntry(curr);
                        continue;
                    }
                    owner=&curr->next;
                } else {
                    *owner=curr->next;
                    FreeScListEntry(curr);
                }
            }
        }
        ++entry;
        ++scoreboard;
    }
}

unsigned char ScoreLAInfo(score_info *info,name *op)
{
    switch (op->n.class) {
    case 2: case 1:
        info->class=7; /* N_ADDRESS. */
        info->symbol.p=op;
        info->offset=0;
        info->index_reg=NO_INDEX;
        info->base=NULL;
        return 1;
    default: return 0;
    }
}
