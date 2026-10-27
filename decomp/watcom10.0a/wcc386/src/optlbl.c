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

void AddLblDef(ins_entry *instr)
{
    struct code_lbl *lbl;
    instr->oc.handle.ref=NULL;
    ++InOptimizer;
    lbl=instr->oc.handle.handle;
    do {
        lbl->ins=instr;
        lbl->label.status|=1; /* CODELABEL */
        lbl=lbl->alias;
    } while (lbl!=NULL);
    --InOptimizer;
}

unsigned char UniqueLabel(struct code_lbl *lbl)
{
    unsigned char result;
    ++InOptimizer;
    result=0;
    while (lbl!=NULL) {
        if (lbl->label.status&0x100) { /* UNIQUE */
            result=1;
            break;
        }
        lbl=lbl->alias;
    }
    --InOptimizer;
    return result;
}

void AddLblRef(ins_entry *instr)
{
    struct code_lbl *lbl;
    lbl=instr->oc.handle.handle;
    ++InOptimizer;
    if (lbl->ins!=NULL) {
        lbl=((ins_entry *)lbl->ins)->oc.handle.handle;
        instr->oc.handle.handle=lbl;
    }
    instr->oc.handle.ref=lbl->refs;
    --InOptimizer;
    lbl->refs=instr;
}

ins_entry *DelInstr(ins_entry *ins);
void ScrapCodeLabel(struct code_lbl *label);
void TryScrapLabel(struct code_lbl *label);
void KillDeadLabels(ins_entry *ins)
{
    struct code_lbl *curr,**owner;
    ++InOptimizer;
    owner=&ins->oc.handle.handle;
    for (;;) {
        curr=*owner;
        if (curr==NULL) break;
        ValidLbl(curr);
        if (curr->refs==NULL && !(curr->label.status&0x82) &&
            (curr->label.status&4)) {
            *owner=curr->alias;
            if (curr->redirect!=NULL) {
                curr->redirect->label.status&=~0x80;
                TryScrapLabel(curr->redirect);
                curr->redirect=NULL;
            }
            ScrapCodeLabel(curr);
        } else owner=&curr->alias;
    }
    if (ins->oc.handle.handle==NULL) DelInstr(ins);
    --InOptimizer;
}
void ScrapCodeLabel(struct code_lbl *label)
{
    struct code_lbl **owner,*redir;
    unsigned char code;
    ++InOptimizer;
    owner=&Handles;
    for (;;) {
        if (*owner==NULL) goto done;
        if (*owner==label) break;
        owner=&(*owner)->label.link;
    }
    *owner=label->label.link;
    redir=label->redirect;
    code=(label->label.status&1)!=0;
    CGFreeSize(label,sizeof(*label));
    if (code && redir!=NULL) {
        redir->label.status&=~0x80;
        TryScrapLabel(redir);
    }
done:
    --InOptimizer;
}
void TryScrapLabel(struct code_lbl *label)
{
    ++InOptimizer;
    if (label->refs==NULL) {
        if (label->ins!=NULL) KillDeadLabels(label->ins);
        else if ((label->label.status&4) && !(label->label.status&0x82))
            ScrapCodeLabel(label);
    }
    --InOptimizer;
}

void DelLblDef(ins_entry *ins)
{
    struct code_lbl *label;
    ++InOptimizer;
    label=ins->oc.handle.handle;
    if (label!=NULL && label->refs==NULL && (label->label.status&4) &&
        !(label->label.status&0x80)) ScrapCodeLabel(label);
    --InOptimizer;
}

ins_entry *AliasLabels(ins_entry *oldlbl,ins_entry *newlbl)
{
    struct code_lbl *old,*new_label;
    ins_entry *old_jump,**owner;
    ++InOptimizer;
    if (oldlbl!=newlbl) {
        if (oldlbl->oc.op.objlen>newlbl->oc.op.objlen)
            newlbl->oc.op.objlen=oldlbl->oc.op.objlen;
        newlbl->oc.op.kind|=oldlbl->oc.op.kind&0x20;
        new_label=newlbl->oc.handle.handle;
        old=oldlbl->oc.handle.handle;
        if (new_label->redirect==old) {
            new_label->redirect=NULL;
            old->label.status&=~0x80;
        } else if (old->redirect==new_label) {
            old->redirect=NULL;
            new_label->label.status&=~0x80;
        }
        do {
            old->ins=new_label->ins;
            old=old->alias;
        } while (old!=NULL);
        old=oldlbl->oc.handle.handle;
        owner=&old->refs;
        for (;;) {
            old_jump=*owner;
            if (old_jump==NULL) break;
            old_jump->oc.handle.handle=new_label;
            owner=(ins_entry **)&old_jump->oc.handle.ref;
        }
        *owner=new_label->refs;
        new_label->refs=old->refs;
        old->refs=NULL;
        while (new_label->alias!=NULL) new_label=new_label->alias;
        new_label->alias=old;
        oldlbl->oc.handle.handle=NULL;
        oldlbl=DelInstr(oldlbl);
    }
    --InOptimizer;
    return oldlbl;
}

void DelRef(ins_entry **owner,ins_entry *entry);
void DelLblRef(ins_entry *entry)
{
    struct code_lbl *label;
    ++InOptimizer;
    label=entry->oc.handle.handle;
    DelRef((ins_entry **)&label->refs,entry);
    TryScrapLabel(label);
    --InOptimizer;
}
