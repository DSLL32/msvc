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
#include "procdef.h"
#include <recon.h>

static unsigned SaveModel;

void BGProcDecl(void *symbol,type_def *type)
{
    unsigned char type_class;
    unsigned short old;
    struct code_lbl *label;
    name *temp;
    hw_reg_set reg;
    SaveModel=TargetModel;
    type_class=AddCallBlock(symbol,type);
    if (type!=TypeNone && type_class==XX) {
        if (CurrProc->state.attr&0x40) {
            old=SetOP(AskBackSeg());
            label=AskForNewLabel();
            DataLabel(label);
            DGUBytes(type->length);
            CurrProc->targ.return_points=SAllocMemory(label,0,1,TypeClass(type),type->length);
            SetOP(old);
        } else {
            reg=CurrProc->state.return_reg;
            if (reg==0) {
                temp=DoParmDecl(NULL,TypeInteger,0);
            } else {
                temp=AllocTemp(U4);
                temp->v.usage|=2;
                AddIns(MakeMove(AllocRegName(reg),temp,U4));
                CurrProc->state.parm.used|=reg;
            }
            CurrProc->targ.return_points=temp;
        }
    }
    if ((CurrProc->state.attr&0x2002)==0x2000) TargetModel|=0x200;
}

void InitTargProc(void)
{
    CurrProc->targ.stack_check=NULL;
    CurrProc->targ.push_local_size=0;
    CurrProc->targ.debug=NULL;
    CurrProc->targ.return_points=NULL;
    CurrProc->targ.sp_frame=0;
    CurrProc->targ.sp_align=0;
    CurrProc->targ.has_fd_temps=0;
    CurrProc->targ.never_sp_frame=0;
}
