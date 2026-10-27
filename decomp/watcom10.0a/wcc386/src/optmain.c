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
#include "codegen.h"
#include "cgmemory.h"
#include "encode.h"
#include "stubs.h"
#include <recon.h>

struct ins_entry;
struct ins_entry *FirstIns,*LastIns,*RetList,*PendingDeletes;
unsigned QCount;
struct code_lbl *Handles;
void *InstrFrl;

void InitQueue(void)
{
    ++InOptimizer;
    QCount=0;
    FirstIns=NULL;
    LastIns=NULL;
    RetList=NULL;
    PendingDeletes=NULL;
    Handles=NULL;
    InitFrl(&InstrFrl);
    --InOptimizer;
}

/* optmain.c's queue ingress; this build has an unlimited Q_MAX sentinel. */
void InputOC(any_oc *record)
{
    unsigned char kind;
    ++InOptimizer;
    DGBlip();
    if (!LDone(record)) {
        kind=record->op.kind&0x0f;
        if (kind!=1 && kind!=6 && PrevClass(NULL)>=0x0c) {
            --InOptimizer;
            return;
        }
        while (QCount>=0xffffffffU) PullQueue();
        AddInstr(NewInstr(record),LastIns);
        kind=LastIns->oc.op.kind&0x0f;
        switch (kind) {
        case 6:
            if (LastIns->oc.handle.handle->label.status&0x100)
                ++LastIns->oc.op.objlen;
            /* fall through */
        case 7:case 8:case 0x0a:case 0x0c:
            LastIns->oc.handle.handle->label.status&=~4;
            break;
        }
        OptPush();
    }
    --InOptimizer;
}

unsigned char LDone(any_oc *record)
{
    struct code_lbl *label;
    ++InOptimizer;
    if (record->op.kind!=0x11) {
        --InOptimizer;
        return 0;
    }
    label=record->handle.handle;
    ValidLbl(label);
    if (!(label->label.status&1)) {
        --InOptimizer;
        return 0;
    }
    label->label.status|=4;
    TryScrapLabel(label);
    --InOptimizer;
    return 1;
}

unsigned short SetOP(unsigned short segment);
void FiniQueue(void)
{
    ++InOptimizer;
    SetOP(AskCodeSeg());
    EmptyQueue();
    --InOptimizer;
}

void EmptyQueue(void)
{
    ++InOptimizer;
    while (QCount!=0) PullQueue();
    --InOptimizer;
}

void OptPull(void);
unsigned char UniqueLabel(struct code_lbl *label);
ins_entry *NextIns(ins_entry *entry);
void OutputOC(any_oc *record,any_oc *next_label);
ins_entry *DelInstr(ins_entry *entry);
void PullQueue(void)
{
    struct code_lbl *label,*next,*back_labels,**owner;
    ins_entry *next_ins;
    unsigned char kind;
    ++InOptimizer;
    for (;;) {
        /* The old physical blip entry also represents the PLBlip role. */
        DGBlip();
        OptPull();
        if (FirstIns==NULL) break;
        if ((FirstIns->oc.op.kind&15)==6) {
            if (UniqueLabel(FirstIns->oc.handle.handle)) --FirstIns->oc.op.objlen;
            back_labels=NULL;
            owner=&FirstIns->oc.handle.handle;
            for (label=FirstIns->oc.handle.handle;label!=NULL;label=next) {
                next=label->alias;
                if (label->label.symbol!=NULL) {
                    *owner=label;
                    owner=&label->alias;
                } else {
                    label->alias=back_labels;
                    back_labels=label;
                }
            }
            *owner=back_labels;
        }
        for (;;) {
            next_ins=NextIns(FirstIns);
            if (next_ins!=NULL && (next_ins->oc.op.kind&15)==6)
                OutputOC(&FirstIns->oc,&next_ins->oc);
            else OutputOC(&FirstIns->oc,NULL);
            kind=FirstIns->oc.op.kind&15;
            if (kind!=6) break;
            label=FirstIns->oc.handle.handle;
            label->ins=NULL;
            next=label->alias;
            if (next==NULL) break;
            label->alias=NULL;
            TryScrapLabel(label);
            FirstIns->oc.handle.handle=next;
        }
        DelInstr(FirstIns);
        FreePendingDeletes();
        if (kind>=12 || FirstIns==NULL) break;
    }
    --InOptimizer;
}
