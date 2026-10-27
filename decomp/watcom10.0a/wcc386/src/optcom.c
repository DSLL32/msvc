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

void TraceCommon(ins_entry *lbl_ins)
{
    ins_entry *ref,*ins;
    ++InOptimizer;
    for (;;) {
        ref=lbl_ins->oc.handle.handle->refs;
        for (;;) {
            for (;;) {
                if (ref==NULL) goto done;
                ins=ref;
                ref=ref->oc.handle.ref;
                if ((ins->oc.op.kind&15)==0x0c) break;
            }
            if (ComCode(ins)) break;
        }
        if (ref==NULL) break;
    }
done:
    --InOptimizer;
}

ins_entry *NextIns(ins_entry *instr);
ins_entry *DelInstr(ins_entry *instr);
void ChgLblRef(ins_entry *instr,struct code_lbl *label);
unsigned char CommonInstr(ins_entry *first,ins_entry *second)
{
    unsigned char result;
    ++InOptimizer;
    result=0;
    if (Model&0x80000000U) goto done;
    if (first->oc.op.kind!=second->oc.op.kind ||
        first->oc.op.reclen!=second->oc.op.reclen) goto done;
    switch (first->oc.op.kind&15) {
    case 0: goto done;
    case 6: case 7: case 8: case 12:
        if (first->oc.handle.handle!=second->oc.handle.handle) goto done;
        break;
    case 10:
        if (first->oc.jcond.cond!=second->oc.jcond.cond) goto done;
        if (first->oc.jcond.handle!=second->oc.jcond.handle) goto done;
        break;
    case 14:
        if (first->oc.ret.pops!=second->oc.ret.pops) goto done;
        break;
    default:
        if (!Equal((unsigned char *)&second->oc,(unsigned char *)&first->oc,
                   second->oc.op.reclen)) goto done;
        break;
    }
    result=1;
done:
    --InOptimizer;
    return result;
}
unsigned char FindShort(ins_entry *ins,ins_entry *label);
struct code_lbl *AddNewLabel(ins_entry *ins,int align) { STUB_TRAP();return NULL; }
unsigned char ComCode(ins_entry *jmp)
{
    ins_entry *next,*com;
    int align;
    unsigned char common;
    ++InOptimizer;
    common=0;
    next=jmp->oc.handle.handle->ins;
    if (next!=NULL) {
        align=next->oc.op.objlen;
        for (;;) {
            for (;;) {
                next=PrevIns(next);
                if (next==NULL || next==jmp || (next->oc.op.kind&15)!=6) break;
            }
            if (next==NULL || next==jmp) break;
            com=PrevIns(jmp);
            if (com==NULL || !CommonInstr(next,com)) break;
            com=PrevIns(DelInstr(com));
            common=1;
            if ((jmp->oc.op.kind&15)==0) goto done;
        }
        if (common) {
            if (FindShort(next,NULL)) align=0;
            ChgLblRef(jmp,AddNewLabel(next,align));
            Untangle(NextIns(next));
            Untangle(NextIns(jmp));
            Untangle(com);
        }
    }
    if ((jmp->oc.op.kind&15)!=0)
        common|=ComTail(jmp->oc.handle.handle->refs,jmp);
done:
    --InOptimizer;
    return common;
}

#pragma pack(4)
typedef struct common_info {
    ins_entry *start_com,*start_del;
    unsigned save;
} common_info;
#pragma pack()
typedef char old_common_info_size[(sizeof(common_info)==12)?1:-1];
void FindCommon(common_info *info,ins_entry *first,ins_entry *second)
{
    ++InOptimizer;
    info->save=0;
    for (;;) {
        if (!CommonInstr(first,second)) break;
        info->start_com=first;
        info->start_del=second;
        info->save+=first->oc.op.objlen;
        for (;;) {
            first=PrevIns(first);
            if (first==NULL) goto done;
            if ((first->oc.op.kind&15)!=6) break;
        }
        second=PrevIns(second);
        if (second==NULL) break;
    }
done:
    --InOptimizer;
}
unsigned char JustMoveLabel(common_info *info,ins_entry *ins)
{
    unsigned char kind,result;
    ins_entry *label,*add,*next;
    ++InOptimizer;
    result=0;
    if (PrevClass(info->start_del)!=6) goto done;
    label=PrevIns(info->start_del);
    if (label->oc.op.kind&0x20) goto done;
    kind=PrevClass(label);
    if (kind<12) goto done;
    DeleteQueue(label);
    InsertQueue(label,PrevIns(info->start_com));
    add=PrevIns(info->start_del);
    for (;;) {
        next=NextIns(add);
        DelInstr(next);
        if (next==ins) break;
    }
    Untangle(label);
    result=1;
done:
    --InOptimizer;
    return result;
}
void TransformJumps(ins_entry *ins,ins_entry *first) { STUB_TRAP(); }
void AddNewJump(ins_entry *ins,struct code_lbl *label) { STUB_TRAP(); }
ins_entry *IsolatedCode(ins_entry *ins);
int OptInsSize(unsigned char kind,int destination);
unsigned char ComTail(ins_entry *list,ins_entry *ins)
{
    ins_entry *trial,*add,*next,*lbl,*first;
    common_info common,max;
    int align;
    unsigned char result;
    ++InOptimizer;
    align=(list->oc.op.kind&15)==6?list->oc.op.objlen:0;
    trial=list;
    max.save=0;
    do {
        if (trial!=ins) {
            FindCommon(&common,trial,ins);
            if (common.save>max.save) {
                max=common;
                first=trial;
            }
        }
        trial=trial->oc.handle.ref;
    } while (trial!=NULL);
    result=0;
    if (max.save==0) goto done;
    if (JustMoveLabel(&max,ins)) { result=1;goto done; }
    if (OptForSize<25 || max.save<=(unsigned)OptInsSize(12,1)) goto done;
    TransformJumps(ins,first);
    add=PrevIns(max.start_del);
    trial=PrevIns(max.start_com);
    if (FindShort(trial,NULL)) align=0;
    AddNewJump(add,AddNewLabel(PrevIns(max.start_com),align));
    lbl=PrevIns(max.start_com);
    add=NextIns(add);
    do {
        next=NextIns(add);
        DelInstr(next);
    } while (next!=ins);
    Untangle(lbl);
    IsolatedCode(add);
    result=1;
done:
    --InOptimizer;
    return result;
}
