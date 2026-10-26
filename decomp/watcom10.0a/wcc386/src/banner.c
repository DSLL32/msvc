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


#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include "command.h"
#include <recon.h>

void CBanner(void)
{
    if (CompFlags.banner_printed || CompFlags.quiet_mode) return;
    BannerMsg("WATCOM C32 Optimizing Compiler  Version 10.0a");
    BannerMsg("Copyright by WATCOM International Corp. 1984, 1994. All rights reserved.");
    BannerMsg("WATCOM is a trademark of WATCOM International Corp.");
    fflush(stdout);
    CompFlags.banner_printed=1;
}

/* The old host callback is a single tail transfer to puts. */
void BannerMsg(const char *line)
{
    puts(line);
}

void ConsMsg(const char *line)
{
    /* The original loop writes the first character before testing the
     * following one; all help table entries are nonempty. */
    do {
        putchar((unsigned char)*line++);
    } while (*line!='\0');
    putchar('\n');
}

/* Descriptive name for the old host's variadic vprintf forwarding entry. */
int __cdecl ConPrintf(const char *format,...)
{
    int result;
    va_list args;
    va_start(args,format);
    result=vprintf(format,args);
    va_end(args);
    return result;
}
