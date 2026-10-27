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

/* bld/plusplus/c/stats.c initial commit6b9cb443. Actual1994 clears
 * four counters and compiler-flag byte0 bits2/7, with no extra calls. */
#include <stdio.h>
#include "compiler.h"
#include "frontend.h"
#include "initstate.h"
#include <recon.h>

unsigned SrcLineCount, IncLineCount;

void statsInit(INITFINI *entry)
{
    (void)entry;
    SrcLineCount = 0;
    IncLineCount = 0;
    WngCount = 0;
    ErrCount = 0;
    CompFlags.stats_printed = 0;
    CompFlags.extra_stats_wanted = 0;
}

/* The old implementation prints directly rather than constructing the
 * later VBUF. putc's period macro supplies its ordinary FILE fast path. */
void intPrint(char *thing,char *end,unsigned count)
{
    if (!count) printf("no ");
    else printf("%u ",count);
    printf("%s",thing);
    if (count!=1) putc('s',stdout);
    printf("%s",end);
}
void statsPrint(INITFINI *defn)
{
    (void)defn;
    if (CompFlags.stats_printed || CompFlags.quiet_mode || !WholeFName) return;
    printf("%s: ",WholeFName);
    intPrint("line",", ",SrcLineCount);
    if (IncLineCount) printf("included %u, ",IncLineCount);
    intPrint("warning",", ",WngCount);
    intPrint("error","\n",ErrCount);
    fflush(stdout);
    CompFlags.stats_printed=1;
}
