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

/* Adapted from bld/cg/c/memmgr.c at Open Watcom's initial commit
 * 6b9cb4438959d845a2b4dd4bf19f7908ecd608e9. Checked against actual
 * WPP386 10.0a CF0x81ba3..0x81c00. Its private compiler allocator is
 * App code even though startup reaches it through malloc/nmalloc aliases.
 * Recovered tag/word size4, minimum allocation16 and maximum class7.
 * The surrounding allocator is introduced only as its bodies are reached. */
#include "memory.h"
#include <recon.h>

int Initialized = 0;

void *MemAlloc(unsigned amount)
{
    char *chunk;
    int cls;

    if (!Initialized) MemInit();
    if (!amount) return 0;
    amount = (amount + 7) & ~3u;
    if (amount < 16) amount = 16;
    cls = SizeToClass(amount);
    for (;;) {
        chunk = (char *)GetFromFrl(amount, cls);
        if (chunk) return chunk + 4;
        if (cls == 7) break;
        ++cls;
    }
    return MemFromSys(amount);
}
