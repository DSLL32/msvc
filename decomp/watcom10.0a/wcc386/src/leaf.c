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
#include <string.h>
#include "quads.h"
#include <recon.h>
#ifdef __WATCOMC__
#pragma intrinsic(memcpy)
#endif

int LeafRepCount;
void LeafReplace(LEAFPTR leaf,unsigned index)
{
    LEAFPTR dst;
    ++LeafRepCount;
    if (index>=0xfffb) printf("***** LeafReplace: leaf %x being updated\n",index);
    dst=AccessFileInfo(index,&LeafFileInfo);
    memcpy(dst,leaf,sizeof(LEAF));
    LeafFileInfo.curr_buf_dirty=1;
}
