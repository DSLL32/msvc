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


#include "block.h"
#include "score.h"
#include "stubs.h"
#include <recon.h>

void AddRegs(void)
{
    AllocRegName(0x1000U); /* HW_DS */
    AllocRegName(0x2000U); /* HW_ES */
    AllocRegName(0x8000U); /* HW_SS */
}

unsigned char ScRealRegister(name *reg_name)
{
    return reg_name->n.name_class!=XX;
}

unsigned char RegsEqual(score *sc,int left,int right);
void RegInsert(score *sc,int left,int right);
void ScInitRegs(score *sc)
{
    int xs,ds;
    ds=AllocRegName(0x1000U)->r.reg_index;
    if (!(TargetModel&0x400U)) {
        if (!(TargetModel&0x100U)) {
            xs=AllocRegName(0x2000U)->r.reg_index;
            if (!RegsEqual(sc,xs,ds)) RegInsert(sc,xs,ds);
        }
        if (!(TargetModel&0x200U)) {
            xs=AllocRegName(0x8000U)->r.reg_index;
            if (!RegsEqual(sc,xs,ds)) RegInsert(sc,xs,ds);
        }
    }
}

/* Released386score.c explicitly leaves this target-specific hook empty.
 * DoScore's caller discards the result; preserve the definition's void ABI. */
void ScoreSegments(score *sc)
{
    sc=sc;
}

unsigned char ScConvert(instruction *ins)
{
    /* The386 hook has no special register-conversion exclusion. */
    ins=ins;
    return 0;
}

unsigned char CanReplace(instruction *ins)
{
    ins=ins;
    return 1;
}

unsigned char ScAddOk(hw_reg_set left,hw_reg_set right)
{
    if ((left&CurrProc->state.unalterable) && left!=0x1000U && left!=0x8000U) return 0;
    if ((right&CurrProc->state.unalterable) && right!=0x1000U && right!=0x8000U) return 0;
    return 1;
}
