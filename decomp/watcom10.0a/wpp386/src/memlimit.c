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


/* WCC's established old memlimit.c body, locally adapted and checked
 * against the actualWPP78d83 sequence and compiler-private MemSize owner. */
#include "codegen.h"
#include "cgmemory.h"
#include "pending.h"
#include <recon.h>
static unsigned MemLimit;
static unsigned char IckyWicky;
unsigned MemSize(void);
void CalcMemLimit(void)
{
    unsigned size=MemSize();
    MemLimit=size-size/4;
    IckyWicky=0;
}

unsigned FrlSize;
unsigned char MemLow(void)
{
    return ChkMemLimit(MemLimit);
}
/* The first five bytes fall through to this separately callable entry. */
unsigned char ChkMemLimit(unsigned limit)
{
    if (Model&0x100000) return 0;
    if (MemInUse()-FrlSize<=limit) return 0;
    FlushSomeOpt(MemInUse()-limit-FrlSize);
    if (MemInUse()-FrlSize<=limit) return 0;
    return 1;
}
unsigned MemInUse(void) { STUB_TRAP();return 0; }
unsigned char FlushSomeOpt(unsigned size) { (void)size;STUB_TRAP();return 0; }
