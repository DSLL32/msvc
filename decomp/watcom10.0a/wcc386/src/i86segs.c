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
* Description:  i386 segmented-pointer type selection.
*
****************************************************************************/


#include "address.h"
#include "options.h"
#include <recon.h>

/* i86segs.c; old FE and back-handle code/data distinction. */
cg_type NamePtrType(name *op)
{
    void *symbol;
    if (op->n.class==1) { /* N_MEMORY */
        if (op->m.memory_type==CG_FE || op->m.memory_type==CG_BACK) {
            symbol=op->v.symbol;
            if (AskNameCode(symbol,op->m.memory_type)) {
                if (op->m.memory_type==CG_FE && (FEAttr(symbol)&1)) {
                    if (IsFarFunc(symbol)) return T_LONG_CODE_PTR;
                    return T_NEAR_CODE_PTR;
                }
                return T_HUGE_POINTER;
            }
            if (AskSegPrivate(AskSegID(symbol,op->m.memory_type)) ||
                (TargetModel&FLOATING_DS)) {
                if (op->m.memory_type==CG_FE && (FEAttr(symbol)&0x2000))
                    return T_POINTER;
                return T_HUGE_POINTER;
            }
            return T_POINTER;
        }
        if ((TargetModel&FLOATING_DS) || op->m.memory_type==CG_CLB)
            return T_HUGE_POINTER;
        return T_POINTER;
    }
    if (op->n.class==2) { /* N_TEMP */
        if (TargetModel&(0x200|FLOATING_DS)) return T_LONG_POINTER;
        return T_POINTER;
    }
    if (TargetModel&FLOATING_DS) return T_HUGE_POINTER;
    return T_POINTER;
}
