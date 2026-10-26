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


/* cgosi.c backend break flag; unsigned byte load is explicit in1994. */
#include "compiler.h"
#include "cgmemory.h"
#include "osi-globals.h"
#include <recon.h>
int TBreak(void)
{
    int brk;
    brk=(unsigned char)*_BreakFlagPtr;
    *_BreakFlagPtr=0;
    return brk;
}

/* CheckEvents was folded into DGBlip in this build. The unknown interrupt
 * failure path still reaches the real pending FatalError entry. */
void DGBlip(void)
{
    if (*_BreakFlagPtr!=0) {
        *_BreakFlagPtr=0;
        FatalError("Program interrupted from keyboard");
    }
}
