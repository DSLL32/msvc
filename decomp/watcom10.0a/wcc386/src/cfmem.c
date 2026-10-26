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
#include "cfloat.h"
#include "cgmemory.h"
#include <recon.h>

/* The older cfmem.c has a normal code-generator free-list head, before
 * the released allocator callback table and miniFrl implementation. */
static void *cfFrlList;

void CFInit(void)
{
    InitFrl(&cfFrlList);
}

struct cfloat *CFAlloc(int size)
{
    struct cfloat *number;
    if (size<=17) {
        size=33;
        number=AllocFrl(&cfFrlList,size);
    } else {
        size+=17;
        number=CGAlloc(size);
    }
    number->sign=0;
    number->exp=1;
    number->len=1;
    number->mant[0]='0';
    number->mant[1]='\0';
    number->alloc=size;
    return number;
}

void CFFree(struct cfloat *number)
{
    if (number->alloc==33) FrlFreeSize(&cfFrlList,number,33);
    else CGFreeSize(number,number->alloc);
}
