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

#include "cmdline.h"
#include "fixup.h"
#include <recon.h>

/* These preexisting globals still have provisional role names. */
extern void *OverlayList;              /* original4500f8 */
extern void *OverlayWork;          /* original450114 */
seg_leader *OvlRefs;        /* original450158 */

void FreeOvlStruct( void )
{
    OverlayList = 0;
    OverlayWork = 0;
    FreeList( (path_entry *)OvlRefs );
    FreeDistStuff();
}

group_entry *OvlGroup;            /* original450128 */
address OvltabAddr;                /* original450168 */
unsigned OvltabSize;               /* original4500dc */

extern void GetVecAddr( unsigned, address * );

void OvlForceVect( fixup_ref *target, unsigned char indirect )
{
    symbol *sym;
    unsigned short state;

    if( !(FmtData.type & 0x0080) || !(target->flags & 0x10) ) return;
    sym = target->u.sym;
    if( (sym->info & 15) <= 3 ) return;
    state = (unsigned short)sym->u.value;
    if( ((state >> 12) & 3) != 2 ) return;
    if( !indirect && !(state & 0x4000) ) return;
    GetVecAddr( state & 0x0fff, &target->addr );
}
