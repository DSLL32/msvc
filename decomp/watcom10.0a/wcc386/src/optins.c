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
