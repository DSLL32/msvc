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


#include "procdef.h"
#include "cgmemory.h"
#include <recon.h>

#pragma pack(4)
struct frame_patch { struct frame_patch *next;void *patch; };
#pragma pack()
typedef char old_frame_patch_size[(sizeof(struct frame_patch)==8)?1:-1];
void AbsPatch(void *handle,long value);
void PatchBigLabels(long stack)
{
    struct frame_patch *patch,*junk;
    patch=CurrProc->frame_index;
    while (patch!=NULL) {
        AbsPatch(patch->patch,stack);
        junk=patch;
        patch=patch->next;
        CGFreeSize(junk,sizeof(*junk));
    }
    CurrProc->frame_index=NULL;
}

hw_reg_set DisplayReg(void);
unsigned char AskIsFrameIndex(name *operand)
{
    name *bp;
    bp=AllocRegName(DisplayReg());
    return operand==AllocIndex(bp,NULL,-1,bp->n.name_class);
}
