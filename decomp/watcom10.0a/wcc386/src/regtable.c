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
#include "options.h"
#include <recon.h>

/* Encountered mutable386rgtbl.c parameter-register lists. */
static hw_reg_set FPParm2Reg[]={0,0};
hw_reg_set STParmReg[]={
    0x00020000U, /* ST1 */
    0x00040000U, /* ST2 */
    0x00080000U, /* ST3 */
    0x00100000U, /* ST4 */
    0x00200000U, /* ST5 */
    0x00400000U, /* ST6 */
    0x00800000U, /* ST7 */
    0,0
};

void InitRegTbl(void)
{
    if (((CGProcessorVersion&0xf0)&0x7f)>=0x10) STParmReg[Max87Stk]=0;
    if (TargetModel&0x1000) FPParm2Reg[0]=HW_ECX|HW_ESI;
    else FPParm2Reg[0]=HW_ECX|HW_EBX;
}
