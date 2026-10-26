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


/* The original OSI calloc[2ff13,2ff2b) calls the segment wrapper at
 * 2f4cd, which calls App MemAlloc, then zeroes the product. Shipped NT calloc
 * instead reaches its own NT heap. Host calloc would likewise bypass genuine
 * private-heap calls and produce blocks unsuitable for later App MemFree.
 * These safe names retain that proved callback variant in both builds,
 * only for callers opting in through allocator.h. They never interpose on
 * host/shim allocations. This is justified CRT compatibility, not App credit.
 * The original wrapper's ES=DS adaptation is unnecessary for flat Win32 and
 * native pointers; its genuine App call is preserved.
 */
#include <string.h>
#include "allocator.h"
#include <recon.h>

void *malloc_segment_wrapper(unsigned amount)
{
    return MemAlloc(amount);
}

void *wcc_calloc(unsigned count,unsigned element_size)
{
    void *memory;
    unsigned bytes;

    bytes=count*element_size;
    memory=malloc_segment_wrapper(bytes);
    if (memory) memset(memory,0,bytes);
    return memory;
}

/* Matched14-byte OSI ES=DS wrapper used by CRT stream-node destruction. */
void free_segment_wrapper(void *memory)
{
    MemFree((char *)memory);
}
