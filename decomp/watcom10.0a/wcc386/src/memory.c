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


/* Adapted from the initial Open Watcom memmgr.c (2002), checked against
 * WCC3861994 CF47c77..47cd4. This private compiler heap is App code;
 * the malloc/nmalloc wrapper does not turn it into CRT support. */
#include "memory.h"
#include <recon.h>

int Initialized;
unsigned AllocSize,MemorySize;

void *MemAlloc(unsigned amount)
{
    char *chunk;
    unsigned cls;

    if (!Initialized) MemInit();
    if (!amount) return 0;
    amount=(amount+7)&~3u;
    if (amount<16) amount=16;
    cls=SizeToClass(amount);
    for (;;) {
        chunk=(char *)GetFromFrl(amount,cls);
        if (chunk) return chunk+4;
        if (cls==7) break;
        ++cls;
    }
    return MemFromSys(amount);
}

/* Actual1994 MemInit shares the former CalcMemSize tail, without a call
 * vertex. Its default is4MiB (2002's16MiB differs); its free-list sentinel is
 * statically initialized elsewhere, not filled here as in later MemInit. */
void MemInit(void)
{
    unsigned requested;
    int query_max,query_size;
    char buff[80];

    if (Initialized) return;
    Initialized=1;
    AllocSize=0;
    Initialized=2;
    requested=0;
    query_max=0;
    query_size=0;
    if (GetEnvVar("WCGMEMORY",buff,9)) {
        if (buff[0]=='?' && !buff[1]) query_max=1;
        else if (buff[0]=='#' && !buff[1]) query_size=1;
        else requested=(unsigned)myatoi(buff)*1024;
    }
    MemorySize=requested ? requested : 4u*1024*1024;
    if (query_max || query_size) {
        myitoa(4096,CopyStr("Maximum WCGMEMORY=",buff));
        write(1,buff,Length(buff));
        write(1,"\r\n",2);
    }
}

unsigned SizeToClass(unsigned amount)
{
    unsigned cls;

    if (amount>=2048) return 7;
    if (amount<32) return 0;
    amount>>=4;
    cls=0;
    do {
        ++cls;
        amount>>=1;
    } while (amount);
    return cls;
}
