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

static struct code_lbl *Handle;
ins_entry *NextIns(ins_entry *entry);
int OptInsSize(unsigned char kind,int destination);
unsigned long AskLocation(void);
unsigned char Jmp_to_lbl(ins_entry *entry)
{
    if ((entry->oc.op.kind&15)!=12) return 0;
    return entry->oc.handle.handle==Handle;
}
unsigned char CanReach(struct code_lbl *label,ins_entry **add_ptr,ins_entry **jump_ptr)
{
    ins_entry *add,*jump,*entry,*label_ins;
    unsigned long length;
    add=NULL;
    jump=NULL;
    if (label->label.status&0x400) {
        /* UNREACHABLE */
    } else if (label->label.address==0xffffffffUL) {
        if (label->label.status&0x10) return 1;
        length=OptInsSize(12,0);
        /* This is genuinely the current branch's Handle, not label. */
        label_ins=Handle->ins;
        entry=FirstIns;
        for (;;) {
            entry=NextIns(entry);
            if (entry==NULL) break;
            length+=entry->oc.op.objlen;
            if (length>127) break;
            if (Jmp_to_lbl(entry)) jump=entry;
            else if ((entry->oc.op.kind&15)>=12) add=entry;
            if (entry==label_ins) return 1;
        }
    } else if (AskLocation()-label->label.address<=126) return 1;
    if (add_ptr!=NULL) *add_ptr=add;
    if (jump_ptr!=NULL) *jump_ptr=jump;
    return 0;
}
void HndlRedirect(struct code_lbl *label)
{
    struct code_lbl *redirect;
    ++InOptimizer;
    redirect=Handle->redirect;
    if (redirect!=NULL && redirect!=label) {
        redirect->label.status&=~0x80;
        TryScrapLabel(redirect);
    }
    Handle->redirect=label;
    if (label!=NULL) label->label.status|=0x80;
    --InOptimizer;
}
void SetShort(void)
{
    ins_entry *label_ins;
    int size;
    unsigned char kind;
    ++InOptimizer;
    kind=FirstIns->oc.op.kind&15;
    size=OptInsSize(kind,0);
    FirstIns->oc.op.objlen=(unsigned char)size;
    /* Before the later ATTR_FLOAT preservation: reset the complete class. */
    FirstIns->oc.op.kind=kind==12?12:10;
    label_ins=FirstIns->oc.handle.handle->ins;
    if (label_ins!=NULL) label_ins->oc.op.kind|=0x20;
    --InOptimizer;
}
void BigBranch(ins_entry *add,ins_entry *jump) { STUB_TRAP(); }
unsigned char InRange(void) { STUB_TRAP();return 0; }
ins_entry *NextIns(ins_entry *entry);
unsigned char ReverseCondition(unsigned char condition);
void ChgLblRef(ins_entry *entry,struct code_lbl *label);
void SetBranches(void)
{
    ins_entry *add,*jump,*next;
    unsigned char was_keep;
    ++InOptimizer;
    Handle=FirstIns->oc.handle.handle;
    if (CanReach(Handle,&add,&jump)) {
        Handle->label.status|=0x10;
        HndlRedirect(NULL);
        SetShort();
    } else if (FirstIns->oc.op.kind&0x20) {
        next=NextIns(FirstIns);
        if ((next->oc.op.kind&15)==12 && CanReach(next->oc.handle.handle,NULL,NULL)) {
            FirstIns->oc.jcond.cond=ReverseCondition(FirstIns->oc.jcond.cond);
            was_keep=(Handle->label.status&2)!=0;
            Handle->label.status|=2;
            ChgLblRef(FirstIns,next->oc.handle.handle);
            ChgLblRef(next,Handle);
            if (!was_keep) Handle->label.status&=~2;
        } else BigBranch(add,jump);
        SetShort();
    } else if (OptForSize>50 && InRange()) {
        ChgLblRef(FirstIns,Handle->redirect);
        SetShort();
    }
    --InOptimizer;
}
