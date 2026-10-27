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


/* Initial6b9cb443 cgbkmain.c, checked against the actual1994 controller.
 * Only the encountered opening phase is reconstructed so far. */
#include "compiler.h"
#include "frontend.h"
#include "pending.h"
#include "cmdline.h"
#include "codegen.h"
#include <recon.h>
static unsigned max_inline_depth=3;
void CDoptBackEnd(void);
void MarkFuncsToGen(unsigned max_depth);
void CgBackEnd(void)
{
    cg_init_info info;
    CompFlags.codegen_active=1;
    CDoptBackEnd();
    MarkFuncsToGen(max_inline_depth);
    CtxSetContext(8);
    info=BEInit(GenSwitches,TargetSwitches,OptSize,(unsigned short)CpuSwitches);
    (void)info;
    /* The remaining generation lifecycle is pending in both builds. */
    STUB_TRAP();
}
