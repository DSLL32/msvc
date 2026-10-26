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


#include "memory.h"
#include "compiler.h"
#include "symbols.h"
#include <recon.h>

unsigned NextFilePage;

SEGADDR_T AllocSegment(struct seg_info *segment)
{
    segment->in_farmem=0;
    segment->in_page_file=0;
    segment->index=(SEGADDR_T)FEmalloc(0x4000);
    if (segment->index!=0) {
        segment->in_farmem=1;
        segment->allocated=1;
        return segment->index;
    }
    if (PageHandle==-1) OpenPageFile();
    segment->index=NextFilePage;
    segment->in_page_file=1;
    segment->allocated=1;
    ++NextFilePage;
    return 0;
}

unsigned SegAlignment[11];
void SegInit(void)
{
    __STOSD(SegAlignment,4,11);
}
