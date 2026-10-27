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
#include "stubs.h"
#include <recon.h>

unsigned char InsDelete;

void OptPush(void)
{
    ins_entry *entry;
    ++InOptimizer;
    entry=LastIns;
    for (;;) {
        InsDelete=0;
        switch (entry->oc.op.kind&0x0f) {
        case 1:
            MultiLineNums(entry);
            break;
        case 6:
            Untangle(entry);
            if (!InsDelete) TraceCommon(entry);
            break;
        case 8:
            if (!(Model&0x80000000U)) CallRet(entry);
            break;
        case 0x0c:
            Untangle(PrevIns(entry));
            if (!InsDelete) {
                ComCode(entry);
                JmpRet(entry);
            }
            break;
        case 0x0e:
            if (!(Model&0x80000000U)) {
                RetAftrLbl(entry);
                RetAftrCall(entry);
            }
            if (!InsDelete) ComTail(RetList,entry);
            break;
        }
        if (!InsDelete) break;
        FreePendingDeletes();
        entry=LastIns;
        while (entry!=NULL && (entry->oc.op.kind&0x0f)==1)
            entry=entry->ins.prev;
        if (entry==NULL) break;
    }
    --InOptimizer;
}

unsigned char UniqueLabel(struct code_lbl *lbl);
unsigned char NextClass(ins_entry *instr);
ins_entry *AliasLabels(ins_entry *old,ins_entry *new_label);
ins_entry *NextIns(ins_entry *instr);

unsigned char CompressLabels(ins_entry *label)
{
    ins_entry *other_label;
    unsigned char lbl_unique;
    ++InOptimizer;
    lbl_unique=UniqueLabel(label->oc.handle.handle);
    while (PrevClass(label)==6) {
        other_label=PrevIns(label);
        if (lbl_unique && UniqueLabel(other_label->oc.handle.handle)) break;
        AliasLabels(other_label,label);
        if ((label->oc.op.kind&15)==0) { --InOptimizer; return 0; }
    }
    while (NextClass(label)==6) {
        other_label=NextIns(label);
        if (lbl_unique && UniqueLabel(other_label->oc.handle.handle)) break;
        AliasLabels(other_label,label);
        if ((label->oc.op.kind&15)==0) { --InOptimizer; return 0; }
    }
    --InOptimizer;
    return 1;
}
ins_entry *DelInstr(ins_entry *instr);
unsigned char ReverseCondition(unsigned char cond) { STUB_TRAP(); return 0; }
void ChgLblRef(ins_entry *instr,struct code_lbl *label) { STUB_TRAP(); }
unsigned char UnTangle1(ins_entry *jmp,ins_entry **instr)
{
    ins_entry *c_jmp;
    unsigned char cl,old_kind;
    if (jmp==NULL) return 0;
    cl=jmp->oc.op.kind&15;
    if (cl!=0x0a && cl!=0x0c) return 0;
    if (jmp->oc.handle.handle==(*instr)->oc.handle.handle) {
        *instr=DelInstr(jmp);
        return 1;
    }
    if (cl!=0x0c) return 0;
    c_jmp=PrevIns(jmp);
    if (c_jmp==NULL || (c_jmp->oc.op.kind&15)!=0x0a) return 0;
    if (c_jmp->oc.handle.handle==(*instr)->oc.handle.handle && !(jmp->oc.op.kind&0x10)) {
        c_jmp->oc.jcond.cond=ReverseCondition(c_jmp->oc.jcond.cond);
        old_kind=jmp->oc.op.kind;
        jmp->oc.op.kind=0x21; /* OC_DEAD_JMP, suppress dead-code removal */
        ChgLblRef(c_jmp,jmp->oc.handle.handle);
        jmp->oc.op.kind=old_kind;
        *instr=DelInstr(jmp);
        return 1;
    }
    if (c_jmp->oc.handle.handle==jmp->oc.handle.handle) {
        DelInstr(c_jmp);
        return 1;
    }
    return 0;
}
ins_entry *Redirect(ins_entry *l_ins,ins_entry *j_ins) { STUB_TRAP(); return NULL; }
unsigned char UnTangle2(ins_entry *jmp,ins_entry **instr)
{
    unsigned char cl;
    ins_entry *ins;
    if (Model&0x80000000U) return 0;
    if (jmp==NULL) return 0;
    cl=jmp->oc.op.kind&15;
    if (cl==0x0e) return RetAftrLbl(jmp);
    if (cl!=0x0c || (jmp->oc.op.kind&0x10)) return 0;
    if ((*instr)->oc.handle.handle->ins==NULL) return 0;
    if (jmp->oc.handle.handle->redirect==(*instr)->oc.handle.handle) return 0;
    if (jmp->oc.handle.handle==(*instr)->oc.handle.handle) return 0;
    *instr=Redirect(*instr,jmp);
    if (*instr==NULL) return 0;
    cl=PrevClass(*instr);
    if (cl<0x0c) return 1;
    ins=*instr;
    for (;;) {
        if (ins==NULL || (ins->oc.op.kind&15)==6) break;
        if ((ins->oc.op.kind&15)==1) ins=ins->ins.next;
        else ins=DelInstr(ins);
    }
    *instr=ins;
    return 1;
}


ins_entry *Untangle(ins_entry *instr)
{
    ins_entry *jmp;
    unsigned char change;
    ++InOptimizer;
    for (;;) {
        if (instr==NULL || (instr->oc.op.kind&15)!=6) break;
        if (!CompressLabels(instr)) break;
        jmp=PrevIns(instr);
        change=UnTangle1(jmp,&instr);
        if (instr==NULL || (instr->oc.op.kind&15)!=6) break;
        if (!CompressLabels(instr)) break;
        jmp=NextIns(instr);
        change|=UnTangle2(jmp,&instr);
        if (!change) break;
    }
    --InOptimizer;
    return instr;
}

void CloneCode(struct code_lbl *label);
ins_entry *IsolatedCode(ins_entry *ins);
unsigned char StraightenCode(ins_entry *jump);
void CheckStraightenCode(ins_entry *label);
void SetBranches(void);
void OptPull(void)
{
    unsigned char kind;
    ++InOptimizer;
    for (;;) {
        InsDelete=0;
        kind=FirstIns->oc.op.kind&15;
        switch (kind) {
        case 6:
            Untangle(FirstIns);
            if (!(Model&0x80000000U) && !InsDelete)
                CloneCode(FirstIns->oc.handle.handle);
            break;
        case 8:
            if (!(Model&0x80000000U)) CallRet(FirstIns);
            break;
        case 12:
            if (!(Model&0x80000000U)) {
                IsolatedCode(FirstIns);
                if (!InsDelete) {
                    StraightenCode(FirstIns);
                    JmpRet(FirstIns);
                    if (!InsDelete) {
                        CheckStraightenCode(NextIns(FirstIns));
                        if (!InsDelete) CloneCode(FirstIns->oc.handle.handle);
                    }
                }
            }
            break;
        case 13:
        case 14:
            if (!(Model&0x80000000U)) {
                IsolatedCode(FirstIns);
                if (!InsDelete) CheckStraightenCode(NextIns(FirstIns));
            }
            break;
        case 10:
            Untangle(NextIns(FirstIns));
            break;
        }
        FreePendingDeletes();
        if (FirstIns==NULL) goto done;
        if (!InsDelete) break;
    }
    switch (kind) {
    case 10:case 12:
        if (!(FirstIns->oc.op.kind&0x10)) SetBranches();
        /* fall through */
    case 7:case 8:
        FirstIns->oc.handle.handle->label.status|=2;
        break;
    }
done:
    --InOptimizer;
}
