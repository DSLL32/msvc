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
* Description:  Basic type initialization and type-node allocation.
*
****************************************************************************/


/* cgen2.c's emission-state initializer. In the earlier target the next
 * imported segment starts at -1 directly, before the later SegData field. */
#include "encode.h"
#include "cgmemory.h"
#include "procdef.h"
#include "stubs.h"
#include <recon.h>

void AddInstr(ins_entry *entry,ins_entry *after)
{
    ++InOptimizer;
    InsertQueue(entry,after);
    switch (entry->oc.op.kind&0x0f) {
    case 6:
        ValidLbl(entry->oc.handle.handle);
        AddLblDef(entry);
        break;
    case 7:case 8:case 0x0a:case 0x0c:
        ValidLbl(entry->oc.handle.handle);
        AddLblRef(entry);
        break;
    case 0x0e:
        entry->oc.handle.ref=RetList;
        RetList=entry;
        break;
    }
    --InOptimizer;
}

unsigned char PrevClass(ins_entry *instr)
{
    unsigned char kind;
    ++InOptimizer;
    instr=PrevIns(instr);
    kind=instr!=NULL?(instr->oc.op.kind&15):0;
    --InOptimizer;
    return kind;
}

ins_entry *PrevIns(ins_entry *instr)
{
    ++InOptimizer;
    if (instr==NULL) instr=LastIns;
    else if ((instr->oc.op.kind&15)==0) instr=NULL;
    else instr=instr->ins.prev;
    while (instr!=NULL && (instr->oc.op.kind&15)==1) instr=instr->ins.prev;
    --InOptimizer;
    return instr;
}

ins_entry *NextIns(ins_entry *instr);
unsigned char NextClass(ins_entry *instr)
{
    unsigned char kind;
    ++InOptimizer;
    instr=NextIns(instr);
    kind=instr!=NULL?(instr->oc.op.kind&15):0;
    --InOptimizer;
    return kind;
}

ins_entry *NextIns(ins_entry *instr)
{
    ++InOptimizer;
    if (instr==NULL) instr=FirstIns;
    else if ((instr->oc.op.kind&15)==0) instr=NULL;
    else instr=instr->ins.next;
    while (instr!=NULL && (instr->oc.op.kind&15)==1) instr=instr->ins.next;
    --InOptimizer;
    return instr;
}

ins_entry *ValidIns(ins_entry *ins)
{
    ++InOptimizer;
    while (ins!=NULL && (ins->oc.op.kind&15)==0) ins=ins->ins.next;
    --InOptimizer;
    return ins;
}
/* SafeRecurse's source-level transport is a generic pointer callback; the
 * genuine helper will recover ins_entry immediately inside its definition. */
void DelLblDef(ins_entry *ins);
void DelLblRef(ins_entry *ins);
void DelRef(ins_entry **owner,ins_entry *ins)
{
    ++InOptimizer;
    while (*owner!=ins) owner=(ins_entry **)&(*owner)->oc.handle.ref;
    --InOptimizer;
    *owner=ins->oc.handle.ref;
}
ins_entry *IsolatedCode(ins_entry *ins);
void *DelInstr_Helper(void *value)
{
    ins_entry *old=value,*next;
    UnLinkInstr(old);
    switch (old->oc.op.kind&15) {
    case 6:
        if (old->oc.handle.handle!=NULL) ValidLbl(old->oc.handle.handle);
        DelLblDef(old);
        next=ValidIns(old->ins.next);
        switch (PrevClass(next)) {
        case 12: case 13: IsolatedCode(PrevIns(next)); break;
        case 10: Untangle(PrevIns(next)->oc.handle.handle->ins); break;
        }
        break;
    case 7: case 8: case 10: case 12:
        ValidLbl(old->oc.handle.handle);
        DelLblRef(old);
        break;
    case 14: DelRef(&RetList,old); break;
    }
    old->oc.op.kind=0;
    next=ValidIns(old->ins.next);
    while (next!=NULL && (next->oc.op.kind&15)==1) next=next->ins.next;
    if (next!=NULL && next->ins.prev!=NULL) MultiLineNums(next->ins.prev);
    return next;
}
ins_entry *DelInstr(ins_entry *ins)
{
    if ((ins->oc.op.kind&15)!=0) return SafeRecurse(DelInstr_Helper,ins);
    return ValidIns(ins->ins.next);
}

void DeleteQueue(ins_entry *ins);
void UnLinkInstr(ins_entry *ins)
{
    ins_entry *old_pending;
    InsDelete=1;
    ++InOptimizer;
    DeleteQueue(ins);
    old_pending=PendingDeletes;
    --InOptimizer;
    PendingDeletes=ins;
    ins->ins.prev=old_pending;
}

void FreeInstr(ins_entry *ins);
void FreePendingDeletes(void)
{
    ins_entry *temp;
    ++InOptimizer;
    while (PendingDeletes!=NULL) {
        temp=PendingDeletes->ins.prev;
        FreeInstr(PendingDeletes);
        PendingDeletes=temp;
    }
    --InOptimizer;
}
