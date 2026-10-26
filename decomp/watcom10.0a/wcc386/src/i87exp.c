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
#include "codegen.h"
#include "pragma.h"
#include <recon.h>

int Max87Stk;
unsigned char Used87;

void InitFP(void)
{
    Max87Stk=(int)FEAuxInfo(NULL,40);
    if (Max87Stk>8) Max87Stk=8;
    if (Max87Stk<4) Max87Stk=4;
    Used87=0;
    InitFPStkReq();
}

/* i87reg.c's387 intrinsic stack requirements. These nineteen operation
 * roles precede the later P5 division addition. COS/SIN are slots6/7. */
static unsigned char StackReq387[]={
    2, /* POW */
    0, /* ATAN2 */
    0, /* FMOD */
    0, /* NEGATE */
    0, /* COMPLEMENT */
    1, /* LOG */
    1, /* COS */
    1, /* SIN */
    1, /* TAN */
    0, /* SQRT */
    0, /* FABS */
    2, /* ACOS */
    2, /* ASIN */
    1, /* ATAN */
    2, /* COSH */
    2, /* EXP */
    1, /* LOG10 */
    2, /* SINH */
    2  /* TANH */
};

void InitFPStkReq(void)
{
    if (TargetModel&1) {
        StackReq387[7]=0;
        StackReq387[6]=0;
    }
}
