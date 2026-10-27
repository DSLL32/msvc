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
#include "genopt.h"
#include "regsave.h"
#include "stubs.h"
#include <recon.h>

long PushSize(long size);
void PropLocal(name *temp);
unsigned char UnChangeable(instruction *ins);
void PushLocals(void)
{
    instruction *move;
    name *src,*dst;
    long curr_offset;
    if (BlockByBlock || Save.store_cost[U4]<=Save.push_cost[U4]) return;
    /* AssignPushLocals is inlined in this older target. */
    move=HeadBlock->ins.hd.next;
    curr_offset=0;
    for (;;) {
        if (CurrProc->prolog_state&1) break;
        if (DoesSomething(move)) {
            if (move->head.opcode!=0x26 || UnChangeable(move)) break;
            src=move->operands[0];
            dst=move->result;
            if (src->n.class!=3 || src->n.name_class==FS || src->n.name_class==FD) break;
            if (dst->n.class!=2 || (dst->n.size!=2 && dst->n.size!=4)) break;
            curr_offset-=PushSize(dst->n.size);
            if (DeAlias(dst)!=dst) break;
            if (dst->v.usage&0x40) {
                if (dst->t.location!=curr_offset) break;
            } else {
                CurrProc->targ.push_local_size+=PushSize(dst->n.size);
                dst->t.location=-CurrProc->targ.push_local_size;
                dst->v.usage|=0x40;
                PropLocal(dst);
            }
            move->head.state=1;
            DoNothing(move);
        }
        move=move->head.next;
    }
}

unsigned char TempAllocBefore(void *left,void *right)
{
    name *t1,*t2;
    t1=left;
    t2=right;
    /* The later stack-alignment reversal is absent in this comparator. */
    if ((t1->t.temp_flags&0x200) && !(t2->t.temp_flags&0x200)) return 1;
    if ((t2->t.temp_flags&0x200) && !(t1->t.temp_flags&0x200)) return 0;
    if (t1->n.size<t2->n.size) return 1;
    if (t1->n.size!=t2->n.size) return 0;
    return t1->t.id.id==t2->t.id.id && t1->v.offset>t2->v.offset;
}

void SetTempLocation(name *temp,long size)
{
    CurrProc->locals.size=(long)((unsigned long)CurrProc->locals.size+size);
    temp->t.location=(long)(0UL-(unsigned long)CurrProc->locals.size-CurrProc->locals.base);
}

void AdjustPushLocal(name *temp)
{
    name *scan;
    if (!(temp->t.temp_flags&0x40)) { /* PUSH_LOCAL: already adjusted */
        temp->t.location=(long)((unsigned long)temp->t.location-
                               CurrProc->locals.size-CurrProc->locals.base);
        scan=(name *)temp->t.alias;
        while (scan!=temp) {
            scan->t.location=temp->t.location;
            scan->v.usage|=0x40;
            scan->t.temp_flags|=0x40;
            scan=(name *)scan->t.alias;
        }
    }
}

void RelocParms(void)
{
    name *temp;
    for (temp=Names[2];temp!=NULL;temp=temp->n.next_name) {
        if ((temp->v.usage&0x40) && (temp->t.temp_flags&0x20))
            temp->t.location+=CurrProc->parms.base;
    }
}
