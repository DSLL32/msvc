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
* Description: Original-source attribution retained for the adapted entry.
*
****************************************************************************/

/* Adapted from bld/sdk/rc/wres/c/wridexby.c, commit
 * 6b9cb4438959d845a2b4dd4bf19f7908ecd608e9. CF21670..21682. */
#include "wasm.h"
#include <recon.h>

int WResIDExtraBytes(const WResID *id)
{
    if (id->IsName) {
        return id->ID.Name.NumChars - 1;
    }
    return 0;
}

/* CF204b0..204b8, old packed ID has a16-bit numeric field. */
void WResInitIDFromNum(long num, WResID *id)
{
    id->IsName = 0;
    id->ID.Num = num;
}

/* Adapted from wridcmp.c, CF21570..215aa. */
int WResIDCmp(const WResID *first, const WResID *second)
{
    if (first->IsName && second->IsName)
        return WResIDNameCmp(&first->ID.Name, &second->ID.Name) == 0;
    if (!first->IsName && !second->IsName) return first->ID.Num == second->ID.Num;
    return 0;
}
