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
* Description:  Code-generator memory-pressure limits.
*
****************************************************************************/


#include "memory.h"
#include "cgmemory.h"
#include "codegen.h"
#include <recon.h>
static unsigned MemLimit;
static unsigned char IckyWicky;

void CalcMemLimit(void)
{
    unsigned size;
    size=MemSize();
    MemLimit=size-size/4;
    IckyWicky=0;
}

unsigned FrlSize;

/* ChkMemLimit is inlined into MemLow in the old build; MemCritical also
 * reaches its internal entry. Keep the public graph free of a new helper. */
unsigned char MemLow(void)
{
    unsigned limit=MemLimit;
    if (Model&0x100000) return 0;
    if (MemInUse()-FrlSize<=limit) return 0;
    FlushSomeOpt(MemInUse()-limit-FrlSize);
    if (MemInUse()-FrlSize<=limit) return 0;
    return 1;
}
