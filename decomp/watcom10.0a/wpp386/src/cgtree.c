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
* Description:  Code-generator type identities, aliases and scalar definitions.
*
****************************************************************************/


#include "codegen.h"
#include "pending.h"
#include <recon.h>
/* tree.c's subtraction matrix before 64-bit integer/long-double classes. */
static unsigned char SubMat[XX][XX]={
/* U1 I1 U2 I2 U4 I4 CP PT FS FD */
 {U1,U1,U2,I2,U4,I4,CP,PT,FS,FD},
 {U1,I1,U2,I2,U4,I4,CP,PT,FS,FD},
 {U2,U2,U2,U2,U4,I4,CP,PT,FS,FD},
 {I2,I2,U2,I2,U4,I4,CP,PT,FS,FD},
 {U4,U4,U4,U4,U4,U4,CP,PT,FS,FD},
 {I4,I4,I4,I4,U4,I4,CP,PT,FS,FD},
 {CP,CP,CP,CP,CP,CP,XX,PT,ER,ER},
 {PT,PT,PT,PT,PT,PT,PT,XX,ER,ER},
 {FS,FS,FS,FS,FS,FS,ER,ER,FS,FD},
 {FD,FD,FD,FD,FD,FD,ER,ER,FD,FD}
};
static void *TreeFrl;
static unsigned char IsAddress;

void TInit(void)
{
    IsAddress=0;
    SubMat[CP][CP]=TypeClass(TypeLongInteger);
    SubMat[PT][PT]=TypeClass(TypeHugeInteger);
    InitFrl(&TreeFrl);
}


void InitFrl(void **list)
{
    *list=NULL;
}
