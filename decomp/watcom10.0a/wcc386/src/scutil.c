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
#include "score.h"
#include "cgmemory.h"
#include "stubs.h"
#include <recon.h>

extern score_reg **ScoreList;
extern void *ScListFrl;
extern name *Names[5];
void AddRegs(void);
unsigned char ScRealRegister(name *reg_name);
hw_reg_set LowOffsetReg(hw_reg_set reg);
hw_reg_set HighOffsetReg(hw_reg_set reg);

void ScoreCalcList(void) {
/*******************************/

    score_reg   *curr;
    name        *reg_name;
    int         i;
    hw_reg_set  reg;

    ScoreCount = 0;
    AddRegs();
    reg_name = Names[3];
    while( reg_name != NULL ) {
        if( ScRealRegister( reg_name ) ) {
            reg = HighOffsetReg( reg_name->r.reg );
            if( !(reg==0) ) {
                AllocRegName( reg );
            }
            reg = LowOffsetReg( reg_name->r.reg );
            if( !(reg==0) ) {
                AllocRegName( reg );
            }
        }
        reg_name = reg_name->n.next_name;
    }
    reg_name = Names[3];
    while( reg_name != NULL ) {
        if( ScRealRegister( reg_name ) ) {
            ++ ScoreCount;
        }
        reg_name = reg_name->n.next_name;
    }
    if( ScoreCount != 0 ) {
        ScoreList = CGAlloc( ScoreCount * ( sizeof(void *)+sizeof(score_reg) ) );
        curr = (score_reg *)&ScoreList[  ScoreCount  ];
        reg_name = Names[3];
        i = 0;
        while( reg_name != NULL ) {
            if( ScRealRegister( reg_name ) ) {
                curr->reg_name = (register_name *)reg_name;
                curr->reg = reg_name->r.reg;
                curr->class = reg_name->n.name_class;
                curr->size  = reg_name->n.size;
                curr->high_of = NO_INDEX;
                curr->low_of = NO_INDEX;
                reg_name->r.reg_index = i;
                ScoreList[  i  ] = curr; /*% This avoids MANY multiplies!*/
                ++curr;
                ++i;
            }
            reg_name = reg_name->n.next_name;
        }
        i = ScoreCount;
        while( curr != ScoreList[ 0 ] ) {
            --curr;
            --i;
            reg = HighOffsetReg( curr->reg );
            if( !(reg==0) ) {
                reg_name = AllocRegName( reg );
                if( !ScRealRegister( reg_name )
                 || reg_name->r.reg_index == NO_INDEX ) {
                    curr->high = NO_INDEX;
                } else {
                    curr->high = reg_name->r.reg_index;
                    ScoreList[ curr->high ]->high_of = i;
                }
            } else {
                curr->high = NO_INDEX;
            }
            reg = LowOffsetReg( curr->reg );
            if( !(reg==0) ) {
                reg_name = AllocRegName( reg );
                if( !ScRealRegister( reg_name )
                 || reg_name->r.reg_index == NO_INDEX ) {
                    curr->low = NO_INDEX;
                } else {
                    curr->low = reg_name->r.reg_index;
                    ScoreList[ curr->low ]->low_of = i;
                }
            } else {
                curr->low = NO_INDEX;
            }
        }
    }
}




void *ScAlloc(int size)
{
    return CGAlloc(size);
}

void ScoreClear(score *p)
{
    int i;
    list_head *list_heads;
    list_heads=(list_head *)&p[ScoreCount];
    *list_heads=NULL;
    i=0;
    for (;;) {
        ++list_heads;
        p->list=list_heads;
        *list_heads=NULL;
        p->next_reg=p;
        p->prev_reg=p;
        p->index=i;
        p->generation=0;
        ++p;
        if (++i==ScoreCount) break;
    }
}

void ScoreFreeList(score *sc);
void FreeScoreBoard(score *p)
{
    int i;
    list_head *list_heads;
    score *q;
    if (p!=NULL) {
        i=ScoreCount;
        q=p;
        do {
            --i;
            ScoreFreeList(q);
            ++q;
        } while (i!=0);
        i=ScoreCount;
        list_heads=(list_head *)&p[i];
        *list_heads=NULL;
        do {
            --i;
            ++list_heads;
            p->list=list_heads;
            *list_heads=NULL;
            p->next_reg=p;
            p->prev_reg=p;
            p->generation=0;
            ++p;
        } while (i!=0);
    }
}
void FreeScListEntry(score_list *entry);
void ScoreFreeList(score *sc)
{
    score_list *curr,*next;
    if (sc->list!=NULL) {
        curr=*sc->list;
        while (curr!=NULL) {
            next=curr->next;
            FreeScListEntry(curr);
            curr=next;
        }
        *sc->list=NULL;
    }
}
void FreeScListEntry(score_list *entry)
{
    FrlFreeSize(&ScListFrl,entry,sizeof(score_list));
}

score_list *NewScListEntry(void)
{
    return AllocFrl(&ScListFrl,sizeof(score_list));
}

void ScFree(void *owner,int size)
{
    if (owner!=NULL) CGFreeSize(owner,size);
}

void FreeScListEntry(score_list *entry);
void MemChanged(score *sc,unsigned char statics_too)
{
    int i;
    score_list **owner,*curr;
    unsigned char keep;
    i=ScoreCount;
    for (;;) {
        --i;
        if (sc->list!=NULL) {
            owner=sc->list;
            for (;;) {
                curr=*owner;
                if (curr==NULL) break;
                keep=0;
                switch (curr->info.class) {
                case 0: keep=1;break;
                case 2:
                    if (!(curr->info.symbol.v->usage&8)) keep=1;
                    break;
                case 1:
                    if (!statics_too) keep=1;
                    break;
                case 4:
                    if (curr->info.base!=NULL && curr->info.base->n.class==1 && !statics_too)
                        keep=1;
                    break;
                }
                if (keep) owner=&curr->next;
                else {
                    *owner=curr->next;
                    FreeScListEntry(curr);
                }
            }
        }
        ++sc;
        if (i==0) break;
    }
}
