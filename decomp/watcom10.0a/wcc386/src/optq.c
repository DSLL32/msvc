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
#include <recon.h>

void InsertQueue(ins_entry *entry,ins_entry *after)
{
    ++InOptimizer;
    if (FirstIns==NULL) {
        FirstIns=entry;
        entry->ins.prev=NULL;
        LastIns=entry;
        entry->ins.next=NULL;
    } else if (after==NULL) {
        FirstIns->ins.prev=entry;
        entry->ins.prev=NULL;
        entry->ins.next=FirstIns;
        FirstIns=entry;
    } else if (after==LastIns) {
        after->ins.next=entry;
        entry->ins.next=NULL;
        entry->ins.prev=after;
        LastIns=entry;
    } else {
        entry->ins.next=after->ins.next;
        entry->ins.prev=after;
        after->ins.next->ins.prev=entry;
        after->ins.next=entry;
    }
    ++QCount;
    --InOptimizer;
}

void DeleteQueue(ins_entry *ins)
{
    ++InOptimizer;
    if (ins==FirstIns) {
        FirstIns=ins->ins.next;
        if (FirstIns!=NULL) FirstIns->ins.prev=NULL;
        else LastIns=NULL;
    } else if (ins==LastIns) {
        LastIns=ins->ins.prev;
        LastIns->ins.next=NULL;
    } else {
        ins->ins.next->ins.prev=ins->ins.prev;
        ins->ins.prev->ins.next=ins->ins.next;
    }
    --QCount;
    --InOptimizer;
}
