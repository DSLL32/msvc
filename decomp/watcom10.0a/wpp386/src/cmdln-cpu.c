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

/* Initial plusplus/c/cmdlnx86.c at6b9cb443;1994 accepts a caller-owned
 * scratch buffer and knows only CPUs0..5. */
#include <string.h>
#include <stdlib.h>
#include "cmdline.h"
#include <recon.h>
#if !defined(__linux__)
/* The original copies this literal inline; preserve that compiler intrinsic. */
#pragma intrinsic(strcpy)
#endif

void defineM_IX86Macro(char *buffer)
{
    unsigned cpu=3;
    strcpy(buffer,"_M_IX86=");
    switch (CpuSwitches&15) {
    case 0: cpu=0; break;
    case 1: cpu=1; break;
    case 2: cpu=2; break;
    case 3: break;
    case 4: cpu=4; break;
    case 5: cpu=5; break;
    }
    ConcatBase10(buffer,cpu*100);
    PreDefineStringMacro(buffer);
}

void ConcatBase10(char *buffer, unsigned value)
{
    char *end=buffer+strlen(buffer);
    utoa(value,end,10);
}
