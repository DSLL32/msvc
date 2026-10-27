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
* Description:  Front-end and runtime call auxiliary lookup.
*
****************************************************************************/


#include "block.h"
#include "codegen.h"
#include "pragma.h"
#include "stubs.h"
#include <recon.h>

unsigned char AskIfRTLabel(struct code_lbl *label);
void *BEAuxInfo(void *handle,unsigned char request);
void *FindAuxInfo(name *op,unsigned char request)
{
    struct bck_info *back;
    void *info;
    if (op==NULL || op->n.class!=1) {
        info=FEAuxInfo(NULL,0);
    } else if (op->m.memory_type==CG_FE) {
        info=FEAuxInfo(op->v.symbol,0);
    } else if (op->m.memory_type==CG_BACK) {
        back=op->v.symbol;
        info=FEAuxInfo(AskForLblSym(back->label),0);
    } else if (op->m.memory_type==CG_LBL && AskIfRTLabel(op->v.symbol)) {
        info=BEAuxInfo(op->v.symbol,0);
        if (info!=NULL) return BEAuxInfo(info,request);
        info=FEAuxInfo(NULL,0);
    } else info=FEAuxInfo(NULL,0);
    return FEAuxInfo(info,request);
}
