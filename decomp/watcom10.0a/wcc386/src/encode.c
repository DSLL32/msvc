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
* Description:  Object-queue records and conditional branch generation.
*
****************************************************************************/


/* cgen2.c's emission-state initializer. In the earlier target the next
 * imported segment starts at -1 directly, before the later SegData field. */
#include "encode.h"
#include "block.h"
#include "genopt.h"
#include "stubs.h"
#include <recon.h>

void CodeHandle(unsigned char kind,unsigned char length,struct code_lbl *label)
{
    any_oc temp;
    temp.op.kind=kind;
    temp.op.reclen=sizeof(oc_handle);
    temp.op.objlen=length;
    /* The old compiler leaves the unused reference field unspecified.
     * Queue insertion supplies reference links; don't reproduce stack bytes. */
    temp.handle.ref=NULL;
    temp.handle.handle=label;
    InputOC(&temp);
}

void CodeLabel(struct code_lbl *label,unsigned align)
{
    if (OptForSize>50 || align==0) align=1;
    CodeHandle(6,(unsigned char)(align-1),label); /* OC_LABEL */
}

struct code_lbl *LocateLabel(instruction *cond,int index)
{
    if (index==2) return NULL;
    do cond=cond->head.next; while (cond->head.opcode!=0x4b);
    return (struct code_lbl *)((block_ins *)cond)->blk->edge[index].destination;
}
unsigned char CondCode(instruction *cond);
int OptInsSize(unsigned char kind,int destination);
void GenJumpLabel(struct code_lbl *label);
void DoCondJump(instruction *cond)
{
    any_oc temp;
    struct code_lbl *dest_true,*dest_false,*dest_next;
    instruction *next=cond->head.next;
    while (next->head.opcode!=0x4b) next=next->head.next;
    dest_next=NULL;
    if (((block_ins *)next)->blk->next_block!=NULL)
        dest_next=((block_ins *)next)->blk->next_block->label;
    dest_false=LocateLabel(cond,cond->flags.byte>>4);
    dest_true=LocateLabel(cond,cond->flags.byte&15);
    if (dest_next!=NULL && dest_true==dest_next && dest_false!=NULL) {
        FlipCond(cond);
        dest_true=dest_false;
        dest_false=dest_next;
    }
    if (dest_true!=NULL && dest_false!=dest_true &&
        (dest_true!=dest_next || dest_false==NULL)) {
        temp.op.kind=0x0a;
        if ((CGProcessorVersion&15)<3) temp.op.kind|=0x20;
        temp.op.objlen=(unsigned char)OptInsSize(0x0a,1);
        temp.op.reclen=sizeof(oc_jcond);
        temp.jcond.ref=NULL; /* Uninitialized original queue link is supplied by insertion. */
        temp.jcond.cond=CondCode(cond);
        temp.jcond.handle=dest_true;
        InputOC(&temp);
    }
    if (dest_false!=dest_next && dest_false!=NULL) GenJumpLabel(dest_false);
}
void GenSetCC(instruction *cond);
void GenJumpLabel(struct code_lbl *label);
void GenCondJump(instruction *cond)
{
    struct code_lbl *dest;
    if (cond->result==NULL) {
        if ((cond->flags.byte&15)==(cond->flags.byte>>4)) {
            dest=LocateLabel(cond,cond->flags.byte>>4);
            if (dest!=NULL) GenJumpLabel(dest);
        } else DoCondJump(cond);
    } else GenSetCC(cond);
}

void GenJumpLabel(struct code_lbl *label)
{
    CodeHandle(12,(unsigned char)OptInsSize(12,1),label);
}
