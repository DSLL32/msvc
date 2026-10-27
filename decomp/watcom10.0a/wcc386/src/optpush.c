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
* Description:  Object-queue return and line-number optimizations.
*
****************************************************************************/


/* cgen2.c's emission-state initializer. In the earlier target the next
 * imported segment starts at -1 directly, before the later SegData field. */
#include "encode.h"
#include "cgmemory.h"
#include <recon.h>

void MultiLineNums(ins_entry *entry)
{
    ins_entry *previous;
    ++InOptimizer;
    if (entry->oc.op.kind==1) {
        previous=entry->ins.prev;
        while (previous!=NULL && (previous->oc.op.kind&0x0f)==1) {
            if (previous->oc.op.kind==1) {
                entry=previous->ins.next;
                UnLinkInstr(previous);
                previous->oc.op.kind=0;
                previous=entry->ins.prev;
            } else {
                previous=previous->ins.prev;
            }
        }
    }
    --InOptimizer;
}

void JmpToRet(ins_entry *ins,ins_entry *ret);
ins_entry *DelInstr(ins_entry *ins);
unsigned char RetAftrLbl(ins_entry *ret)
{
    ins_entry *ref,*ins;
    struct code_lbl *label;
    unsigned char change;
    ++InOptimizer;
    change=0;
    if (PrevClass(ret)==6) {
        label=PrevIns(ret)->oc.handle.handle;
        ref=label->refs;
        while (ref!=NULL) {
            ins=ref;
            ref=ref->oc.handle.ref;
            if ((ins->oc.op.kind&15)==12) {
                JmpToRet(ins,ret);
                change=1;
            } else if ((ins->oc.op.kind&15)==8 && !(ret->oc.op.kind&0x40)) {
                DelInstr(ins);
                change=1;
            }
        }
    }
    --InOptimizer;
    return change;
}

int OptInsSize(unsigned char kind,int destination);
ins_entry *IsolatedCode(ins_entry *ins);
void RetAftrCall(ins_entry *ret_instr)
{
    ins_entry *call_instr;
    unsigned char call_attr,ret_attr;
    ++InOptimizer;
    if (TargetModel&0x8000) goto done;
    call_instr=ret_instr;
    while (PrevClass(call_instr)==6) call_instr=PrevIns(call_instr);
    if (PrevClass(call_instr)!=8) goto done;
    call_instr=PrevIns(call_instr);
    call_attr=call_instr->oc.op.kind&0xf0;
    if (call_attr&0x40) goto done;
    ret_attr=ret_instr->oc.op.kind&0xf0;
    if (ret_attr&0x40) goto done;
    if ((call_attr^ret_attr)&0x10) goto done;
    InsDelete=1;
    call_instr->oc.op.kind=call_attr|12;
    if (call_instr->oc.op.objlen==OptInsSize(8,2)) {
        call_instr->oc.op.objlen=(unsigned char)OptInsSize(12,1);
        call_instr->oc.op.kind&=~0x10;
    } else if (call_instr->oc.op.objlen==OptInsSize(8,3))
        call_instr->oc.op.objlen=(unsigned char)OptInsSize(12,3);
    else call_instr->oc.op.objlen=(unsigned char)OptInsSize(12,1);
    IsolatedCode(call_instr);
    Untangle(PrevIns(call_instr));
done:
    --InOptimizer;
}
