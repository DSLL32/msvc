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


#include "encode.h"
#include "cgmemory.h"
#include "stubs.h"
#include <recon.h>

ins_entry *NextIns(ins_entry *ins);
void JmpToRet(ins_entry *ins,ins_entry *ret) { STUB_TRAP(); }
void JmpRet(ins_entry *ins)
{
    ins_entry *ret;
    ++InOptimizer;
    if (InsDelete) goto done;
    ret=ins->oc.handle.handle->ins;
    if (ret==NULL) goto done;
    ret=NextIns(ret);
    if (ret==NULL || (ret->oc.op.kind&15)!=14) goto done;
    JmpToRet(ins,ret);
done:
    --InOptimizer;
}

unsigned char NextClass(ins_entry *entry);
ins_entry *DelInstr(ins_entry *entry);
void CallRet(ins_entry *entry)
{
    ins_entry *label;
    ++InOptimizer;
    if (entry->oc.op.kind&0x40) goto done;
    label=entry->oc.handle.handle->ins;
    if (label==NULL) goto done;
    if (NextClass(label)!=14) goto done;
    DelInstr(entry);
done:
    --InOptimizer;
}

unsigned char CodeHasAbsPatch(oc_header *code);
unsigned char FindShort(ins_entry *ins,ins_entry *end)
{
    while (ins!=NULL && ins!=end) {
        if ((ins->oc.op.kind&15)==6) {
            if (ins->oc.op.kind&0x20) return 1; /* ATTR_SHORT */
            ins->oc.handle.handle->label.status&=~0x10u; /* SHORTREACH */
        }
        ins=NextIns(ins);
    }
    return 0;
}
void DoCloneCode(ins_entry *jump,ins_entry *hoist)
{
    ins_entry *point,*clone;
    unsigned char kind;
    point=PrevIns(jump);
    for (;;) {
        kind=hoist->oc.op.kind&15;
        switch (kind) {
        case 1:                         /* OC_INFO */
        case 6:                         /* OC_LABEL */
        case 7:                         /* OC_LREF */
            break;
        default:
            clone=NewInstr(&hoist->oc);
            AddInstr(clone,point);
            if (kind>=12) return;
            point=clone;
            break;
        }
        hoist=NextIns(hoist);
    }
}
void CloneCode(struct code_lbl *label)
{
    ins_entry *next,*hoist,*jump;
    unsigned size,max_size;
    unsigned char kind;
    if (label->ins==NULL) return;
    hoist=NextIns(label->ins);
    next=hoist;
    size=0;
    for (;;) {
        if (next==NULL) return;
        kind=next->oc.op.kind&15;
        if (kind==2 && CodeHasAbsPatch(&next->oc.op)) return;
        if (kind!=6) {
            size+=next->oc.op.objlen;
            if (size>40) return;
            if (kind>=12) break;
        }
        next=NextIns(next);
    }
    if ((next->oc.op.kind&15)==12 && next->oc.handle.handle==label) return;
    for (jump=label->refs;jump!=NULL;jump=jump->oc.handle.ref) {
        if (next==jump || (jump->oc.op.kind&15)<12) continue;
        max_size=jump->oc.op.objlen;
        if (size>max_size && FindShort(jump,NULL)) continue;
        if (OptForSize<50) max_size*=(100-OptForSize)/25;
        if (size>max_size) continue;
        DoCloneCode(jump,hoist);
        DelInstr(jump);
    }
}
ins_entry *IsolatedCode(ins_entry *entry);
void DeleteQueue(ins_entry *entry);
struct code_lbl *AddNewLabel(ins_entry *entry,int align);
void ChgLblRef(ins_entry *entry,struct code_lbl *label);
unsigned char StraightenCode(ins_entry *jump)
{
    ins_entry *next,*insert,*hoist,*end_hoist;
    unsigned char kind,result;
    int align;
    ++InOptimizer;
    result=0;
    hoist=jump->oc.handle.handle->ins;
    if (hoist==NULL || hoist==LastIns) goto done;
    if (PrevClass(hoist)<12) goto done;
    end_hoist=NULL;
    next=hoist;
    for (;;) {
        if (next==jump) {
            if (end_hoist==NULL || FindShort(hoist,end_hoist)) goto done;
            break;
        }
        if (next==NULL) {
            if (FindShort(jump,hoist)) goto done;
            break;
        }
        kind=next->oc.op.kind&15;
        if (end_hoist==NULL && kind>=12) end_hoist=next;
        next=NextIns(next);
    }
    align=hoist->oc.op.objlen;
    insert=jump;
    for (;;) {
        if (hoist==NULL) {
            ChgLblRef(jump,AddNewLabel(LastIns,align));
            next=LastIns;
            break;
        }
        next=NextIns(hoist);
        DeleteQueue(hoist);
        InsertQueue(hoist,insert);
        if (hoist==jump) goto done;
        insert=hoist;
        kind=hoist->oc.op.kind&15;
        if (kind>=12) {
            IsolatedCode(insert);
            break;
        }
        hoist=next;
    }
    InsDelete=1;
    Untangle(next);
    if ((jump->oc.op.kind&15)!=0) Untangle(jump->oc.handle.handle->ins);
    result=1;
done:
    --InOptimizer;
    return result;
}
void CheckStraightenCode(ins_entry *label)
{
    ins_entry *jump;
    ++InOptimizer;
    if (label!=NULL) {
        for (jump=label->oc.handle.handle->refs;jump!=NULL;
             jump=jump->oc.handle.ref) {
            if ((jump->oc.op.kind&15)<12) continue;
            if (StraightenCode(jump)) break;
        }
    }
    --InOptimizer;
}


ins_entry *IsolatedCode(ins_entry *entry)
{
    ins_entry *next;
    ++InOptimizer;
    next=NextIns(entry);
    for (;;) {
        if (next==NULL) goto done;
        if ((next->oc.op.kind&15)==6) break;
        if ((next->oc.op.kind&15)==1) next=next->ins.next;
        else next=DelInstr(next);
        if ((entry->oc.op.kind&15)==0) break;
    }
    next=Untangle(next);
done:
    --InOptimizer;
    return next;
}
