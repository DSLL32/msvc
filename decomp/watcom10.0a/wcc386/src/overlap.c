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
* Description:  Name overlap queries.
*
****************************************************************************/


#include "block.h"
#include "stubs.h"
#include <recon.h>
unsigned char ovNo(name *left,name *right)
{
    (void)left;(void)right;
    return 0;
}
/* Source ovIndex and ovYes fold into the same physical true-return entry. */
unsigned char ovYes(name *left,name *right) { STUB_TRAP();return 0; }
unsigned char ovTemp(name *left,name *right) { STUB_TRAP();return 0; }
unsigned char ovReg(name *left,name *right) { STUB_TRAP();return 0; }
unsigned char ovUses(name *left,name *index) { STUB_TRAP();return 0; }
/* Complete encountered overlap.c matrix at7c1a4..7c208, source roles intact. */
static unsigned char (*OverlapTable[5][5])(name *,name *)={
    {ovNo,ovNo,ovNo,ovNo,ovNo},
    {ovNo,ovYes,ovNo,ovNo,ovNo},
    {ovNo,ovNo,ovTemp,ovNo,ovUses},
    {ovNo,ovNo,ovNo,ovReg,ovUses},
    {ovNo,ovNo,ovNo,ovNo,ovYes}
};
unsigned char Overlaps(name *result,name *op)
{
    if (result==NULL || op==NULL) return 0;
    if (result==op) return 1;
    if (result->n.class>4 || op->n.class>4) STUB_TRAP();
    return OverlapTable[result->n.class][op->n.class](result,op);
}
